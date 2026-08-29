#pragma once
#include "common.h"

// 按字节寻址的内存，支持 8/16/32 位小端读写
class Memory {
private:
    std::vector<u8> mem_;
public:
    explicit Memory(size_t size = 1u << 20);   // 默认 1 MB

    // 把机器码写入内存，返回起始地址
    u32 loadProgram(const std::vector<u32>& code, u32 base = 0);

    u8  loadByte(u32 addr) const;
    u16 loadHalf(u32 addr) const;   // 小端
    u32 loadWord(u32 addr) const;   // 小端

    void storeByte(u32 addr, u8  v);
    void storeHalf(u32 addr, u16 v);
    void storeWord(u32 addr, u32 v);

    void dump(u32 start, u32 len) const;   // 调试打印一段内存


};
