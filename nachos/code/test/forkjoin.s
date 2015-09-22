	.file	1 "forkjoin.c"
	.rdata
	.align	2
$LC0:
	.ascii	"Parent PID: \000"
	.align	2
$LC1:
	.ascii	"Child PID: \000"
	.align	2
$LC2:
	.ascii	"Child's parent PID: \000"
	.align	2
$LC3:
	.ascii	"Child called sleep at time: \000"
	.align	2
$LC4:
	.ascii	"Child returned from sleep at time: \000"
	.align	2
$LC5:
	.ascii	"Child executed \000"
	.align	2
$LC6:
	.ascii	" instructions.\n\000"
	.align	2
$LC7:
	.ascii	"Parent after fork waiting for child: \000"
	.align	2
$LC8:
	.ascii	"Parent executed \000"
	.text
	.align	2
	.globl	main
	.ent	main
main:
	.frame	$fp,40,$31		# vars= 16, regs= 2/0, args= 16, extra= 0
	.mask	0xc0000000,-4
	.fmask	0x00000000,0
	subu	$sp,$sp,40
	sw	$31,36($sp)
	sw	$fp,32($sp)
	move	$fp,$sp
	jal	__main
	sw	$0,16($fp)
	la	$4,$LC0
	jal	system_PrintString
	jal	system_GetPID
	move	$4,$2
	jal	system_PrintInt
	li	$4,10			# 0xa
	jal	system_PrintChar
	jal	system_Fork
	sw	$2,16($fp)
	lw	$2,16($fp)
	bne	$2,$0,$L2
	la	$4,$LC1
	jal	system_PrintString
	jal	system_GetPID
	move	$4,$2
	jal	system_PrintInt
	li	$4,10			# 0xa
	jal	system_PrintChar
	la	$4,$LC2
	jal	system_PrintString
	jal	system_GetPPID
	move	$4,$2
	jal	system_PrintInt
	li	$4,10			# 0xa
	jal	system_PrintChar
	jal	system_GetTime
	sw	$2,20($fp)
	li	$4,100			# 0x64
	jal	system_Sleep
	jal	system_GetTime
	sw	$2,24($fp)
	la	$4,$LC3
	jal	system_PrintString
	lw	$4,20($fp)
	jal	system_PrintInt
	li	$4,10			# 0xa
	jal	system_PrintChar
	la	$4,$LC4
	jal	system_PrintString
	lw	$4,24($fp)
	jal	system_PrintInt
	li	$4,10			# 0xa
	jal	system_PrintChar
	la	$4,$LC5
	jal	system_PrintString
	jal	system_GetNumInstr
	move	$4,$2
	jal	system_PrintInt
	la	$4,$LC6
	jal	system_PrintString
	j	$L3
$L2:
	la	$4,$LC7
	jal	system_PrintString
	lw	$4,16($fp)
	jal	system_PrintInt
	li	$4,10			# 0xa
	jal	system_PrintChar
	lw	$4,16($fp)
	jal	system_Join
	la	$4,$LC8
	jal	system_PrintString
	jal	system_GetNumInstr
	move	$4,$2
	jal	system_PrintInt
	la	$4,$LC6
	jal	system_PrintString
$L3:
	move	$2,$0
	move	$sp,$fp
	lw	$31,36($sp)
	lw	$fp,32($sp)
	addu	$sp,$sp,40
	j	$31
	.end	main
