; ===================================================
; checkpointBackupReg
; ===================================================
    .global checkpointBackupReg
 
checkpointBackupReg: .asmfunc
    dint
    nop

    ; R12 假設放的是 buffer address
    mov.w   r1,   2(r12)
    mov.w   r2,   4(r12)
    mov.w   r3,   6(r12)
    mov.w   r4,   8(r12)
    mov.w   r5,  10(r12)
    mov.w   r6,  12(r12)
    mov.w   r7,  14(r12)
    mov.w   r8,  16(r12)
    mov.w   r9,  18(r12)
    mov.w   r10, 20(r12)
    mov.w   r11, 22(r12)
    mov.w   r12, 24(r12)
    mov.w   r13, 26(r12)
    mov.w   r14, 28(r12)
    mov.w   r15, 30(r12)

    ret
    .endasmfunc