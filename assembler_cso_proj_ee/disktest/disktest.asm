# disktest.asm
# Computes sector8[i] = sector0[i] + sector1[i] + ... + sector7[i] for i in 0..127
#
# Memory layout:
#   0x200 (512): disk read buffer, 128 words
#   0x280 (640): accumulator buffer, 128 words (zero-initialized)
#
# IO registers:
#   14 = diskcmd     (1 = read, 2 = write)
#   15 = disksector  (sector number)
#   16 = diskbuffer  (memory address of buffer)
#   17 = diskstatus  (0 = free, 1 = busy)
#
# Register usage:
#   $t0 = current sector (0..7)
#   $t1 = inner loop index i (0..127)
#   $t2 = temp / address computation
#   $a3 = buffer[i]
#   $s0 = buffer base address (0x200, permanent)
#   $s1 = accum base address (0x280, permanent)
#   $a0 = 128 (inner loop bound)
#   $a1 = 8 (sector count)
#   $a2 = accum[i] value
#
# opcode, rd, rs, rt, imm1, imm2

add $s0, $zero, $imm2, 0, 0x200        # $s0 = 0x200 (buffer base, permanent)
add $s1, $zero, $imm2, 0, 0x280        # $s1 = 0x280 (accum base, permanent)
add $t0, $zero, $zero, 0, 0            # sector = 0
add $a0, $zero, $imm1, 128, 0          # $a0 = 128
add $a1, $zero, $imm1, 8, 0            # $a1 = 8

sector_loop:
in $t2, $zero, $imm1, 17, 0            # $t2 = diskstatus
bne $imm1, $t2, $zero, sector_loop, 0  # poll until disk is free

out $t0, $zero, $imm1, 15, 0           # disksector = sector
out $s0, $zero, $imm1, 16, 0           # diskbuffer = 0x200
add $t2, $zero, $imm1, 1, 0            # $t2 = 1 (read command)
out $t2, $zero, $imm1, 14, 0           # diskcmd = 1

wait_read:
in $t2, $zero, $imm1, 17, 0            # $t2 = diskstatus
bne $imm1, $t2, $zero, wait_read, 0    # wait until read completes

add $t1, $zero, $zero, 0, 0            # i = 0

accum_loop:
add $t2, $s0, $t1, 0, 0                # $t2 = buffer base + i
lw $a3, $t2, $zero, 0, 0               # $a3 = buffer[i]
add $t2, $s1, $t1, 0, 0                # $t2 = accum base + i
lw $a2, $t2, $zero, 0, 0               # $a2 = accum[i]
add $a2, $a2, $a3, 0, 0                # $a2 += buffer[i]
sw $a2, $t2, $zero, 0, 0               # accum[i] = $a2  ($t2 still holds accum base + i)
add $t1, $t1, $imm1, 1, 0              # i++
blt $imm1, $t1, $a0, accum_loop, 0    # if i < 128, continue

add $t0, $t0, $imm1, 1, 0              # sector++
blt $imm1, $t0, $a1, sector_loop, 0   # if sector < 8, continue

write_wait:
in $t2, $zero, $imm1, 17, 0            # $t2 = diskstatus
bne $imm1, $t2, $zero, write_wait, 0   # wait until disk is free

add $t2, $zero, $imm1, 8, 0            # $t2 = 8
out $t2, $zero, $imm1, 15, 0           # disksector = 8
out $s1, $zero, $imm1, 16, 0           # diskbuffer = 0x280
add $t2, $zero, $imm1, 2, 0            # $t2 = 2 (write command)
out $t2, $zero, $imm1, 14, 0           # diskcmd = 2

write_done:
in $t2, $zero, $imm1, 17, 0            # $t2 = diskstatus
bne $imm1, $t2, $zero, write_done, 0   # wait until write completes

halt $zero, $zero, $zero, 0, 0
