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
 * This project provides two demo applications.  A simple blinky style project,
 * and a more comprehensive test and demo application.  The
 * mainCREATE_SIMPLE_BLINKY_DEMO_ONLY setting (defined in this file) is used to
 * select between the two.  The simply blinky demo is implemented and described
 * in main_blinky.c.  The more comprehensive test and demo application is
 * implemented and described in main_full.c.
 *
 * This file implements the code that is not demo specific, including the
 * hardware setup and standard FreeRTOS hook functions.
 *
 * ENSURE TO READ THE DOCUMENTATION PAGE FOR THIS PORT AND DEMO APPLICATION ON
 * THE http://www.FreeRTOS.org WEB SITE FOR FULL INFORMATION ON USING THIS DEMO
 * APPLICATION, AND ITS ASSOCIATE FreeRTOS ARCHITECTURE PORT!
 *
 */

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "checkpoint.h"

/* Standard demo includes, used so the tick hook can exercise some FreeRTOS
functionality in an interrupt. */
#include "EventGroupsDemo.h"
#include "TaskNotify.h"
//#include "ParTest.h" /* LEDs - a historic name for "Parallel Port". */

/* TI includes. */
#include "driverlib.h"

#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>

/* Set mainCREATE_SIMPLE_BLINKY_DEMO_ONLY to one to run the simple blinky demo,
or 0 to run the more comprehensive test and demo application. */
#define mainCREATE_SIMPLE_BLINKY_DEMO_ONLY	1

/*-----------------------------------------------------------*/

/*
 * Configure the hardware as necessary to run this demo.
 */
static void prvSetupHardware( void );
static void prvCheckpointCopy( volatile uint8_t *pucDestination, const volatile uint8_t *pucSource, size_t xLength );
static uint32_t prvCheckpointChecksumBytes( const volatile uint8_t *pucData, size_t xLength, uint32_t ulSeed );
static uint16_t prvCheckpointSramOffset( void );
static uint16_t prvCheckpointSramSize( void );
static uint8_t prvCheckpointFindBestSlot( void );
static int prvCheckpointSlotIsValid( uint8_t ucSlot );
static uint32_t prvCheckpointCalculateSlotChecksum( uint8_t ucSlot );

/*
 * main_blinky() is used when mainCREATE_SIMPLE_BLINKY_DEMO_ONLY is set to 1.
 * main_full() is used when mainCREATE_SIMPLE_BLINKY_DEMO_ONLY is set to 0.
 */
#if( mainCREATE_SIMPLE_BLINKY_DEMO_ONLY == 1 )
	extern void main_lab( void );
#else
	extern void main_full( void );
#endif /* #if mainCREATE_SIMPLE_BLINKY_DEMO_ONLY == 1 */

/* Prototypes for the standard FreeRTOS callback/hook functions implemented
within this file. */
void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );
void vApplicationTickHook( void );
void vApplicationSetupTimerInterrupt( void );

extern uint8_t __checkpoint_bss_start;
extern uint8_t __checkpoint_bss_end;
extern uint8_t __checkpoint_data_start;
extern uint8_t __checkpoint_data_end;

/* The heap is allocated here so the "persistent" qualifier can be used.  This
requires configAPPLICATION_ALLOCATED_HEAP to be set to 1 in FreeRTOSConfig.h.
See http://www.freertos.org/a00111.html for more information. */
#ifdef __ICC430__
	__persistent 					/* IAR version. */
#else
	#pragma PERSISTENT( ucHeap ) 	/* CCS version. */
#endif
uint8_t ucHeap[ configTOTAL_HEAP_SIZE ] = { 0 };

/*-----------------------------------------------------------*/

#define checkpointNUM_SLOTS              ( 2U )
#define checkpointMAGIC                  ( 0x43504B54UL )
#define checkpointCOMMITTED              ( 0x544B5043UL )
#define checkpointINVALID_SLOT           ( 0xFFU )
#define checkpointFR5994_RAM_START       ( 0x1C00U )
#define checkpointFR5994_RAM_SIZE        ( 0x1000U )
#define checkpointFR5969_RAM_SIZE        ( 0x0800U )

#if defined( __MSP430FR5994__ )
	#define checkpointSRAM_BASE          checkpointFR5994_RAM_START
	#define checkpointSRAM_BACKUP_SIZE   checkpointFR5994_RAM_SIZE
#elif defined( __MSP430FR5969__ )
	#define checkpointSRAM_BASE          checkpointFR5994_RAM_START
	#define checkpointSRAM_BACKUP_SIZE   checkpointFR5969_RAM_SIZE
#else
	#define checkpointSRAM_BASE          checkpointFR5994_RAM_START
	#define checkpointSRAM_BACKUP_SIZE   checkpointFR5994_RAM_SIZE
#endif

typedef struct CheckpointCpuContext
{
	jmp_buf xEnvironment;
} CheckpointCpuContext_t;

typedef struct CheckpointSlot
{
	uint32_t ulMagic;
	uint32_t ulCommitted;
	uint32_t ulSequence;
	uint32_t ulChecksum;
	uint16_t usSramOffset;
	uint16_t usSramSize;
	uint16_t usHeapSize;
	CheckpointCpuContext_t xCpuContext;
	uint8_t ucHeapCopy[ configTOTAL_HEAP_SIZE ];
	uint8_t ucSramCopy[ checkpointSRAM_BACKUP_SIZE ];
} CheckpointSlot_t;

typedef struct CheckpointMeta
{
	uint32_t ulMagic;
	uint32_t ulSequence;
	uint8_t ucActiveSlot;
	uint8_t ucRestoreRequested;
} CheckpointMeta_t;

#if defined( __TI_COMPILER_VERSION__ )
	#pragma DATA_SECTION( xCheckpointSlots, ".checkpoint_backup" )
	#pragma RETAIN( xCheckpointSlots )
	#pragma DATA_SECTION( xCheckpointMeta, ".checkpoint_meta" )
	#pragma RETAIN( xCheckpointMeta )
#endif
static CheckpointSlot_t xCheckpointSlots[ checkpointNUM_SLOTS ];
static CheckpointMeta_t xCheckpointMeta;

int main( void )
{
	/* See http://www.FreeRTOS.org/MSP430FR5969_Free_RTOS_Demo.html */

	/* Configure the hardware ready to run the demo. */
	prvSetupHardware();

	/* Restore the most recent complete FRAM checkpoint, when one exists. */
	FreeRTOSLab_CheckpointRestore();

	/* The mainCREATE_SIMPLE_BLINKY_DEMO_ONLY setting is described at the top
	of this file. */
	#if( mainCREATE_SIMPLE_BLINKY_DEMO_ONLY == 1 )
	{
		main_lab();
	}
	#else
	{
		main_full();
	}
	#endif

	return 0;
}
/*-----------------------------------------------------------*/

void FreeRTOSLab_CheckpointCommit( void )
{
uint8_t ucNextSlot;
volatile int iSetjmpResult;

	taskDISABLE_INTERRUPTS();

	if( xCheckpointMeta.ulMagic != checkpointMAGIC )
	{
		xCheckpointMeta.ulMagic = checkpointMAGIC;
		xCheckpointMeta.ulSequence = 0UL;
		xCheckpointMeta.ucActiveSlot = checkpointINVALID_SLOT;
		xCheckpointMeta.ucRestoreRequested = 0U;
	}

	ucNextSlot = ( xCheckpointMeta.ucActiveSlot == 0U ) ? 1U : 0U;
	xCheckpointSlots[ ucNextSlot ].ulMagic = checkpointMAGIC;
	xCheckpointSlots[ ucNextSlot ].ulCommitted = 0UL;
	xCheckpointSlots[ ucNextSlot ].ulSequence = xCheckpointMeta.ulSequence + 1UL;
	xCheckpointSlots[ ucNextSlot ].usHeapSize = ( uint16_t ) configTOTAL_HEAP_SIZE;
	xCheckpointSlots[ ucNextSlot ].usSramOffset = prvCheckpointSramOffset();
	xCheckpointSlots[ ucNextSlot ].usSramSize = prvCheckpointSramSize();

	prvCheckpointCopy( xCheckpointSlots[ ucNextSlot ].ucHeapCopy, ucHeap, configTOTAL_HEAP_SIZE );
	prvCheckpointCopy( xCheckpointSlots[ ucNextSlot ].ucSramCopy,
					   ( const volatile uint8_t * ) ( ( uintptr_t ) checkpointSRAM_BASE + xCheckpointSlots[ ucNextSlot ].usSramOffset ),
					   xCheckpointSlots[ ucNextSlot ].usSramSize );

	iSetjmpResult = setjmp( xCheckpointSlots[ ucNextSlot ].xCpuContext.xEnvironment );

	if( iSetjmpResult == 0 )
	{
		xCheckpointSlots[ ucNextSlot ].ulChecksum = prvCheckpointCalculateSlotChecksum( ucNextSlot );
		xCheckpointSlots[ ucNextSlot ].ulCommitted = checkpointCOMMITTED;
		xCheckpointMeta.ulSequence = xCheckpointSlots[ ucNextSlot ].ulSequence;
		xCheckpointMeta.ucActiveSlot = ucNextSlot;
		xCheckpointMeta.ucRestoreRequested = 0U;

		taskENABLE_INTERRUPTS();
	}
	else
	{
		xCheckpointMeta.ucRestoreRequested = 0U;
		taskENABLE_INTERRUPTS();
	}
}
/*-----------------------------------------------------------*/

void FreeRTOSLab_CheckpointRestore( void )
{
uint8_t ucSlot;

	ucSlot = prvCheckpointFindBestSlot();

	if( ucSlot == checkpointINVALID_SLOT )
	{
		return;
	}

	prvCheckpointCopy( ucHeap, xCheckpointSlots[ ucSlot ].ucHeapCopy, configTOTAL_HEAP_SIZE );
	prvCheckpointCopy( ( volatile uint8_t * ) ( ( uintptr_t ) checkpointSRAM_BASE + xCheckpointSlots[ ucSlot ].usSramOffset ),
					   xCheckpointSlots[ ucSlot ].ucSramCopy,
					   xCheckpointSlots[ ucSlot ].usSramSize );

	/* main() is bypassed by longjmp, so re-arm the FreeRTOS tick first. */
	vApplicationSetupTimerInterrupt();
	longjmp( xCheckpointSlots[ ucSlot ].xCpuContext.xEnvironment, 1 );
}
/*-----------------------------------------------------------*/

void FreeRTOSLab_RequestPowerFail( void )
{
	xCheckpointMeta.ucRestoreRequested = 1U;
	PMM_turnOffRegulator();
	__bis_SR_register( LPM4_bits | GIE );

	for( ;; )
	{
		__no_operation();
	}
}
/*-----------------------------------------------------------*/

uint32_t FreeRTOSLab_GetCheckpointSequence( void )
{
	if( xCheckpointMeta.ulMagic != checkpointMAGIC )
	{
		return 0UL;
	}

	return xCheckpointMeta.ulSequence;
}
/*-----------------------------------------------------------*/

static uint8_t prvCheckpointFindBestSlot( void )
{
uint8_t ucBestSlot = checkpointINVALID_SLOT;
uint8_t ucSlot;

	if( xCheckpointMeta.ulMagic == checkpointMAGIC )
	{
		if( ( xCheckpointMeta.ucActiveSlot < checkpointNUM_SLOTS ) &&
			( prvCheckpointSlotIsValid( xCheckpointMeta.ucActiveSlot ) != 0 ) )
		{
			ucBestSlot = xCheckpointMeta.ucActiveSlot;
		}
	}

	for( ucSlot = 0U; ucSlot < checkpointNUM_SLOTS; ucSlot++ )
	{
		if( prvCheckpointSlotIsValid( ucSlot ) != 0 )
		{
			if( ( ucBestSlot == checkpointINVALID_SLOT ) ||
				( xCheckpointSlots[ ucSlot ].ulSequence > xCheckpointSlots[ ucBestSlot ].ulSequence ) )
			{
				ucBestSlot = ucSlot;
			}
		}
	}

	return ucBestSlot;
}
/*-----------------------------------------------------------*/

static int prvCheckpointSlotIsValid( uint8_t ucSlot )
{
	if( ucSlot >= checkpointNUM_SLOTS )
	{
		return 0;
	}

	if( xCheckpointSlots[ ucSlot ].ulMagic != checkpointMAGIC )
	{
		return 0;
	}

	if( xCheckpointSlots[ ucSlot ].ulCommitted != checkpointCOMMITTED )
	{
		return 0;
	}

	if( xCheckpointSlots[ ucSlot ].usHeapSize != ( uint16_t ) configTOTAL_HEAP_SIZE )
	{
		return 0;
	}

	if( xCheckpointSlots[ ucSlot ].usSramSize > checkpointSRAM_BACKUP_SIZE )
	{
		return 0;
	}

	return xCheckpointSlots[ ucSlot ].ulChecksum == prvCheckpointCalculateSlotChecksum( ucSlot );
}
/*-----------------------------------------------------------*/

static uint32_t prvCheckpointCalculateSlotChecksum( uint8_t ucSlot )
{
uint32_t ulChecksum = 2166136261UL;

	ulChecksum = prvCheckpointChecksumBytes( xCheckpointSlots[ ucSlot ].ucHeapCopy,
											 xCheckpointSlots[ ucSlot ].usHeapSize,
											 ulChecksum );
	ulChecksum = prvCheckpointChecksumBytes( xCheckpointSlots[ ucSlot ].ucSramCopy,
											 xCheckpointSlots[ ucSlot ].usSramSize,
											 ulChecksum );
	ulChecksum = prvCheckpointChecksumBytes( ( const volatile uint8_t * ) &( xCheckpointSlots[ ucSlot ].xCpuContext ),
											 sizeof( xCheckpointSlots[ ucSlot ].xCpuContext ),
											 ulChecksum );
	ulChecksum ^= xCheckpointSlots[ ucSlot ].ulSequence;
	ulChecksum *= 16777619UL;

	return ulChecksum;
}
/*-----------------------------------------------------------*/

static void prvCheckpointCopy( volatile uint8_t *pucDestination, const volatile uint8_t *pucSource, size_t xLength )
{
	while( xLength > 0U )
	{
		*pucDestination = *pucSource;
		pucDestination++;
		pucSource++;
		xLength--;
	}
}
/*-----------------------------------------------------------*/

static uint32_t prvCheckpointChecksumBytes( const volatile uint8_t *pucData, size_t xLength, uint32_t ulSeed )
{
	while( xLength > 0U )
	{
		ulSeed ^= ( uint32_t ) *pucData;
		ulSeed *= 16777619UL;
		pucData++;
		xLength--;
	}

	return ulSeed;
}
/*-----------------------------------------------------------*/

static uint16_t prvCheckpointSramOffset( void )
{
uintptr_t uxBssStart = ( uintptr_t ) &__checkpoint_bss_start;
uintptr_t uxDataStart = ( uintptr_t ) &__checkpoint_data_start;
uintptr_t uxStart = ( uxBssStart < uxDataStart ) ? uxBssStart : uxDataStart;

	return ( uint16_t ) ( uxStart - ( uintptr_t ) checkpointSRAM_BASE );
}
/*-----------------------------------------------------------*/

static uint16_t prvCheckpointSramSize( void )
{
uintptr_t uxBssStart = ( uintptr_t ) &__checkpoint_bss_start;
uintptr_t uxBssEnd = ( uintptr_t ) &__checkpoint_bss_end;
uintptr_t uxDataStart = ( uintptr_t ) &__checkpoint_data_start;
uintptr_t uxDataEnd = ( uintptr_t ) &__checkpoint_data_end;
uintptr_t uxStart = ( uxBssStart < uxDataStart ) ? uxBssStart : uxDataStart;
uintptr_t uxEnd = ( uxBssEnd > uxDataEnd ) ? uxBssEnd : uxDataEnd;
uintptr_t uxMaxEnd = ( uintptr_t ) checkpointSRAM_BASE + checkpointSRAM_BACKUP_SIZE;

	if( uxEnd > uxMaxEnd )
	{
		uxEnd = uxMaxEnd;
	}

	if( uxEnd <= uxStart )
	{
		return 0U;
	}

	return ( uint16_t ) ( uxEnd - uxStart );
}
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */

	/* Force an assert. */
	configASSERT( ( volatile void * ) NULL );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected.
	See http://www.freertos.org/Stacks-and-stack-overflow-checking.html */

	/* Force an assert. */
	configASSERT( ( volatile void * ) NULL );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
    __bis_SR_register( LPM4_bits + GIE );
    __no_operation();
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{
	#if( mainCREATE_SIMPLE_BLINKY_DEMO_ONLY == 0 )
	{
		/* Call the periodic event group from ISR demo. */
		vPeriodicEventGroupsProcessing();

		/* Call the code that 'gives' a task notification from an ISR. */
		xNotifyTaskFromISR();
	}
	#endif
}
/*-----------------------------------------------------------*/

/* The MSP430X port uses this callback function to configure its tick interrupt.
This allows the application to choose the tick interrupt source.
configTICK_VECTOR must also be set in FreeRTOSConfig.h to the correct
interrupt vector for the chosen tick interrupt source.  This implementation of
vApplicationSetupTimerInterrupt() generates the tick from timer A0, so in this
case configTICK_VECTOR is set to TIMER0_A0_VECTOR. */
void vApplicationSetupTimerInterrupt( void )
{
const unsigned short usACLK_Frequency_Hz = 32768;

	/* Ensure the timer is stopped. */
	TA0CTL = 0;

	/* Run the timer from the ACLK. */
	TA0CTL = TASSEL_1;

	/* Clear everything to start with. */
	TA0CTL |= TACLR;

	/* Set the compare match value according to the tick rate we want. */
	TA0CCR0 = usACLK_Frequency_Hz / configTICK_RATE_HZ;

	/* Enable the interrupts. */
	TA0CCTL0 = CCIE;

	/* Start up clean. */
	TA0CTL |= TACLR;

	/* Up mode. */
	TA0CTL |= MC_1;
}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
    /* Stop Watchdog timer. */
    WDT_A_hold( __MSP430_BASEADDRESS_WDT_A__ );

	/* Set all GPIO pins to output and low. */
	GPIO_setOutputLowOnPin( GPIO_PORT_P1, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7 );
	GPIO_setOutputLowOnPin( GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7 );
	GPIO_setOutputLowOnPin( GPIO_PORT_P3, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7 );
	GPIO_setOutputLowOnPin( GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7 );
	GPIO_setOutputLowOnPin( GPIO_PORT_PJ, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7 | GPIO_PIN8 | GPIO_PIN9 | GPIO_PIN10 | GPIO_PIN11 | GPIO_PIN12 | GPIO_PIN13 | GPIO_PIN14 | GPIO_PIN15 );
	GPIO_setAsOutputPin( GPIO_PORT_P1, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7 );
	GPIO_setAsOutputPin( GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7 );
	GPIO_setAsOutputPin( GPIO_PORT_P3, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7 );
	GPIO_setAsOutputPin( GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7 );
	GPIO_setAsOutputPin( GPIO_PORT_PJ, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7 | GPIO_PIN8 | GPIO_PIN9 | GPIO_PIN10 | GPIO_PIN11 | GPIO_PIN12 | GPIO_PIN13 | GPIO_PIN14 | GPIO_PIN15 );

	/* Configure P2.0 - UCA0TXD and P2.1 - UCA0RXD. */
	GPIO_setOutputLowOnPin( GPIO_PORT_P2, GPIO_PIN0 );
	GPIO_setAsOutputPin( GPIO_PORT_P2, GPIO_PIN0 );
	GPIO_setAsPeripheralModuleFunctionInputPin( GPIO_PORT_P2, GPIO_PIN1, GPIO_SECONDARY_MODULE_FUNCTION );
	GPIO_setAsPeripheralModuleFunctionOutputPin( GPIO_PORT_P2, GPIO_PIN0, GPIO_SECONDARY_MODULE_FUNCTION );

	/* Set PJ.4 and PJ.5 for LFXT. */
	GPIO_setAsPeripheralModuleFunctionInputPin(  GPIO_PORT_PJ, GPIO_PIN4 + GPIO_PIN5, GPIO_PRIMARY_MODULE_FUNCTION  );

	/* Set DCO frequency to 8 MHz. */
	CS_setDCOFreq( CS_DCORSEL_0, CS_DCOFSEL_6 );

	/* Set external clock frequency to 32.768 KHz. */
	CS_setExternalClockSource( 32768, 0 );

	/* Set ACLK = LFXT. */
	CS_initClockSignal( CS_ACLK, CS_LFXTCLK_SELECT, CS_CLOCK_DIVIDER_1 );

	/* Set SMCLK = DCO with frequency divider of 1. */
	CS_initClockSignal( CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1 );

	/* Set MCLK = DCO with frequency divider of 1. */
	CS_initClockSignal( CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1 );

	/* Start XT1 with no time out. */
	CS_turnOnLFXT( CS_LFXT_DRIVE_0 );

	/* Disable the GPIO power-on default high-impedance mode. */
	PMM_unlockLPM5();
}
/*-----------------------------------------------------------*/

int _system_pre_init( void )
{
    /* Stop Watchdog timer. */
    WDT_A_hold( __MSP430_BASEADDRESS_WDT_A__ );

    /* Return 1 for segments to be initialised. */
    return 1;
}


