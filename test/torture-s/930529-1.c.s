	.text
	.file	"/b/build/slave/linux/build/src/src/work/gcc/gcc/testsuite/gcc.c-torture/execute/930529-1.c"
	.section	.text.dd,"ax",@progbits
	.hidden	dd
	.globl	dd
	.type	dd,@function
dd:                                     # @dd
	.param  	i32, i32
	.result 	i32
# BB#0:                                 # %entry
	i32.div_s	$push0=, $0, $1
	return  	$pop0
.Lfunc_end0:
	.size	dd, .Lfunc_end0-dd

	.section	.text.main,"ax",@progbits
	.hidden	main
	.globl	main
	.type	main,@function
main:                                   # @main
	.result 	i32
# BB#0:                                 # %entry
	i32.const	$push0=, 0
	call    	exit@FUNCTION, $pop0
	unreachable
.Lfunc_end1:
	.size	main, .Lfunc_end1-main


	.ident	"clang version 3.8.0 "
	.section	".note.GNU-stack","",@progbits
