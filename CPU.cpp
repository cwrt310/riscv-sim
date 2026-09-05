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
    lastInst_ = 0;         // 清掉，界面高亮跟着熄灭
    rf_ = RegisterFile();
    mem_ = Memory();
}

u32 CPU::fetch() const {
    return mem_.loadWord(pc_);
}

void CPU::step() {
    u32 inst = fetch();
    lastInst_ = inst;  // 记下这条指令，界面好高亮它走过的部件
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
        case 1: rf_.write(rd, rf_.read(rs1) << (imm & 0x1F)); break; // slli
        case 2: rf_.write(rd, ((i32)rf_.read(rs1) < imm) ? 1 : 0); break; // slti
        case 3: rf_.write(rd, (rf_.read(rs1) < u32(imm)) ? 1 : 0); break; // sltiu
        case 4: rf_.write(rd, rf_.read(rs1) ^ imm); break;   // xori   
        case 5: rf_.write(rd, rf_.read(rs1) >> (imm & 0x1F)); break; // srli
        case 6: rf_.write(rd, rf_.read(rs1) | imm); break;   // ori 
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
        
        case 1: // sll
            rf_.write(rd, rf_.read(rs1) << (rf_.read(rs2) & 0x1F));
            break;
        case 4: // xor
            rf_.write(rd, rf_.read(rs1) ^ rf_.read(rs2));
            break;
        case 5: // srl
            if (funct7 != 0x00) {
                std::cerr << "未实现 sra（算术右移）\n";
                throw std::runtime_error("unimplemented instruction");
            }
            rf_.write(rd, rf_.read(rs1) >> (rf_.read(rs2) & 0x1F));
            break;
        case 6: // or
            rf_.write(rd, rf_.read(rs1) | rf_.read(rs2));
            break;
        case 7: // and
            rf_.write(rd, rf_.read(rs1) & rf_.read(rs2));
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
    case Op::BRANCH:{
        int rs2 = int(bits(inst,24,20));
        i32 imm = signExtend((bits(inst,31,31)<<12) | (bits(inst,7,7)<<11) | (bits(inst,30,25)<<5) | (bits(inst,11,8)<<1),13);
        switch (funct3) {
            case 0: // beq
                if(rf_.read(rs1) == rf_.read(rs2)){
                    pc_ = pc_ - 4 + imm; // pc_已经+4了，所以这里要减去4
                }
                break;
            case 1: // bne
                if(rf_.read(rs1) != rf_.read(rs2)){
                    pc_ = pc_ - 4 + imm;
                }
                break;
            default:
                std::cerr << "未实现的 BRANCH funct3=" << funct3 << "\n";
                throw std::runtime_error("unimplemented instruction");
        }
        break;
    }
    case Op::JAL:{
        i32 imm = signExtend((bits(inst,31,31)<<20) | (bits(inst,30,21)<<1) | (bits(inst,20,20)<<11) | (bits(inst,19,12)<<12),21);
        rf_.write(rd, pc_ );
        pc_ = pc_ - 4 + imm;
        break;
    }
    case Op::STORE:{
        int rs2 = int(bits(inst,24,20));
        i32 imm = signExtend((bits(inst,31,25)<<5)|(bits(inst,11,7)),12);
        switch (funct3) {
            case 0: // sb
                mem_.storeByte(rf_.read(rs1) + u32(imm), u8(rf_.read(rs2) & 0xFF));
                break;
            case 1: // sh
                mem_.storeHalf(rf_.read(rs1) + u32(imm), u16(rf_.read(rs2) & 0xFFFF));
                break;
            case 2: // sw
                mem_.storeWord(rf_.read(rs1) + u32(imm), u32(rf_.read(rs2)));
                break;
            default:
                std::cerr << "未实现的 STORE funct3=" << funct3 << "\n";
                throw std::runtime_error("unimplemented instruction");
        }
        break;
    }
    case Op::LOAD:{
        i32 imm = signExtend(bits(inst,31,20),12);
        switch (funct3) {
            case 0: // lb
                rf_.write(rd, signExtend(mem_.loadByte(rf_.read(rs1) + u32(imm)),8));
                break;
            case 1: // lh
                rf_.write(rd, signExtend(mem_.loadHalf(rf_.read(rs1) + u32(imm)),16));
                break;
            case 2: // lw
                rf_.write(rd, mem_.loadWord(rf_.read(rs1) + u32(imm)));
                break;
            default:
                std::cerr << "未实现的 LOAD funct3=" << funct3 << "\n";
                throw std::runtime_error("unimplemented instruction");
        }
        break;
    }
    // TODO: AUIPC / JAL / JALR / BRANCH / LOAD / STORE
    default:
        std::cerr << "未实现的 opcode=0x" << std::hex << opcode << std::dec << "\n";
        throw std::runtime_error("unimplemented instruction");
    }
}
