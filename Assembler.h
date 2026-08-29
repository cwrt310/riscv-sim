#pragma once
#include "common.h"

// 汇编器：汇编文本 ⇆ 机器码（目前只支持 addi/add/sub/lui）
class Assembler {
public:
    u32 assembleLine(const std::string& line);          // 一行 → 机器码
    std::vector<u32> assemble(const std::string& src);  // 整段文本 → 机器码
    std::string disassemble(u32 inst) const;            // 机器码 → 汇编文本（调试用）
};
