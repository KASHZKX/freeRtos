    .text
    .align 2
    .ref g_backupRegs
    .global checkpointBackupReg0
    .global checkpointBackupReg1
    .def checkpointBackupReg0
    .def checkpointBackupReg1

    .if $DEFINED( __LARGE_CODE_MODEL__ )
        .define "reta", ret_addr
    .else
        .define "ret",  ret_addr
    .endif

checkpointBackupReg0: .asmfunc
    dint
    nop

    movx.a r1,   g_backupRegs + 4
    movx.a r2,   g_backupRegs + 8
    movx.a r3,   g_backupRegs + 12
    movx.a r4,   g_backupRegs + 16
    movx.a r5,   g_backupRegs + 20
    movx.a r6,   g_backupRegs + 24
    movx.a r7,   g_backupRegs + 28
    movx.a r8,   g_backupRegs + 32
    movx.a r9,   g_backupRegs + 36
    movx.a r10,  g_backupRegs + 40
    movx.a r11,  g_backupRegs + 44
    movx.a r12,  g_backupRegs + 48
    movx.a r13,  g_backupRegs + 52
    movx.a r14,  g_backupRegs + 56
    movx.a r15,  g_backupRegs + 60

    ret_addr
    .endasmfunc

checkpointBackupReg1: .asmfunc
    dint
    nop

    movx.a r1,   g_backupRegs + 64 + 4
    movx.a r2,   g_backupRegs + 64 + 8
    movx.a r3,   g_backupRegs + 64 + 12
    movx.a r4,   g_backupRegs + 64 + 16
    movx.a r5,   g_backupRegs + 64 + 20
    movx.a r6,   g_backupRegs + 64 + 24
    movx.a r7,   g_backupRegs + 64 + 28
    movx.a r8,   g_backupRegs + 64 + 32
    movx.a r9,   g_backupRegs + 64 + 36
    movx.a r10,  g_backupRegs + 64 + 40
    movx.a r11,  g_backupRegs + 64 + 44
    movx.a r12,  g_backupRegs + 64 + 48
    movx.a r13,  g_backupRegs + 64 + 52
    movx.a r14,  g_backupRegs + 64 + 56
    movx.a r15,  g_backupRegs + 64 + 60

    ret_addr
    .endasmfunc