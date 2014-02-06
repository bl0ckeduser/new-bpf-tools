##########################################
##### jump tables prototype for x86 ######
##### Wed Feb  5 21:40:24 EST 2014 #######
##########################################

.section .rodata
_echo_format: .string "jump ptr=0x%x\n"
_str_const_0: .string "foo\n"
_str_const_1: .string "bar\n"
_str_const_2: .string "baz\n"
_line: .string "--------------------\n"

########################## JUMP TABLE #########################
jt0: .long jt0_l0, jt0_l1, jt0_l2
###############################################################

.section .text
.globl main

.type proc, @function
proc:
pushl %ebp
movl %esp, %ebp
subl $68, %esp

###########################

# arg: 0=>foo, 1=>bar, 2=>baz
movl 8(%ebp), %eax
movl $jt0, %ebx
imull $4, %eax
addl %eax, %ebx

pushl %ebx
call echo
addl $4, %esp

jmp (%ebx)

jt0_l0:
movl $_str_const_0, %esi
movl %esi, 0(%esp)	# argument 0 to printf
call printf
addl $4, %esp	# throw off arg
movl %eax, -4(%ebp)
jmp end_jt0

jt0_l1:
subl $4, %esp
movl $_str_const_1, %esi
movl %esi, 0(%esp)	# argument 0 to printf
call printf
addl $4, %esp	# throw off arg
movl %eax, -4(%ebp)
jmp end_jt0

jt0_l2:
subl $4, %esp
movl $_str_const_2, %esi
movl %esi, 0(%esp)	# argument 0 to printf
call printf
addl $4, %esp	# throw off arg
movl %eax, -4(%ebp)
jmp end_jt0

end_jt0:

pushl $_line
call printf
addl $4, %esp

# clean up stack and return
addl $68, %esp
movl %ebp, %esp
popl %ebp
ret

.type main, @function
main:
pushl %ebp
movl %esp, %ebp
subl $68, %esp

subl $4, %esp

####################### proc(0, 1, 2)	###############
pushl $0
call proc
addl $4, %esp

pushl $1
call proc
addl $4, %esp

pushl $2
call proc
addl $4, %esp
########################################################

addl $68, %esp
movl %ebp, %esp
popl %ebp
ret

.type echo, @function
echo:
pushl $0
pushl 8(%esp)
pushl $_echo_format
call printf
addl $12, %esp  # get rid of the printf args
ret
