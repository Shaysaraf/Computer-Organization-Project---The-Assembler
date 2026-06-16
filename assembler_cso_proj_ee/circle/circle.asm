lw $s1, $zero, $imm1, 0x100, 0    # $s1 = Center Address (flat frame buffer index)
lw $s0, $zero, $imm1, 0x101, 0    # $s0 = Radius (R)
mul $t0, $s0, $s0, 0, 0           # $t0 = R^2 (Radius squared for bounding check)
sub $t1, $zero, $s0, 0, 0         # $t1 = dy (Starts at -R)
LoopY:
    bgt $imm1, $t1, $s0, End, 0   # if dy > R, drawing loop is complete -> jump to End
    sub $t2, $zero, $s0, 0, 0     # $t2 = dx (Resets to -R for each new row)
LoopX:
    bgt $imm1, $t2, $s0, NextY, 0 # if dx > R, row is complete -> jump to NextY
    mul $a0, $t2, $t2, 0, 0       # $a0 = dx^2
    mul $a1, $t1, $t1, 0, 0       # $a1 = dy^2
    add $a0, $a0, $a1, 0, 0       # $a0 = dx^2 + dy^2
    bgt $imm1, $a0, $t0, NextX, 0 # if (dx^2 + dy^2) > R^2, pixel is outside -> skip drawing
    sll $a1, $t1, $imm1, 8, 0     # $a1 = dy * 256 (Shift left by 8 is equivalent to multiplying by 256)
    add $a1, $a1, $t2, 0, 0       # $a1 = (dy * 256) + dx (Total relative coordinate offset)
    add $a2, $s1, $a1, 0, 0       # $a2 = Center Address + relative offset
    out $a2, $zero, $imm1, 20, 0  # Write the target pixel offset to monitoraddr (Register 20)
    out $imm1, $zero, $zero, 21, 255 # Write White color value (255) to monitordata (Register 21)
    out $imm1, $zero, $zero, 22, 1   # Write Command Strobe (1) to monitorcmd (Register 22) to draw

NextX:
    add $t2, $t2, $imm1, 1, 0     # dx = dx + 1
    beq $imm1, $zero, $zero, LoopX, 0 # Unconditional branch back to LoopX

NextY:
    add $t1, $t1, $imm1, 1, 0     # dy = dy + 1
    beq $imm1, $zero, $zero, LoopY, 0 # Unconditional branch back to LoopY

End:
    halt $zero, $zero, $zero, 0, 0 # Safely stop execution and turn control back to simulator
    .word 0x100, 32896                # Vector coordinate baseline (Index: 128*256 + 128)
    .word 0x101, 30                   # Circle Radius setup parameter (30 pixels)