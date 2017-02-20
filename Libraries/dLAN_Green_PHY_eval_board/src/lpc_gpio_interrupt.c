#include "lpc_gpio_interrupt.h"

struct interruptHandlerGPIO {
	interruptHandlerFunc func;
	int port;
	int pin;
};

#define NUMBER_OF_INTERRUPT_HANDLER 5

struct interruptDispatcherGPIO {
	struct interruptHandlerGPIO handler[NUMBER_OF_INTERRUPT_HANDLER];
	int used;
};

static struct interruptDispatcherGPIO dispatcher = {
		.handler = {{NULL,0,0}},
		.used = 0
};

Status registerInterruptHandlerGPIO(int port, int pin, interruptHandlerFunc func)
{
	Status rv = ERROR;
	if(func && dispatcher.used < NUMBER_OF_INTERRUPT_HANDLER)
	{
		int i;
		Bool found = FALSE;
		portENTER_CRITICAL();
		for(i=0;i<dispatcher.used;i+=1)
		{
			if(dispatcher.handler[i].port == port && dispatcher.handler[i].pin == pin)
			{
				found = TRUE;
			}
		}
		if(!found)
		{
			if(dispatcher.used == 0)
			{
				NVIC_SetPriority(EINT3_IRQn, configGPIO_INTERRUPT_PRIORITY);
				NVIC_EnableIRQ(EINT3_IRQn);
			}
			dispatcher.handler[dispatcher.used].func = func;
			dispatcher.handler[dispatcher.used].port = port;
			dispatcher.handler[dispatcher.used].pin  = pin;
			dispatcher.used += 1;
			rv = SUCCESS;
		}
		portEXIT_CRITICAL();
	}
	return rv;
}

Status unregisterInterruptHandlerGPIO(int port, int pin)
{
	Status rv=ERROR;
	int i;
	portENTER_CRITICAL();
	for(i=0;i<dispatcher.used;i+=1)
	{
		if(dispatcher.handler[i].port == port && dispatcher.handler[i].pin == pin)
		{
			dispatcher.used -= 1;
			if(dispatcher.used == 0)
			{
				NVIC_DisableIRQ(EINT3_IRQn);
			}
			else if( i != dispatcher.used )
			{
				dispatcher.handler[i].func = dispatcher.handler[dispatcher.used].func;
				dispatcher.handler[i].port = dispatcher.handler[dispatcher.used].port;
				dispatcher.handler[i].pin = dispatcher.handler[dispatcher.used].pin;
			}
			dispatcher.handler[dispatcher.used].func = NULL;
			dispatcher.handler[dispatcher.used].port = 0;
			dispatcher.handler[dispatcher.used].pin  = 0;
			rv = SUCCESS;
			break;
		}
	}
	portEXIT_CRITICAL();
	return rv;
}

void EINT3_IRQHandler (void)
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	int i;
	uint32_t pins[4] = {}; /* There are 4 GPIO ports, but only 0 and 2 are used for interrupts */

	pins[GPIOINT_PORT0] = ( Chip_GPIOINT_GetStatusFalling(LPC_GPIOINT, GPIOINT_PORT0)
						  | Chip_GPIOINT_GetStatusRising(LPC_GPIOINT, GPIOINT_PORT0) );

	pins[GPIOINT_PORT2] = ( Chip_GPIOINT_GetStatusFalling(LPC_GPIOINT, GPIOINT_PORT2)
						  | Chip_GPIOINT_GetStatusRising(LPC_GPIOINT, GPIOINT_PORT2) );

	for(i=0;i<dispatcher.used;i+=1)
	{
		if( pins[dispatcher.handler[i].port] & (1 << dispatcher.handler[i].pin) )
		{
			dispatcher.handler[i].func(&xHigherPriorityTaskWoken);
		}
	}

	/* Clear all possible interrupt sources */
	Chip_GPIOINT_ClearIntStatus(LPC_GPIOINT, GPIOINT_PORT0, pins[GPIOINT_PORT0]);
	Chip_GPIOINT_ClearIntStatus(LPC_GPIOINT, GPIOINT_PORT2, pins[GPIOINT_PORT2]);

	portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}

/*-----------------------------------------------------------*/
