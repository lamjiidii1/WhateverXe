#include "kern_display13_edp_blank.hpp"
#include <assert.h>
#include <map>
#include <stdio.h>
#include <vector>
using namespace Display13EDPBlank;
struct IO {
    std::map<uint32_t,uint32_t> r;
    std::vector<uint32_t> writes;
    uint32_t read(uint32_t reg) {assert(r.count(reg));return r[reg];}
    void write(uint32_t reg,uint32_t v) {assert(r.count(reg));r[reg]=v;writes.push_back(reg);}
};
static IO live() {
    IO io;
    io.r[PlaneCtl]=0x84001008U; io.r[PlaneSurf]=0x00200000U;
    io.r[CurCtl]=0x04000027U; io.r[CurBase]=0x01230000U;
    return io;
}
int main() {
    // Decisions.
    assert(decide(false,2,1,false)==Action::Native);
    assert(decide(true,2,1,false)==Action::Blank);
    assert(decide(true,2,0,false)==Action::Blank);
    assert(decide(true,1,0,true)==Action::Keep);
    assert(decide(true,1,2,true)==Action::Unblank);
    assert(decide(true,0,2,true)==Action::Unblank);
    assert(decide(true,2,2,false)==Action::Keep);
    assert(decide(true,1,2,false)==Action::Native);      // native doze stays native
    assert(decide(true,2,3,false)==Action::Native);      // unknown state
    assert(decide(false,1,2,true)==Action::Unblank);     // never leave the panel dark
    assert(decide(false,1,1,true)==Action::Native);

    // Blank clears only the enables and latches each plane once.
    auto io=live();State s;
    blank(io,s);
    assert(s.blanked && s.blanks==1);
    assert(io.r[PlaneCtl]==0x04001008U && io.r[PlaneSurf]==0x00200000U);
    assert(io.r[CurCtl]==0x04000000U && io.r[CurBase]==0x01230000U);
    assert((io.writes==std::vector<uint32_t>{CurCtl,CurBase,PlaneCtl,PlaneSurf}));

    // Unblank restores the enables, latching after each control write.
    io.writes.clear();
    unblank(io,s);
    assert(!s.blanked && s.unblanks==1);
    assert(io.r[PlaneCtl]==0x84001008U && io.r[CurCtl]==0x04000027U);
    assert((io.writes==std::vector<uint32_t>{PlaneCtl,PlaneSurf,CurCtl,CurBase}));

    // A donor reprogramming while dark is kept; only the enable comes back.
    io=live();State t;blank(io,t);
    io.r[PlaneCtl]=0x04003008U; io.r[PlaneSurf]=0x00400000U;
    unblank(io,t);
    assert(io.r[PlaneCtl]==0x84003008U && io.r[PlaneSurf]==0x00400000U);

    // A cursor the donor already re-enabled is not rewritten.
    io=live();State u;blank(io,u);
    io.r[CurCtl]=0x04000022U;io.writes.clear();
    unblank(io,u);
    assert((io.writes==std::vector<uint32_t>{PlaneCtl,PlaneSurf}));
    assert(io.r[CurCtl]==0x04000022U);

    // A plane that was already off stays off.
    io=live();io.r[PlaneCtl]=0x04001008U;State v;blank(io,v);unblank(io,v);
    assert(io.r[PlaneCtl]==0x04001008U);
    puts("display13_edp_blank: ok");
    return 0;
}
