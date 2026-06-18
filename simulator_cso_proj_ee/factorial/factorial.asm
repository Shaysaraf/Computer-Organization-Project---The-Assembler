# factorial.asm
# Computes n! recursively for the SIMP processor.
# Input:  MEM[0x100] = n
# Output: MEM[0x101] = n!
#
# Register usage:
#   $a0  ($4)  - argument: n
#   $v0  ($3)  - return value: n!
#   $ra  ($15) - return address
#   $sp  ($14) - stack pointer (grows downward from 0xFFF)
#
# Stack frame (2 words per call), $sp points to last used slot:
#   $sp+1 = saved $ra
#   $sp+0 = saved $a0 (n)  <-- $sp points here after prologue
#
# opcode, rd, rs, rt, imm1, imm2

# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------

# Initialize stack pointer to top of memory (0xFFF = 4095)
# Must use $imm2 for full 32-bit constant; $imm1 would sign-extend 0xFFF to -1
add $sp, $zero, $imm2, 0, 0xFFF        # $sp = 0xFFF

# Load n from MEM[0x100]
lw $a0, $zero, $imm2, 0, 0x100        # $a0 = MEM[0x100] = n

# Call factorial
jal $ra, $imm2, $zero, 0, factorial   # $ra = next pc, pc = factorial

# Store result to MEM[0x101]
sw $v0, $zero, $imm2, 0, 0x101        # MEM[0x101] = $v0

# Done
halt $zero, $zero, $zero, 0, 0        # halt execution

# ---------------------------------------------------------------------------
# factorial(n)  --  int factorial(int n) { if(n==0) return 1; return n*factorial(n-1); }
# ---------------------------------------------------------------------------
factorial:
# Prologue: push $ra then $a0 onto stack (stack grows downward)
sub $sp, $sp, $imm1, 1, 0             # $sp-- (allocate slot for $ra)
sw $ra, $sp, $zero, 0, 0              # MEM[$sp] = $ra
sub $sp, $sp, $imm1, 1, 0             # $sp-- (allocate slot for $a0)
sw $a0, $sp, $zero, 0, 0              # MEM[$sp] = $a0 (save n)

# Base case: if (n == 0) goto base
beq $imm1, $a0, $zero, base, 0        # if ($a0 == 0) pc = base

# Recursive case: call factorial(n-1)
sub $a0, $a0, $imm1, 1, 0             # $a0 = n - 1
jal $ra, $imm2, $zero, 0, factorial   # call factorial(n-1); $ra = return address

# Multiply: result = n * factorial(n-1)
lw $a0, $sp, $zero, 0, 0             # restore n from stack
mul $v0, $a0, $v0, 0, 0              # $v0 = n * factorial(n-1)
beq $imm1, $zero, $zero, epilogue, 0  # jump to epilogue (unconditional)

base:
# Return 1
add $v0, $zero, $imm1, 1, 0          # $v0 = 1

epilogue:
# Restore $a0 slot, restore $ra, free $ra slot, return
add $sp, $sp, $imm1, 1, 0            # $sp++ (free $a0 slot)
lw $ra, $sp, $zero, 0, 0             # $ra = MEM[$sp] (restore return address)
add $sp, $sp, $imm1, 1, 0            # $sp++ (free $ra slot)
jal $zero, $ra, $zero, 0, 0          # return: pc = $ra, link discarded ($zero)

# ---------------------------------------------------------------------------
# Data
# ---------------------------------------------------------------------------
.word 0x100 5                          # MEM[0x100] = 5  (input: compute 5! = 120)