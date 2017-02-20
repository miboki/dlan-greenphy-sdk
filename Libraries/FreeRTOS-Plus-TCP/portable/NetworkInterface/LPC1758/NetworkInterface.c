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

/* LPCOpen includes. */
#include "chip.h"
#include "lpc_phy.h"

/* The size of the stack allocated to the task that handles Rx packets. */
#define nwRX_TASK_STACK_SIZE	140

#ifndef	PHY_LS_HIGH_CHECK_TIME_MS
	/* Check if the LinkSStatus in the PHY is still high after 15 seconds of not
	receiving packets. */
	#define PHY_LS_HIGH_CHECK_TIME_MS	15000
#endif

#ifndef	PHY_LS_LOW_CHECK_TIME_MS
	/* Check if the LinkSStatus in the PHY is still low every second. */
	#define PHY_LS_LOW_CHECK_TIME_MS	1000
#endif

#ifndef configUSE_RMII
	#define configUSE_RMII 1
#endif

#ifndef configNUM_RX_DESCRIPTORS
	#define configNUM_RX_DESCRIPTORS 4
#endif

#ifndef configNUM_TX_DESCRIPTORS
	#define configNUM_TX_DESCRIPTORS 4
#endif

#ifndef NETWORK_IRQHandler
	#error NETWORK_IRQHandler must be defined to the name of the function that is installed in the interrupt vector table to handle Ethernet interrupts.
#endif

#if !defined( MAC_FF_HMC )
	/* Hash for multicast. */
	#define MAC_FF_HMC     ( 1UL << 2UL )
#endif

#ifndef iptraceEMAC_TASK_STARTING
	#define iptraceEMAC_TASK_STARTING()	do { } while( 0 )
#endif

 /* If ipconfigETHERNET_DRIVER_FILTERS_FRAME_TYPES is set to 1, then the Ethernet
 driver will filter incoming packets and only pass the stack those packets it
 considers need processing. */
 #if( ipconfigETHERNET_DRIVER_FILTERS_FRAME_TYPES == 0 )
 	#define ipCONSIDER_FRAME_FOR_PROCESSING( pucEthernetBuffer ) eProcessBuffer
 #else
 	#define ipCONSIDER_FRAME_FOR_PROCESSING( pucEthernetBuffer ) eConsiderFrameForProcessing( ( pucEthernetBuffer ) )
 #endif

/** @brief Receive group interrupts
 */
#define RXINTGROUP (ENET_INT_RXOVERRUN | ENET_INT_RXERROR | ENET_INT_RXDONE)

/** @brief Transmit group interrupts
 */
#define TXINTGROUP (ENET_INT_TXUNDERRUN | ENET_INT_TXERROR | ENET_INT_TXDONE)

#define BUFFER_SIZE ( ipTOTAL_ETHERNET_FRAME_SIZE + ipBUFFER_PADDING )
#define BUFFER_SIZE_ROUNDED_UP ( ( BUFFER_SIZE + 7 ) & ~0x07UL )

/* EMAC variables located in AHB SRAM bank 1 and 2 */
#define AHB_SRAM_BANK1_BASE  0x2007C000UL

#define RX_DESC_BASE        (AHB_SRAM_BANK1_BASE)
#define RX_STAT_BASE        (RX_DESC_BASE + configNUM_RX_DESCRIPTORS*(2*4))     /* 2 * uint32_t, see RX_DESC_TypeDef */
#define TX_DESC_BASE        (RX_STAT_BASE + configNUM_RX_DESCRIPTORS*(2*4))     /* 2 * uint32_t, see RX_STAT_TypeDef */
#define TX_STAT_BASE        (TX_DESC_BASE + configNUM_TX_DESCRIPTORS*(2*4))     /* 2 * uint32_t, see TX_DESC_TypeDef */
#define ETH_BUF_BASE		(TX_STAT_BASE + configNUM_TX_DESCRIPTORS*(1*4))     /* 1 * uint32_t, see TX_STAT_TypeDef */

/* RX and TX descriptor and status definitions. */
//#define RX_DESC(i)          (*(ENET_RXDESC_T *)(RX_DESC_BASE   + 8*i))
//#define RX_STAT(i)          (*(ENET_RXSTAT_T *)(RX_STAT_BASE   + 8*i))
//#define TX_DESC(i)          (*(ENET_TXDESC_T *)(TX_DESC_BASE   + 8*i))
//#define TX_STAT(i)          (*(ENET_TXSTAT_T *)(TX_STAT_BASE   + 4*i))
//#define ETH_BUF(i)          ( (uint8_t (*)[BUFFER_SIZE_ROUNDED_UP]) (ETH_BUF_BASE + BUFFER_SIZE_ROUNDED_UP*i))
#define xDMARxDescriptors    ( *(ENET_RXDESC_T (*)[configNUM_RX_DESCRIPTORS]) (RX_DESC_BASE) )
#define xDMARxStatus          ( *(ENET_RXSTAT_T (*)[configNUM_RX_DESCRIPTORS]) (RX_STAT_BASE) )
#define xDMATxDescriptors    ( *(ENET_TXDESC_T (*)[configNUM_TX_DESCRIPTORS]) (TX_DESC_BASE) )
#define xDMATxStatus          ( *(ENET_TXSTAT_T (*)[configNUM_TX_DESCRIPTORS]) (TX_STAT_BASE) )
#define ucBuffers            ( *(uint8_t (*)[ ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS ][BUFFER_SIZE_ROUNDED_UP]) (ETH_BUF_BASE) )
//static uint8_t ucBuffers[ ipconfigNUM_NETWORK_BUFFERS ][ BUFFER_SIZE_ROUNDED_UP ];

static UBaseType_t ulTxDMACleanupIndex = 0;
/*-----------------------------------------------------------*/

/*
 * Delay function passed into the library.  The implementation uses FreeRTOS
 * calls so the scheduler must be started before the driver can be used.
 */
static void prvDelay( uint32_t ulMilliSeconds );

/*
 * Initialises the Tx and Rx descriptors respectively.
 */
static void prvSetupTxDescriptors( void );
static void prvSetupRxDescriptors( void );

/*
 * A task that processes received frames.
 */
static void prvEMACHandlerTask( void *pvParameters );

/*
 * Sets up the MAC with the results of an auto-negotiation.
 */
static BaseType_t prvSetLinkSpeed( void );

/*-----------------------------------------------------------*/

/* A copy of PHY register 1: 'PHY_REG_01_BMSR' */
static uint32_t ulPHYLinkStatus = 0;

/* Must be defined externally - the demo applications define this in main.c. */
extern uint8_t ucMACAddress[ 6 ];

/* The handle of the task that processes Rx packets.  The handle is required so
the task can be notified when new packets arrive. */
static TaskHandle_t xRxHanderTask = NULL;

#if( ipconfigUSE_LLMNR == 1 )
	static const uint8_t xLLMNR_MACAddress[] = { '\x01', '\x00', '\x5E', '\x00', '\x00', '\xFC' };
#endif	/* ipconfigUSE_LLMNR == 1 */

/*-----------------------------------------------------------*/

static void prvDelay( uint32_t ulMilliSeconds )
{
	/* Ensure the scheduler was started before attempting to use the scheduler to
	create a delay. */
	configASSERT( xTaskGetSchedulerState() == taskSCHEDULER_RUNNING );

	vTaskDelay( pdMS_TO_TICKS( ulMilliSeconds ) );
}
/*-----------------------------------------------------------*/

BaseType_t xNetworkInterfaceInitialise( void )
{
BaseType_t xReturn = pdPASS;

	/* The interrupt will be turned on when a link is established. */
	NVIC_DisableIRQ( ETHERNET_IRQn );

	/* Disable packet reception. */
	Chip_ENET_RXDisable(LPC_ETHERNET);
	Chip_ENET_TXDisable(LPC_ETHERNET);

	/* Call the LPCOpen function to initialise the hardware. */
#if( configUSE_RMII == 1 )
	Chip_ENET_Init(LPC_ETHERNET, true);
#else
	Chip_ENET_Init(LPC_ETHERNET, false);
#endif

	/* Initialize the PHY */
#define LPC_PHYDEF_PHYADDR 1
	Chip_ENET_SetupMII(LPC_ETHERNET, Chip_ENET_FindMIIDiv(LPC_ETHERNET, 2500000), LPC_PHYDEF_PHYADDR);
#if( configUSE_RMII == 1 )
	if( lpc_phy_init( pdTRUE, prvDelay ) != SUCCESS )
	{
		return pdFAIL;
	}
#else
	if( lpc_phy_init( pdFALSE, prvDelay ) != SUCCESS )
	{
		return pdFAIL;
	}
#endif

	/* Save MAC address. */
	Chip_ENET_SetADDR( LPC_ETHERNET, ucMACAddress );

	/* Guard against the task being created more than once and the
	descriptors being initialised more than once. */
	if( xRxHanderTask == NULL )
	{
		xReturn = xTaskCreate( prvEMACHandlerTask, "EMAC", nwRX_TASK_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, &xRxHanderTask );
		configASSERT( xReturn );
	}

	if( xReturn != pdPASS )
		return xReturn;

	/* Auto-negotiate was already started.  Wait for it to complete. */
	xReturn = prvSetLinkSpeed();

	/* Initialise the descriptors. */
	prvSetupTxDescriptors();
	prvSetupRxDescriptors();

	/* Enable packet reception */
	/* Perfect match and Broadcast enabled */
	Chip_ENET_EnableRXFilter(LPC_ETHERNET, ENET_RXFILTERCTRL_APE | ENET_RXFILTERCTRL_ABE);

	/* Clear and enable RX/TX interrupts */
	Chip_ENET_EnableInt(LPC_ETHERNET, RXINTGROUP | TXINTGROUP);

	/* Enable RX and TX */
	Chip_ENET_TXEnable(LPC_ETHERNET);
	Chip_ENET_RXEnable(LPC_ETHERNET);

	/* Enable interrupts in the NVIC. */
	NVIC_SetPriority( ETHERNET_IRQn, configETHERNET_INTERRUPT_PRIORITY );
	NVIC_EnableIRQ( ETHERNET_IRQn );

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

static void prvSetupTxDescriptors( void )
{
	BaseType_t x;

	#if( ipconfigZERO_COPY_TX_DRIVER == 0 )
		#error "ipconfigZERO_COPY_TX_DRIVER should be set to 1"
	#endif

	/* Build TX descriptors for local buffers */
	for (x = 0; x < configNUM_TX_DESCRIPTORS; x++) {
		xDMATxDescriptors[x].Packet = 0;
		xDMATxDescriptors[x].Control = 0;
		xDMATxStatus[x].StatusInfo = 0xFFFFFFFF; // ML: why not 0 ??
	}

	/* Setup pointers to TX structures */
	Chip_ENET_InitTxDescriptors(LPC_ETHERNET, xDMATxDescriptors, xDMATxStatus, configNUM_TX_DESCRIPTORS);
}
/*-----------------------------------------------------------*/

static void prvSetupRxDescriptors( void )
{
BaseType_t x;
NetworkBufferDescriptor_t *pxBuffer;

	/* Clear RX descriptor list. */
	memset( ( void * )  xDMARxDescriptors, 0, sizeof( xDMARxDescriptors ) );

	for( x = 0; x < configNUM_RX_DESCRIPTORS; x++ )
	{
		pxBuffer = pxGetNetworkBufferWithDescriptor(ipTOTAL_ETHERNET_FRAME_SIZE, ( TickType_t ) 0);
		xDMARxDescriptors[ x ].Packet = (uint32_t) pxBuffer->pucEthernetBuffer;

		/* Set RX interrupt and buffer size */
		xDMARxDescriptors[ x ].Control = ENET_RCTRL_INT | (( uint32_t ) ENET_RCTRL_SIZE( ipTOTAL_ETHERNET_FRAME_SIZE ));
		/* Initialize Status */
		xDMARxStatus[ x ].StatusInfo = 0xFFFFFFFF; // ML: why not 0 ??
		xDMARxStatus[ x ].StatusHashCRC = 0xFFFFFFFF; // ML: why not 0 ??
	}

	/* Point the DMA to the base of the descriptor list. */
	Chip_ENET_InitRxDescriptors(LPC_ETHERNET, xDMARxDescriptors, xDMARxStatus, configNUM_RX_DESCRIPTORS);
}
/*-----------------------------------------------------------*/

BaseType_t xNetworkInterfaceOutput( NetworkBufferDescriptor_t * const pxDescriptor, BaseType_t bReleaseAfterSend )
{
BaseType_t xReturn = pdFAIL, x;
const BaseType_t xMaxAttempts = 15;
const TickType_t xTimeBetweenTxAttemtps = pdMS_TO_TICKS( 5 );
UBaseType_t ulTxProduceIndex;
ENET_TXDESC_T *pxDMATxDescriptor;

	/* Attempt to obtain access to a Tx descriptor. */
	for( x = 0; x < xMaxAttempts; x++ )
	{
		if( !Chip_ENET_IsTxFull( LPC_ETHERNET ) )
		{
			ulTxProduceIndex = Chip_ENET_GetTXProduceIndex( LPC_ETHERNET );
			pxDMATxDescriptor = &xDMATxDescriptors[ulTxProduceIndex];

			pxDMATxDescriptor->Packet = ( uint32_t ) pxDescriptor->pucEthernetBuffer;
			pxDMATxDescriptor->Control = ( uint32_t ) ENET_TCTRL_SIZE( pxDescriptor->xDataLength ) | ENET_TCTRL_INT | ENET_TCTRL_LAST;

			iptraceNETWORK_INTERFACE_TRANSMIT();

			/* Move onto the next descriptor, wrapping if necessary. */
			Chip_ENET_IncTXProduceIndex(LPC_ETHERNET);

			/* The Tx has been initiated. */
			xReturn = pdPASS;
			break;
		}
		else
		{
			/* The next Tx descriptor is still in use - wait a while for it to
			become free. */
			iptraceWAITING_FOR_TX_DMA_DESCRIPTOR();
			vTaskDelay( xTimeBetweenTxAttemtps );
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

static void prvEMACHandlerTask( void *pvParameters )
{
TickType_t xPhyRemTime;
/* for buffer usage statistics */
UBaseType_t uxLastMinBufferCount = 0;
UBaseType_t uxCurrentCount;

/* PHY status */
uint32_t ulStatus = pdFALSE;

const UBaseType_t uxMinimumBuffersRemaining = 2UL;
uint32_t ulNotificationValue;
eFrameProcessingResult_t eResult;
UBaseType_t ulRxConsumeIndex;
size_t xDataLength;
ENET_RXDESC_T *pxDMARxDescriptor;
ENET_RXSTAT_T *pxDMARxStatus;
static NetworkBufferDescriptor_t *pxNewNetworkBuffer, *pxDMANetworkBuffer;
IPStackEvent_t xRxEvent = { eNetworkRxEvent, NULL };

	/* Remove compiler warning about unused parameter. */
	( void ) pvParameters;

	/* A possibility to set some additional task properties. */
	iptraceEMAC_TASK_STARTING();

	xPhyRemTime = pdMS_TO_TICKS( PHY_LS_LOW_CHECK_TIME_MS );

	for( ;; )
	{
		uxCurrentCount = uxGetMinimumFreeNetworkBuffers();
		if( uxLastMinBufferCount != uxCurrentCount )
		{
			/* The logging produced below may be helpful
			while tuning +TCP: see how many buffers are in use. */
			uxLastMinBufferCount = uxCurrentCount;
			FreeRTOS_printf( ( "Network buffers: %lu lowest %lu\n",
				uxGetNumberOfFreeNetworkBuffers(), uxCurrentCount ) );
		}

		#if( ipconfigCHECK_IP_QUEUE_SPACE != 0 )
		{
		static UBaseType_t uxLastMinQueueSpace = 0;

			uxCurrentCount = uxGetMinimumIPQueueSpace();
			if( uxLastMinQueueSpace != uxCurrentCount )
			{
				/* The logging produced below may be helpful
				while tuning +TCP: see how many buffers are in use. */
				uxLastMinQueueSpace = uxCurrentCount;
				FreeRTOS_printf( ( "Queue space: lowest %lu\n", uxCurrentCount ) );
			}
		}
		#endif /* ipconfigCHECK_IP_QUEUE_SPACE */

		/* Wait until a packet has been received or PHY link status times out */
		ulNotificationValue = ulTaskNotifyTake( pdTRUE, xPhyRemTime );

		if( ulNotificationValue == 1 )
		{
			/* At least one packet has been received. */
			while( !Chip_ENET_IsRxEmpty(LPC_ETHERNET) )
			{
				ulRxConsumeIndex = Chip_ENET_GetRXConsumeIndex( LPC_ETHERNET );
				pxDMARxDescriptor = &xDMARxDescriptors[ulRxConsumeIndex];
				pxDMARxStatus     = &xDMARxStatus[ulRxConsumeIndex];

				xDataLength = ( size_t ) ENET_RINFO_SIZE( pxDMARxStatus->StatusInfo ) - 4; /* Remove FCS */

				if( xDataLength > 0U )
				{
					eResult = ipCONSIDER_FRAME_FOR_PROCESSING( ( const uint8_t * const ) ( pxDMARxDescriptor->Packet ) );
					if( eResult == eProcessBuffer )
					{
						/* Don't consume all the buffers if there are bursts of
						traffic. */
						if( uxGetNumberOfFreeNetworkBuffers() > uxMinimumBuffersRemaining )
						{
							/* Obtain the associated network buffer to pass this
							data into the stack. */
						    pxDMANetworkBuffer = *( NetworkBufferDescriptor_t ** ) ( pxDMARxDescriptor->Packet - ipBUFFER_PADDING );

							/* Update the the length of the network buffer descriptor
							with the number of received bytes */
							pxDMANetworkBuffer->xDataLength = xDataLength;

						    /* Obtain a new network buffer for DMA */
						    pxNewNetworkBuffer = pxGetNetworkBufferWithDescriptor( ipTOTAL_ETHERNET_FRAME_SIZE, 0 );
						    pxDMARxDescriptor->Packet = (uint32_t) pxNewNetworkBuffer->pucEthernetBuffer;

							xRxEvent.pvData = ( void * ) pxDMANetworkBuffer;

							/* Data was received and stored.  Send a message to the IP
							task to let it know. */
							if( xSendEventStructToIPTask( &xRxEvent, ( TickType_t ) 0 ) == pdFAIL )
							{
								/* Could not send the descriptor into the TCP/IP
								stack, it must be released. */
								vReleaseNetworkBufferAndDescriptor( pxDMANetworkBuffer );
								iptraceETHERNET_RX_EVENT_LOST();
							}

							iptraceNETWORK_INTERFACE_RECEIVE();
						}
						else
						{
							// ML: Todo: No buffer available, drop packet
						}
					}
				}


				/* Release the DMA descriptor. */
				Chip_ENET_IncRXConsumeIndex(LPC_ETHERNET);
			}

			if( ( ulPHYLinkStatus & PHY_LINK_CONNECTED ) == 0 )
			{
				ulPHYLinkStatus |= PHY_LINK_CONNECTED;
				FreeRTOS_printf( ( "prvEMACHandlerTask: PHY LS now %d (message received)\n", ( ulPHYLinkStatus & PHY_LINK_CONNECTED ) != 0 ) );
			}
			/* A packet was received. No need to check for the PHY status now,
			but set a timer to check it later on. */
			xPhyRemTime = pdMS_TO_TICKS( PHY_LS_HIGH_CHECK_TIME_MS );
		}
		else
		{
			/* A timeout happened, check PHY link status now */
			ulStatus = lpcPHYStsPoll();

			if( ( ulPHYLinkStatus & PHY_LINK_CONNECTED ) != ( ulStatus & PHY_LINK_CONNECTED ) )
			{
				ulPHYLinkStatus = ulStatus;
				FreeRTOS_printf( ( "prvEMACHandlerTask: PHY LS now %d (polled PHY)\n", ( ulPHYLinkStatus & PHY_LINK_CONNECTED ) != 0 ) );
				prvSetLinkSpeed();
			}

			if( ( ulPHYLinkStatus & PHY_LINK_CONNECTED ) != 0 )
			{
				xPhyRemTime = pdMS_TO_TICKS( PHY_LS_HIGH_CHECK_TIME_MS );
			}
			else
			{
				xPhyRemTime = pdMS_TO_TICKS( PHY_LS_LOW_CHECK_TIME_MS );
			}
		}


	}
}
/*-----------------------------------------------------------*/

void NETWORK_IRQHandler( void )
{
BaseType_t xHigherPriorityTaskWoken = pdFALSE;
uint32_t ulInterrupts;

	configASSERT( xRxHanderTask );

	/* Get pending interrupts. */
	ulInterrupts = Chip_ENET_GetIntStatus(LPC_ETHERNET);

	/* RX group interrupt(s). */
	if( ( ulInterrupts & RXINTGROUP ) != 0x00 )
	{
		if( ( ulInterrupts & ENET_INT_RXDONE ) != 0x00 )
		{
			/* A packet may be waiting to be received. */
			xHigherPriorityTaskWoken = pdFALSE;
			vTaskNotifyGiveFromISR( xRxHanderTask, &xHigherPriorityTaskWoken );
		}
		// ML: TODO ENET_INT_RXOVERRUN and ENET_INT_RXERROR
	}

	/* TX group interrupt(s). */
	if( ( ulInterrupts & TXINTGROUP ) != 0x00 )
	{
		if( ( ulInterrupts & ENET_INT_TXDONE ) != 0x00 )
		{
			UBaseType_t ulTxConsumeIndex = Chip_ENET_GetTXConsumeIndex( LPC_ETHERNET );
			while( ulTxDMACleanupIndex != ulTxConsumeIndex ) {
				vNetworkBufferReleaseFromISR( *( NetworkBufferDescriptor_t ** )
						( xDMATxDescriptors[ulTxDMACleanupIndex].Packet - ipBUFFER_PADDING ) );
				xDMATxDescriptors[ulTxDMACleanupIndex].Packet = 0;

				++ulTxDMACleanupIndex;
				if( ulTxDMACleanupIndex >= configNUM_RX_DESCRIPTORS )
					ulTxDMACleanupIndex = 0;
			}
		}
		// ML: TODO ENET_INT_TXUNDERRUN and ENET_INT_TXERROR
	}

	/* Clear pending interrupts */
	Chip_ENET_ClearIntStatus(LPC_ETHERNET, ulInterrupts);

	/* Context switch needed? */
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}
/*-----------------------------------------------------------*/

BaseType_t xGetPhyLinkStatus( void )
{
BaseType_t xReturn;

	if( ( ulPHYLinkStatus & PHY_LINK_CONNECTED ) == 0 )
	{
		xReturn = pdFALSE;
	}
	else
	{
		xReturn = pdTRUE;
	}

	return xReturn;
}

/*-----------------------------------------------------------*/

static BaseType_t prvSetLinkSpeed( void )
{
BaseType_t xReturn = pdFAIL;
TickType_t xTimeOnEntering;
uint32_t ulPhyStatus;
const TickType_t xAutoNegotiateDelay = pdMS_TO_TICKS( 5000UL );

	/* Ensure polling does not starve lower priority tasks by temporarily
	setting the priority of this task to that of the idle task. */
	vTaskPrioritySet( NULL, tskIDLE_PRIORITY );

	xTimeOnEntering = xTaskGetTickCount();
	do
	{
		ulPhyStatus = lpcPHYStsPoll();
		if( ( ulPhyStatus & PHY_LINK_CONNECTED ) != 0x00 )
		{
			/* Set interface speed and duplex. */
			if( ( ulPhyStatus & PHY_LINK_SPEED100 ) != 0x00 )
			{
				Chip_ENET_Set100Mbps( LPC_ETHERNET );
			}
			else
			{
				Chip_ENET_Set10Mbps( LPC_ETHERNET );
			}

			if( ( ulPhyStatus & PHY_LINK_FULLDUPLX ) != 0x00 )
			{
				Chip_ENET_SetFullDuplex( LPC_ETHERNET );
			}
			else
			{
				Chip_ENET_SetHalfDuplex( LPC_ETHERNET );
			}

			xReturn = pdPASS;
			break;
		}
	} while( ( xTaskGetTickCount() - xTimeOnEntering ) < xAutoNegotiateDelay );

	/* Reset the priority of this task back to its original value. */
	vTaskPrioritySet( NULL, ipconfigIP_TASK_PRIORITY );

	return xReturn;
}
