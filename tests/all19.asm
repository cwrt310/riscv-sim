# 19 条指令全体检：算术/逻辑/移位/比较/高位/分支/跳转/访存
# 预期见《测试手册_斐波那契与指令体检.md》测试三；日志 23 行（25 条中跳过 2 条）
addi x1, x0, 5
addi x2, x0, 10
add x3, x1, x2
sub x4, x2, x1
andi x5, x1, 3
ori x6, x1, 8
xori x7, x1, -1
slli x8, x1, 2
slti x9, x1, 10
sltiu x10, x1, 10
lui x11, 0x12345
addi x12, x0, 100
addi x13, x0, -1
sb x13, 0(x12)
lb x14, 0(x12)
addi x15, x0, 300
sh x15, 2(x12)
lh x16, 2(x12)
sw x15, 0(x12)
lw x17, 0(x12)
beq x1, x1, 8
addi x18, x0, 999
bne x1, x2, 8
addi x19, x0, 888
jal x0, 4
