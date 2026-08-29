#include "CPU.h"

CPU::CPU() {}

void CPU::loadProgram(const std::vector<u32>& code, u32 base) {
    pc_ = mem_.loadProgram(code, base);
    progEnd_ = base + u32(code.size() * 4);
    rf_ = RegisterFile();
}

void CPU::reset() {
    pc_ = 0;
    progEnd_ = 0;
    rf_ = RegisterFile();
    mem_ = Memory();
}

u32 CPU::fetch() const {
    return mem_.loadWord(pc_);
}

void CPU::step() {
    u32 inst = fetch();
    pc_ += 4;          // 先让 PC 指向下一条，再执行（RISC-V 约定）
    execute(inst);
}

void CPU::run(size_t maxSteps) {
    for (size_t i = 0; i < maxSteps && pc_ < progEnd_; ++i)
        step();
}

void CPU::execute(u32 inst) {
    u32 opcode = bits(inst, 6, 0);
    int rd     = int(bits(inst, 11, 7));
    int rs1    = int(bits(inst, 19, 15));
    int funct3 = int(bits(inst, 14, 12));

    switch (opcode) {
    case Op::OP_IMM: {   // I 型算术：addi 等
        i32 imm = signExtend(bits(inst, 31, 20), 12);
        switch (funct3) {
        case 0: rf_.write(rd, rf_.read(rs1) + imm); break;   // addi
        // TODO: andi / ori / xori / slti / slli ...
        case 7: rf_.write(rd, rf_.read(rs1) & imm); break;   // andi
        default:
            std::cerr << "未实现的 OP-IMM funct3=" << funct3 << "\n";
            throw std::runtime_error("unimplemented instruction");
        }
        break;
    }
    case Op::OP: {       // R 型：add / sub 等
        int rs2    = int(bits(inst, 24, 20));
        int funct7 = int(bits(inst, 31, 25));
        switch (funct3) {
        case 0:   // add / sub，靠 funct7 第 30 位区分
            rf_.write(rd, (funct7 == 0x20)
                              ? rf_.read(rs1) - rf_.read(rs2)
                              : rf_.read(rs1) + rf_.read(rs2));
            break;
        // TODO: sll / slt / xor / and / or ...
        default:
            std::cerr << "未实现的 OP funct3=" << funct3 << "\n";
            throw std::runtime_error("unimplemented instruction");
        }
        break;
    }
    case Op::LUI: {      // 加载高 20 位
        u32 imm = inst & 0xFFFFF000u;
        rf_.write(rd, imm);
        break;
    }
    // TODO: AUIPC / JAL / JALR / BRANCH / LOAD / STORE
    default:
        std::cerr << "未实现的 opcode=0x" << std::hex << opcode << std::dec << "\n";
        throw std::runtime_error("unimplemented instruction");
    }
}
