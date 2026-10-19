.text
.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    subq $64, %rsp
    movl $10, %eax
    movl %eax, -8(%rbp)
    movl $20, %eax
    movl %eax, -16(%rbp)
    movl $10, %eax
    movl $20, %ecx
    cmpl %ecx, %eax
    setl %al
    movzbl %al, %eax
    movl %eax, -24(%rbp)
    movl -24(%rbp), %eax
    cmpl $0, %eax
    je L1
    movl -8(%rbp), %eax
    addl $1, %eax
    movl %eax, -32(%rbp)
    movl -32(%rbp), %eax
    movl %eax, -8(%rbp)
    jmp L2

L1:
    movl -8(%rbp), %eax
    addl $2, %eax
    movl %eax, -40(%rbp)
    movl -40(%rbp), %eax
    movl %eax, -8(%rbp)

L2:

L3:
    movl -8(%rbp), %eax
    movl $15, %ecx
    cmpl %ecx, %eax
    setl %al
    movzbl %al, %eax
    movl %eax, -48(%rbp)
    movl -48(%rbp), %eax
    cmpl $0, %eax
    je L4
    movl -8(%rbp), %eax
    addl $1, %eax
    movl %eax, -56(%rbp)
    movl -56(%rbp), %eax
    movl %eax, -8(%rbp)
    jmp L3

L4:
    movl -8(%rbp), %eax
    leave
    ret
.section .note.GNU-stack,"",@progbits
