[繁體中文](README.zh-TW.md) | [English](README.en.md)

# FreeRTOS Lab 5 Checkpoint

本專案在 MSP430FR5994 FreeRTOS demo 上實作 Lab 5 checkpoint。系統會定期把 FreeRTOS heap、SRAM 中的 `.bss/.data`、以及 CPU context 備份到 FRAM，重開機或重新供電後可從最近一次完整 checkpoint 繼續執行。

## 實作重點

- `main.c` 提供 `FreeRTOSLab_CheckpointCommit()` 與 `FreeRTOSLab_CheckpointRestore()`。
- `ucHeap` 由 `configAPPLICATION_ALLOCATED_HEAP` 配置，commit 時會複製到 checkpoint slot。
- `.bss` 與 `.data` 的範圍由 linker symbols `__checkpoint_bss_start/end` 與 `__checkpoint_data_start/end` 決定。
- CPU context 使用 `setjmp()` 存入 FRAM slot，restore 時在恢復 heap 與 SRAM 後用 `longjmp()` 回到 commit 之後。
- checkpoint 使用雙份 slot 與 `ucActiveSlot` valid index。slot 完整寫入並通過 checksum 後，才更新 active slot。
- `Blinky_Demo/main_blinky.c` 改為 checkpoint test task：每 10 次 iteration commit 一次，之後以 pseudo-random 條件進入 LPM4.5。

## 記憶體配置

- `lnk_msp430fr5994.cmd`
  - `.checkpoint_meta` 放在 `FRAM2`，保留 active slot 與序號。
  - `.checkpoint_backup` 放在 `FRAM2`，保留兩份 heap、SRAM 與 CPU context。
  - `.bss` / `.data` 加上 RUN_START/RUN_END symbols，供 checkpoint 計算 SRAM backup 範圍。
- `lnk_msp430fr5969.cmd` 也加上相同 section 與 symbols；目前 CCS 產物使用 MSP430FR5994 設定。

## 測試方式

1. 使用 CCS 匯入 `freeRtos/lab5`，確認 build target 為 MSP430FR5994 與 `lnk_msp430fr5994.cmd`。
2. Build 後查看 `.map`，確認 `.checkpoint_meta`、`.checkpoint_backup`、`.bss`、`.data` 與 `ucHeap` 位置。
3. 燒錄並執行。console 可用時會看到 `checkpoint i=... seq=...` 持續遞增，LED 會在 commit 時切換。
4. 讓程式進入 LPM4.5 後按 reset，或在 commit 之後移除電源再重新供電。
5. 確認 iteration 會從最近一次 checkpoint 後繼續，而不是從 0 重新開始。

## Checkpoint Checklist

- FRAM backup space: `.checkpoint_meta` 與 `.checkpoint_backup`。
- Commit order: `ucHeap` -> SRAM `.bss/.data` -> CPU context。
- Restore order: `ucHeap` -> SRAM `.bss/.data` -> CPU context。
- Double backup: two slots with checksum and valid active index.
- First boot behavior: no valid slot means normal boot without restore.
