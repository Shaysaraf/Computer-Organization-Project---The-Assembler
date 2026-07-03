# Bubble Sort 16 numbers in descending order (0x100 to 0x10F)
# Format: opcode rd, rs, rt, imm1, imm2

add $a0, $zero, $imm1, 0x100, 0          # $a0 = 0x100 (base array pointer)
add $a1, $zero, $imm1, 16, 0             # $a1 = 16 (array size)
add $s0, $zero, $zero, 0, 0              # $s0 = i = 0 (outer loop index)

L_OUTER: bge $imm1, $s0, $a1, L_END, 0            # if (i >= 16) goto L_END
add $s1, $zero, $zero, 0, 0              # $s1 = j = 0 (inner loop index)

L_INNER: sub $a2, $imm1, $s0, 15, 0               # $a2 = 15 - i (inner loop bound)
bge $imm1, $s1, $a2, L_INNER_END, 0      # if (j >= 15 - i) goto L_INNER_END

add $t0, $a0, $s1, 0, 0                  # $t0 = &data[j]
add $a3, $t0, $imm1, 1, 0                # $a3 = &data[j+1]

lw $t1, $t0, $zero, 0, 0                 # $t1 = data[j]
lw $t2, $a3, $zero, 0, 0                 # $t2 = data[j+1]

bge $imm1, $t1, $t2, L_SKIP_SWAP, 0      # if (data[j] >= data[j+1]) skip swap (descending order)

sw $t2, $t0, $zero, 0, 0                 # data[j] = $t2
sw $t1, $a3, $zero, 0, 0                 # data[j+1] = $t1

L_SKIP_SWAP: add $s1, $s1, $imm1, 1, 0                # j++
beq $imm1, $zero, $zero, L_INNER, 0      # unconditional jump to L_INNER

L_INNER_END: add $s0, $s0, $imm1, 1, 0                # i++
beq $imm1, $zero, $zero, L_OUTER, 0      # unconditional jump to L_OUTER

L_END: halt $zero, $zero, $zero, 0, 0           # Halt simulator execution
.word 0x100, 23               
.word 0x101, 30                   
.word 0x102, 13
.word 0x103, 10
.word 0x104, 71
.word 0x105, 2               
.word 0x106, 80                   
.word 0x107, 5
.word 0x108, 12
.word 0x109, 1
.word 0x10A, 2               
.word 0x10B, 70                   
.word 0x10C, 6
.word 0x10D, 111
.word 0x10E, 17
.word 0x10F, 26