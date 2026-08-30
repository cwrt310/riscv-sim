# RISC-V 模拟器（C++ 课程设计 · 起点工程）

最小可跑的 RV32I 子集模拟器，作为 6 人团队协作的起始代码。

## 架构

```
汇编文本 ──Assembler──▶ 机器码 ──Memory──▶ CPU 取指/执行 ──▶ 寄存器/内存状态
```

四个核心类 + 一个公共头：

| 文件 | 职责 | 认领 |
|---|---|---|
| common.h | 类型别名、opcode 常量、位操作工具 | 全组共用 |
| RegisterFile.* | 32 个寄存器，x0 恒 0 | 执行组 A |
| Memory.* | 字节寻址内存，小端读写 | 内存组 |
| CPU.* | 取指→译码→执行→写回 | 执行组 B |
| Assembler.* | 汇编文本 ⇆ 机器码 | 汇编器组 |
| main.cpp + CMakeLists | 演示 + 构建 | 集成负责人 |

## 已实现

**指令（13 条，全部测试通过）**：

`addi` `slli` `slti` `sltiu` `xori` `ori` `andi` `add` `sub` `lui` `beq` `bne` `jal`

**测试程序**：斐波那契 F(10)=55 循环程序已跑通（源码在 `workload.txt`）。

**Qt 图形界面**：打开文件 / 单步 / 运行 / 重置 / 帮助按钮；寄存器表（x0~x31）、内存表（随程序大小动态显示）、PC、输出日志。

## 待补

- **CPU.cpp**：访存 `lw/sw`、`jalr`、`auipc`，以及其余指令（移位 / 比较 / 字节半字访存 / 其余分支）
- **Assembler.cpp**：标签跳转（`loop:`）、伪指令（`mv` / `li` / `j` / `nop`）
- **界面**：数据通路图 / 流水线动画（加分项）

## 构建说明

### 前置依赖
- 编译器：g++（Windows 用 MinGW，Linux/WSL 自带）
- 构建工具：CMake ≥ 3.10
- Qt 6（Widgets）：图形界面需要

### Windows（生成 .exe）

用 Qt 自带的 MinGW（或 MSYS2 的 MinGW）：

```bat
cmake -B build -G "MinGW Makefiles"
cmake --build build
build\riscv-sim.exe
```

### Linux / WSL

```bash
cmake -B build
cmake --build build
./build/riscv-sim
```

### 界面使用

点「打开」选 `workload.txt` → 点「运行」执行 → 寄存器表看 x2=34、x3=55（斐波那契 F(9)/F(10)）；点「单步」逐条看 PC 和寄存器变化；点「帮助」看已实现指令列表。

### 生成可独立分发的 exe（Release + 打包）

1. Qt Creator 左下角切到 **Release** 编译；
2. Git Bash 里执行（Qt 装在 E 盘，路径按实际改）：

```bash
cd /c/Users/cwrt/Desktop/riscv-sim/build/Desktop_Qt_6_11_2_MinGW_64_bit_Release
"/e/qt/6.11.2/mingw_64/bin/windeployqt.exe" riscv-sim.exe
```

3. 打包后整个 `..._Release` 目录（exe + Qt DLL + `platforms/`）即可双击运行、分发到没装 Qt 的电脑。
