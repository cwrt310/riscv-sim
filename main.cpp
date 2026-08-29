#include <cstdint>
#include "CPU.h"
#include "Assembler.h"
#include <iostream>

int main() {
    
    Assembler as;
    CPU cpu;

    auto code = as.assemble(
        "addi x1, x0, 5\n"    // x1 = 5
        "addi x2, x0, 7\n"    // x2 = 7
        "add  x3, x1, x2\n"   // x3 = 12
        "sub  x4, x2, x1\n"   // x4 = 2
        "andi x5, x3, 0xF\n"    // x5 = 12 & 0xF = 12
        "addi x6, x0, 5\n"    // x6 = 5
        "addi x7, x0, 5\n"    // x7 = 5（相等）
        "beq  x6, x7, 8\n"    // beq 跳（x6==x7）
        "addi x8, x0, 1\n"    // 跳过 → x8 = 0
        "addi x9, x0, 99\n"   // x9 = 99
        "addi x10, x0, 7\n"   // x10 = 7（和 x6 不同）
        "bne  x6, x10, 8\n"   // bne 跳（x6 != x10）
        "addi x11, x0, 1\n"   // 跳过 → x11 = 0
        "addi x12, x0, 88\n"  // x12 = 88
    );
    cpu.loadProgram(code);

    std::cout << "=== 单步执行（前两条）===\n";
    cpu.step();
    std::cout << "pc=" << cpu.pc() << "  x1=" << cpu.reg(1) << "（期望 5）\n";
    cpu.step();
    std::cout << "pc=" << cpu.pc() << "  x2=" << cpu.reg(2) << "（期望 7）\n";

    std::cout << "=== 连续执行剩余指令 ===\n";
    cpu.run();

    std::cout << "x3=" << cpu.reg(3) << "（期望 12）\n";
    std::cout << "x4=" << cpu.reg(4) << "（期望 2）\n";
    std::cout << "x5=" << cpu.reg(5) << "（期望 12）\n";
    std::cout << "x6=" << cpu.reg(6) << "（期望 5）\n";
    std::cout << "x7=" << cpu.reg(7) << "（期望 5）\n";
    std::cout << "x8=" << cpu.reg(8) << "（期望 0）\n";
    std::cout << "x9=" << cpu.reg(9) << "（期望 99）\n";
    std::cout << "x10=" << cpu.reg(10) << "（期望 7）\n";
    std::cout << "x11=" << cpu.reg(11) << "（期望 0）\n";
    std::cout << "x12=" << cpu.reg(12) << "（期望 88）\n";
    return 0;
}
