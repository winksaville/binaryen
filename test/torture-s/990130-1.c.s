	.text
	.file	"/b/build/slave/linux/build/src/src/work/gcc/gcc/testsuite/gcc.c-torture/execute/990130-1.c"
	.section	.text.main,"ax",@progbits
	.hidden	main
	.globl	main
	.type	main,@function
main:                                   # @main
	.result 	i32
	.local  	i32, i32, i32, i32
# BB#0:                                 # %entry
	i32.const	$1=, 0
	i32.load	$2=, count($1)
	i32.load	$0=, dummy($1)
	i32.const	$3=, 1
	i32.add 	$3=, $2, $3
	i32.store	$discard=, count($1), $3
	#APP
	#NO_APP
	i32.store	$discard=, dummy($1), $0
	block   	.LBB0_2
	br_if   	$2, .LBB0_2
# BB#1:                                 # %if.end
	call    	exit@FUNCTION, $1
	unreachable
.LBB0_2:                                # %if.then
	call    	abort@FUNCTION
	unreachable
.Lfunc_end0:
	.size	main, .Lfunc_end0-main

	.hidden	count                   # @count
	.type	count,@object
	.section	.bss.count,"aw",@nobits
	.globl	count
	.align	2
count:
	.int32	0                       # 0x0
	.size	count, 4

	.hidden	dummy                   # @dummy
	.type	dummy,@object
	.section	.bss.dummy,"aw",@nobits
	.globl	dummy
	.align	2
dummy:
	.int32	0                       # 0x0
	.size	dummy, 4


	.ident	"clang version 3.8.0 "
	.section	".note.GNU-stack","",@progbits
