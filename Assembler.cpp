#include "Assembler.h"
#include <sstream>

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
}

u32 Assembler::assembleLine(const std::string& line) {
    std::string l = trim(line);
    if (l.empty() || l[0] == '#') return 0;   // 空行 / 注释

    std::stringstream ss(l);
    std::string mnemonic, rest;
    ss >> mnemonic;
    std::getline(ss, rest);
    auto a = splitArgs(rest);

    if (mnemonic == "addi") {
        int rd = regNum(a[0]), rs1 = regNum(a[1]);
        i32 imm = parseImm(a[2]);
        return (u32(imm) & 0xFFF) << 20 | u32(rs1) << 15 | u32(rd) << 7 | Op::OP_IMM;
    }
    if (mnemonic == "andi"){
        int rd = regNum(a[0]),rs1 = regNum(a[1]);
        i32 imm = parseImm(a[2]);
        return (u32(imm) & 0xFFF) << 20 | u32(rs1) << 15 | 7u << 12 | u32(rd) << 7 | Op::OP_IMM;
    }
    if (mnemonic == "add" || mnemonic == "sub") {
        int rd = regNum(a[0]), rs1 = regNum(a[1]), rs2 = regNum(a[2]);
        u32 funct7 = (mnemonic == "sub") ? 0x20u : 0x00u;
        return funct7 << 25 | u32(rs2) << 20 | u32(rs1) << 15 | u32(rd) << 7 | Op::OP;
    }
    if (mnemonic == "lui") {
        int rd = regNum(a[0]);
        i32 imm = parseImm(a[1]);
        return (u32(imm) & 0xFFFFF) << 12 | u32(rd) << 7 | Op::LUI;
    }
    if (mnemonic == "beq" || mnemonic == "bne") {
        int rs1 = regNum(a[0]), rs2 = regNum(a[1]);
        i32 imm = parseImm(a[2]);
        u32 funct3 = (mnemonic == "beq") ? 0u : 1u;
        return ((imm >> 12) & 0x1) << 31 | ((imm >> 5) & 0x3F) << 25 | u32(rs2) << 20 | u32(rs1) << 15 | funct3 << 12 | ((imm >> 1) & 0xF) << 8 | ((imm >> 11) & 0x1) << 7 | Op::BRANCH;
    }
    if (mnemonic == "slli") {
        int rd = regNum(a[0]), rs1 = regNum(a[1]);
        i32 imm = parseImm(a[2]);
        return (u32(imm) & 0x1F) << 20 | u32(rs1) << 15 | 1u << 12 | u32(rd) << 7 | Op::OP_IMM;
    }
    if (mnemonic == "slti") {
        int rd = regNum(a[0]), rs1 = regNum(a[1]);
        i32 imm = parseImm(a[2]);
        return (u32(imm) & 0xFFF) << 20 | u32(rs1) << 15 | 2u << 12 | u32(rd) << 7 | Op::OP_IMM;
    }
    if (mnemonic == "sltiu") {
        int rd = regNum(a[0]), rs1 = regNum(a[1]);
        i32 imm = parseImm(a[2]);
        return (u32(imm) & 0xFFF) << 20 | u32(rs1) << 15 | 3u << 12 | u32(rd) << 7 | Op::OP_IMM;
    }
    if (mnemonic == "xori") {
        int rd = regNum(a[0]), rs1 = regNum(a[1]);
        i32 imm = parseImm(a[2]);
        return (u32(imm) & 0xFFF) << 20 | u32(rs1) << 15 | 4u << 12 | u32(rd) << 7 | Op::OP_IMM;
    }
    if (mnemonic == "ori") {
        int rd = regNum(a[0]), rs1 = regNum(a[1]);
        i32 imm = parseImm(a[2]);
        return (u32(imm) & 0xFFF) << 20 | u32(rs1) << 15 | 6u << 12 | u32(rd) << 7 | Op::OP_IMM;
    }
    if (mnemonic == "jal") {
        int rd = regNum(a[0]);
        i32 imm = parseImm(a[1]);
        return ((imm >> 20) & 0x1) << 31 | ((imm >> 1) & 0x3FF) << 21 | ((imm >> 11) & 0x1) << 20 | ((imm >> 12) & 0xFF) << 12 | u32(rd) << 7 | Op::JAL;
    }
    if (mnemonic == "lw") {
        int rd = regNum(a[0]);
        std::string offsetBase = a[1];
        size_t pos = offsetBase.find('(');
        if (pos == std::string::npos) throw std::runtime_error("invalid lw format");
        i32 imm = parseImm(offsetBase.substr(0, pos));
        int rs1 = regNum(offsetBase.substr(pos + 1, offsetBase.length() - pos - 2));
        return (u32(imm) & 0xFFF) << 20 | u32(rs1) << 15 | 2u << 12 | u32(rd) << 7 | Op::LOAD;
    }
    if (mnemonic == "sw") {
        int rs2 = regNum(a[0]);
        std::string offsetBase = a[1];
        size_t pos = offsetBase.find('(');
        if (pos == std::string::npos) throw std::runtime_error("invalid sw format");
        i32 imm = parseImm(offsetBase.substr(0, pos));
        int rs1 = regNum(offsetBase.substr(pos + 1, offsetBase.length() - pos - 2));
        u32 u = u32(imm);
        return ((u >> 5) & 0x7F) << 25 |    // imm[11:5] → bits 31:25
                u32(rs2) << 20 |             // rs2 → bits 24:20
                u32(rs1) << 15 |             // rs1 → bits 19:15
                2u << 12 |                   // funct3=2 → bits 14:12
                ((u & 0x1F) << 7) |          // imm[4:0] → bits 11:7
                Op::STORE;
    }
    if (mnemonic == "lb"){
        int rd = regNum(a[0]);
        std::string offsetBase = a[1];
        size_t pos = offsetBase.find('(');
        if (pos == std::string::npos) throw std::runtime_error("invalid lb format");
        i32 imm = parseImm(offsetBase.substr(0, pos));
        int rs1 = regNum(offsetBase.substr(pos + 1, offsetBase.length() - pos - 2));
        return (u32(imm) & 0xFFF) << 20 | u32(rs1) << 15 | 0u << 12 | u32(rd) << 7 | Op::LOAD;
    }
    if (mnemonic == "sb"){
        std::string offsetBase = a[1];
        size_t pos = offsetBase.find('(');
        if (pos == std::string::npos) throw std::runtime_error("invalid sb format");
        i32 imm = parseImm(offsetBase.substr(0, pos));
        int rs1 = regNum(offsetBase.substr(pos + 1, offsetBase.length() - pos - 2));
        int rs2 = regNum(a[0]);
        u32 u = u32(imm);
        return ((u >> 5) & 0x7F) << 25 |    // imm[11:5] → bits 31:25
                u32(rs2) << 20 |             // rs2 → bits 24:20
                u32(rs1) << 15 |             // rs1 → bits 19:15
                0u << 12 |                   // funct3=0 → bits 14:12
                ((u & 0x1F) << 7) |          // imm[4:0] → bits 11:7
                Op::STORE;
    }
    if (mnemonic == "lh"){
        int rd = regNum(a[0]);
        std::string offsetBase = a[1];
        size_t pos = offsetBase.find('(');
        if (pos == std::string::npos) throw std::runtime_error("invalid lh format");
        i32 imm = parseImm(offsetBase.substr(0, pos));
        int rs1 = regNum(offsetBase.substr(pos + 1, offsetBase.length() - pos - 2));
        return (u32(imm) & 0xFFF) << 20 | u32(rs1) << 15 | 1u << 12 | u32(rd) << 7 | Op::LOAD;
    }
    if (mnemonic == "sh"){
        std::string offsetBase = a[1];
        size_t pos = offsetBase.find('(');
        if (pos == std::string::npos) throw std::runtime_error("invalid sh format");
        i32 imm = parseImm(offsetBase.substr(0, pos));
        int rs1 = regNum(offsetBase.substr(pos + 1, offsetBase.length() - pos - 2));
        int rs2 = regNum(a[0]);
        u32 u = u32(imm);
        return ((u >> 5) & 0x7F) << 25 |    // imm[11:5] → bits 31:25
                u32(rs2) << 20 |             // rs2 → bits 24:20
                u32(rs1) << 15 |             // rs1 → bits 19:15
                1u << 12 |                   // funct3=1 → bits 14:12
                ((u & 0x1F) << 7) |          // imm[4:0] → bits 11:7
                Op::STORE;
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
