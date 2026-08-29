#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <array>
#include <stdexcept>
#include <iostream>
#include <unordered_map>

// ---- 全组共用的类型别名 ----
using u32 = uint32_t;   // 机器码 / 寄存器值
using i32 = int32_t;    // 有符号立即数 / 算术
using u16 = uint16_t;   // 16 位无符号（loadHalf/storeHalf 用）
using u8  = uint8_t;    // 内存字节

constexpr int REG_COUNT = 32;   // x0 ~ x31

// ---- RISC-V 主 opcode（机器码最低 7 位）----
namespace Op {
    constexpr u32 OP_IMM = 0b0010011; // addi / andi / ori ...
    constexpr u32 OP     = 0b0110011; // add  / sub  / and ...
    constexpr u32 LUI    = 0b0110111;
    constexpr u32 AUIPC  = 0b0010111;
    constexpr u32 JAL    = 0b1101111;
    constexpr u32 JALR   = 0b1100111;
    constexpr u32 BRANCH = 0b1100011;
    constexpr u32 LOAD   = 0b0000011;
    constexpr u32 STORE  = 0b0100011;
}

// ---- 寄存器名 → 编号（支持 x0 和 ABI 名）----
inline int regNum(const std::string& name) {
    if (name.empty()) throw std::runtime_error("empty register name");
    if (name[0] == 'x') return std::stoi(name.substr(1));
    static const std::unordered_map<std::string, int> abi = {
        {"zero",0},{"ra",1},{"sp",2},{"gp",3},{"tp",4},
        {"t0",5},{"t1",6},{"t2",7},{"s0",8},{"fp",8},{"s1",9},
        {"a0",10},{"a1",11},{"a2",12},{"a3",13},{"a4",14},{"a5",15},
        {"a6",16},{"a7",17},{"s2",18},{"s3",19},{"s4",20},{"s5",21},
        {"s6",22},{"s7",23},{"s8",24},{"s9",25},{"s10",26},{"s11",27},
        {"t3",28},{"t4",29},{"t5",30},{"t6",31},
    };
    auto it = abi.find(name);
    if (it == abi.end()) throw std::runtime_error("unknown register: " + name);
    return it->second;
}

// ---- 位操作小工具 ----
inline u32 bits(u32 x, int hi, int lo) {
    return (x >> lo) & ((1u << (hi - lo + 1)) - 1);
}

// 把 n 位的立即数字段符号扩展到 32 位有符号数
inline i32 signExtend(u32 val, int n) {
    u32 sign = 1u << (n - 1);
    return i32((val ^ sign) - sign);
}
