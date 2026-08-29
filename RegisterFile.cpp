#include "RegisterFile.h"

RegisterFile::RegisterFile() {
    regs_.fill(0);
}

u32 RegisterFile::read(int n) const {
    if (n < 0 || n >= REG_COUNT) throw std::runtime_error("bad register index");
    return (n == 0) ? 0 : regs_[n];
}

void RegisterFile::write(int n, u32 value) {
    if (n == 0) return;   // x0 写不进去，这是 RISC-V 的硬性规定
    if (n < 0 || n >= REG_COUNT) throw std::runtime_error("bad register index");
    regs_[n] = value;
}

void RegisterFile::dump() const {
    for (int i = 1; i < REG_COUNT; ++i) {   // 跳过 x0
        if (regs_[i] != 0)
            std::cout << "x" << i << " = " << regs_[i] << "\n";
    }
}
