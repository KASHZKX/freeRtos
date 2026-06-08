/*
 * Checkpoint support for the FreeRTOS Lab 5 checkpoint exercise.
 */

#ifndef FREERTOS_LAB5_CHECKPOINT_H
#define FREERTOS_LAB5_CHECKPOINT_H

#include <stdint.h>

void checkpointRestore( void );
void checkpointCommit( void );
void checkpointPowerOff( void );
extern void checkpointRestoreReg0(void);
extern void checkpointRestoreReg1(void);
extern void checkpointBackupReg0(void);
extern void checkpointBackupReg1(void);
#endif /* FREERTOS_LAB5_CHECKPOINT_H */
