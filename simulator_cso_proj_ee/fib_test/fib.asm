	add $t2, $zero, $imm1, 1, 0			# $t2 = 1
	out $t2, $zero, $imm1, 2, 0			# enable irq2
	add $sp, $imm1, $imm2, 1024, 1024		# $sp = 2048
	add $t2, $zero, $imm1, L3, 0			# $t2 = address of L3
	out $t2, $zero, $imm1, 6, 0			# set irqhandler as L3
	lw $a0, $zero, $imm1, 256, 0			# get x from address 256
	jal $ra, $imm1, $zero, fib, 0			# calc $v0 = fib(x)
	sw $v0, $zero, $imm1, 257, 0			# store fib(x) in 257
	halt $zero, $zero, $zero, 0, 0			# halt
fib:
	add $sp, $sp, $imm1, -3, 0			# adjust stack for 3 items
	sw $s0, $sp, $imm1, 2, 0			# save $s0
	sw $ra, $sp, $imm1, 1, 0			# save return address
	sw $a0, $sp, $imm1, 0, 0			# save argument
	add $t2, $zero, $imm1, 1, 0			# $t2 = 1
	bgt $imm1, $a0, $t2, L1, 0			# jump to L1 if x > 1
	add $v0, $a0, $zero, 0, 0			# otherwise, fib(x) = x, copy input
	beq $imm1, $zero, $zero, L2, 0			# jump to L2
L1:
	sub $a0, $a0, $imm1, 1, 0			# calculate x - 1
	jal $ra, $imm1, $zero, fib, 0			# calc $v0=fib(x-1)
	add $s0, $v0, $zero, 0, 0			# $s0 = fib(x-1)
	lw $a0, $sp, $imm1, 0, 0			# restore $a0 = x
	sub $a0, $a0, $imm1, 2, 0			# calculate x - 2
	jal $ra, $imm1, $zero, fib, 0			# calc fib(x-2)
	add $v0, $v0, $s0, 0, 0				# $v0 = fib(x-2) + fib(x-1)
	lw $a0, $sp, $imm1, 0, 0			# restore $a0
	lw $ra, $sp, $imm1, 1, 0			# restore $ra
	lw $s0, $sp, $imm1, 2, 0			# restore $s0
L2:
	add $sp, $sp, $imm1, 3, 0			# pop 3 items from stack
	add $t0, $a0, $zero, 0, 0			# $t0 = $a0
	sll $t0, $t0, $imm1, 16, 0			# $t0 = $t0 << 16
	add $t0, $t0, $v0, 0, 0				# $t0 = $t0 + $v0
	out $t0, $zero, $imm1, 10, 0			# write $t0 to display
	beq $ra, $zero, $zero, 0, 0			# and return
L3:
	in $t1, $zero, $imm1, 9, 0			# read leds register into $t1
	sll $t1, $t1, $imm1, 1, 0			# left shift led pattern to the left
	or $t1, $t1, $imm1, 1, 0			# lit up the rightmost led
	out $t1, $zero, $imm1, 9, 0			# write the new led pattern
	add $t1, $zero, $imm1, 255, 0			# $t1 = 255
	out $t1, $zero, $imm1, 21, 0			# set pixel color to white
	add $t1, $zero, $imm1, 1, 0			# $t1 = 1
	out $t1, $zero, $imm1, 22, 0			# draw pixel
	in $t1, $zero, $imm1, 20, 0			# read pixel address
	add $t1, $t1, $imm1, 257, 0			# $t1 += 257
	out $t1, $zero, $imm1, 20, 0			# update address
	out $zero, $zero, $imm1, 5, 0			# clear irq2 status
	reti $zero, $zero, $zero, 0, 0			# return from interrupt
	.word 256 7
