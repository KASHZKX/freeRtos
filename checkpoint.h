/*
 * Checkpoint support for the FreeRTOS Lab 5 checkpoint exercise.
 */

#ifndef FREERTOS_LAB5_CHECKPOINT_H
#define FREERTOS_LAB5_CHECKPOINT_H

#include <stdint.h>


void checkpointCommit( void );
void checkpointPowerOff( void );
extern void checkpointBackupReg(uint16_t *buf);

#endif /* FREERTOS_LAB5_CHECKPOINT_H */
