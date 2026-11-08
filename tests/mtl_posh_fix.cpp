#include "kern_mtl_posh_fix.hpp"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <vector>
using namespace MTLPoshFix;
int main() {
    std::vector<uint8_t> file(0x80000,0xCC);
    memcpy(&file[0x7ED6B],Find,sizeof(Find));
    size_t hits=forEachMatch(&file[0x7C000],0x4000,[](uint8_t *at){assert(apply(at));});
    assert(hits==1);
    assert(file[0x7ED87]==0x90 && file[0x7ED88]==0x90);
    for(size_t i=0;i<sizeof(Find);++i) if(i!=JumpOffset && i!=JumpOffset+1) assert(file[0x7ED6B+i]==Find[i]);
    assert(forEachMatch(&file[0x7C000],0x4000,[](uint8_t *){assert(false);})==0); // idempotent
    // Ranges that do not start on the 0x7E000 page still find the site.
    std::vector<uint8_t> f2(0x80000,0xCC);memcpy(&f2[0x7ED6B],Find,sizeof(Find));
    assert(forEachMatch(&f2[0x7D800],0x1800,[](uint8_t *at){assert(apply(at));})==1);
    // Truncated ranges and other data are never touched.
    std::vector<uint8_t> f3(0x1000,0);memcpy(&f3[0xFD6],Find,sizeof(Find)-1);
    assert(forEachMatch(f3.data(),f3.size(),[](uint8_t *){assert(false);})==0);
    assert(forEachMatch(nullptr,0x1000,[](uint8_t *){assert(false);})==0);
    std::vector<uint8_t> f4(0x1000,0);memcpy(&f4[0x100],Find,sizeof(Find));f4[0x103]^=1;
    assert(forEachMatch(f4.data(),f4.size(),[](uint8_t *){assert(false);})==0);
    puts("mtl_posh_fix: ok");
    return 0;
}
