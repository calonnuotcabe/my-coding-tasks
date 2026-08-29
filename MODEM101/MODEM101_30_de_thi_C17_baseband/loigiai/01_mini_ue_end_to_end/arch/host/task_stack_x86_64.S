.text
.globl bb_arch_run_task_host
.type bb_arch_run_task_host, @function
bb_arch_run_task_host:
    pushq %r12
    movq %rsp, %r12
    movq %rcx, %rsp
    andq $-16, %rsp
    movq %rdi, %rax
    movq %rsi, %rdi
    movq %rdx, %rsi
    call *%rax
    movq %r12, %rsp
    popq %r12
    ret
.size bb_arch_run_task_host, . - bb_arch_run_task_host

.section .note.GNU-stack,"",@progbits
