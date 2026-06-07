[繁體中文](README.zh-TW.md) | [English](README.en.md)

# FreeRTOS Lab 5 Checkpoint

This project implements the Lab 5 checkpoint mechanism on the MSP430FR5994 FreeRTOS demo. The system periodically backs up the FreeRTOS heap, SRAM `.bss/.data`, and CPU context into FRAM, then resumes from the latest complete checkpoint after reset or power restoration.

## Implementation Notes

- `main.c` provides `FreeRTOSLab_CheckpointCommit()` and `FreeRTOSLab_CheckpointRestore()`.
- `ucHeap` is application allocated through `configAPPLICATION_ALLOCATED_HEAP`; each commit copies it into the checkpoint slot.
- The `.bss` and `.data` ranges are discovered through linker symbols: `__checkpoint_bss_start/end` and `__checkpoint_data_start/end`.
- CPU context is saved with `setjmp()` into the FRAM slot. Restore copies heap and SRAM first, then uses `longjmp()` to resume after the commit point.
- The checkpoint store uses two slots plus an `ucActiveSlot` valid index. The active slot is updated only after a slot is fully copied and its checksum is written.
- `Blinky_Demo/main_blinky.c` now runs a checkpoint test task: it commits every 10 iterations, then occasionally enters LPM4.5 through a pseudo-random condition.

## Memory Layout

- `lnk_msp430fr5994.cmd`
  - `.checkpoint_meta` is placed in `FRAM2` for the active slot and sequence metadata.
  - `.checkpoint_backup` is placed in `FRAM2` for two copies of heap, SRAM, and CPU context.
  - `.bss` / `.data` have RUN_START/RUN_END symbols so the checkpoint code can calculate the SRAM backup range.
- `lnk_msp430fr5969.cmd` has the same section and symbol additions; the checked-in CCS build artifacts currently target MSP430FR5994.

## Test Steps

1. Import `freeRtos/lab5` in CCS and confirm the target is MSP430FR5994 with `lnk_msp430fr5994.cmd`.
2. Build the project and inspect the `.map` file for `.checkpoint_meta`, `.checkpoint_backup`, `.bss`, `.data`, and `ucHeap`.
3. Flash and run the board. When console output is available, `checkpoint i=... seq=...` should continue increasing; the LED toggles on commits.
4. After LPM4.5, press reset, or remove power after a commit and reconnect power.
5. Confirm the iteration resumes from the latest checkpoint instead of restarting at 0.

## Checkpoint Checklist

- FRAM backup space: `.checkpoint_meta` and `.checkpoint_backup`.
- Commit order: `ucHeap` -> SRAM `.bss/.data` -> CPU context.
- Restore order: `ucHeap` -> SRAM `.bss/.data` -> CPU context.
- Double backup: two slots with checksum and a valid active index.
- First boot behavior: no valid slot means normal boot without restore.
