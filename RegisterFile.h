#pragma once
#include "common.h"

// 32 个 32 位寄存器，x0 硬件规定恒为 0
class RegisterFile {
private:
    std::array<u32, REG_COUNT> regs_{};
public:
    RegisterFile();

    u32  read(int n) const;         // 读 x[n]；读 x0 永远返回 0
    void write(int n, u32 value);   // 写 x[n]；写 x0 直接忽略

    void dump() const;              // 调试：打印所有非零寄存器

};
