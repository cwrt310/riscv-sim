#include "Memory.h"

Memory::Memory(size_t size) : mem_(size, 0) {}

u32 Memory::loadProgram(const std::vector<u32>& code, u32 base) {
    for (size_t i = 0; i < code.size(); ++i)
        storeWord(base + u32(i * 4), code[i]);
    return base;
}

u8 Memory::loadByte(u32 addr) const {
    if (addr >= mem_.size()) throw std::runtime_error("memory read out of range");
    return mem_[addr];
}

u16 Memory::loadHalf(u32 addr) const {
    return u16(loadByte(addr)) | u16(loadByte(addr + 1)) << 8;   // 小端
}

u32 Memory::loadWord(u32 addr) const {
    return u32(loadByte(addr)) | u32(loadByte(addr + 1)) << 8 |
           u32(loadByte(addr + 2)) << 16 | u32(loadByte(addr + 3)) << 24;
}

void Memory::storeByte(u32 addr, u8 v) {
    if (addr >= mem_.size()) throw std::runtime_error("memory write out of range");
    mem_[addr] = v;
}

void Memory::storeHalf(u32 addr, u16 v) {
    storeByte(addr,     u8(v & 0xFF));
    storeByte(addr + 1, u8(v >> 8));
}

void Memory::storeWord(u32 addr, u32 v) {
    storeByte(addr,     u8(v & 0xFF));
    storeByte(addr + 1, u8((v >> 8)  & 0xFF));
    storeByte(addr + 2, u8((v >> 16) & 0xFF));
    storeByte(addr + 3, u8((v >> 24) & 0xFF));
}

void Memory::dump(u32 start, u32 len) const {
    for (u32 a = start; a < start + len; a += 4) {
        std::cout << "0x" << std::hex << a << " : 0x" << loadWord(a) << std::dec << "\n";
    }
}
