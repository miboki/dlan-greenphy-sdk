/* LPCOpen includes. */
#include "board.h"

/* FreeRTOS includes. */
#include <FreeRTOS.h>
#include "task.h"

/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook(void) {
	DEBUGSTR("Malloc failed!\r\n");
	for (;;)
		;
}
/*-----------------------------------------------------------*/

void vApplicationTickHook(void) {
	/* Called from every tick interrupt */
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
	/* This function configures a timer that is used as the time base when
	 collecting run time statistical information - basically the percentage
	 of CPU time that each task is utilising.  It is called automatically when
	 the scheduler is started (assuming configGENERATE_RUN_TIME_STATS is set
	 to 1). */

	/* Power up and feed the timer. */
	Chip_TIMER_Init(LPC_TIMER0);

	Chip_Clock_SetPCLKDiv(SYSCTL_PCLK_TIMER0, SYSCTL_CLKDIV_1);

	/* Reset Timer 0 */
	Chip_TIMER_Reset(LPC_TIMER0);

	/* Just count up. */
	Chip_TIMER_TIMER_SetCountClockSrc(LPC_TIMER0, 0, 0);

	/* Prescale to a frequency that is good enough to get a decent resolution,
	 but not too fast so as to overflow all the time. */
	Chip_TIMER_PrescaleSet(LPC_TIMER0, ( configCPU_CLOCK_HZ / 10000UL) - 1UL);

	/* Start the counter. */
	Chip_TIMER_Enable(LPC_TIMER0);
}
/*-----------------------------------------------------------*/
