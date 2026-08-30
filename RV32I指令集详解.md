# RV32I 指令集详解（团队参考手册）

> 定位：riscv-sim 团队的指令参考。读完能知道每条指令"干什么、opcode/funct3/funct7 是多少、怎么编码"。
> 配套：`53_CPU工作原理与指令编码.md`（编码原理）、`给模拟器加一条指令_andi实例.md`（怎么加）。

---

## 一、寄存器（x0~x31）

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

## 二、六种指令格式（切分规则）

每条指令是 32 位，按"格式"切成不同字段。**先看 opcode（最低 7 位）决定格式**：

```
R 型（opcode=0x33）：[funct7 7][rs2 5][rs1 5][funct3 3][rd 5][opcode 7]
I 型（opcode=0x13/0x03/0x67）：[imm 12][rs1 5][funct3 3][rd 5][opcode 7]
S 型（opcode=0x23）：[imm[11:5] 7][rs2 5][rs1 5][funct3 3][imm[4:0] 5][opcode 7]
B 型（opcode=0x63）：[imm[12|10:5] 7][rs2 5][rs1 5][funct3 3][imm[4:1|11] 5][opcode 7]
U 型（opcode=0x37/0x17）：[imm[31:12] 20][rd 5][opcode 7]
J 型（opcode=0x6F）：[imm[20|10:1|11|19:12] 20][rd 5][opcode 7]
```

> ⚠️ **S 型、B 型、J 型的立即数是"打乱的"**（imm 被拆成几段塞进去）。这是 RISC-V 最难记的地方，实现时务必照手册来。

> 💡 **funct3 不是每种格式都有**：R / I / S / B 型有 funct3（bits 14:12）；**U 型**（lui/auipc）和 **J 型**（jal）**没有 funct3**（那几位让给立即数了）。注意 `jalr` 是 **I 型**，所以它有 funct3，值为 0。判断依据是"**格式**"，不是"这个 opcode 下有几条指令"。

**六种格式各自装了谁**（对应关系速查）：

| 格式 | 装的指令 |
|---|---|
| R 型 | add / sub / sll / slt / sltu / xor / srl / sra / or / and |
| I 型 | addi / slli / slti / sltiu / xori / srli / srai / ori / andi ｜ lw / lh / lb / lbu / lhu ｜ jalr |
| S 型 | sw / sh / sb |
| B 型 | beq / bne / blt / bge / bltu / bgeu |
| U 型 | lui / auipc |
| J 型 | jal |

**opcode 从哪来、和「格式」什么关系**（新手最容易混）：

1. **opcode 是 RISC-V 官方规范定死的，全球统一**。全世界所有 RISC-V 芯片/模拟器/编译器，`addi` 的 opcode 都是 `0x13`、`jal` 都是 `0x6F`，谁改谁就不能互通。我们只是「抄官方表」塞进 `common.h` 的 `namespace Op` 里。

2. **opcode 是 CPU 的「第一道分类」**：它永远在机器码最低 7 位，CPU 先看这 7 位，就知道这条指令是什么格式、大概干什么。

3. **「格式」和「opcode」是两个维度，别混**：

   | 维度 | 回答的问题 | 例子 |
   |---|---|---|
   | 格式（R/I/S/B/U/J） | 字段**怎么切** | addi 和 lw 都是 I 型：`imm+rs1+funct3+rd` |
   | opcode | 这条指令**大概干什么** | addi=运算(0x13)，lw=读内存(0x03) |

   **格式相同 ≠ opcode 相同**。I 型这一个「切法」被三种功能共用，所以 I 型有 **3 个 opcode**：`0x13`（运算）、`0x03`（读内存）、`0x67`（jalr）。

4. **完整对应表（格式 ← opcode）**：

   | 格式 | 对应的 opcode | 装的指令 |
   |---|---|---|
   | R 型 | `0x33`（OP） | add/sub/sll/slt/…（寄存器运算） |
   | I 型 | `0x13`（OP-IMM）/ `0x03`（LOAD）/ `0x67`（JALR） | 立即数运算 / 读内存 / 间接跳转 |
   | S 型 | `0x23`（STORE） | sw/sh/sb（写内存） |
   | B 型 | `0x63`（BRANCH） | beq/bne/blt/…（条件跳转） |
   | U 型 | `0x37`（LUI）/ `0x17`（AUIPC） | 高位立即数 |
   | J 型 | `0x6F`（JAL） | jal |

   规律：一个格式可能占好几个 opcode（I 型 3 个、U 型 2 个），但**一个 opcode 只属于一个格式**。

5. **这些具体数值没有「数学规律」可背**，是官方设计者当年为「让解码硬件更简单」拍板的结果。对初学者当「规定」抄表即可，不必深究「为什么是这些数」。

> 💡 **第三节的分组是"格式为主 + 少量功能拆分"**：本质按格式（R/I/S/B/U/J）归类；但 Load / Store / Branch / Jump 这几组"格式恰好等于功能"（Load 都是 I 型、Store 都是 S 型、Branch 都是 B 型），就直接用功能名命名。注意 **I 型横跨三处**：运算（addi 等）、Load（lw 等）、jalr。

---

## 三、完整指令表（按格式分组）

### 1. R 型运算（opcode = 0x33，寄存器-寄存器）

| 指令 | funct3 | funct7 | 语义 | 说明 |
|---|---|---|---|---|
| `add` | 0 | 0x00 | rd = rs1 + rs2 | 加 |
| `sub` | 0 | 0x20 | rd = rs1 - rs2 | 减 |
| `sll` | 1 | 0x00 | rd = rs1 << rs2 | 逻辑左移 |
| `slt` | 2 | 0x00 | rd = (rs1 < rs2) ? 1 : 0 | 有符号小于 |
| `sltu` | 3 | 0x00 | rd = (rs1 < rs2) ? 1 : 0 | 无符号小于 |
| `xor` | 4 | 0x00 | rd = rs1 ^ rs2 | 异或 |
| `srl` | 5 | 0x00 | rd = rs1 >> rs2 | 逻辑右移 |
| `sra` | 5 | 0x20 | rd = rs1 >> rs2（算术） | 算术右移（保留符号） |
| `or` | 6 | 0x00 | rd = rs1 \| rs2 | 或 |
| `and` | 7 | 0x00 | rd = rs1 & rs2 | 与 |

> 注意 `add/sub` 和 `srl/sra` 都是"opcode+funct3 相同、靠 funct7 区分"。

### 2. I 型运算（opcode = 0x13，寄存器-立即数）

| 指令 | funct3 | 语义 | 说明 |
|---|---|---|---|
| `addi` | 0 | rd = rs1 + imm | 加立即数 |
| `slli` | 1 | rd = rs1 << shamt | 逻辑左移（shamt=imm[4:0]） |
| `slti` | 2 | rd = (rs1 < imm) ? 1 : 0 | 有符号小于立即数 |
| `sltiu` | 3 | rd = (rs1 < imm) ? 1 : 0 | 无符号小于立即数 |
| `xori` | 4 | rd = rs1 ^ imm | 异或立即数 |
| `srli` | 5 | rd = rs1 >> shamt | 逻辑右移（imm[11:5]=0x00） |
| `srai` | 5 | rd = rs1 >> shamt（算术） | 算术右移（imm[11:5]=0x20） |
| `ori` | 6 | rd = rs1 \| imm | 或立即数 |
| `andi` | 7 | rd = rs1 & imm | 与立即数 |

> 移位指令 `slli/srli/srai` 的"立即数"其实是**移位数**（shamt），只取低 5 位；`srli` 和 `srai` 靠 imm 的高位（相当于 funct7）区分。

### 3. 访存 Load（opcode = 0x03，从内存读）

| 指令 | funct3 | 语义 | 说明 |
|---|---|---|---|
| `lb` | 0 | rd = 符号扩展(mem[rs1+imm] 的 1 字节) | 读字节（有符号） |
| `lh` | 1 | rd = 符号扩展(mem[rs1+imm] 的 2 字节) | 读半字（有符号） |
| `lw` | 2 | rd = mem[rs1+imm] 的 4 字节 | 读字 |
| `lbu` | 4 | rd = 零扩展(mem[rs1+imm] 的 1 字节) | 读字节（无符号） |
| `lhu` | 5 | rd = 零扩展(mem[rs1+imm] 的 2 字节) | 读半字（无符号） |

> 地址 = rs1 + imm（imm 是符号扩展的 12 位立即数）。

### 4. 访存 Store（opcode = 0x23，写内存）

| 指令 | funct3 | 语义 |
|---|---|---|
| `sb` | 0 | mem[rs1+imm] = rs2 的低 1 字节 |
| `sh` | 1 | mem[rs1+imm] = rs2 的低 2 字节 |
| `sw` | 2 | mem[rs1+imm] = rs2 的 4 字节 |

> Store 是 **S 型**（立即数打乱成两段），且**没有 rd**（是 rs2 提供要写的数据）。

### 5. 分支 Branch（opcode = 0x63，条件跳转）

| 指令 | funct3 | 跳转条件 | 说明 |
|---|---|---|---|
| `beq` | 0 | rs1 == rs2 | 相等跳 |
| `bne` | 1 | rs1 != rs2 | 不等跳 |
| `blt` | 4 | rs1 < rs2（有符号） | 小于跳 |
| `bge` | 5 | rs1 >= rs2（有符号） | 大于等于跳 |
| `bltu` | 6 | rs1 < rs2（无符号） | 无符号小于跳 |
| `bgeu` | 7 | rs1 >= rs2（无符号） | 无符号大于等于跳 |

> 跳转目标 = **PC + imm**（imm 是 13 位符号扩展偏移，B 型打乱）。这是循环/if 的基础。

### 6. 跳转 Jump

| 指令 | opcode | 语义 | 说明 |
|---|---|---|---|
| `jal` | 0x6F | rd = PC+4; PC = PC + imm | 跳转并链接（函数调用） |
| `jalr` | 0x67（funct3=0） | rd = PC+4; PC = rs1 + imm | 跳转寄存器（间接调用/返回） |

> `jal` 是 J 型（20 位偏移），`jalr` 是 I 型。它们把"返回地址"写进 rd（通常是 ra=x1）。

### 7. 高位立即数

| 指令 | opcode | 语义 |
|---|---|---|
| `lui` | 0x37 | rd = imm << 12（高 20 位，低 12 位清零） |
| `auipc` | 0x17 | rd = PC + (imm << 12) |

> 用来构造 32 位大立即数：`lui` 填高 20 位，`addi` 补低 12 位。

### 8. 系统 / 杂项（暂时可跳过）

| 指令 | opcode | 语义 |
|---|---|---|
| `ecall` | 0x73（imm=0） | 环境调用（系统调用） |
| `ebreak` | 0x73（imm=1） | 断点 |
| `fence` | 0x0F | 内存屏障 |

---

## 四、进度与分级建议

**当前已实现（12 条，全部测试通过）**：

`addi` `slli` `slti` `sltiu` `xori` `ori` `andi` `add` `sub` `lui` `beq` `bne`

> 已超过及格线（≥10 条），覆盖 R / I / U / B 四种格式；还差 S 型访存（lw/sw）、J 型跳转（jal）、jalr 和 auipc。

**及格线（≥10 条，建议优先实现）**：

`addi` ✅ `add` ✅ `sub` ✅ `andi` ✅ + `ori`、`xori`、`slli`、`srli`、`beq`、`bne`、`jal`、`lw`、`sw`、`lui`

**加分（完整 RV32I）**：

`slt/sltu/slti/sltiu`、`sll/srl/sra`、`srai`、`lb/lh/lbu/lhu`、`sb/sh`、`blt/bge/bltu/bgeu`、`jalr`、`auipc`

**建议顺序**：

1. 先做"立即数逻辑"（ori/xori，照 andi 套路，最简单）→ 练手；
2. 再做"分支"（beq/bne，解锁循环）→ 关键；
3. 再做"访存"（lw/sw，解锁数组）→ 关键；
4. 再做"跳转"（jal/jalr，解锁函数调用）；
5. 最后补全其余（移位、比较、字节/半字访存、auipc）。

---

## 五、快速记忆口诀

- **opcode 定格式**（先看最低 7 位）；
- **funct3 定小类**，**funct7 兜底**（add/sub、srl/sra、srli/srai 都是靠它区分）；
- **rd = 结果写到哪，rs1/rs2 = 从哪读**；
- **立即数带符号扩展**（负数要用 signExtend）；
- **分支/跳转 = 改写 PC**，其余 = 顺序往下。
