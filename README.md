# RISC-V 模拟器（C++ 课程设计）

RV32I 子集模拟器：读入汇编，逐条取指 / 译码 / 执行，Qt 图形界面展示寄存器、内存与**数据通路高亮动画**。

## 架构

```
汇编文本 ──Assembler──▶ 机器码 ──Memory──▶ CPU 取指/执行 ──▶ 寄存器/内存状态
                                                    └──▶ 数据通路图高亮（按 opcode）
```

五个核心类 + 一个公共头 + 界面：

| 文件 | 职责 | 认领 |
|---|---|---|
| common.h | 类型别名、opcode 常量、位操作工具 | 全组共用 |
| RegisterFile.* | 32 个寄存器，x0 恒 0 | 执行组 A |
| Memory.* | 字节寻址内存（1MB），小端读写 | 内存组 |
| CPU.* | 取指→译码→执行→写回；`step()`/`run()`/`finished()`/`inst()` | 执行组 B |
| Assembler.* | 汇编文本 ⇆ 机器码 | 汇编器组 |
| mainwindow.* | 全部 Qt 界面 + 数据通路绘制 | 界面组 |
| main.cpp + CMakeLists | 入口 + 构建 | 集成负责人 |
| make_release.sh | 一键发布脚本（编译 → 打包 → zip） | 集成负责人 |

## 已实现

**指令（19 条，全部测试通过）**：

| 类别 | 指令 |
|---|---|
| 算术 | `add` `sub` `addi` |
| 逻辑 | `andi` `ori` `xori` |
| 移位 | `slli` |
| 比较 | `slti` `sltiu` |
| 高位立即数 | `lui` |
| 分支 | `beq` `bne` |
| 跳转 | `jal` |
| 访存·读 | `lw` `lb` `lh` |
| 访存·写 | `sw` `sb` `sh` |

> CPU.cpp 里另有 `sll`/`srl`/`or`/`and`/`xor`/`srli` 的执行分支，但汇编器尚未同步，暂不计入。

**测试程序**：斐波那契 F(10)=55 循环程序已跑通（源码在 `tests/workload.txt`，运行后 x2=34、x3=55）。

**Qt 图形界面**：

- **菜单栏**：文件（新建/打开/保存/另存为）、模拟器（运行/单步/暂停/重置）、视图（显隐三面板/清日志/刷新）、帮助（指令列表/关于）；
- **执行控制**：单步 / 运行 / 暂停 / 重置，**速度输入框（0.2~50 条/秒）**；
- **状态展示**：寄存器表（x0~x31）、内存表（随程序大小动态）、PC 标签、逐条执行日志；
- **数据通路可视化**：6 个部件一行排开，按当前指令的 opcode 高亮实际用到的部件，写回总线与 PC 跳转回路分层绘制；运行时 QTimer 逐条推进，灯效逐条可见。

> **完成人**：指令集 + 汇编器 + 斐波那契测试 + 界面框架由**架构负责人**完成；队友贡献 UI 类重构（PR #2）。

## 待补

- **CPU.cpp**：`jalr`、`auipc`、`sra`、`slt/sltu`、`lbu/lhu`、`blt/bge/bltu/bgeu`
- **Assembler.cpp**：标签跳转（`loop:`）、伪指令（`mv` / `li` / `j` / `nop`）、`sll/srl/or/and/xor/srli` 同步
- **界面**：流水线动画（加分项，需先改成多周期/流水线模型）

## 文档导航

| 文档（`参考文档/` 下） | 内容 |
|---|---|
| `测试手册_斐波那契与指令体检.md` | **界面测试用**：4 个测试程序 + 预期结果 + 灯效对照表（配 `tests/*.asm`） |
| `发布Release流程.md` | 发版全流程：一键脚本用法、网页发布 6 步、手动排查、坑速查、版本号约定 |
| `RV32I指令集详解.md` | 指令编码格式（R/I/S/B/U/J）逐条拆解 |
| `给模拟器加一条指令_andi实例.md` | 「加一条新指令」的完整流程模板 |
| `学习笔记_CPU与RISC-V基础.md` | CPU 工作原理、PC 语义、符号扩展 |
| `学习笔记_Qt界面开发.md` | Qt 语法、QTimer 驱动动画、QPainter 自绘 |
| `开发踩坑记录.md` | 全部坑的「现象→原因→解决」清单 |
| `调试方法论_一个bug的完整复盘.md` | **怎么找到坑**：一次错误诊断与正确诊断的对比 |
| `Qt套件配置问题总结.md` | Qt Creator 套件配置排查（给队友） |
| `项目状态速览.md` | 当前进度速览（上下文交接用） |

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

1. 点「文件 → 打开」选 `tests/fib.asm`（或直接在左侧编辑区敲汇编）；
2. **速度输入框**填 `0.5`（两秒一条，慢放看灯）或 `20`（快速跑完）；
3. 点「运行」——数据通路图上的部件会**逐条亮起**，日志区同步打出 `[N] 助记符`，图顶部显示「第 N 拍：指令（PC → 0x..）」；
4. 点「单步」逐条推进，寄存器表看 x2=34、x3=55（斐波那契 F(9)/F(10)）；
5. 点「帮助」看已实现的 19 条指令。

**看灯对不对的自检**：`addi` 亮寄存器堆+ALU；`lw` 多亮数据存储器；`sw` 亮数据存储器但**写回总线不亮**；`beq`/`jal` 上方虚线（PC 回路）变橙。

### 发布 Release（一键脚本）

> 完整流程（网页发布详解、手动排查、坑速查、版本号约定）见《`参考文档/发布Release流程.md`》。

```bash
./make_release.sh V2.0
```

脚本自动完成三步：**① 编译 Release → ② windeployqt 拷 Qt DLL → ③ 打 zip 到桌面**（`riscv-sim-V2.0-win64.zip`，解压双击 exe 即可运行，不用装 Qt）。最后打印网页发布步骤。

- **首次使用前**：先在 Qt Creator 左下角切到 Release 构建一次，让构建目录生成出来；
- **换电脑 / Qt 装别处**：不用改脚本，用环境变量覆盖：

```bash
QT_ROOT=/c/Qt/6.11.2/mingw_64 ./make_release.sh V2.0
```

脚本跑完后，到 GitHub 网页完成最后一步：`https://github.com/cwrt310/riscv-sim/releases` → **Draft a new release** → tag 填版本号（选 Create new tag on publish）→ 描述写更新内容 → 把桌面 zip 拖进 **Attach binaries** → **Publish release**。

<details>
<summary>手动流程（脚本背后的原理，排查用）</summary>

```bash
# ① 编译 Release（用 Qt Creator 切 Release 也可）
cmake --build build/Desktop_Qt_6_11_2_MinGW_64_bit_Release

# ② windeployqt：把 Qt DLL 拷到 exe 旁边
cd build/Desktop_Qt_6_11_2_MinGW_64_bit_Release
"/e/qt/6.11.2/mingw_64/bin/windeployqt.exe" --no-translations riscv-sim.exe

# ③ 打包（只挑运行必需文件，Windows 自带 tar 支持 zip）
/c/Windows/System32/tar.exe -a -c -f ~/Desktop/riscv-sim-V2.0-win64.zip \
    riscv-sim.exe *.dll platforms styles imageformats iconengines networkinformation generic
```

</details>
