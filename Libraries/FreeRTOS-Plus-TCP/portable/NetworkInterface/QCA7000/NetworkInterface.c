/*
 * FreeRTOS+TCP Labs Build 160916 (C) 2016 Real Time Engineers ltd.
 * Authors include Hein Tibosch and Richard Barry
 *
 *******************************************************************************
 ***** NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ***
 ***                                                                         ***
 ***                                                                         ***
 ***   FREERTOS+TCP IS STILL IN THE LAB (mainly because the FTP and HTTP     ***
 ***   demos have a dependency on FreeRTOS+FAT, which is only in the Labs    ***
 ***   download):                                                            ***
 ***                                                                         ***
 ***   FreeRTOS+TCP is functional and has been used in commercial products   ***
 ***   for some time.  Be aware however that we are still refining its       ***
 ***   design, the source code does not yet quite conform to the strict      ***
 ***   coding and style standards mandated by Real Time Engineers ltd., and  ***
 ***   the documentation and testing is not necessarily complete.            ***
 ***                                                                         ***
 ***   PLEASE REPORT EXPERIENCES USING THE SUPPORT RESOURCES FOUND ON THE    ***
 ***   URL: http://www.FreeRTOS.org/contact  Active early adopters may, at   ***
 ***   the sole discretion of Real Time Engineers Ltd., be offered versions  ***
 ***   under a license other than that described below.                      ***
 ***                                                                         ***
 ***                                                                         ***
 ***** NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ***
 *******************************************************************************
 *
 * FreeRTOS+TCP can be used under two different free open source licenses.  The
 * license that applies is dependent on the processor on which FreeRTOS+TCP is
 * executed, as follows:
 *
 * If FreeRTOS+TCP is executed on one of the processors listed under the Special
 * License Arrangements heading of the FreeRTOS+TCP license information web
 * page, then it can be used under the terms of the FreeRTOS Open Source
 * License.  If FreeRTOS+TCP is used on any other processor, then it can be used
 * under the terms of the GNU General Public License V2.  Links to the relevant
 * licenses follow:
 *
 * The FreeRTOS+TCP License Information Page: http://www.FreeRTOS.org/tcp_license
 * The FreeRTOS Open Source License: http://www.FreeRTOS.org/license
 * The GNU General Public License Version 2: http://www.FreeRTOS.org/gpl-2.0.txt
 *
 * FreeRTOS+TCP is distributed in the hope that it will be useful.  You cannot
 * use FreeRTOS+TCP unless you agree that you use the software 'as is'.
 * FreeRTOS+TCP is provided WITHOUT ANY WARRANTY; without even the implied
 * warranties of NON-INFRINGEMENT, MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. Real Time Engineers Ltd. disclaims all conditions and terms, be they
 * implied, expressed, or statutory.
 *
 * 1 tab == 4 spaces!
 *
 * http://www.FreeRTOS.org
 * http://www.FreeRTOS.org/plus
 * http://www.FreeRTOS.org/labs
 *
 */

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* LPCOpen includes. */
#include "board.h"

/* QCA7k includes */
#include "qca_7k.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_IP_Private.h"
#include "NetworkBufferManagement.h"
#include "NetworkInterface.h"

/* TODO: Move buffer management into own header */

#ifndef configNUM_RX_DESCRIPTORS
	#define configNUM_RX_DESCRIPTORS 4
#endif

#ifndef configNUM_TX_DESCRIPTORS
	#define configNUM_TX_DESCRIPTORS 4
#endif

 /* If ipconfigETHERNET_DRIVER_FILTERS_FRAME_TYPES is set to 1, then the Ethernet
 driver will filter incoming packets and only pass the stack those packets it
 considers need processing. */
 #if( ipconfigETHERNET_DRIVER_FILTERS_FRAME_TYPES == 0 )
 	#define ipCONSIDER_FRAME_FOR_PROCESSING( pucEthernetBuffer ) eProcessBuffer
 #else
 	#define ipCONSIDER_FRAME_FOR_PROCESSING( pucEthernetBuffer ) eConsiderFrameForProcessing( ( pucEthernetBuffer ) )
 #endif

#define BUFFER_SIZE ( ipTOTAL_ETHERNET_FRAME_SIZE + ipBUFFER_PADDING )
#define BUFFER_SIZE_ROUNDED_UP ( ( BUFFER_SIZE + 7 ) & ~0x07UL )

/* EMAC variables located in AHB SRAM bank 1 and 2 */
#define AHB_SRAM_BANK1_BASE  0x2007C000UL

#define RX_DESC_BASE        (AHB_SRAM_BANK1_BASE)
#define RX_STAT_BASE        (RX_DESC_BASE + configNUM_RX_DESCRIPTORS*sizeof(ENET_RXDESC_T))
#define TX_DESC_BASE        (RX_STAT_BASE + configNUM_RX_DESCRIPTORS*sizeof(ENET_RXSTAT_T))
#define TX_STAT_BASE        (TX_DESC_BASE + configNUM_TX_DESCRIPTORS*sizeof(ENET_TXDESC_T))
#define ETH_BUF_BASE		(TX_STAT_BASE + configNUM_TX_DESCRIPTORS*sizeof(ENET_TXSTAT_T))

/* RX and TX descriptor and status definitions. */
#define xDMARxDescriptors   ( *(ENET_RXDESC_T (*)[configNUM_RX_DESCRIPTORS]) (RX_DESC_BASE) )
#define xDMARxStatus        ( *(ENET_RXSTAT_T (*)[configNUM_RX_DESCRIPTORS]) (RX_STAT_BASE) )
#define xDMATxDescriptors   ( *(ENET_TXDESC_T (*)[configNUM_TX_DESCRIPTORS]) (TX_DESC_BASE) )
#define xDMATxStatus        ( *(ENET_TXSTAT_T (*)[configNUM_TX_DESCRIPTORS]) (TX_STAT_BASE) )
#define ucBuffers           ( *(uint8_t (*)[ ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS ][BUFFER_SIZE_ROUNDED_UP]) (ETH_BUF_BASE) )

/*-----------------------------------------------------------*/
void GreenPHY_GPIO_IRQHandler (portBASE_TYPE * xHigherPriorityTaskWoken);
void GreenPHY_DMA_IRQHandler (portBASE_TYPE * xHigherPriorityTaskWoken);
/*-----------------------------------------------------------*/

/* The struct to hold all QCA7k and SPI related information */
static struct qcaspi qca;

/* The handle of the task that processes Rx packets.  The handle is required so
the task can be notified when new packets arrive. */
static TaskHandle_t xGreenPHYHandlerTask = NULL;
SemaphoreHandle_t xGreenPHY_DMASemaphore;

/*-----------------------------------------------------------*/

BaseType_t xNetworkInterfaceInitialise( void )
{
BaseType_t xReturn = pdPASS;

	Chip_GPDMA_Init(LPC_GPDMA);
	Board_SSP_Init(LPC_SSP0, true);

	qca.SSPx = LPC_SSP0;
	qca.sync = QCASPI_SYNC_UNKNOWN;
	qca.txQueue = xQueueCreate( configNUM_TX_DESCRIPTORS, sizeof( NetworkBufferDescriptor_t *) );
	qca.rx_desc = NULL;

	qca.rx_buffer = malloc(QCASPI_BURST_LEN);
	qca.buffer_size = QCASPI_BURST_LEN;
	QcaFrmFsmInit(&qca.lFrmHdl);
	xGreenPHY_DMASemaphore = xSemaphoreCreateBinary();

	/* Interrupt pin setup */
	Chip_GPIO_SetPinDIRInput(LPC_GPIO,    GREEN_PHY_INTERRUPT_PORT, GREEN_PHY_INTERRUPT_PIN);
	Chip_GPIOINT_SetIntFalling(LPC_GPIOINT, GREEN_PHY_INTERRUPT_PORT, (1 << GREEN_PHY_INTERRUPT_PIN));
	Chip_GPIOINT_SetIntRising(LPC_GPIOINT, GREEN_PHY_INTERRUPT_PORT, (1 << GREEN_PHY_INTERRUPT_PIN));

	/* QCA7000 reset pin setup */
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, GREEN_PHY_RESET_PORT, GREEN_PHY_RESET_PIN);

	xTaskCreate( qcaspi_spi_thread, "GreenPhyIntHandler", 240, &qca, tskIDLE_PRIORITY+4, &xGreenPHYHandlerTask);

	registerInterruptHandlerGPIO(GREEN_PHY_INTERRUPT_PORT, GREEN_PHY_INTERRUPT_PIN, GreenPHY_GPIO_IRQHandler);

	return xReturn;
}
/*-----------------------------------------------------------*/

/* Next provide the vNetworkInterfaceAllocateRAMToBuffers() function, which
simply fills in the pucEthernetBuffer member of each descriptor. */
void vNetworkInterfaceAllocateRAMToBuffers(
    NetworkBufferDescriptor_t pxNetworkBuffers[ ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS ] )
{
BaseType_t x;

    for( x = 0; x < ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS; x++ )
    {
        /* pucEthernetBuffer is set to point ipBUFFER_PADDING bytes in from the
        beginning of the allocated buffer. */
        pxNetworkBuffers[ x ].pucEthernetBuffer = &( ucBuffers[x][ ipBUFFER_PADDING ] );

        /* The following line is also required, but will not be required in
        future versions. */
        *( ( uint32_t * ) &ucBuffers[x][ 0 ] ) = ( uint32_t ) &( pxNetworkBuffers[ x ] );
    }
}
/*-----------------------------------------------------------*/

BaseType_t xNetworkInterfaceOutput( NetworkBufferDescriptor_t * const pxDescriptor, BaseType_t bReleaseAfterSend )
{
BaseType_t xReturn = pdFAIL;

	// ML: Increase timeout and return buffer on fail
	xReturn = xQueueSend(qca.txQueue, ( void * ) &pxDescriptor, 0);

	xTaskNotify( xGreenPHYHandlerTask, QCAGP_TX_FLAG, eSetBits );

	return xReturn;
}
/*-----------------------------------------------------------*/

void GreenPHY_GPIO_IRQHandler (portBASE_TYPE * xHigherPriorityTaskWoken)
{
	/* wake up the handler task */
	xTaskNotifyFromISR( xGreenPHYHandlerTask,  QCAGP_INT_FLAG, eSetBits, xHigherPriorityTaskWoken );

	/* Interrupt status clear and context switching
	 * is done by GPIO Interrupt handler */
}
/*-----------------------------------------------------------*/

void GreenPHY_DMA_IRQHandler (portBASE_TYPE * xHigherPriorityTaskWoken)
{
	xSemaphoreGiveFromISR( xGreenPHY_DMASemaphore, xHigherPriorityTaskWoken );
}
/*-----------------------------------------------------------*/

