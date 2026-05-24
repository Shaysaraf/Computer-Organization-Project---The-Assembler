    lw $s1, $zero, $imm1, 0x100, 0    # Load Center Address into $s1 from memory address 0x100
    lw $s0, $zero, $imm1, 0x101, 0    # Load Radius (R) into $s0 from memory address 0x101
    mul $t0, $s0, $s0, 0, 0           # Calculate R^2 and store in $t0
    sub $t1, $zero, $s0, 0, 0         # Initialize dy = -R (store in $t1)
LoopY:
    bgt $imm2, $t1, $s0, 0, End       # if dy > R, we are done with the circle, jump to End
    sub $t2, $zero, $s0, 0, 0         # Initialize dx = -R (store in $t2)
LoopX:
    bgt $imm2, $t2, $s0, 0, NextY     # if dx > R, we are done with this row, jump to NextY
    mul $a0, $t2, $t2, 0, 0           # Calculate dx^2 and store in $a0
    mul $a1, $t1, $t1, 0, 0           # Calculate dy^2 and store in $a1
    add $a0, $a0, $a1, 0, 0           # Calculate dist = dx^2 + dy^2 (store in $a0)
    bgt $imm2, $a0, $t0, 0, NextX     # if dist > R^2, pixel is outside the circle, jump to NextX
    sll $a1, $t1, $imm1, 8, 0         # Pixel is inside the circle! Calculate its address. Calculate offset = (dy * 256) + dx
    add $a1, $a1, $t2, 0, 0           # Add dx to complete the offset (offset = dy * 256 + dx)
    add $a2, $s1, $a1, 0, 0           # pixel_addr = center_address + offset
    out $a2, $zero, $zero, 20, 0      # Write the pixel address to monitoraddr (I/O port 20)
    out $imm2, $zero, $zero, 21, 255  # Write the white color (255) to monitordata (I/O port 21)
    out $imm2, $zero, $zero, 22, 1    # Write 1 to monitorcmd (I/O port 22) to command the monitor to draw
NextX:
    add $t2, $t2, $imm1, 1, 0         # Increment dx by 1
    beq $imm2, $zero, $zero, 0, LoopX # Unconditional jump back to LoopX
NextY:
    add $t1, $t1, $imm1, 1, 0             # Increment dy by 1  
    beq $imm2, $zero, $zero, 0, LoopY     # Unconditional jump back to LoopY
End:
    halt $zero, $zero, $zero, 0, 0        # Halt execution
    .word 0x100, 32896                    # Example center address (e.g. y=128, x=128 -> 128*256 + 128)
    .word 0x101, 30                       # Example radius (30 pixels)