# 测试 add/sub/addi/and/or/xor/andi/ori/xori
lui x1, 0x12345
addi x1, x1, 0x678
lui x2, 0x00FF0
addi x2, x2, 0x0FF
add x3, x1, x2
sub x4, x1, x2
addi x5, x1, 0x0FF
and x6, x1, x2
or x7, x1, x2
xor x8, x1, x2
andi x9, x1, 0x0FF
ori x10, x1, 0x0FF
xori x11, x1, -1

# 测试 sll/srl/sra/slli/srli/srai
# 预期：x3=4, x6=0x3FFFFFFC, x7=0xFFFFFFFC(-4), x8=4, x9=0x7FFFFFF8, x10=0xFFFFFFF8(-8)
addi x1, x0, 1
addi x2, x0, 2
sll x3, x1, x2
addi x4, x0, -16
addi x5, x0, 2
srl x6, x4, x5
sra x7, x4, x5
slli x8, x1, 2
srli x9, x4, 1
srai x10, x4, 1

# 测试 slt/sltu/slti/sltiu
# 预期：x3=1, x4=0, x5=0, x6=1, x7=1, x8=0, x9=0, x10=1
addi x1, x0, -5
addi x2, x0, 3
slt x3, x1, x2
slt x4, x2, x1
sltu x5, x1, x2
sltu x6, x2, x1
slti x7, x1, 3
slti x8, x2, -5
sltiu x9, x1, 3
sltiu x10, x2, -5

# 测试 lui/auipc/jal/jalr/beq/bne/blt/bge/bltu/bgeu
# 预期（跳转都成立，被跳过的 addi 不执行）：x5=1, x7=1, x10=1, x13=1, x16=1, x19=1, x22=1, x25=1
lui x1, 0x12345
auipc x2, 0x10000
addi x3, x0, 164    # jalr 目标 = 下方第 2 条（跳过 addi x7,0），原来 32 会跳回文件开头死循环
jal x4, 8
addi x5, x0, 0
addi x5, x0, 1
jalr x6, 0(x3)
addi x7, x0, 0
addi x7, x0, 1
addi x8, x0, 5
addi x9, x0, 5
beq x8, x9, 8
addi x10, x0, 0
addi x10, x0, 1
addi x11, x0, 3
addi x12, x0, 5
bne x11, x12, 8
addi x13, x0, 0
addi x13, x0, 1
addi x14, x0, -1
addi x15, x0, 1
blt x14, x15, 8
addi x16, x0, 0
addi x16, x0, 1
addi x17, x0, 5
addi x18, x0, 5
bge x17, x18, 8
addi x19, x0, 0
addi x19, x0, 1
addi x20, x0, -1
addi x21, x0, 1
bltu x21, x20, 8
addi x22, x0, 0
addi x22, x0, 1
addi x23, x0, 1
addi x24, x0, -1
bgeu x24, x23, 8
addi x25, x0, 0
addi x25, x0, 1

# 测试 lb/lh/lw/lbu/lhu/sb/sh/sw
# 预期：x3=0xFFFFFF80(lb符号扩展), x4=0x00000080(lbu零扩展), x5=0x00000080(lh), x6=0x00000080(lhu), x7=0x12345678(lw)
addi x30, x0, 0x100
addi x1, x0, 0x80
sb x1, 0(x30)
sh x1, 4(x30)
lui x2, 0x12345
addi x2, x2, 0x678
sw x2, 8(x30)
lb x3, 0(x30)
lbu x4, 0(x30)
lh x5, 4(x30)
lhu x6, 4(x30)
lw x7, 8(x30)
