/*
 * FreeRTOS V202107.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/******************************************************************************
 * NOTE 1:  This project provides two demo applications.  A simple blinky
 * style project, and a more comprehensive test and demo application.  The
 * mainCREATE_SIMPLE_BLINKY_DEMO_ONLY setting in main.c is used to select
 * between the two.  See the notes on using mainCREATE_SIMPLE_BLINKY_DEMO_ONLY
 * in main.c.  This file implements the simply blinky style version.
 *
 * NOTE 2:  This file only contains the source code that is specific to the
 * basic demo.  Generic functions, such FreeRTOS hook functions, and functions
 * required to configure the hardware are defined in main.c.
 ******************************************************************************
 *
 * For Lab 5 this file runs a single checkpoint test task.  The task commits a
 * full checkpoint every ten iterations, occasionally enters LPM4.5 to simulate
 * power failure, and then continues from the last committed CPU context after
 * reset or power restoration.
 */

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "checkpoint.h"

/* Standard demo includes. */
#include "partest.h"

#include <stdio.h>

/* Priorities at which the tasks are created. */
#define mainCHECKPOINT_TASK_PRIORITY		( tskIDLE_PRIORITY + 2 )

/* The task commits every ten iterations, matching the checkpoint guide. */
#define mainCHECKPOINT_PERIOD				( 10UL )
#define mainTASK_PERIOD_MS					( pdMS_TO_TICKS( 200 ) )
#define mainPOWER_FAIL_MINIMUM_ITERATION	( mainCHECKPOINT_PERIOD * 2UL )

/* The LED toggled by the Rx task. */
#define mainTASK_LED						( 0 )

/*-----------------------------------------------------------*/

/*
 * Called by main when mainCREATE_SIMPLE_BLINKY_DEMO_ONLY is set to 1 in
 * main.c.
 */
void main_lab( void );

/*
 * The checkpoint task described in the comments at the top of this file.
 */
static void prvCheckpointTask( void *pvParameters );
static uint32_t prvNextPseudoRandom( uint32_t ulState );
static BaseType_t prvShouldSimulatePowerFail( uint32_t ulIteration, uint32_t ulRandomState );

/*-----------------------------------------------------------*/

/* Deliberately stored in .bss so the SRAM checkpoint is observable. */
static volatile uint32_t ulCheckpointIterations;

/*-----------------------------------------------------------*/

void main_lab( void )
{
	xTaskCreate( prvCheckpointTask,
				 "CHK",
				 configMINIMAL_STACK_SIZE,
				 NULL,
				 mainCHECKPOINT_TASK_PRIORITY,
				 NULL );

	vTaskStartScheduler();

	/* If all is well, the scheduler will now be running, and the following
	line will never be reached.  If the following line does execute, then
	there was insufficient FreeRTOS heap memory available for the Idle and/or
	timer tasks to be created.  See the memory management section on the
	FreeRTOS web site for more details on the FreeRTOS heap
	http://www.freertos.org/a00111.html. */
	for( ;; );
}
/*-----------------------------------------------------------*/

static void prvCheckpointTask( void *pvParameters )
{
TickType_t xNextWakeTime;
uint32_t ulIteration = ulCheckpointIterations;
uint32_t ulRandomState = 0xA5A55A5AUL ^ FreeRTOSLab_GetCheckpointSequence();

	/* Remove compiler warning about unused parameter. */
	( void ) pvParameters;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	for( ;; )
	{
		vTaskDelayUntil( &xNextWakeTime, mainTASK_PERIOD_MS );

		ulIteration++;
		ulCheckpointIterations = ulIteration;
		ulRandomState = prvNextPseudoRandom( ulRandomState + ulIteration );

		if( ( ulIteration % mainCHECKPOINT_PERIOD ) == 0UL )
		{
			vParTestToggleLED( mainTASK_LED );
			FreeRTOSLab_CheckpointCommit();
		}

		printf( "checkpoint i=%u seq=%u\r\n",
				( unsigned int ) ulIteration,
				( unsigned int ) FreeRTOSLab_GetCheckpointSequence() );

		if( prvShouldSimulatePowerFail( ulIteration, ulRandomState ) != pdFALSE )
		{
			FreeRTOSLab_RequestPowerFail();
		}
	}
}
/*-----------------------------------------------------------*/

static uint32_t prvNextPseudoRandom( uint32_t ulState )
{
	ulState ^= ulState << 13;
	ulState ^= ulState >> 17;
	ulState ^= ulState << 5;

	return ulState;
}
/*-----------------------------------------------------------*/

static BaseType_t prvShouldSimulatePowerFail( uint32_t ulIteration, uint32_t ulRandomState )
{
	if( ulIteration < mainPOWER_FAIL_MINIMUM_ITERATION )
	{
		return pdFALSE;
	}

	if( ( ulIteration % mainCHECKPOINT_PERIOD ) == 0UL )
	{
		return pdFALSE;
	}

	return ( ( ulRandomState & 0x1FU ) == 0U ) ? pdTRUE : pdFALSE;
}
/*-----------------------------------------------------------*/

