# RV32I 指令集详解（团队参考手册）

> 定位：riscv-sim 团队的指令参考。读完能知道每条指令"干什么、opcode/funct3/funct7 是多少、怎么编码"。
> 配套：`学习笔记_CPU与RISC-V基础.md`（编码原理）、`给模拟器加一条指令_andi实例.md`（怎么加）。

---

## 怎么用这本手册（先看这里）

| 我想… | 去哪看 |
|---|---|
| 查某条指令的 opcode / funct3 / funct7 | →「一、总表」 |
| 搞懂 32 位机器码怎么切成字段 | →「二、指令格式」 |
| 查寄存器 ABI 名（`regNum` 支持的写法） | →「三、寄存器」 |
| 看当前做到哪、下一步补什么 | →「四、进度与建议」 |

---

## 一、总表：所有指令的编码与语义

> **进制约定**：opcode 用**十六进制**（`0x33`），funct3 用**十进制**（`0`~`7`），funct7 用**十六进制**（`0x00` / `0x20`）。这与代码一致——`CPU.cpp` 里 `case 7` 是 funct3 十进制，`funct7 == 0x20` 是十六进制。
> 每组标题给出 opcode；表内列的是**组内区分字段**（R 型多一个 funct7，U / J / 系统组则用 opcode 区分）。

### R 型 —— 寄存器运算（opcode = `0x33`）

| 指令 | funct3 | funct7 | 语义 |
|---|---|---|---|
| `add` | 0 | 0x00 | rd = rs1 + rs2 |
| `sub` | 0 | 0x20 | rd = rs1 - rs2 |
| `sll` | 1 | 0x00 | rd = rs1 << rs2 |
| `slt` | 2 | 0x00 | rd = (rs1 < rs2) ? 1 : 0（有符号） |
| `sltu` | 3 | 0x00 | rd = (rs1 < rs2) ? 1 : 0（无符号） |
| `xor` | 4 | 0x00 | rd = rs1 ^ rs2 |
| `srl` | 5 | 0x00 | rd = rs1 >> rs2（逻辑） |
| `sra` | 5 | 0x20 | rd = rs1 >> rs2（算术，保留符号） |
| `or` | 6 | 0x00 | rd = rs1 \| rs2 |
| `and` | 7 | 0x00 | rd = rs1 & rs2 |

> ⚠️ `add`/`sub`、`srl`/`sra` 都是「funct3 相同，靠 funct7 区分」。

### I 型 —— 立即数运算（opcode = `0x13`）

| 指令 | funct3 | 语义 |
|---|---|---|
| `addi` | 0 | rd = rs1 + imm |
| `slli` | 1 | rd = rs1 << shamt（shamt = imm[4:0]） |
| `slti` | 2 | rd = (rs1 < imm) ? 1 : 0（有符号） |
| `sltiu` | 3 | rd = (rs1 < imm) ? 1 : 0（无符号） |
| `xori` | 4 | rd = rs1 ^ imm |
| `srli` | 5 | rd = rs1 >> shamt（逻辑，imm[11:5]=0x00） |
| `srai` | 5 | rd = rs1 >> shamt（算术，imm[11:5]=0x20） |
| `ori` | 6 | rd = rs1 \| imm |
| `andi` | 7 | rd = rs1 & imm |

> ⚠️ 移位 `slli`/`srli`/`srai` 的"立即数"其实是**移位数**（shamt），只取低 5 位；`srli`/`srai` 靠 imm 的高位（相当于 funct7）区分。

### I 型 —— 读内存 Load（opcode = `0x03`）

| 指令 | funct3 | 语义 |
|---|---|---|
| `lb` | 0 | rd = 符号扩展(mem[rs1+imm] 的 1 字节) |
| `lh` | 1 | rd = 符号扩展(mem[rs1+imm] 的 2 字节) |
| `lw` | 2 | rd = mem[rs1+imm] 的 4 字节 |
| `lbu` | 4 | rd = 零扩展(mem[rs1+imm] 的 1 字节) |
| `lhu` | 5 | rd = 零扩展(mem[rs1+imm] 的 2 字节) |

> 地址 = rs1 + imm（符号扩展的 12 位立即数）。

### I 型 —— 间接跳转 jalr（opcode = `0x67`）

| 指令 | funct3 | 语义 |
|---|---|---|
| `jalr` | 0 | rd = PC+4；PC = rs1 + imm |

### S 型 —— 写内存 Store（opcode = `0x23`）

| 指令 | funct3 | 语义 |
|---|---|---|
| `sb` | 0 | mem[rs1+imm] = rs2 的低 1 字节 |
| `sh` | 1 | mem[rs1+imm] = rs2 的低 2 字节 |
| `sw` | 2 | mem[rs1+imm] = rs2 的 4 字节 |

> ⚠️ Store 是 S 型（立即数打乱成两段），**没有 rd**——要写的数据由 rs2 提供。

### B 型 —— 条件分支（opcode = `0x63`）

| 指令 | funct3 | 跳转条件 |
|---|---|---|
| `beq` | 0 | rs1 == rs2 |
| `bne` | 1 | rs1 != rs2 |
| `blt` | 4 | rs1 < rs2（有符号） |
| `bge` | 5 | rs1 >= rs2（有符号） |
| `bltu` | 6 | rs1 < rs2（无符号） |
| `bgeu` | 7 | rs1 >= rs2（无符号） |

> 跳转目标 = **PC + imm**（13 位符号扩展偏移，B 型打乱）。这是循环 / if 的基础。

### U 型 —— 高位立即数（opcode = `0x37` / `0x17`）

| 指令 | opcode | 语义 |
|---|---|---|
| `lui` | 0x37 | rd = imm << 12（低 12 位清零） |
| `auipc` | 0x17 | rd = PC + (imm << 12) |

> 用来构造 32 位大立即数：`lui` 填高 20 位，`addi` 补低 12 位。

### J 型 —— 跳转并链接（opcode = `0x6F`）

| 指令 | opcode | 语义 |
|---|---|---|
| `jal` | 0x6F | rd = PC+4；PC = PC + imm |

> `jal` 把返回地址写进 rd（通常是 ra=x1）。

### 系统 / 杂项（暂时可跳过）

| 指令 | opcode | 语义 |
|---|---|---|
| `ecall` | 0x73（imm=0） | 环境调用（系统调用） |
| `ebreak` | 0x73（imm=1） | 断点 |
| `fence` | 0x0F | 内存屏障 |

---

## 二、指令格式：32 位机器码怎么切

每条指令是 32 位，按"格式"切成不同字段。**先看 opcode（最低 7 位）决定格式**，再按格式切剩下位。下面从高位（bit31）到低位（bit0）列出：

```
R 型（opcode=0x33）            [funct7 7][rs2 5][rs1 5][funct3 3][rd 5][opcode 7]
I 型（opcode=0x13/0x03/0x67）  [imm 12][rs1 5][funct3 3][rd 5][opcode 7]
S 型（opcode=0x23）            [imm[11:5] 7][rs2 5][rs1 5][funct3 3][imm[4:0] 5][opcode 7]
B 型（opcode=0x63）            [imm[12|10:5] 7][rs2 5][rs1 5][funct3 3][imm[4:1|11] 5][opcode 7]
U 型（opcode=0x37/0x17）       [imm[31:12] 20][rd 5][opcode 7]
J 型（opcode=0x6F）            [imm[20|10:1|11|19:12] 20][rd 5][opcode 7]
```

> ⚠️ **S / B / J 型的立即数是"打乱的"**（imm 被拆成几段塞进不同位置）。这是 RISC-V 最难记的地方，实现时务必照手册来。

### 格式 vs opcode：两个维度，别混

| 维度 | 回答的问题 | 例子 |
|---|---|---|
| 格式（R/I/S/B/U/J） | 字段**怎么切** | addi 和 lw 都是 I 型：`imm+rs1+funct3+rd` |
| opcode | 这条指令**大概干什么** | addi=运算(0x13)，lw=读内存(0x03) |

**格式相同 ≠ opcode 相同**：I 型这一种「切法」被三种功能共用，所以 I 型有 **3 个 opcode**（`0x13` 运算、`0x03` 读内存、`0x67` jalr）。反过来，**一个 opcode 只属于一种格式**。

完整对应：

| 格式 | 对应的 opcode | 装的指令 |
|---|---|---|
| R 型 | `0x33`（OP） | add/sub/sll/slt/…（寄存器运算） |
| I 型 | `0x13`（OP-IMM）/ `0x03`（LOAD）/ `0x67`（JALR） | 立即数运算 / 读内存 / 间接跳转 |
| S 型 | `0x23`（STORE） | sw/sh/sb（写内存） |
| B 型 | `0x63`（BRANCH） | beq/bne/blt/…（条件跳转） |
| U 型 | `0x37`（LUI）/ `0x17`（AUIPC） | 高位立即数 |
| J 型 | `0x6F`（JAL） | jal |

### funct3 / funct7 的作用

- **funct3 定小类**：R / I / S / B 型都有 funct3（bits 14:12），区分组内各条指令。
- **funct7 兜底**：只 R 型（bits 31:25）和移位类 I 型（用 imm[11:5] 顶替）用到，区分 `add/sub`、`srl/sra`、`srli/srai` 这种 funct3 相同的指令。
- **U 型（lui/auipc）和 J 型（jal）没有 funct3**——那几位让给立即数了。注意 `jalr` 是 **I 型**，所以它有 funct3（值 0）。判断依据是"**格式**"，不是"这个 opcode 下有几条指令"。

> 💡 opcode 是 RISC-V 官方规范定死的、全球统一，我们只是「抄官方表」塞进 `common.h` 的 `namespace Op` 里。这些具体数值没有数学规律可背，是官方为「让解码硬件更简单」拍板的结果——当「规定」抄表即可，不必深究「为什么是这些数」。

---

## 三、寄存器（x0~x31）

32 个通用寄存器，每个 32 位：

| 编号 | ABI 名 | 用途 | 说明 |
|---|---|---|---|
| x0 | zero | 常数 0 | **恒为 0**，读永远 0，写被丢弃 |
| x1 | ra | 返回地址 | 函数调用用（jal 会把返回地址写这里） |
| x2 | sp | 栈指针 | 指向栈顶 |
| x3 | gp | 全局指针 | |
| x4 | tp | 线程指针 | |
| x5~x7 | t0~t2 | 临时寄存器 | |
| x8 | s0 / fp | 保存寄存器 / 帧指针 | |
| x9 | s1 | 保存寄存器 | |
| x10~x17 | a0~a7 | 函数参数/返回值 | |
| x18~x27 | s2~s11 | 保存寄存器 | |
| x28~x31 | t3~t6 | 临时寄存器 | |

> 记三个关键的就行：**x0 = 0**、**x1 = ra（返回地址）**、**x2 = sp（栈指针）**。其余按需查表。

---

## 四、进度与建议

**当前已实现（13 条，全部测试通过）**：

`addi` `slli` `slti` `sltiu` `xori` `ori` `andi` `add` `sub` `lui` `beq` `bne` `jal`

> 已超过及格线（≥10 条），覆盖 R / I / U / B / J 五种格式；还差 S 型访存（lw/sw）、jalr、auipc。第一个循环程序（斐波那契）已跑通。

### 实战示例：斐波那契（第一个循环程序）

算 F(10) = 55，用 `addi`（算术）+ `add`（加法）+ `beq`（循环退出）+ `jal`（循环回跳）串成一个循环：

```asm
addi x1, x0, 9     # 计数器（倒计数，算 F(10) 需迭代 9 次）
addi x2, x0, 0     # a = F(0) = 0
addi x3, x0, 1     # b = F(1) = 1
beq  x1, x0, 24    # x1==0 → 跳到地址 36（结束）
add  x4, x3, x2    # temp = a + b
addi x2, x3, 0     # a = b
addi x3, x4, 0     # b = temp
addi x1, x1, -1    # 计数器 -1
jal  x0, -20       # 跳回循环头（地址 12）
```

要点：

- **倒计数 + 和 x0 比**：`beq x1, x0` 拿计数器直接和恒 0 的 x0 比，省掉一个「n」比较基准寄存器（比「正计数拿 i 和 n 比」更简洁）；
- **`addi x2, x3, 0`** = 伪指令 `mv x2, x3`（x3 + 0 = x3）；
- **手算偏移**：beq 在地址 12 跳 +24 到地址 36（结束）、jal 在地址 32 跳 −20 回地址 12（循环头），偏移 = 目标 − 所在；
- 结果：x2 = F(9) = 34、x3 = F(10) = 55。

### 建议实现顺序

1. 先做"立即数逻辑"（ori/xori，照 andi 套路，最简单）→ 练手；
2. 再做"分支"（beq/bne，解锁循环）→ 关键；
3. 再做"访存"（lw/sw，解锁数组）→ 关键；
4. 再做"跳转"（jal/jalr，解锁函数调用）；
5. 最后补全其余（移位、比较、字节/半字访存、auipc）。

**及格线（≥10 条，建议优先实现）**：

`addi` ✅ `add` ✅ `sub` ✅ `andi` ✅ + `ori`、`xori`、`slli`、`srli`、`beq`、`bne`、`jal`、`lw`、`sw`、`lui`

**加分（完整 RV32I）**：

`slt/sltu/slti/sltiu`、`sll/srl/sra`、`srai`、`lb/lh/lbu/lhu`、`sb/sh`、`blt/bge/bltu/bgeu`、`jalr`、`auipc`

---

## 五、快速记忆口诀

- **opcode 定格式**（先看最低 7 位）；
- **funct3 定小类**，**funct7 兜底**（add/sub、srl/sra、srli/srai 都是靠它区分）；
- **rd = 结果写到哪，rs1/rs2 = 从哪读**；
- **立即数带符号扩展**（负数要用 signExtend）；
- **分支/跳转 = 改写 PC**，其余 = 顺序往下。
