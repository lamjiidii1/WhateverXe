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
    // Floor and clamp.
    assert(plan(Caps{0,0x06,0},0,2,0).value==Floor);
    assert(plan(Caps{0,0x06,0},0,2,0x12345).value==0xFFFF);
    // 8-bit panel: MSB only, never 0.
    p=plan(Caps{0,0x02,0},0,2,0x0180);
    assert(p.levelLength==1 && p.level[0]==1);
    p=plan(Caps{0,0x02,0},0,2,0xAB00);
    assert(p.levelLength==1 && p.level[0]==0xAB);
    // Intel interface: Galaxy Book3 Ultra caps 01 57 01 00.
    assert((IntelCaps{{0x01,0x57,0x01,0x00}}.usable()));
    assert((!IntelCaps{{0x01,0x47,0x01,0x00}}.usable() && !IntelCaps{{0x01,0x57,0x00,0x00}}.usable()));
    assert(nits(0)==IntelMinNits && nits(0xFFFF)==IntelMaxNits && nits(0x7FFF)==201 && nits(0x20000)==IntelMaxNits);
    puts("display13_edp_auxbl: ok");
    return 0;
}
