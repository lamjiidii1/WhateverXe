// -wex-mtl-poshfix: WindowServer memory leak in the TGL Metal donor driver
// (AppleIntelTGLGraphicsMTLDriver 16.0.0, GPUDriversIntel-165, file sha256
// 1e5e4f5b...c6c from the WhateverXe Graphics package).
//
// -[MTLIGAccelCommandBuffer initWithQueue:...] allocates a 0x12010-byte POSH
// target (operator new, 96 KB in the medium malloc zone) whenever the queue
// has no active command buffer and the queue's shared m_pPoshUsedAsTarget is
// NULL, and marks the buffer m_isOnlyActiveCommandBuffer. The queue's shared
// pointer is only ever cleared (queue init) and never assigned, so that path
// is taken for nearly every command buffer. -dealloc deletes the POSH target
// only when !m_isOnlyActiveCommandBuffer, so every one of them leaks:
// WindowServer grows by roughly one 96 KB block per composited frame.
//
// The buffer is owned by the command buffer alone (the queue never holds it),
// and dealloc runs after completion, so deleting it there is what the
// non-only-active path already does. The fix drops the m_isOnlyActive test in
// -dealloc (jne -> 2 NOPs) when the page holding it is validated on page-in.
#ifndef kern_mtl_posh_fix_hpp
#define kern_mtl_posh_fix_hpp
#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace MTLPoshFix {
// -[MTLIGAccelCommandBuffer dealloc] +0xB1 .. +0xDC (file offset 0x7ED6B):
//   mov rbx,[rip+ivar _pPoshUsedAsTarget]; mov rdi,[r14+rbx]; test rdi,rdi; je;
//   mov rax,[rip+ivar m_isOnlyActiveCommandBuffer]; cmp byte [r14+rax],0;
//   jne skip; call operator delete; mov qword [r14+rbx],0
constexpr uint8_t Find[43] {
    0x48,0x8B,0x1D,0x3E,0xB1,0x41,0x00,0x49,0x8B,0x3C,0x1E,0x48,0x85,0xFF,0x74,0x1B,
    0x48,0x8B,0x05,0x26,0xB1,0x41,0x00,0x41,0x80,0x3C,0x06,0x00,0x75,0x0D,0xE8,0x7A,
    0xFA,0x01,0x00,0x49,0xC7,0x04,0x1E,0x00,0x00,0x00,0x00};
constexpr size_t PageSize=0x1000, PageOffset=0x7ED6B&(PageSize-1), JumpOffset=0x1C;
static_assert(PageOffset+sizeof(Find)<=PageSize,"pattern must not cross a page");

inline bool matches(const uint8_t *page) {
    return page && !memcmp(page+PageOffset,Find,sizeof(Find));
}
// Patch an already matched page in place. Returns true when bytes changed.
inline bool apply(uint8_t *page) {
    if(!matches(page))return false;
    page[PageOffset+JumpOffset]=0x90;page[PageOffset+JumpOffset+1]=0x90;
    return true;
}
// Pages in [data, data+size) that need the patch (validated range).
template<class F> size_t forEachMatch(const uint8_t *data,uint64_t size,F f) {
    size_t n=0;
    if(!data)return 0;
    for(uint64_t o=0;o+PageSize<=size;o+=PageSize)
        if(matches(data+o)){f(const_cast<uint8_t *>(data+o));++n;}
    return n;
}
}
#endif
