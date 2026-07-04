add $sp, $zero, $imm1, 2048, 0		# Initialize stack pointer

	# ---------------------------------------------------------
	# 1. Write 5 times to 7-segment display and LEDs
	# ---------------------------------------------------------
	add $t0, $zero, $imm1, 5, 0		# $t0 = 5 (loop counter)
	add $t1, $zero, $imm1, 1, 0		# $t1 = 1 (initial display value)
L_DISP:
	out $t1, $zero, $imm1, 9, 0		# write to leds
	out $t1, $zero, $imm1, 10, 0		# write to display7seg
	sll $t1, $t1, $imm1, 1, 0		# shift left to change value
	sub $t0, $t0, $imm1, 1, 0		# counter--
	bgt $imm1, $t0, $zero, L_DISP, 0	# loop 5 times

	# ---------------------------------------------------------
	# 2. Run Timer for 3 ticks via ISR
	# ---------------------------------------------------------
	add $s0, $zero, $zero, 0, 0		# $s0 = tick counter (used by ISR)
	add $t0, $zero, $imm1, ISR, 0		# get ISR address
	out $t0, $zero, $imm1, 6, 0		# irqhandler = ISR
	add $t0, $zero, $imm1, 1, 0		
	out $t0, $zero, $imm1, 0, 0		# irq0enable = 1
	add $t0, $zero, $imm1, 100, 0		
	out $t0, $zero, $imm1, 13, 0		# timermax = 100
	add $t0, $zero, $imm1, 1, 0
	out $t0, $zero, $imm1, 11, 0		# timerenable = 1
L_TIMER:
	add $t0, $zero, $imm1, 3, 0
	blt $imm1, $s0, $t0, L_TIMER, 0		# Poll $s0 until it reaches 3
	out $zero, $zero, $imm1, 11, 0		# Stop timer

	# ---------------------------------------------------------
	# 3. Draw 10 first pixels of the monitor
	# ---------------------------------------------------------
	add $t0, $zero, $zero, 0, 0		# Pixel address = 0
	add $t1, $zero, $imm1, 10, 0		# Counter = 10
	add $t2, $zero, $imm1, 255, 0		# Color = 255 (White)
L_PIX:
	out $t0, $zero, $imm1, 20, 0		# monitoraddr
	out $t2, $zero, $imm1, 21, 0		# monitordata
	add $t3, $zero, $imm1, 1, 0
	out $t3, $zero, $imm1, 22, 0		# monitorcmd = 1 (draw)
	add $t0, $t0, $imm1, 1, 0		# address++
	sub $t1, $t1, $imm1, 1, 0		# count--
	bgt $imm1, $t1, $zero, L_PIX, 0		# loop 10 times

	# ---------------------------------------------------------
	# 4. Change 2 first disk sectors to 0x12345678
	# ---------------------------------------------------------
	add $t0, $zero, $imm1, 1024, 0		# Buffer address
	add $t1, $zero, $imm1, 128, 0		# 1 sector = 128 words
	lw $t2, $zero, $imm1, 256, 0		# Load 0xFEEDFEED from address 256
L_BUF:
	sw $t2, $t0, $zero, 0, 0		# Write to buffer
	add $t0, $t0, $imm1, 1, 0		# buffer_addr++
	sub $t1, $t1, $imm1, 1, 0		# count--
	bgt $imm1, $t1, $zero, L_BUF, 0		# Fill buffer

	# Write buffer to Sector 0
	add $t0, $zero, $imm1, 1024, 0
	out $t0, $zero, $imm1, 16, 0		# diskbuffer = 1024
	out $zero, $zero, $imm1, 15, 0		# disksector = 0
	add $t0, $zero, $imm1, 2, 0
	out $t0, $zero, $imm1, 14, 0		# diskcmd = 2 (write)
L_POLL0:
	in $t0, $zero, $imm1, 17, 0		# Read diskstatus
	bne $imm1, $t0, $zero, L_POLL0, 0	# Wait until disk is free (status == 0)

	# Write buffer to Sector 1
	add $t0, $zero, $imm1, 1, 0
	out $t0, $zero, $imm1, 15, 0		# disksector = 1
	add $t0, $zero, $imm1, 2, 0
	out $t0, $zero, $imm1, 14, 0		# diskcmd = 2 (write)
L_POLL1:
	in $t0, $zero, $imm1, 17, 0		# Read diskstatus
	bne $imm1, $t0, $zero, L_POLL1, 0	# Wait until disk is free

	halt $zero, $zero, $zero, 0, 0		# Done, halt system

# ==============================================================================
# Timer ISR
# ==============================================================================
ISR:
	in $t0, $zero, $imm1, 3, 0		# Read irq0status
	beq $zero, $t0, $zero, ISR_END, 0	# Ensure it was the timer
	add $s0, $s0, $imm1, 1, 0		# Increment our tick counter ($s0)
	out $zero, $zero, $imm1, 3, 0		# Clear irq0status
ISR_END:
	reti $zero, $zero, $zero, 0, 0		# Return to main

# ==============================================================================
# Data Segment
# ==============================================================================
	.word 256 0x12345678			# Hardcode 0x12345678 at address 256
