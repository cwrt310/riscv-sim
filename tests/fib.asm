# 斐波那契 F(10) = 55（迭代 + 倒计数循环）
# 预期：x1=0，x2=34，x3=55，x4=55；日志 58 行
addi x1, x0, 9
addi x2, x0, 0
addi x3, x0, 1
beq x1, x0, 24
add x4, x3, x2
addi x2, x3, 0
addi x3, x4, 0
addi x1, x1, -1
jal x0, -20
