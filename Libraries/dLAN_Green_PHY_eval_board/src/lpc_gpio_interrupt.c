#include "lpc_gpio_interrupt.h"

struct interruptHandlerGPIO {
	interruptHandlerFunc func;
	int port;
	int pin;
};

#define NUMBER_OF_INTERRUPT_HANDLER 5

struct interruptDispatcherGPIO {
	struct interruptHandlerGPIO handlers[NUMBER_OF_INTERRUPT_HANDLER];
	int used;
};

static struct interruptDispatcherGPIO dispatcher = {
		.handlers = {{NULL,0,0}},
		.used = 0
};

Status registerInterruptHandlerGPIO(int port, int pin, interruptHandlerFunc func)
{
Status rv = ERROR;
uint32_t pins;
int i;
Bool found = FALSE;

	if(func && dispatcher.used < sizeof(dispatcher.handlers))
	{
		portENTER_CRITICAL();
		for(i=0;i<dispatcher.used;i+=1)
		{
			if(dispatcher.handlers[i].port == port && dispatcher.handlers[i].pin == pin)
			{
				found = TRUE;
			}
		}
		if(!found)
		{
			/* Interrupt pin setup. */
			Chip_GPIO_SetPinDIRInput(LPC_GPIO, port, pin);
			/* Read current pin settings and enable passed pin. */
			pins = ( Chip_GPIOINT_GetIntFalling(LPC_GPIOINT, port) | (1 << pin) );
			Chip_GPIOINT_SetIntFalling(LPC_GPIOINT, port, pins);
			pins = ( Chip_GPIOINT_GetIntRising(LPC_GPIOINT, port) | (1 << pin) );
			Chip_GPIOINT_SetIntRising(LPC_GPIOINT, port, pins);

			dispatcher.handlers[dispatcher.used].func = func;
			dispatcher.handlers[dispatcher.used].port = port;
			dispatcher.handlers[dispatcher.used].pin  = pin;

			if(dispatcher.used == 0)
			{
				NVIC_SetPriority(EINT3_IRQn, configGPIO_INTERRUPT_PRIORITY);
				NVIC_EnableIRQ(EINT3_IRQn);
			}
			dispatcher.used++;
			rv = SUCCESS;
		}
		portEXIT_CRITICAL();
	}
	return rv;
}

Status unregisterInterruptHandlerGPIO(int port, int pin)
{
Status rv = ERROR;
uint32_t pins;
int i;

	portENTER_CRITICAL();
	for(i=0;i<dispatcher.used;i+=1)
	{
		if(dispatcher.handlers[i].port == port && dispatcher.handlers[i].pin == pin)
		{
			/* Read current pin settings and disable passed pin. */
			pins = ( Chip_GPIOINT_GetIntFalling(LPC_GPIOINT, port) & ~(1 << pin) );
			Chip_GPIOINT_SetIntFalling(LPC_GPIOINT, port, pins);
			pins = ( Chip_GPIOINT_GetIntRising(LPC_GPIOINT, port) & ~(1 << pin) );
			Chip_GPIOINT_SetIntRising(LPC_GPIOINT, port, pins);

			dispatcher.used--;
			if(dispatcher.used == 0)
			{
				NVIC_DisableIRQ(EINT3_IRQn);
			}
			/* Move last element to free space. */
			else if( i != dispatcher.used )
			{
				dispatcher.handlers[i].func = dispatcher.handlers[dispatcher.used].func;
				dispatcher.handlers[i].port = dispatcher.handlers[dispatcher.used].port;
				dispatcher.handlers[i].pin = dispatcher.handlers[dispatcher.used].pin;
			}

			dispatcher.handlers[dispatcher.used].func = NULL;
			dispatcher.handlers[dispatcher.used].port = 0;
			dispatcher.handlers[dispatcher.used].pin  = 0;
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
		if( pins[dispatcher.handlers[i].port] & (1 << dispatcher.handlers[i].pin) )
		{
			dispatcher.handlers[i].func(&xHigherPriorityTaskWoken);
		}
	}

	/* Clear all possible interrupt sources */
	Chip_GPIOINT_ClearIntStatus(LPC_GPIOINT, GPIOINT_PORT0, pins[GPIOINT_PORT0]);
	Chip_GPIOINT_ClearIntStatus(LPC_GPIOINT, GPIOINT_PORT2, pins[GPIOINT_PORT2]);

	portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}

/*-----------------------------------------------------------*/
