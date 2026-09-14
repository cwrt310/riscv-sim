# 表驱动改造实战：riscv-sim 版

> 日期：2026-09-06 ｜ 定位：**动手改造指南**，V3.0 的施工图。
> 前置：《表驱动入门_从函数指针到哈希表.md》（语法看不懂先看那份）
> 背景：《报告素材_展望_指令算子化.md》方向 1、2 是本文的设计依据

---

## 一、为什么现在必须改

### 1.1 现状（2026-09-06 实测）

| 位置 | 现状 | 加一条指令要动 |
|---|---|---|
| `Assembler.cpp` `assembleLine()` | **23 个 `if (mnemonic == ...)` 串成一条链** | 在链里插一段 |
| `CPU.cpp` `execute()` | 7 个外层 case + 每个里面嵌套 funct3 的 switch | 改嵌套 switch |
| `mainwindow.cpp` `datapathMask()` | `switch (op)` 算亮灯掩码 | 加 case |
| `mainwindow.cpp` `describeInst()` | `switch (op)` 翻助记符 | 加 case |
| `Assembler.cpp` `disassemble()` | 只认 4 条的小 switch（半成品） | 加 if |

**加一条指令，要在 5 个地方各改一次**，而且改的都是**同一批函数**。

### 1.2 三个具体痛点

**痛点 1：4 人并行必然冲突**
V3.0 要补 11 条指令，如果四个人同时改 `assembleLine()` 这一个函数，git 合并时必然撞车（队友刚合的 PR #4 就是往这条 if 链里插了 6 段）。

**痛点 2：漏改（已经发生过）**
队友 PR #3 更新了帮助弹窗的"19 条 → 25 条"，但**漏了 `onAbout()` 里的另一处 19 条**。信息散在多处，必然漏。

**痛点 3：性能是退化的哈希表**
`if...else if...` 25 条排成串，查最后一条走 25 步。表驱动是一步到位。

### 1.3 改完之后

加一条指令 = **往表里加一行数据**，`execute()` / `assembleLine()` 一个字都不用改。

---

## 二、改造路线（三步走，别一次到位）

| 步骤 | 改什么 | 难度 | 收益 |
|---|---|---|---|
| **第 1 步** | 汇编器：`insn_table` 键值表 | ⭐⭐ | 消掉 23 个 if，4 人并行不冲突 |
| **第 2 步** | CPU：`dispatch[128]` 数组表 | ⭐⭐⭐ | 消掉嵌套 switch |
| **第 3 步** | 界面：亮灯表 + 助记符表 | ⭐ | 顺手做，改动最小 |

> ⚠️ **不要一上来就做"五合一大表"**。先各自表驱动化，结构稳定后再考虑合并——过度设计会拖垮进度。

---

## 三、第 1 步：汇编器表驱动（先做这个）

### 3.1 现状长什么样

```cpp
// Assembler.cpp 现在：23 个 if 串成链
if (mnemonic == "addi") {
    int rd = regNum(a[0]), rs1 = regNum(a[1]);
    i32 imm = parseImm(a[2]);
    return (u32(imm) & 0xFFF) << 20 | u32(rs1) << 15 | u32(rd) << 7 | Op::OP_IMM;
}
if (mnemonic == "andi") {
    int rd = regNum(a[0]), rs1 = regNum(a[1]);
    i32 imm = parseImm(a[2]);
    return (u32(imm) & 0xFFF) << 20 | u32(rs1) << 15 | 7u << 12 | u32(rd) << 7 | Op::OP_IMM;
}
// ... 再来 21 个几乎一样的块
```

**观察**：`addi` 和 `andi` 的代码**只差一个 funct3**（0 vs 7），其余完全相同。这就是表驱动的信号——**大量重复代码，只有几个数字不同**。

### 3.2 关键洞见：把"变的部分"抽成数据

一条指令的编码信息，只有 4 个数字 + 1 个格式：

| 指令 | opcode | funct3 | funct7 | 格式 |
|---|---|---|---|---|
| addi | OP_IMM | 0 | — | I |
| andi | OP_IMM | 7 | — | I |
| add | OP | 0 | 0x00 | R |
| sub | OP | 0 | 0x20 | R |
| lw | LOAD | 2 | — | I（带括号） |
| sw | STORE | 2 | — | S |

**"怎么把这些数字拼成机器码"只跟格式有关**——所有 I 型指令的拼法完全一样。所以只要 6 个"格式编码器"（R/I/S/B/U/J），加上一张描述表。

### 3.3 目标代码

**① 定义描述结构**（放 `Assembler.h`）：

```cpp
#include <unordered_map>
#include <string>

// 一条指令的编码信息（"数据"部分）
struct InsnDesc {
    u32  opcode;    // 操作码
    int  funct3;    // 功能码 3（用不上填 0）
    int  funct7;    // 功能码 7（用不上填 0）
    char format;    // 格式：'R' 'I' 'S' 'B' 'U' 'J'，'M' = 访存 I 型（带括号语法）
};
```

**② 建表并登记**（放 `Assembler.cpp` 文件顶部的匿名 namespace 里）：

```cpp
// 指令描述表：新增指令 = 往这里加一行
const std::unordered_map<std::string, InsnDesc> insn_table = {
    // 名字      opcode        f3  f7    格式
    {"addi",  {Op::OP_IMM,   0,  0,   'I'}},
    {"slli",  {Op::OP_IMM,   1,  0,   'I'}},
    {"slti",  {Op::OP_IMM,   2,  0,   'I'}},
    {"sltiu", {Op::OP_IMM,   3,  0,   'I'}},
    {"xori",  {Op::OP_IMM,   4,  0,   'I'}},
    {"srli",  {Op::OP_IMM,   5,  0,   'I'}},
    {"ori",   {Op::OP_IMM,   6,  0,   'I'}},
    {"andi",  {Op::OP_IMM,   7,  0,   'I'}},

    {"add",   {Op::OP,       0,  0x00,'R'}},
    {"sub",   {Op::OP,       0,  0x20,'R'}},   // 只有 funct7 不同
    {"sll",   {Op::OP,       1,  0x00,'R'}},
    {"xor",   {Op::OP,       4,  0x00,'R'}},
    {"srl",   {Op::OP,       5,  0x00,'R'}},
    {"or",    {Op::OP,       6,  0x00,'R'}},
    {"and",   {Op::OP,       7,  0x00,'R'}},

    {"lui",   {Op::LUI,      0,  0,   'U'}},
    {"jal",   {Op::JAL,      0,  0,   'J'}},

    {"beq",   {Op::BRANCH,   0,  0,   'B'}},
    {"bne",   {Op::BRANCH,   1,  0,   'B'}},

    {"lb",    {Op::LOAD,     0,  0,   'M'}},   // M = 访存读（imm(rs1) 语法）
    {"lh",    {Op::LOAD,     1,  0,   'M'}},
    {"lw",    {Op::LOAD,     2,  0,   'M'}},

    {"sb",    {Op::STORE,    0,  0,   'S'}},
    {"sh",    {Op::STORE,    1,  0,   'S'}},
    {"sw",    {Op::STORE,    2,  0,   'S'}},
};
```

**③ 六个格式编码器**（每种格式的拼法，写一次就够）：

```cpp
// I 型：imm[11:0] | rs1 | funct3 | rd | opcode
u32 encodeI(const InsnDesc& d, int rd, int rs1, i32 imm) {
    return (u32(imm) & 0xFFF) << 20 | u32(rs1) << 15
         | u32(d.funct3) << 12 | u32(rd) << 7 | d.opcode;
}

// R 型：funct7 | rs2 | rs1 | funct3 | rd | opcode
u32 encodeR(const InsnDesc& d, int rd, int rs1, int rs2) {
    return u32(d.funct7) << 25 | u32(rs2) << 20 | u32(rs1) << 15
         | u32(d.funct3) << 12 | u32(rd) << 7 | d.opcode;
}

// S 型：imm[11:5] | rs2 | rs1 | funct3 | imm[4:0] | opcode
u32 encodeS(const InsnDesc& d, int rs1, int rs2, i32 imm) {
    u32 u = u32(imm);
    return ((u >> 5) & 0x7F) << 25 | u32(rs2) << 20 | u32(rs1) << 15
         | u32(d.funct3) << 12 | ((u & 0x1F) << 7) | d.opcode;
}

// B 型、U 型、J 型同理，把现有代码搬过来即可
```

**④ 改造 assembleLine**：

```cpp
u32 Assembler::assembleLine(const std::string& line) {
    // ...前面的 trim / 分词逻辑不变...

    // 查表（替换 23 个 if）
    auto it = insn_table.find(mnemonic);
    if (it == insn_table.end())
        throw std::runtime_error("unknown instruction: " + mnemonic);
    const InsnDesc& d = it->second;

    // 按格式分派参数解析 + 编码
    switch (d.format) {
    case 'I': return encodeI(d, regNum(a[0]), regNum(a[1]), parseImm(a[2]));
    case 'R': return encodeR(d, regNum(a[0]), regNum(a[1]), regNum(a[2]));
    case 'U': return (u32(parseImm(a[1])) & 0xFFFFF) << 12 | u32(regNum(a[0])) << 7 | d.opcode;
    case 'M': { auto [imm, rs1] = parseMemOperand(a[1]);      // "0(x2)" → {0, 2}
                return encodeI(d, regNum(a[0]), rs1, imm); }
    case 'S': { auto [imm, rs1] = parseMemOperand(a[1]);
                return encodeS(d, rs1, regNum(a[0]), imm); }
    case 'B': /* 现有 beq/bne 编码逻辑 */ ;
    case 'J': /* 现有 jal 编码逻辑 */ ;
    }
    throw std::runtime_error("bad format for: " + mnemonic);
}
```

### 3.4 顺手解决的问题

现在 `lw/lb/lh/sw/sb/sh` 六条各自重复写了一遍"拆括号"的代码，抽成一个辅助函数：

```cpp
// "8(x2)" → {8, 2}
std::pair<i32,int> parseMemOperand(const std::string& s) {
    size_t pos = s.find('(');
    if (pos == std::string::npos) throw std::runtime_error("expected imm(reg): " + s);
    i32 imm = parseImm(s.substr(0, pos));
    int rs1 = regNum(s.substr(pos + 1, s.length() - pos - 2));
    return {imm, rs1};
}
```

### 3.5 顺手修一个已知 bug

踩坑记录 9b：**立即数超范围被悄悄截断**（`addi x1,x0,0x1234` 得 564 而非 4660）。表驱动后只需在 `encodeI` 里加一次检查，**所有 I 型指令一起受益**：

```cpp
u32 encodeI(const InsnDesc& d, int rd, int rs1, i32 imm) {
    if (imm < -2048 || imm > 2047)
        throw std::runtime_error("immediate out of range [-2048, 2047]: " + std::to_string(imm));
    // ...
}
```

**这就是表驱动的复利**：以前要在 8 个 I 型 if 块里各加一次检查，现在加一次。

### 3.6 验收标准

- [ ] 25 条指令全部走表，`assembleLine` 里再无 `if (mnemonic == ...)`
- [ ] `tests/` 4 个测试全部通过（结果与《测试手册》完全一致）
- [ ] 加一条新指令只需往 `insn_table` 加一行

---

## 四、第 2 步：CPU 执行表驱动

### 4.1 关键区别：这里用数组表

**汇编器的键是字符串**（`"addi"`）→ 用 `unordered_map`；
**CPU 的键是 opcode**（0~127 的数字）→ 直接用**数组表**，更快更简单。

```cpp
// CPU.h
using Handler = void (*)(CPU&, u32);   // 统一的算子接口

class CPU {
    // ...
    static Handler dispatch[128];       // 分派表（静态，全类共享）
    static void initDispatch();         // 登记（构造函数里调一次）
};
```

### 4.2 算子函数

把 `execute()` 里每个外层 case 拎成独立函数：

```cpp
// CPU.cpp
static void exec_op_imm(CPU& cpu, u32 inst) {
    int rd = int(bits(inst, 11, 7));
    int rs1 = int(bits(inst, 19, 15));
    int funct3 = int(bits(inst, 14, 12));
    i32 imm = signExtend(bits(inst, 31, 20), 12);
    switch (funct3) {                    // 内层暂时保留 switch，先改外层
    case 0: cpu.setReg(rd, cpu.reg(rs1) + imm); break;   // addi
    // ...
    }
}
static void exec_op    (CPU& cpu, u32 inst) { /* R 型 */ }
static void exec_lui   (CPU& cpu, u32 inst) { /* ... */ }
static void exec_load  (CPU& cpu, u32 inst) { /* ... */ }
static void exec_store (CPU& cpu, u32 inst) { /* ... */ }
static void exec_branch(CPU& cpu, u32 inst) { /* ... */ }
static void exec_jal   (CPU& cpu, u32 inst) { /* ... */ }

// 兜底：不认识的 opcode（替代原来的 default）
static void exec_illegal(CPU& cpu, u32 inst) {
    throw std::runtime_error("unimplemented instruction");
}
```

### 4.3 登记与查表

```cpp
Handler CPU::dispatch[128];

void CPU::initDispatch() {
    for (int i = 0; i < 128; ++i) dispatch[i] = exec_illegal;  // 先全填兜底
    dispatch[Op::OP_IMM] = exec_op_imm;                        // 再登记具体的
    dispatch[Op::OP]     = exec_op;
    dispatch[Op::LUI]    = exec_lui;
    dispatch[Op::LOAD]   = exec_load;
    dispatch[Op::STORE]  = exec_store;
    dispatch[Op::BRANCH] = exec_branch;
    dispatch[Op::JAL]    = exec_jal;
}

// execute() 从 130 行缩成 2 行
void CPU::execute(u32 inst) {
    u32 opcode = bits(inst, 6, 0);
    dispatch[opcode](*this, inst);     // 查表 + 调用
}
```

> 💡 **全填兜底函数的好处**：`execute()` 里不用判空（`if (f == nullptr)`），因为 128 格全都有值。
> 不认识的指令自动落到 `exec_illegal` 抛异常，行为和原来的 `default:` 一模一样。

### 4.4 一个技术障碍：算子函数要能改 CPU 内部状态

算子是**外部函数**，访问不到 `private` 的 `rf_` / `mem_` / `pc_`。三个解法，选一个：

| 方案 | 做法 | 评价 |
|---|---|---|
| **A. 加 public setter** | `void setReg(int n, u32 v)`、`void setPC(u32 v)` | ✅ 推荐，最简单，接口清晰 |
| B. friend 声明 | `friend void exec_op_imm(CPU&, u32);` | 要写 7 行 friend，繁琐 |
| C. 改成成员函数指针 | `using Handler = void (CPU::*)(u32);` | 语法更绕（调用要写 `(this->*f)(inst)`），初学不推荐 |

建议 **A**：给 CPU 加几个 public 的 setter，算子通过它们改状态。

### 4.5 验收标准

- [ ] `execute()` 只剩查表 + 调用两行
- [ ] 7 个算子函数各管一类 opcode
- [ ] `tests/` 4 个测试结果不变

---

## 五、第 3 步：界面两张小表（顺手做）

### 5.1 亮灯表（`datapathMask()`）

现在：

```cpp
switch (op) {
case Op::OP_IMM:
case Op::OP:     m |= STAGE_REG | STAGE_EX | STAGE_WB; break;
case Op::LUI:    m |= STAGE_WB; break;
// ...
}
```

改成表（放 `mainwindow.cpp` 文件级）：

```cpp
// opcode → 该亮哪些部件
static uint32_t lightTable[128] = {0};

static void initLightTable() {
    using D = DatapathWidget;
    uint32_t base = D::STAGE_PC | D::STAGE_IF | D::STAGE_ID;   // 三件套永远亮
    lightTable[Op::OP_IMM] = base | D::STAGE_REG | D::STAGE_EX | D::STAGE_WB;
    lightTable[Op::OP]     = base | D::STAGE_REG | D::STAGE_EX | D::STAGE_WB;
    lightTable[Op::LUI]    = base | D::STAGE_WB;
    lightTable[Op::LOAD]   = base | D::STAGE_REG | D::STAGE_EX | D::STAGE_MEM | D::STAGE_WB;
    lightTable[Op::STORE]  = base | D::STAGE_REG | D::STAGE_EX | D::STAGE_MEM;
    lightTable[Op::BRANCH] = base | D::STAGE_REG | D::STAGE_EX | D::STAGE_PCWR;
    lightTable[Op::JAL]    = base | D::STAGE_WB  | D::STAGE_PCWR;
}

uint32_t MainWindow::datapathMask() const {
    return lightTable[bits(cpu.inst(), 6, 0)];   // 一行搞定
}
```

### 5.2 助记符表（`describeInst()`）

`describeInst()` 里的 `static const char* n[8] = {"addi","slli",...}` **已经是表驱动了**——你早就在无意中用过这个思想：用 `funct3` 当下标直接查名字。可以扩展成完整的"opcode+funct3 → 名字"二维表。

### 5.3 顺手修一个 bug

`onAbout()` 里还写着"支持 RV32I 基础指令集（19 条指令）"，而帮助弹窗已经是 25 条——**队友 PR #3 漏改的地方**。

治本方案：指令条数从表里算出来，不再手写数字：

```cpp
QString::number(insn_table.size())    // 表有多少行就是多少条，永远不会不一致
```

---

## 六、4 人分工建议（V3.0）

表驱动最大的价值是**并行不冲突**。分工方式：

| 人 | 任务 | 改哪些文件 |
|---|---|---|
| 组长 | 搭骨架：`InsnDesc` + `insn_table` + 6 个编码器 + `dispatch` | Assembler.h/.cpp、CPU.h/.cpp |
| A | 补 `jalr` `auipc`（要新增 format 处理） | 往表里加行 + 新算子函数 |
| B | 补分支四条 `blt` `bge` `bltu` `bgeu` | 往表里加行 |
| C | 补 `slt` `sltu` `sra` `lbu` `lhu` | 往表里加行 |

> ⚠️ **骨架必须组长先做完并合入 main**，其他人再基于新结构补指令。
> 否则大家还是在改同一个函数，冲突照旧。

### 6.1 新指令编码速查表（补指令时照抄，别自己猜）

> 2026-09-12 补：队友补 11 条指令时 5 处填错（`jalr`/`auipc` 的 format、`sra` 的 funct7、`lbu`/`lhu` 的 format）——根因是"分工给了、正确值没给"。这张表就是"照着填就对的答案"。

| 指令 | opcode | funct3 | funct7 | format | 备注 |
|---|---|---|---|---|---|
| `jalr` | `Op::JALR` | 0 | 0 | **`M`** | 语法 `jalr rd, imm(rs1)` 带括号 |
| `auipc` | `Op::AUIPC` | 0 | 0 | **`U`** | U 型，和 `lui` 同款 |
| `sra` | `Op::OP` | 5 | **`0x20`** | `R` | 和 `srl` 的唯一区别就是 funct7 |
| `slt` | `Op::OP` | 2 | 0 | `R` | 有符号比较 |
| `sltu` | `Op::OP` | 3 | 0 | `R` | 无符号比较 |
| `lbu` | `Op::LOAD` | 4 | 0 | **`M`** | 带括号，无符号（零扩展） |
| `lhu` | `Op::LOAD` | 5 | 0 | **`M`** | 带括号，无符号 |
| `blt` | `Op::BRANCH` | 4 | 0 | `B` | 有符号 < |
| `bge` | `Op::BRANCH` | 5 | 0 | `B` | 有符号 ≥ |
| `bltu` | `Op::BRANCH` | 6 | 0 | `B` | 无符号 < |
| `bgeu` | `Op::BRANCH` | 7 | 0 | `B` | 无符号 ≥ |

**三个最容易错的点（踩坑速记）**：

1. **`format` 看语法不看 opcode**：`lbu`/`lhu`/`jalr` 虽然编码格式各异，但汇编语法都带括号 → `'M'`；`auipc` 虽然名字带"立即数"，但和 `lui` 一样是 U 型 → `'U'`。
2. **`sra` 和 `srl` 只差 funct7**：`srl` funct7=0x00，`sra` funct7=0x20。填反了就把算术右移编成逻辑右移。
3. **有符号/无符号成对出现**：`slt`/`sltu`、`blt`/`bltu`、`bge`/`bgeu`——带 `u` 的是无符号，funct3 相邻。

**CPU 侧对应改动**（加指令三件套的另外两件，队友容易漏）：
- `executeOP` 补 `slt`(case 2)/`sltu`(case 3)/`sra`(case 5 的 funct7==0x20 分支)
- `executeBRANCH` 补 `blt/bge/bltu/bgeu`(case 4/5/6/7)
- `executeLOAD` 补 `lbu/lhu`(case 4/5，零扩展)
- `jalr`/`auipc` 新建算子 + `initHandlers()` 里登记 `handlers[Op::JALR]`/`handlers[Op::AUIPC]`

---

## 七、常见问题

**Q：表放哪个文件？**
A：`insn_table` 放 `Assembler.cpp` 顶部（匿名 namespace 里，不暴露给外部）；`dispatch` 是 CPU 的静态成员。

**Q：什么时候登记？**
A：`unordered_map` 用初始化列表，程序启动就有；`dispatch` 数组在 CPU 构造函数里调一次 `initDispatch()`（或用 `static bool inited = (initDispatch(), true);` 的技巧只调一次）。

**Q：改完性能会变差吗？**
A：变好。if-else 链是 O(n)，数组表是 O(1)，哈希表接近 O(1)。

**Q：改造过程中怎么保证没改坏？**
A：**每改一步就跑一次 `tests/` 4 个测试**。预期值在《测试手册》里，全部对得上才继续下一步。这是重构的铁律——**小步改，勤验证**。

---

## 八、实际完成记录（2026-09-07 更新：两步全部完成）

**汇编器（第 1~4 步）与 CPU（dispatch 表）均已由组长完成**，与本文档方案的出入记录如下：

| 出入点 | 方案 | 实际 | 影响 |
|---|---|---|---|
| 拆括号函数名 | `parseMemOperand` | `encodehelp` | 名字不达意，功能相同，可后续改名 |
| encodeM | 删除、复用 encodeI | 已删，M 分支调 `encodeI` | ✅ 与方案一致 |
| CPU 表命名 | `dispatch`/`initDispatch` | `handlers`/`initHandlers` | 等价 |
| CPU 算子命名 | `exec_op_imm` 等 | `executeOP_IMM` 等 | 等价 |
| setter 命名 | `setReg`/`setPC`/`memRW` | `setreg`/`setpc`/`getmem` | 等价 |
| Handler 位置 | private | public（`using handler`） | 规避了"private 别名类外不可用"的坑，✅ 比方案好 |

**实际踩过的坑**（踩坑记录 16~22 号）：encodeB 漏 `>>1`、M/S 分支没拆括号、双引号编译错、**算子迁移丢 -4**（21 号）、**`{executeILLEGAL}` 只填第一格**（22 号）。

**成果**：`assembleLine` 230 → 50 行；`execute` 130 行 switch → 2 行；两张表骨架合入后队友并行补指令不再冲突。

**教学收获**（组长视角）：表驱动改造是"提炼 + 对照"的过程——每个编码器/算子都从旧代码逐位抄出、把写死的数字换成 `desc.` 成员或 `cpu.xxx()` 接口；"依赖外部约定的表达式"（如 `-4`）是最容易抄丢的部分，新旧并存跑测试对拍（汇编器）与全量回归（CPU）是重构安全的底气。

---

## 九、与 buckyball 的呼应（报告可用）

这套设计的思想来源是你长线在学的 buckyball：

| buckyball | riscv-sim |
|---|---|
| **ball** = 一个算子的完整软硬件封装 | **指令表项** = 一条指令的完整信息 |
| 标准接口（blink：command/data/status） | 标准签名 `void(CPU&, u32)` |
| `ballIdMappings` 注册表 | `dispatch[opcode]` 分派表 |
| 加算子 = 写一个 ball + 注册一行 | 加指令 = 写一个算子 + 表里加一行 |

**共同本质：把"逻辑"变成"可登记的数据单元"，核心框架不因扩展而改动。**
报告里可以写：本项目的指令算子化设计借鉴了 DSA 加速器的算子封装思想。
