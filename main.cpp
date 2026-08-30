#include <cstdint>
#include "CPU.h"
#include "Assembler.h"
#include <iostream>

int main() {
    Assembler as;
    CPU cpu;

    auto code = as.assemble(
        // ===== 算术 / 逻辑 / 移位（顺序执行）=====
        "addi x1, x0, 5\n"      // x1 = 5
        "addi x2, x0, 3\n"      // x2 = 3
        "add  x3, x1, x2\n"     // x3 = 8
        "sub  x4, x1, x2\n"     // x4 = 2
        "andi x5, x1, 1\n"      // x5 = 1
        "ori  x6, x1, 8\n"      // x6 = 13
        "xori x7, x1, 0xF\n"    // x7 = 10
        "slli x8, x1, 2\n"      // x8 = 20
        "slti x9, x1, 10\n"     // x9 = 1（5<10 真）
        "slti x10, x1, 3\n"     // x10 = 0（5<3 假）
        "lui  x11, 0x12345\n"   // x11 = 0x12345000
        "addi x12, x0, -1\n"    // x12 = -1（0xFFFFFFFF）
        "slti x13, x12, 1\n"    // x13 = 1（有符号：-1 < 1）
        "sltiu x14, x12, 1\n"   // x14 = 0（无符号：4294967295 < 1）

        // ===== 分支（beq/bne）=====
        "beq  x1, x1, 8\n"      // x1==x1 真 → 跳过下一条
        "addi x15, x0, 1\n"     // 跳过，x15 = 0
        "addi x16, x0, 99\n"    // x16 = 99

        "bne  x1, x2, 8\n"      // x1!=x2 真 → 跳过下一条
        "addi x17, x0, 1\n"     // 跳过，x17 = 0
        "addi x18, x0, 88\n"    // x18 = 88
        // ===== 跳转（jal）=====
        "jal  x19, 8\n"        // 跳到 +8，同时 x19 = 返回地址
        "addi x20, x0, 1\n"    // 被跳过，x20 = 0
        "addi x21, x0, 99\n"   // x21 = 99
    );
    cpu.loadProgram(code);
    cpu.run();

    // ===== 输出与期望值对照 =====
    std::cout << "=== 算术 / 逻辑 / 移位 ===\n";
    std::cout << "x3  = " << cpu.reg(3)  << "（期望 8）\n";
    std::cout << "x4  = " << cpu.reg(4)  << "（期望 2）\n";
    std::cout << "x5  = " << cpu.reg(5)  << "（期望 1，andi）\n";
    std::cout << "x6  = " << cpu.reg(6)  << "（期望 13，ori）\n";
    std::cout << "x7  = " << cpu.reg(7)  << "（期望 10，xori）\n";
    std::cout << "x8  = " << cpu.reg(8)  << "（期望 20，slli）\n";
    std::cout << "x9  = " << cpu.reg(9)  << "（期望 1，slti 真）\n";
    std::cout << "x10 = " << cpu.reg(10) << "（期望 0，slti 假）\n";
    std::cout << "x11 = 0x" << std::hex << cpu.reg(11) << std::dec << "（期望 0x12345000，lui）\n";
    std::cout << "x12 = " << cpu.reg(12) << "（期望 0xFFFFFFFF = -1）\n";
    std::cout << "x13 = " << cpu.reg(13) << "（期望 1，slti 有符号）\n";
    std::cout << "x14 = " << cpu.reg(14) << "（期望 0，sltiu 无符号）\n";

    std::cout << "=== 分支 ===\n";
    std::cout << "x15 = " << cpu.reg(15) << "（期望 0，beq 跳过了）\n";
    std::cout << "x16 = " << cpu.reg(16) << "（期望 99）\n";
    std::cout << "x17 = " << cpu.reg(17) << "（期望 0，bne 跳过了）\n";
    std::cout << "x18 = " << cpu.reg(18) << "（期望 88）\n";
    std::cout << "=== 跳转 ===\n";
    std::cout << "x19 = " << cpu.reg(19) << "（期望 84，jal 的返回地址）\n";
    std::cout << "x20 = " << cpu.reg(20) << "（期望 0，jal 跳过了）\n";
    std::cout << "x21 = " << cpu.reg(21) << "（期望 99）\n";
    return 0;
}
