#include "kern_mtl_posh_fix.hpp"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <vector>
using namespace MTLPoshFix;
int main() {
    std::vector<uint8_t> file(0x80000,0xCC);
    memcpy(&file[0x7ED6B],Find,sizeof(Find));
    // The validated range covers the page with the dealloc tail.
    const uint8_t *range=&file[0x7E000];
    assert(matches(range+0x0000) == false || PageOffset==0xD6B);
    size_t hits=forEachMatch(&file[0x7C000],0x4000,[](uint8_t *page){assert(apply(page));});
    assert(hits==1);
    assert(file[0x7ED87]==0x90 && file[0x7ED88]==0x90);
    // Everything else is unchanged, and a second pass finds nothing.
    for(size_t i=0;i<sizeof(Find);++i) if(i!=JumpOffset && i!=JumpOffset+1) assert(file[0x7ED6B+i]==Find[i]);
    assert(forEachMatch(&file[0x7C000],0x4000,[](uint8_t *){assert(false);})==0);
    // Other pages, other offsets and partial ranges are never touched.
    std::vector<uint8_t> other(0x3000,0xCC);
    memcpy(&other[0x1D6C],Find,sizeof(Find)); // shifted by one byte
    assert(forEachMatch(other.data(),other.size(),[](uint8_t *){assert(false);})==0);
    std::vector<uint8_t> partial(0x1000,0);
    memcpy(&partial[PageOffset],Find,sizeof(Find));
    assert(forEachMatch(partial.data(),0xFFF,[](uint8_t *){assert(false);})==0);
    assert(forEachMatch(nullptr,0x1000,[](uint8_t *){assert(false);})==0);
    // A one-byte difference in the ivar displacement rejects the page.
    partial[PageOffset+3]^=1;
    assert(!matches(partial.data()) && !apply(partial.data()));
    puts("mtl_posh_fix: ok");
    return 0;
}
