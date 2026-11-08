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
// Never write 0: an OLED at level 0 is black, not "minimum".
constexpr uint16_t Floor=0x0100;

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

inline uint16_t level(uint32_t brightness) {
    const uint32_t b=brightness>0xFFFF ? 0xFFFF : brightness;
    return static_cast<uint16_t>(b<Floor ? Floor : b);
}

// control/mode: current 0x720/0x721 as just read from the panel.
inline Plan plan(const Caps &caps,uint8_t control,uint8_t mode,uint32_t brightness) {
    Plan p;
    if(!caps.auxSet())return p;
    p.usable=true;
    if((mode&ModeMask)!=ModeDPCD){p.writeMode=true;p.mode=static_cast<uint8_t>((mode&~ModeMask)|ModeDPCD);}
    if(caps.auxEnable() && !(control&BacklightEnable)){p.writeControl=true;p.control=static_cast<uint8_t>(control|BacklightEnable);}
    p.value=level(brightness);
    if(caps.wide()){p.level[0]=static_cast<uint8_t>(p.value>>8);p.level[1]=static_cast<uint8_t>(p.value);p.levelLength=2;}
    else {p.level[0]=static_cast<uint8_t>(p.value>>8);if(!p.level[0])p.level[0]=1;p.levelLength=1;}
    return p;
}
}
#endif
