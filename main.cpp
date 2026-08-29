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
    return 0;
}
