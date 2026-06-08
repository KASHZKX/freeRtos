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
#define mainCHECKPOINT_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )

#define mainCHECKPOINT_DELAY	 	( pdMS_TO_TICKS( 200 ) )
#define mainCHECKPOINT_PERIOD		( 10UL )
/*-----------------------------------------------------------*/

/*
 * Called by main when mainCREATE_SIMPLE_BLINKY_DEMO_ONLY is set to 1 in
 * main.c.
 */
void main_lab( void );

/*
 * The tasks as described in the comments at the top of this file.
 */
static void prvCheckpointTask( void *pvParameters );
static BaseType_t prvRandom( const unsigned long ulCheckpointIterations );

/*-----------------------------------------------------------*/

/* variables */
static volatile uint32_t ulCheckpointIterations;

/*-----------------------------------------------------------*/

void main_lab( void )
{
	/* Start the task as described in the comments at the top of this file. */
	xTaskCreate( prvCheckpointTask,				/* The function that implements the task. */
				"CHK", 								/* The text name assigned to the task - for debug only as it is not used by the kernel. */
				configMINIMAL_STACK_SIZE, 			/* The size of the stack to allocate to the task. */
				NULL, 								/* The parameter passed to the task - not used in this case. */
				mainCHECKPOINT_TASK_PRIORITY,    	/* The priority assigned to the task. */
				NULL );								/* The task handle is not required, so NULL is passed. */

	/* Start the tasks and timer running. */
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
unsigned long ulCheckpointIterations = 0UL;

	/* Remove compiler warning about unused parameter. */
	( void ) pvParameters;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	for( ;; )
	{
	    ulCheckpointIterations++;
		/* Place this task in the blocked state until it is time to run again. */
		vTaskDelayUntil( &xNextWakeTime,  mainCHECKPOINT_DELAY );

		if(prvRandom(ulCheckpointIterations)) checkpointPowerOff(); // Todo: enter LPM4.5
		if(ulCheckpointIterations % mainCHECKPOINT_PERIOD == 0) checkpointCommit();
		printf("%d\n", (int)ulCheckpointIterations);
	}
}
/*-----------------------------------------------------------*/

static BaseType_t prvRandom( const unsigned long ulCheckpointIterations )
{
	if(ulCheckpointIterations % 24 == 0) return pdTRUE;
	return pdFALSE;
}

/*-----------------------------------------------------------*/
