# 访存测试：sw/lw、sb/lb、sh/lh、小端序
# 预期：x6=42，x7=0xFFFFFFFF，x8=0xFFFFFFFE，x9=300，x10=0x44，x11=0x33
addi x2, x0, 100
addi x5, x0, 42
sw x5, 0(x2)
lw x6, 0(x2)
addi x5, x0, -1
sb x5, 0(x2)
lb x7, 0(x2)
addi x5, x0, -2
sh x5, 2(x2)
lh x8, 2(x2)
addi x5, x0, 300
sh x5, 2(x2)
lh x9, 2(x2)
lui x5, 0x11223
addi x5, x5, 0x344
sw x5, 0(x2)
lb x10, 0(x2)
addi x2, x2, 1
lb x11, 0(x2)
