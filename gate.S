    .section .user_text

    .globl __syscall

__syscall:
    scall
    ret
