# 给模拟器加一条指令（andi 实例）

> 定位：riscv-sim 加一条指令的标准流程 + 第一个实例 andi + 踩的坑（运算符优先级）。
> 更新日期：2026-08-28

---

## 一、加一条指令，永远改两个文件

| 文件 | 让什么"会" | 改哪里 |
|---|---|---|
| `CPU.cpp` | 让 CPU 会**执行** | `execute()` 里对应的 `case` |
| `Assembler.cpp` | 让汇编器会**翻译** | `assembleLine()` 里加一个 `if (mnemonic == "xxx")` |

> 顺序建议：先 CPU（能执行），再 Assembler（能翻译），最后写测试验证。

---

## 二、andi 实例（正确代码）

### ① CPU.cpp —— `OP_IMM` 的 switch 里加

```cpp
case 7: rf_.write(rd, rf_.read(rs1) & imm); break;   // andi：按位与
```

（andi 和 addi 都是 OP_IMM 类，靠 funct3 区分：addi=0、andi=7。`case 7` 就是 funct3=7。）

### ② Assembler.cpp —— `assembleLine` 里加

```cpp
if (mnemonic == "andi") {
    int rd  = regNum(a[0]);
    int rs1 = regNum(a[1]);
    i32 imm = parseImm(a[2]);
    return (u32(imm) & 0xFFF) << 20 | u32(rs1) << 15 | (7u << 12) | u32(rd) << 7 | Op::OP_IMM;
}
```

---

## 三、踩的坑：运算符优先级（`&` 的括号不能省）

⚠️ **`(u32(imm) & 0xFFF)` 外面的括号绝对不能丢！**

C++ 优先级：`<<`（左移）> `&`（按位与）> `|`（按位或）。

```cpp
// ❌ 错误：& 优先级低于 <<，会先算 0xFFF << 20，再算 imm & (0xFFF << 20)
//    结果 = 只留 imm 高 12 位，立即数算错（小立即数直接变 0）
(u32(imm)) & 0xFFF << 20

// ✅ 正确：先 &（截到 12 位），再 <<（放到第 31~20 位）
(u32(imm) & 0xFFF) << 20
```

**口诀**：`&` 和 `<<` 连用时，想"先截断再移位"，就必须给 `&` 加括号。

---

## 四、加完怎么验证

1. 重新编译（Qt Creator 里点运行，或 `cmake --build`）；
2. 在 `main.cpp` 里加一行测试，比如：

```cpp
"andi x5, x1, 3\n"    // x1=5(0b101) & 3(0b011) = 1
```

跑完看 `cpu.reg(5)` 是不是 1。

---

## 五、后续指令对照（都按这个套路）

| 指令 | 类别 | funct3 | 执行操作 |
|---|---|---|---|
| addi | OP_IMM | 0 | `+` |
| andi | OP_IMM | 7 | `&` |
| ori | OP_IMM | 6 | `\|` |
| xori | OP_IMM | 4 | `^` |

> 加 ori/xori 时，照抄 andi 块，改指令名、funct3、运算符即可。
