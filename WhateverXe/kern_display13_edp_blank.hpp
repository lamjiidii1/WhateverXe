// -wex-edp-blank (only together with -wex-edp-keep): display sleep for the
// kept firmware eDP link. The donor cannot retrain a firmware panel link it
// did not train (eDP 1.4 rate-table/DSC panels such as the Galaxy Book3 Ultra
// OLED): its Doze -> Wake transition stops the link, runs a complete mode set
// with link training and leaves the panel dark, after which the system hangs.
// Instead, keep the pipe, transcoder and link running and only stop scanning
// out: Doze/Off clear the primary plane and cursor enables (the pipe then
// emits its black background, an OLED emits no light), Wake restores them.
// The same idea as the "backlight only" DPMS of NootedGreen (XE-FBDPMS).
// Plane/cursor registers are latched by the following SURF/BASE write.
#ifndef kern_display13_edp_blank_hpp
#define kern_display13_edp_blank_hpp
#include <stdint.h>

namespace Display13EDPBlank {
constexpr uint32_t PlaneCtl=0x70180, PlaneSurf=0x7019C; // PLANE_CTL_1_A / PLANE_SURF_1_A
constexpr uint32_t CurCtl=0x70080, CurBase=0x70084;     // CUR_CTL_A / CUR_BASE_A
constexpr uint32_t PlaneEnable=0x80000000U;
constexpr uint32_t CursorMode=0x27; // MCURSOR_MODE_MASK; 0 disables the cursor.
// Donor framebuffer power states: 0 off, 1 doze, 2 wake.
constexpr uint32_t Wake=2;

struct State {
    bool blanked {};
    uint32_t plane {}, cursor {}; // Values seen when blanking.
    uint32_t blanks {}, unblanks {};
};
enum class Action : uint32_t { Native, Blank, Unblank, Keep };

// eligible: the framebuffer scans out the adopted boot panel on a kept link.
// current: the donor's recorded power state, requested: the new one.
inline Action decide(bool eligible,uint32_t current,uint32_t requested,bool blanked) {
    if(!eligible || requested>Wake || current>Wake)return blanked && requested==Wake ? Action::Unblank : Action::Native;
    if(requested<Wake)return blanked ? Action::Keep : Action::Blank;
    if(blanked)return Action::Unblank;
    // Already awake: nothing to do. A wake from a native doze stays native.
    return current==Wake ? Action::Keep : Action::Native;
}

template<class IO> void blank(IO &io,State &s) {
    s.plane=io.read(PlaneCtl);s.cursor=io.read(CurCtl);
    io.write(CurCtl,s.cursor&~CursorMode);io.write(CurBase,io.read(CurBase));
    io.write(PlaneCtl,s.plane&~PlaneEnable);io.write(PlaneSurf,io.read(PlaneSurf));
    s.blanked=true;++s.blanks;
}
// Restore only the enables: the donor may legitimately reprogram format,
// tiling or cursor size while the panel is dark (a later flip or mode set).
template<class IO> void unblank(IO &io,State &s) {
    const auto plane=io.read(PlaneCtl);
    io.write(PlaneCtl,plane|(s.plane&PlaneEnable));io.write(PlaneSurf,io.read(PlaneSurf));
    const auto cursor=io.read(CurCtl);
    if(!(cursor&CursorMode) && (s.cursor&CursorMode)) {
        io.write(CurCtl,(cursor&~CursorMode)|(s.cursor&CursorMode));io.write(CurBase,io.read(CurBase));
    }
    s.blanked=false;++s.unblanks;
}
}
#endif
