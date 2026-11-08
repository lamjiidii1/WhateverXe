#include "kern_display13_edp_auxbl.hpp"
#include <assert.h>
#include <stdio.h>
using namespace Display13EDPAuxBL;
int main() {
    // No AUX brightness capability: never touch the panel.
    assert(!plan(Caps{0x04,0x01,0},0,0,0x8000).usable);
    // 16-bit panel, PWM mode and backlight disabled after a doze: re-assert both.
    auto p=plan(Caps{0x06,0x06,0},0x00,0x00,0x1234);
    assert(p.usable && p.writeMode && p.mode==ModeDPCD && p.writeControl && p.control==1);
    assert(p.levelLength==2 && p.level[0]==0x12 && p.level[1]==0x34 && p.value==0x1234);
    // Already in DPCD mode and enabled: only the level is written; other mode bits kept.
    p=plan(Caps{0x06,0x06,0},0x81,0x82,0xFFFF);
    assert(!p.writeMode && !p.writeControl && p.level[0]==0xFF && p.level[1]==0xFF);
    p=plan(Caps{0x06,0x06,0},0x00,0x81,0x8000);
    assert(p.writeMode && p.mode==0x82);
    // No AUX enable capability: 0x720 is left alone.
    assert(!plan(Caps{0x02,0x06,0},0x00,0x02,0x8000).writeControl);
    // Floor and clamp (16-bit range).
    assert(plan(Caps{0,0x06,0},0,2,0).value==0xFF);
    assert(plan(Caps{0,0x06,0},0,2,0x12345).value==0xFFFF);
    // 8-bit panel: whole byte, never 0.
    p=plan(Caps{0,0x02,0},0,2,0x0180);
    assert(p.levelLength==1 && p.level[0]==1);
    p=plan(Caps{0,0x02,0},0,2,0xFFFF);
    assert(p.levelLength==1 && p.level[0]==0xFF);
    p=plan(Caps{0,0x02,0},0,2,0);
    assert(p.level[0]==1);
    // Galaxy Book3 Ultra: 0x702=0xA6 (16-bit, FREQ_AUX_SET), 0x724..0x726 = 00 0b 0b.
    Caps gb{0x9b,0xa6,0x04};
    assert(maxLevel(gb,0x00,0x0b,0x0b)==0x7FF);
    assert(maxLevel(gb,0x0c,0x0b,0x0b)==0x7FF && maxLevel(gb,0x09,0x0b,0x0c)==0x7FF);
    assert(maxLevel(Caps{0,0x06,0},0,0x0b,0x0b)==0xFFFF && maxLevel(Caps{0,0x02,0},0,0,0)==0xFF);
    p=plan(gb,0,2,0xFFFF,0x7FF);
    assert(p.value==0x7FF && p.level[0]==0x07 && p.level[1]==0xFF);
    p=plan(gb,0,2,0x8000,0x7FF);
    assert(p.value==0x400);
    assert(plan(gb,0,2,0,0x7FF).value==7);
    // Monotonic: no wrap anywhere in the range.
    for(uint32_t v=0,prev=0;v<=0xFFFF;v+=0x41){auto q=plan(gb,0,2,v,0x7FF);assert(q.value>=prev && q.value<=0x7FF);prev=q.value;}
    // Intel interface: Galaxy Book3 Ultra caps 01 57 01 00.
    assert((IntelCaps{{0x01,0x57,0x01,0x00}}.usable()));
    assert((!IntelCaps{{0x01,0x47,0x01,0x00}}.usable() && !IntelCaps{{0x01,0x57,0x00,0x00}}.usable()));
    assert(nits(0)==IntelMinNits && nits(0xFFFF)==IntelMaxNits && nits(0x7FFF)==201 && nits(0x20000)==IntelMaxNits);
    puts("display13_edp_auxbl: ok");
    return 0;
}
