/*
 * Checkpoint support for the FreeRTOS Lab 5 checkpoint exercise.
 */

#ifndef FREERTOS_LAB5_CHECKPOINT_H
#define FREERTOS_LAB5_CHECKPOINT_H

#include <stdint.h>

void FreeRTOSLab_CheckpointRestore( void );
void FreeRTOSLab_CheckpointCommit( void );
void FreeRTOSLab_RequestPowerFail( void );
uint32_t FreeRTOSLab_GetCheckpointSequence( void );

#endif /* FREERTOS_LAB5_CHECKPOINT_H */
