; void l2_run_on_stack(void (*fn)(void), void *top)
; Calls fn with sp moved to top (our own stack), then puts sp back.

    .text
    .align 4
    .global l2_run_on_stack
    .type l2_run_on_stack, @function
l2_run_on_stack:
    st      sp, [r1, -4]
    st      blink, [r1, -8]
    sub     sp, r1, 16
    jl      [r0]
    ld      blink, [sp, 8]
    ld      sp, [sp, 12]
    j       [blink]
    .size l2_run_on_stack, . - l2_run_on_stack
