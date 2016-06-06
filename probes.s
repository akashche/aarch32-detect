.syntax unified
.arm
.text

.global probe_mov
.type probe_mov, %function
probe_mov:
    push {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    /* mov r0, #0 */
    .word 0xe1a00000
    pop {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    bx lr

.global probe_udf
.type probe_udf, %function
probe_udf:
    push {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    /* udf #0 */
    .word 0xe7f000f0
    pop {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    bx lr

.global probe_ldrex
.type probe_ldrex, %function
probe_ldrex:
    push {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    /* ldrex r1, [r0] */
    .word 0xe1901f9f
    mov r0, r1
    pop {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    bx lr

.global probe_movt
.type probe_movt, %function
probe_movt:
    push {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    /* movt r1, #1 */
    .word 0xe3401001
    mov r0, r1
    pop {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    bx lr
