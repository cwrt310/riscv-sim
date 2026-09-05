#pragma once
#include "common.h"
#include "RegisterFile.h"
#include "Memory.h"

// CPU：取指 → 译码 → 执行 → 写回 的循环
class CPU {
private:
    u32 fetch() const;      // 取指：读 mem_[pc_]
    void execute(u32 inst); // 译码 + 执行 + 写回，一条指令在这里完成
    //见头文件
    RegisterFile rf_;
    Memory mem_;
    u32 pc_ = 0;
    u32 progEnd_ = 0;       // 程序结束地址，run() 靠它判断何时停
    u32 lastInst_ = 0;      // 最近执行的那条指令（界面高亮数据通路用）
public:
    CPU();

    void loadProgram(const std::vector<u32>& code, u32 base = 0);
    void step();                          // 单步执行一条指令
    void run(size_t maxSteps = 1000000);  // 连续执行到程序结束（maxSteps 防死循环）
    void reset();

    // —— 给“界面 / 测试”同学用的只读接口 ——
    u32 pc() const { return pc_; }
    u32 reg(int n) const { return rf_.read(n); }
    const Memory& mem() const { return mem_; }
    bool finished() const { return pc_ >= progEnd_; }   // 程序是否已执行完
    u32 inst() const { return lastInst_; }              // 最近执行的一条指令


};
