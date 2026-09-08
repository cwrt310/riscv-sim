#include "Assembler.h"
#include <sstream>

const std::unordered_map<std::string, InsnDesc> insnTable = {
    {"addi", {Op::OP_IMM, 0, 0, 'I'}},
    {"andi", {Op::OP_IMM, 7, 0, 'I'}},
    {"add",  {Op::OP,     0, 0, 'R'}},
    {"sub",  {Op::OP,     0, 0x20, 'R'}},
    {"lui",  {Op::LUI,    0, 0, 'U'}},
    {"beq",  {Op::BRANCH, 0, 0, 'B'}},
    {"bne",  {Op::BRANCH, 1, 0, 'B'}},
    {"slli", {Op::OP_IMM, 1, 0, 'I'}},
    {"srli", {Op::OP_IMM, 5, 0, 'I'}},
    {"sll",  {Op::OP,     1, 0, 'R'}},
    {"xor",  {Op::OP,     4, 0, 'R'}},
    {"srl",  {Op::OP,     5, 0, 'R'}},
    {"or",   {Op::OP,     6, 0, 'R'}},
    {"and",  {Op::OP,     7, 0, 'R'}},
    {"slti", {Op::OP_IMM, 2, 0, 'I'}},
    {"sltiu",{Op::OP_IMM,3 ,0 , 'I'}},
    {"xori",{Op::OP_IMM ,4 ,0 , 'I'}},
    {"ori",{Op::OP_IMM ,6 ,0 , 'I'}},
    {"jal",{Op::JAL ,0 ,0 , 'J'}},
    {"lw",{Op::LOAD ,2 ,0 , 'M'}},
    {"sw",{Op::STORE ,2 ,0 , 'S'}},
    {"lb",{Op::LOAD ,0 ,0 , 'M'}},
    {"sb",{Op::STORE ,0 ,0 , 'S'}},
    {"lh",{Op::LOAD ,1 ,0 , 'M'}},
    {"sh",{Op::STORE ,1 ,0 , 'S'}},

    {"jalr",    {Op::JALR,  0,0,'I'}},
    {"auipc",   {Op::AUIPC, 0,0,'I'}},
    {"sra",     {Op::OP,    5,0,'R'}},
    {"slt",     {Op::OP,    2,0,'R'}},
    {"sltu",    {Op::OP,    3,0,'R'}},
    {"lbu",     {Op::LOAD,  4,0,'I'}},
    {"lhu",     {Op::LOAD,  5,0,'I'}},
    {"blt",     {Op::BRANCH,4,0,'B'}},
    {"bge",     {Op::BRANCH,5,0,'B'}},
    {"bltu",    {Op::BRANCH,6,0,'B'}},
    {"bgeu",    {Op::BRANCH,7,0,'B'}}
};

namespace {
    // 去首尾空白
    std::string trim(const std::string& s) {
        size_t a = s.find_first_not_of(" \t\r\n");
        size_t b = s.find_last_not_of(" \t\r\n");
        if (a == std::string::npos) return "";
        return s.substr(a, b - a + 1);
    }

    // "x1, x0, 5" → ["x1","x0","5"]
    std::vector<std::string> splitArgs(const std::string& s) {
        std::vector<std::string> out;
        std::stringstream ss(s);
        std::string tok;
        while (std::getline(ss, tok, ',')) out.push_back(trim(tok));
        return out;
    }

    // 解析立即数（支持十进制、0x 十六进制、负数）
    i32 parseImm(const std::string& s) {
        return std::stoi(s, nullptr, 0);
    }

    u32 encodeR(const InsnDesc& desc,int rd,int rs1,int rs2){
        return (u32(desc.funct7 << 25)) | (u32(rs2) << 20) | (u32(rs1) << 15) | (u32(desc.funct3 << 12)) | (u32(rd) << 7) | desc.opcode;  
    }

    u32 encodeI(const InsnDesc& desc,int rd,int rs1,i32 imm){
        return (u32(imm) & 0xFFF) << 20 | (u32(rs1) << 15) | (u32(desc.funct3 << 12)) | (u32(rd) << 7) | desc.opcode;
    }
    u32 encodeB(const InsnDesc& desc,int rs1,int rs2,i32 imm){
        u32 u =u32(imm);
        return ((u>>12 & 0x1)<<31|(u>>5 & 0x3F)<<25 |u32(rs2) <<20|u32(rs1)<<15|u32(desc.funct3)<<12| ((u>>1 & 0xF)<<8)|((u>>11 & 0x1)<<7)|desc.opcode);
    }
    u32 encodeS(const InsnDesc& desc,int rs1,int rs2,i32 imm){
        u32 u =u32(imm);
        return ((u>>5 & 0x7F)<<25|u32(rs2)<<20|u32(rs1)<<15|u32(desc.funct3)<<12|((u & 0x1F)<<7)|desc.opcode);
    }  
    u32 encodeU(const InsnDesc& desc,int rd,i32 imm){
        return (u32(imm) & 0xFFFFF)<<12|u32(rd)<<7|desc.opcode;
    }
    u32 encodeJ(const InsnDesc& desc,int rd,i32 imm){
        u32 u =u32(imm);
        return ((u>>20 & 0x1)<<31|((u>>1) & 0x3FF)<<21|((u>>11) & 0x1)<<20|((u>>12) & 0xFF)<<12|u32(rd)<<7|desc.opcode);
    }
    std::pair<i32,int> encodehelp(const std::string& s){
        size_t pos = s.find('(');
        if (pos == std::string::npos) throw std::runtime_error("invalid format");
        i32 imm = parseImm(s.substr(0, pos));
        int rs1 = regNum(s.substr(pos + 1, s.length() - pos - 2));
        return {imm, rs1};
    }
}

u32 Assembler::assembleLine(const std::string& line) {
    std::string l = trim(line);
    if (l.empty() || l[0] == '#') return 0;   // 空行 / 注释

    std::stringstream ss(l);
    std::string mnemonic, rest;
    ss >> mnemonic;
    std::getline(ss, rest);
    auto a = splitArgs(rest);
    auto it =insnTable.find(mnemonic);
    if (it != insnTable.end()) {
        const InsnDesc&desc = it->second;
        if(desc.format == 'R'){
            int rd = regNum(a[0]), rs1 = regNum(a[1]), rs2 = regNum(a[2]);
            return encodeR(desc,rd,rs1,rs2);
        }
        if(desc.format == 'I'){
            int rd = regNum(a[0]), rs1 = regNum(a[1]);
            i32 imm = parseImm(a[2]);
            return encodeI(desc,rd,rs1,imm);
        }
        if(desc.format == 'B'){
            int rs1 = regNum(a[0]), rs2 = regNum(a[1]);
            i32 imm = parseImm(a[2]);
            return encodeB(desc,rs1,rs2,imm);
        }
        if(desc.format == 'S'){
            auto pair = encodehelp(a[1]);
            int rs1 = pair.second, rs2 = regNum(a[0]);
            i32 imm = pair.first;
            return encodeS(desc,rs1,rs2,imm);
        }
        if(desc.format == 'U'){
            int rd = regNum(a[0]);
            i32 imm = parseImm(a[1]);
            return encodeU(desc,rd,imm);
        }
        if(desc.format == 'J'){
            int rd = regNum(a[0]);
            i32 imm = parseImm(a[1]);
            return encodeJ(desc,rd,imm);
        }
        if(desc.format == 'M'){
            auto pair = encodehelp(a[1]);
            int rs1 = pair.second, rd = regNum(a[0]);
            i32 imm = pair.first;
            return encodeI(desc,rd,rs1,imm);
        }
    }
    throw std::runtime_error("unknown instruction: " + mnemonic);
}

std::vector<u32> Assembler::assemble(const std::string& src) {
    std::vector<u32> out;
    std::stringstream ss(src);
    std::string line;
    while (std::getline(ss, line)) {
        std::string l = trim(line);
        if (l.empty() || l[0] == '#') continue;
        out.push_back(assembleLine(l));
    }
    return out;
}

std::string Assembler::disassemble(u32 inst) const {
    u32 opcode = bits(inst, 6, 0);
    int rd  = bits(inst, 11, 7);
    int rs1 = bits(inst, 19, 15);
    int f3  = bits(inst, 14, 12);

    if (opcode == Op::OP_IMM && f3 == 0)
        return "addi x" + std::to_string(rd) + ", x" + std::to_string(rs1) + ", " +
               std::to_string(signExtend(bits(inst, 31, 20), 12));
    if (opcode == Op::OP && f3 == 0) {
        int rs2 = bits(inst, 24, 20);
        bool sub = (bits(inst, 31, 25) == 0x20);
        return std::string(sub ? "sub " : "add ") + "x" + std::to_string(rd) +
               ", x" + std::to_string(rs1) + ", x" + std::to_string(rs2);
    }
    if (opcode == Op::LUI)
        return "lui x" + std::to_string(rd) + ", " +
               std::to_string(int((inst & 0xFFFFF000u) >> 12));
    return "?";
}
