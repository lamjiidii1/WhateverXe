// -wex-edp-auxbl: brightness of a kept firmware eDP panel whose VBT backlight
// control is the VESA eDP AUX interface (BDB 43 type 5, e.g. Samsung OLED
// ATNA60CL03). The donor's hwSetBacklight only programs the PCH PWM duty,
// which such a panel ignores. Here the requested 16-bit level is written to
// the panel's DPCD brightness registers instead (VESA eDP 1.4b, 0x701..0x723):
//   0x701 general cap 1: bit2 BACKLIGHT_AUX_ENABLE_CAP
//   0x702 adjustment cap: bit1 BRIGHTNESS_AUX_SET_CAP, bit2 BYTE_COUNT (16-bit)
//   0x720 display control: bit0 BACKLIGHT_ENABLE
//   0x721 mode set: bits1:0, 2 = DPCD brightness
//   0x722/0x723 brightness MSB/LSB
// Panels may reset 0x720/0x721 after a display doze, so both are reread and
// re-asserted on every request instead of being cached.
#ifndef kern_display13_edp_auxbl_hpp
#define kern_display13_edp_auxbl_hpp
#include <stdint.h>

namespace Display13EDPAuxBL {
constexpr uint32_t CapsAddress=0x701,ControlAddress=0x720,ModeAddress=0x721,LevelAddress=0x722;
constexpr uint8_t ModeMask=0x03,ModeDPCD=0x02,BacklightEnable=0x01;
// Never write 0: an OLED at level 0 is black, not "minimum" (floor max/256).

struct Caps {
    uint8_t general1 {},adjust {},adjust2 {};
    bool auxSet() const {return adjust&0x02;}
    bool wide() const {return adjust&0x04;}
    bool auxEnable() const {return general1&0x04;}
};

struct Plan {
    bool usable {};             // panel supports AUX brightness at all
    bool writeMode {};uint8_t mode {};
    bool writeControl {};uint8_t control {};
    uint8_t level[2] {};uint8_t levelLength {};
    uint16_t value {};
};

// Intel eDP HDR/SDR TCON interface (Linux intel_dp_aux_backlight.c), which
// Windows uses on Samsung OLEDs: 0x341 bit4 brightness in nits, 0x342 bit0
// SDR brightness over AUX, 0x344 bit4 brightness AUX enable, 0x354/0x355 nits.
constexpr uint32_t IntelCapsAddress=0x340,IntelControlAddress=0x344,IntelNitsAddress=0x354;
constexpr uint8_t IntelBrightnessAuxEnable=0x10;
constexpr uint16_t IntelMinNits=2,IntelMaxNits=400;
struct IntelCaps {
    uint8_t cap[4] {};
    bool usable() const {return (cap[1]&0x10) && (cap[2]&0x01);}
};
inline uint16_t nits(uint32_t brightness) {
    const uint32_t b=brightness>0xFFFF ? 0xFFFF : brightness;
    return static_cast<uint16_t>(IntelMinNits+(uint64_t(IntelMaxNits-IntelMinNits)*b+0x7FFF)/0xFFFF);
}

// Brightness range. With BACKLIGHT_FREQ_AUX_SET_CAP (0x702 bit5) the level
// register is only PWMGEN_BIT_COUNT (0x724, clamped to 0x725..0x726) bits wide
// (Linux drm_edp_backlight_probe_max): the Samsung ATNA60CL03 reports 11 bits,
// so 16-bit levels above 0x7FF wrapped and the panel flashed on slow ramps.
constexpr uint32_t BitCountAddress=0x724;
inline uint16_t maxLevel(const Caps &caps,uint8_t count,uint8_t minimum,uint8_t maximum) {
    if(!caps.wide())return 0xFF;
    if(!(caps.adjust&0x20))return 0xFFFF;
    uint8_t lo=minimum&0x1F,hi=maximum&0x1F,pn=count&0x1F;
    if(!pn)pn=hi;
    if(lo && pn<lo)pn=lo;
    if(hi && pn>hi)pn=hi;
    if(!pn || pn>16)return 0xFFFF;
    return static_cast<uint16_t>((1U<<pn)-1);
}
inline uint16_t level(uint32_t brightness,uint16_t max=0xFFFF) {
    const uint32_t b=brightness>0xFFFF ? 0xFFFF : brightness;
    const uint32_t floor=max>>8 ? max>>8 : 1;
    const uint32_t v=static_cast<uint32_t>((uint64_t(b)*max+0x7FFF)/0xFFFF);
    return static_cast<uint16_t>(v<floor ? floor : v);
}

// control/mode: current 0x720/0x721 as just read from the panel.
inline Plan plan(const Caps &caps,uint8_t control,uint8_t mode,uint32_t brightness,uint16_t max=0) {
    Plan p;
    if(!caps.auxSet())return p;
    if(!max)max=caps.wide() ? 0xFFFF : 0xFF;
    p.usable=true;
    if((mode&ModeMask)!=ModeDPCD){p.writeMode=true;p.mode=static_cast<uint8_t>((mode&~ModeMask)|ModeDPCD);}
    if(caps.auxEnable() && !(control&BacklightEnable)){p.writeControl=true;p.control=static_cast<uint8_t>(control|BacklightEnable);}
    p.value=level(brightness,max);
    if(caps.wide()){p.level[0]=static_cast<uint8_t>(p.value>>8);p.level[1]=static_cast<uint8_t>(p.value);p.levelLength=2;}
    else {p.level[0]=static_cast<uint8_t>(p.value);p.levelLength=1;}
    return p;
}
}
#endif
