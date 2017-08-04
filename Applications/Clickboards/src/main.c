/*
===============================================================================
 Name        : main.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include "debug.h"
#include "clickboardIO.h"


// TODO: insert other include files here
#include <FreeRTOS.h>
#include "task.h"

// TODO: insert other definitions and declarations here
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook(void) {
	printToUart("Malloc failed!\r\n");
	for (;;)
		;
}

/*-----------------------------------------------------------*/

void vApplicationTickHook(void) {
	/* Called from every tick interrupt */

	DEBUG_EXECUTE(
			{ static size_t old_mem = 0; size_t mem = xPortGetFreeHeapSize(); if(old_mem != mem) { DEBUG_PRINT(DEBUG_INFO,"application free heap: %d(0x%x)\r\n",mem,mem); old_mem = mem; } });
}

/*-----------------------------------------------------------*/

void vApplicationIdleHook(void) {
	/* Called from idle task */
}

/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook(xTaskHandle pxTask, signed char *pcTaskName) {
	/* This function will get called if a task overflows its stack. */

	(void) pxTask;
	(void) pcTaskName;

	for (;;)
		;
}

/*-----------------------------------------------------------*/

void vConfigureTimerForRunTimeStats(void) {
	//const unsigned long TCR_COUNT_RESET = 2, CTCR_CTM_TIMER = 0x00,
	    //        TCR_COUNT_ENABLE = 0x01;

	    /* This function configures a timer that is used as the time base when
	     collecting run time statistical information - basically the percentage
	     of CPU time that each task is utilising.  It is called automatically when
	     the scheduler is started (assuming configGENERATE_RUN_TIME_STATS is set
	     to 1). */

	    /* Power up and feed the timer. */
	    //LPC_SC->PCONP |= 0x02UL;
	    Chip_TIMER_Init(LPC_TIMER0);

	    //LPC_SC->PCLKSEL0 = (LPC_SC->PCLKSEL0 & (~(0x3 << 2))) | (0x01 << 2);
	    Chip_Clock_SetPCLKDiv(SYSCTL_PCLK_TIMER0, SYSCTL_CLKDIV_1);

	    /* Reset Timer 0 */
	    // LPC_TIM0->TCR = TCR_COUNT_RESET;
	    Chip_TIMER_Reset(LPC_TIMER0);

	    /* Just count up. */
	    //LPC_TIM0->CTCR = CTCR_CTM_TIMER;
	    Chip_TIMER_TIMER_SetCountClockSrc(LPC_TIMER0, 0, 0);

	    /* Prescale to a frequency that is good enough to get a decent resolution,
	     but not too fast so as to overflow all the time. */
	    //LPC_TIM0->PR = ( configCPU_CLOCK_HZ / 10000UL) - 1UL;
	    Chip_TIMER_PrescaleSet(LPC_TIMER0, ( configCPU_CLOCK_HZ / 10000UL) - 1UL);

	    /* Start the counter. */
	    //LPC_TIM0->TCR = TCR_COUNT_ENABLE;
	    Chip_TIMER_Enable(LPC_TIMER0);
}



int main(void) {
#if defined (__USE_LPCOPEN)
    // Read clock settings and update SystemCoreClock variable
    SystemCoreClockUpdate();
#if !defined(NO_BOARD_LIB)
    // Set up and initialize all required blocks and
    // functions related to the board hardware
    Board_Init();
#endif
#endif

    // TODO: insert code here

    // Force the counter to be placed into memory

    Clickboard_Init();

	printInit(LPC_USART0);
	debug_init(LPC_USART0);

	printToUart("\r\ndevolo dLAN Green PHY Module\r\n");

	// Set the LED to the state of "On"
	Board_LED_Set(LEDS_LED1, USR_LED_ON);

	// TODO: need to start a Task before reaching here
	vTaskStartScheduler();

    return 0 ;
}
