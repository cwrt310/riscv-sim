#include "CPU.h"

namespace {
    void executeOP_IMM(CPU& cpu, u32 inst) {
        int rd     = int(bits(inst, 11, 7));
        int rs1    = int(bits(inst, 19, 15));
        int funct3 = int(bits(inst, 14, 12));
        i32 imm = signExtend(bits(inst, 31, 20), 12);
        switch (funct3) {
        case 0: cpu.setreg(rd, cpu.reg(rs1) + imm); break;   // addi
        case 1: cpu.setreg(rd, cpu.reg(rs1) << (imm & 0x1F)); break; // slli
        case 2: cpu.setreg(rd, ((i32)cpu.reg(rs1) < imm) ? 1 : 0); break; // slti
        case 3: cpu.setreg(rd, (cpu.reg(rs1) < u32(imm)) ? 1 : 0); break; // sltiu
        case 4: cpu.setreg(rd, cpu.reg(rs1) ^ imm); break;   // xori
        case 5:
            if(bits(imm,11,5) == 0x00 ){
                cpu.setreg(rd, u32(cpu.reg(rs1)) >> u32(imm & 0x1F)); /*srli*/
            }else{
                cpu.setreg(rd, signExtend(32-u32(imm & 0x1f) , u32(cpu.reg(rs1)) >> u32(imm & 0x1f)) );   /*srai*/
            }
            break;
        case 6: cpu.setreg(rd, cpu.reg(rs1) | imm); break;   // ori 
        case 7: cpu.setreg(rd, cpu.reg(rs1) & imm); break;   // andi
        default:
            std::cerr << "未实现的 OP-IMM funct3=" << funct3 << "\n";
            throw std::runtime_error("unimplemented instruction");
        }
    }
    void executeOP(CPU& cpu, u32 inst) {
        int rd     = int(bits(inst, 11, 7));
        int rs1    = int(bits(inst, 19, 15));
        int rs2    = int(bits(inst, 24, 20));
        int funct3 = int(bits(inst, 14, 12));
        int funct7 = int(bits(inst, 31, 25));
        switch (funct3) {
        case 0:   // add / sub，靠 funct7 第 30 位区分
            cpu.setreg(rd, (funct7 == 0x20)
                              ? cpu.reg(rs1) - cpu.reg(rs2)
                              : cpu.reg(rs1) + cpu.reg(rs2));
            break;
        
        case 1: // sll
            cpu.setreg(rd, cpu.reg(rs1) << (cpu.reg(rs2) & 0x1F));
            break;
        case 2: /*slt*/
            cpu.setreg(rd, (cpu.reg(rs1) < cpu.reg(rs2)) ? 1:0);
            break;
        case 3: /*sltu*/
            cpu.setreg(rd, (u32(cpu.reg(rs1)) < u32(cpu.reg(rs2))) ? 1:0);
            break;

        case 4: // xor
            cpu.setreg(rd, cpu.reg(rs1) ^ cpu.reg(rs2));
            break;
        case 5: // srl / sra  靠 funct7 第 30 位区分
            if (funct7  == 0x00) {
                cpu.setreg(rd, u32(cpu.reg(rs1)) >> u32(cpu.reg(rs2) & 0x1F));  /*srl zero extend*/
            }else{
                cpu.setreg(rd, signExtend(cpu.reg(rs1) >> u32(cpu.reg(rs2) & 0x1F) , 32 - u32(cpu.reg(rs2) & 0x1F) ) );
            }
            break;
        case 6: // or
            cpu.setreg(rd, cpu.reg(rs1) | cpu.reg(rs2));
            break;
        case 7: // and
            cpu.setreg(rd, cpu.reg(rs1) & cpu.reg(rs2));
            break;
        default:
            std::cerr << "未实现的 OP funct3=" << funct3 << "\n";
            throw std::runtime_error("unimplemented instruction");
        }
    }
    void executeLUI(CPU& cpu, u32 inst) {
        int rd = int(bits(inst, 11, 7));
        u32 imm = inst & 0xFFFFF000u;
        cpu.setreg(rd, imm);
    }
    void executeAUIPC(CPU& cpu, u32 inst){
        int rd = int(bits(inst, 11, 7));
        u32 imm = inst & 0xFFFFF000u;
        cpu.setreg(rd,cpu.pc()+imm);
    }

    void executeJAL(CPU& cpu, u32 inst) {
        int rd = int(bits(inst, 11, 7));
        i32 imm = signExtend((bits(inst,31,31)<<20) | (bits(inst,30,21)<<1)
                           | (bits(inst,20,20)<<11) | (bits(inst,19,12)<<12), 21);
        cpu.setreg(rd, cpu.pc());
        cpu.setpc(cpu.pc() - 4 + imm);
    }
    void executeJALR(CPU& cpu, u32 inst) {
        int rd = int(bits(inst, 11, 7));
        int rs1    = int(bits(inst, 19, 15));
        i32 imm = signExtend((bits(inst,31,31)<<20) | (bits(inst,30,21)<<1)
                           | (bits(inst,20,20)<<11) | (bits(inst,19,12)<<12), 21);
        cpu.setreg(rd, cpu.pc()+1);
        cpu.setpc(cpu.reg(rs1)+ imm);
    }
    void executeBRANCH(CPU& cpu, u32 inst) {
        int rs1 = int(bits(inst, 19, 15));
        int rs2 = int(bits(inst, 24, 20));
        i32 imm = signExtend((bits(inst,31,31)<<12) | (bits(inst,7,7)<<11) | (bits(inst,30,25)<<5) |
                             (bits(inst,11,8)<<1), 13);
        int funct3 = int(bits(inst, 14, 12));
        bool takeBranch = false;
        switch (funct3) {
        case 0: takeBranch = (cpu.reg(rs1) == cpu.reg(rs2)); break; // beq
        case 1: takeBranch = (cpu.reg(rs1) != cpu.reg(rs2)); break; // bne
        default:
            std::cerr << "未实现的 BRANCH funct3=" << funct3 << "\n";
            throw std::runtime_error("unimplemented instruction");
        }
        if (takeBranch)
            cpu.setpc(cpu.pc() - 4 + imm);
    }

    void executeLOAD(CPU& cpu, u32 inst) {
        int rd = int(bits(inst, 11, 7));
        int rs1 = int(bits(inst, 19, 15));
        i32 imm = signExtend(bits(inst, 31, 20), 12);
        int funct3 = int(bits(inst, 14, 12));
        u32 addr = cpu.reg(rs1) + imm;
        switch (funct3) {
        case 0: // lb
            cpu.setreg(rd, signExtend(cpu.getmem().loadByte(addr), 8));
            break;
        case 1: // lh
            cpu.setreg(rd, signExtend(cpu.getmem().loadHalf(addr), 16));
            break;
        case 2: // lw
            cpu.setreg(rd, cpu.getmem().loadWord(addr));
            break;

        case 4: /*lbu*/
            cpu.setreg(rd, u32(cpu.getmem().loadByte(addr)));
            break;
        case 5: /*lhu*/
            cpu.setreg(rd, u32(cpu.getmem().loadHalf(addr)));
            break;
        default:
            std::cerr << "未实现的 LOAD funct3=" << funct3 << "\n";
            throw std::runtime_error("unimplemented instruction");
        }
    }
    void executeSTORE(CPU& cpu, u32 inst) {
        int rs1    = int(bits(inst, 19, 15));
        int rs2    = int(bits(inst, 24, 20));
        int funct3 = int(bits(inst, 14, 12));
        i32 imm = signExtend((bits(inst,31,25)<<5)|(bits(inst,11,7)), 12);
        u32 addr = cpu.reg(rs1) + u32(imm);
        switch (funct3) {
        case 0: cpu.getmem().storeByte(addr, u8(cpu.reg(rs2) & 0xFF)); break;   // sb
        case 1: cpu.getmem().storeHalf(addr, u16(cpu.reg(rs2) & 0xFFFF)); break; // sh
        case 2: cpu.getmem().storeWord(addr, u32(cpu.reg(rs2))); break;         // sw
        default:
            std::cerr << "未实现的 STORE funct3=" << funct3 << "\n";
            throw std::runtime_error("unimplemented instruction");
        }
    }
    void executeILLEGAL(CPU&, u32) {
        throw std::runtime_error("unimplemented instruction");
    }
}

CPU::CPU() {
    static bool inited = false;     // static：只初始化一次
    if (!inited) {
        initHandlers();
        inited = true;
    }
}

CPU::handler CPU::handlers[128];

void CPU::initHandlers() {
    for (int i = 0; i < 128; ++i)
        handlers[i] = executeILLEGAL;     
    handlers[Op::OP_IMM] = executeOP_IMM;
    handlers[Op::OP]     = executeOP;
    handlers[Op::LUI]    = executeLUI;
    handlers[Op::BRANCH] = executeBRANCH;
    handlers[Op::JAL]    = executeJAL;
    handlers[Op::LOAD]   = executeLOAD;
    handlers[Op::STORE]  = executeSTORE;
}

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
    handlers[bits(inst, 6, 0)] (*this, inst);
}
