    .cdecls C,LIST,"msp430.h"
    .text
    .align 2

    .global checkpointPowerOff
    .def checkpointPowerOff

checkpointPowerOff: .asmfunc

    bic.w   #GIE, SR
    nop
    mov.b   #PMMPW_H, &PMMCTL0_H
    bis.b   #PMMREGOFF, &PMMCTL0_L
    bic.b   #SVSHE, &PMMCTL0_L
    mov.b   #000h, &PMMCTL0_H
    bis.w   #CPUOFF+OSCOFF+SCG0+SCG1, SR
    nop

    .endasmfunc