.syntax unified
.arm
.text

.global run_mov
.type run_mov, %function
run_mov:
    dsb sy
    push {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    mov r0, #0
    pop {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    bx lr

.global run_udf
.type run_udf, %function
run_udf:
    dsb sy
    push {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    udf #0

.global run_ldrex
.type run_ldrex, %function
run_ldrex:
    dsb sy
    push {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    ldrex r1, [r0]
    mov r0, r1
    pop {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    bx lr
