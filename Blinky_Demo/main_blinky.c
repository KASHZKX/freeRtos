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

/* Kernel includes. */
#include <stdio.h>
#include <inttypes.h>
#include "FreeRTOS.h"
#include "task.h"

/* Standard demo includes. */
#include "partest.h"

/* Priorities at which the tasks are created. */
#define mainLAB_TASK_SET            1
#define mainEDF_TASK_PRIORITY		( tskIDLE_PRIORITY + 2 )
#define mainEDF_LOG_TASK_PRIORITY   ( tskIDLE_PRIORITY + 1 )
#define mainEDF_TASK_STACK_SIZE     configMINIMAL_STACK_SIZE

typedef struct LabTaskConfig
{
    const char * cName;
    TickType_t xExecutionTime;
    TickType_t xPeriod;
    TaskHandle_t xHandle;
} LabTaskConfig_t;
/*-----------------------------------------------------------*/

#if ( mainLAB_TASK_SET == 1 )

    static const char pcTaskSetHeader[] =
        "T1 c:1 p:3\n"
        "T2 c:3 p:5\n";

    static LabTaskConfig_t xLabTasks[] =
    {
        { "T2", ( TickType_t ) 3, ( TickType_t ) 5, NULL },
        { "T1", ( TickType_t ) 1, ( TickType_t ) 3, NULL }
    };

    static LabTaskConfig_t xLogTasks[] =
    {
        { "LOG", ( TickType_t ) 0, ( TickType_t ) 0, NULL },
    };

#elif ( mainLAB_TASK_SET == 2 )

    static const char pcTaskSetHeader[] =
        "T1 c:1 p:4\n"
        "T2 c:2 p:5\n"
        "T3 c:2 p:10\n";

    static LabTaskConfig_t xLabTasks[] =
    {
        { "T2", ( TickType_t ) 2, ( TickType_t ) 5, NULL },
        { "T3", ( TickType_t ) 2, ( TickType_t ) 10, NULL },
        { "T1", ( TickType_t ) 1, ( TickType_t ) 4, NULL }
    };

    static LabTaskConfig_t xLogTasks[] =
    {
        { "LOG", ( TickType_t ) 0, ( TickType_t ) 0, NULL },
    };

#else
    #error Unsupported mainLAB_TASK_SET value.
#endif

#define mainTASK_COUNT    ( sizeof( xLabTasks ) / sizeof( xLabTasks[ 0 ] ) )

/*
 * Called by main when mainCREATE_SIMPLE_BLINKY_DEMO_ONLY is set to 1 in
 * main.c.
 */
void main_lab( void );
static void prvEDFTask( void * pvParameters );
static void prvLogTask( void * pvParameters );

void main_lab( void )
{
    UBaseType_t uxTaskIndex;

//    vTaskClearEDFLogBuffer();
    ( void ) xTaskCreate( prvLogTask,
                          xLogTasks[ 0 ].cName,
                          mainEDF_TASK_STACK_SIZE,
                          &( xLogTasks[ 0 ] ),
                          mainEDF_LOG_TASK_PRIORITY,
                          &( xLogTasks[ 0 ].xHandle ) );

    for( uxTaskIndex = ( UBaseType_t ) 0; uxTaskIndex < ( UBaseType_t ) mainTASK_COUNT; uxTaskIndex++ )
    {
        ( void ) xTaskCreate( prvEDFTask,
                              xLabTasks[ uxTaskIndex ].cName,
                              mainEDF_TASK_STACK_SIZE,
                              &( xLabTasks[ uxTaskIndex ] ),
                              mainEDF_TASK_PRIORITY,
                              &( xLabTasks[ uxTaskIndex ].xHandle ) );
    }

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

static void prvEDFTask( void * pvParameters )
{
    LabTaskConfig_t * const pxTaskConfig = ( LabTaskConfig_t * ) pvParameters;
    for( ;; )
    {
        prvConsumeExecutionTicks ( pxTaskConfig->xExecutionTime );
        printf("NAME: %s, C: %d, P: %d\n", pxTaskConfig->cName, (unsigned int)pxTaskConfig->xExecutionTime, (unsigned int)pxTaskConfig->xPeriod);
        vTaskDelay( 500 );
    }
}
/*-----------------------------------------------------------*/

static void prvLogTask( void * pvParameters )
{
    LabTaskConfig_t * const pxTaskConfig = ( LabTaskConfig_t * ) pvParameters;

    for( ;; )
    {
        printf("NAME: %s, C: %d, P: %d\n", pxTaskConfig->cName, (unsigned int)pxTaskConfig->xExecutionTime, (unsigned int)pxTaskConfig->xPeriod);
        vTaskDelay( 500 );
    }
}
/*-----------------------------------------------------------*/

