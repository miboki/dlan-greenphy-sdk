#include "lpc_dma.h"

struct interruptHandlerDMA {
	interruptHandlerFunc func;
};

struct interruptDispatcherDMA {
	struct interruptHandlerDMA handler[GPDMA_NUMBER_CHANNELS];
	int used;
};

static struct interruptDispatcherDMA dispatcher = {
		.handler = {{NULL}},
		.used = 0
};


Status registerInterruptHandlerDMA( uint8_t ChannelNum, interruptHandlerFunc func )
{
	Status rv = ERROR;

	portENTER_CRITICAL();
	if( func && !dispatcher.handler[ChannelNum].func ) {
		if(dispatcher.used == 0)
		{
			NVIC_SetPriority(DMA_IRQn, configDMA_INTERRUPT_PRIORITY);
			NVIC_EnableIRQ(DMA_IRQn);
		}
		dispatcher.handler[ChannelNum].func = func;
		dispatcher.used += 1;
		rv = SUCCESS;
	}
	portEXIT_CRITICAL();

	return rv;
}

Status unregisterInterruptHandlerDMA( uint8_t ChannelNum )
{
	Status rv = ERROR;

	portENTER_CRITICAL();
	if( dispatcher.handler[ChannelNum].func )
	{
		dispatcher.handler[ChannelNum].func = NULL;
		dispatcher.used -= 1;
		if(dispatcher.used == 0)
		{
			NVIC_DisableIRQ(DMA_IRQn);
		}
		rv = SUCCESS;
	}
	portEXIT_CRITICAL();

	return rv;
}

void DMA_IRQHandler( void )
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	int ChannelNum;

	for( ChannelNum = 0; ChannelNum < GPDMA_NUMBER_CHANNELS; ++ChannelNum )
	{
		if( dispatcher.handler[ChannelNum].func )
		{
			if( Chip_GPDMA_Interrupt(LPC_GPDMA, ChannelNum) == SUCCESS )
			{
				dispatcher.handler[ChannelNum].func( &xHigherPriorityTaskWoken );
				unregisterInterruptHandlerDMA( ChannelNum );
			}
		}
	}

	portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}
