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

## 已实现（作为示范）

- `addi` / `add` / `sub` / `lui`
- 单步执行 `step()`、连续执行 `run()`、寄存器堆、小端内存

## 待补（TODO 都在代码里标好了）

- **CPU.cpp**：分支 `beq`、跳转 `jal/jalr`、`auipc`、load/store、移位、逻辑运算
- **Assembler.cpp**：更多指令、标签跳转、伪指令
- **界面**：Qt 对话框，只调 `cpu.step()/reg()/mem()`

## 构建说明

### 前置依赖
- 编译器：g++（Windows 用 MinGW，Linux/WSL 自带）
- 构建工具：CMake ≥ 3.10
- （可选）Qt 6：仅做图形界面时需要，命令行版不需要

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

### 期望输出

```
=== 单步执行（前两条）===
pc=4  x1=5（期望 5）
pc=8  x2=7（期望 7）
=== 连续执行剩余指令 ===
x3=12（期望 12）
x4=2（期望 2）
```

### 加 Qt 界面后（第 2-3 周）

在 `CMakeLists.txt` 里加 `find_package(Qt6 COMPONENTS Widgets)` 后重新构建，最后用 `windeployqt` 把 Qt 的 DLL 跟 exe 打包到一起，拷到没装 Qt 的电脑也能跑：

```bat
windeployqt build\riscv-sim.exe
```
