// Samsung Galaxy Book3 Ultra (Raptor Lake-H A7A0, ATNA60CL03 2880x1800 OLED):
// the GOP drives the panel on DDI A with DSC (DSS_CTL2 left/right VDSC,
// DSS_CTL1 DSC joiner) and leaves transcoder A in CMTG secondary mode.
// Registers are the N97Display13Boot readout of that machine.
#include "kern_alderlake_n.hpp"
#include <map>
#include <cassert>
#include <cstdio>
using namespace AlderLakeN;
struct IO {
    std::map<uint32_t,uint32_t> regs;
    bool readable(uint32_t reg) {return reg<0x1000000;}
    uint32_t read(uint32_t reg) {auto i=regs.find(reg);return i==regs.end()?0:i->second;}
    void write(uint32_t reg,uint32_t v) {regs[reg]=v;}
    void delay() {}
};
static IO samsung() {
    IO io;
    io.regs={
        {0x42000,0x8E380000U},
        {0x42084,0x00000000U},
        {0x44300,0xC040C000U},
        {0x4438C,0x1C000700U},
        {0x45008,0xC040C000U},
        {0x4500C,0x0000FFFFU},
        {0x45010,0x00000000U},
        {0x45014,0x00000000U},
        {0x45400,0x00005405U},
        {0x45404,0x0000FC0FU},
        {0x45408,0x40000000U},
        {0x4540C,0x00005405U},
        {0x45420,0x0A5294A1U},
        {0x45424,0x000000E3U},
        {0x45444,0x00000003U},
        {0x45450,0x00000001U},
        {0x45454,0x00000003U},
        {0x4545C,0x00000001U},
        {0x46000,0x00380458U},
        {0x46004,0x00000000U},
        {0x46140,0x10000000U},
        {0x46144,0x00000000U},
        {0x46148,0x00000000U},
        {0x60000,0x0BA30B3FU},
        {0x60004,0x0BA30B3FU},
        {0x60008,0x0B670B5FU},
        {0x6000C,0x07270707U},
        {0x60010,0x07270707U},
        {0x60014,0x0717070FU},
        {0x6001C,0x0B3F0707U},
        {0x60030,0x7E308734U},
        {0x60034,0x00800000U},
        {0x60040,0x0009B4A4U},
        {0x60044,0x00080000U},
        {0x60400,0x8A010106U},
        {0x60404,0x00000008U},
        {0x60540,0x80040300U},
        {0x61000,0x00000000U},
        {0x61004,0x00000000U},
        {0x61008,0x00000000U},
        {0x6100C,0x00000000U},
        {0x61010,0x00000000U},
        {0x61014,0x00000000U},
        {0x6101C,0x00000000U},
        {0x61030,0x00000000U},
        {0x61034,0x00000000U},
        {0x61040,0x00000000U},
        {0x61044,0x00000000U},
        {0x61400,0x00030000U},
        {0x61404,0x00000000U},
        {0x61540,0x00000000U},
        {0x62000,0x00000000U},
        {0x62004,0x00000000U},
        {0x62008,0x00000000U},
        {0x6200C,0x00000000U},
        {0x62010,0x00000000U},
        {0x62014,0x00000000U},
        {0x6201C,0x00000000U},
        {0x62030,0x00000000U},
        {0x62034,0x00000000U},
        {0x62040,0x00000000U},
        {0x62044,0x00000000U},
        {0x62400,0x00030000U},
        {0x62404,0x00000000U},
        {0x62540,0x00000000U},
        {0x64000,0x80000006U},
        {0x64100,0x00000080U},
        {0x64200,0x00000080U},
        {0x68170,0x00000000U},
        {0x68174,0x00000000U},
        {0x68180,0x00000200U},
        {0x68270,0x00000000U},
        {0x68274,0x00000000U},
        {0x68280,0x00000200U},
        {0x68970,0x00000000U},
        {0x68974,0x00000000U},
        {0x68980,0x00000200U},
        {0x68A70,0x00000000U},
        {0x68A74,0x00000000U},
        {0x68A80,0x00000200U},
        {0x69170,0x00000000U},
        {0x69174,0x00000000U},
        {0x69180,0x00000200U},
        {0x69270,0x00000000U},
        {0x69274,0x00000000U},
        {0x69280,0x00000200U},
        {0x70008,0xC0000000U},
        {0x70028,0x00002A0FU},
        {0x70040,0x000013C1U},
        {0x70080,0x00000000U},
        {0x70084,0x00000000U},
        {0x70180,0x94000008U},
        {0x70188,0x000000B4U},
        {0x7018C,0x00000000U},
        {0x70190,0x07070B3FU},
        {0x7019C,0x00000000U},
        {0x701A4,0x00000000U},
        {0x701AC,0x00000000U},
        {0x7027C,0x07FF0000U},
        {0x70280,0x00000008U},
        {0x7037C,0x00000000U},
        {0x70380,0x00000008U},
        {0x70480,0x00000008U},
        {0x70580,0x00000008U},
        {0x70680,0x00000000U},
        {0x70780,0x00000000U},
        {0x71008,0x00000000U},
        {0x71028,0x00000A0FU},
        {0x71040,0x00000000U},
        {0x71080,0x00000000U},
        {0x71084,0x00000000U},
        {0x71180,0x00000008U},
        {0x71188,0x00000000U},
        {0x7118C,0x00000000U},
        {0x71190,0x00000000U},
        {0x7119C,0x00000000U},
        {0x711A4,0x00000000U},
        {0x711AC,0x00000000U},
        {0x7127C,0x00000000U},
        {0x71280,0x00000008U},
        {0x7137C,0x00000000U},
        {0x71380,0x00000008U},
        {0x71480,0x00000008U},
        {0x71580,0x00000008U},
        {0x71680,0x00000000U},
        {0x71780,0x00000000U},
        {0x72008,0x00000000U},
        {0x72028,0x00000A0FU},
        {0x72040,0x00000000U},
        {0x72080,0x00000000U},
        {0x72084,0x00000000U},
        {0x72180,0x00000008U},
        {0x72188,0x00000000U},
        {0x7218C,0x00000000U},
        {0x72190,0x00000000U},
        {0x7219C,0x00000000U},
        {0x721A4,0x00000000U},
        {0x721AC,0x00000000U},
        {0x7227C,0x00000000U},
        {0x72280,0x00000008U},
        {0x7237C,0x00000000U},
        {0x72380,0x00000008U},
        {0x72480,0x00000008U},
        {0x72580,0x00000008U},
        {0x72680,0x00000000U},
        {0x72780,0x00000000U},
        {0x78000,0x40000000U},
        {0x78004,0x80008000U},
        {0x78200,0x00000000U},
        {0x78204,0x00000000U},
        {0x78400,0x00000000U},
        {0x78404,0x00000000U},
        {0x164280,0x01E07800U},
        {0x164284,0x00E001A5U},
        {0x164288,0x00000048U},
        {0x16428C,0x018001D4U}
    };
    return io;
}
static bool source(IO io,bool panel=true,unsigned ddi=0) {return dpSource(readDisplay(io,true),0,ddi,nullptr,panel);}
int main() {
    auto io=samsung();auto boot=readDisplay(io,true);LinearPlan plan;
    assert(boot.status==ReadoutStatus::Complete && boot.activePipes==1);
    // Default: a compressed source is not admitted.
    admitDSCPanel=false;
    assert(!source(io) && !source(io,false));
    assert(linearPlan(boot,plan,0,GOPProtocol::EDP)==LinearStatus::InvalidDPSource);
    // -wex-edp-dsc: only the panel source on DDI A/B, only the DSC/CMTG bits.
    admitDSCPanel=true;
    assert(source(io) && !source(io,false) && !source(io,true,1));
    assert(dpSourceUnchanged(io,boot,0,0,0,nullptr,true) && !dpSourceUnchanged(io,boot,0,0));
    const struct {uint32_t reg,bits;} foreign[] {{0x60404,1U<<4},{0x60404,1U},{0x78000,1U<<31},{0x78000,1U<<29},
        {0x78000,1U<<28},{0x78000,1U<<20},{0x78004,1U},{0x78004,1U<<16}};
    for(auto f:foreign){auto x=samsung();x.regs[f.reg]|=f.bits;assert(!source(x));}
    {auto x=samsung();x.regs[0x60404]=0;assert(source(x));}          // no CMTG
    {auto x=samsung();x.regs[0x78000]=0;x.regs[0x78004]=0;assert(source(x));} // uncompressed
    auto drift=samsung();drift.regs[0x78004]=0x80000000U;assert(!dpSourceUnchanged(drift,boot,0,0,0,nullptr,true));
    const auto status=linearPlan(boot,plan,0,GOPProtocol::EDP);
    printf("eDP DSC source admitted; linear plan status %u (1 = Ready), pipe %u\n",unsigned(status),plan.pipe);
    admitDSCPanel=false;
    printf("PASS eDP DSC admission on the Galaxy Book3 Ultra readout\n");
}
