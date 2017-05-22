/*====================================================================*
 *
 *
 *   Copyright (c) 2011, 2012, Qualcomm Atheros Communications Inc.
 *
 *   Permission to use, copy, modify, and/or distribute this software
 *   for any purpose with or without fee is hereby granted, provided
 *   that the above copyright notice and this permission notice appear
 *   in all copies.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *   WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 *   THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 *   CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 *   LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 *   NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 *   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *--------------------------------------------------------------------*/

/*====================================================================*
 *
 *   qca_spi.c
 *
 *   This module implements the Qualcomm Atheros SPI protocol for
 *   kernel-based SPI device; it is essentially an Ethernet-to-SPI
 *   serial converter;
 *
 *--------------------------------------------------------------------*/

/*====================================================================*
 *   system header files;
 *--------------------------------------------------------------------*/
/* Standard includes. */
#include <stdint.h>

/* LPCOpen includes */
#include "board.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
#include "NetworkBufferManagement.h"
#include "FreeRTOS_Routing.h"
#include "FreeRTOS_Bridge.h"

/*====================================================================*
 *   custom header files;
 *--------------------------------------------------------------------*/

/* QCA7k includes */
#include "qca_spi.h"
#include "qca_framing.h"
#include "qca_7k.h"

#if configREAD_MAC_FROM_GREENPHY
	#include "mme_handler.h"
#endif

/*====================================================================*
 *
 * Disables all SPI interrupts and returns the old value of the
 * interrupt enable register.
 *
 *--------------------------------------------------------------------*/

uint32_t
disable_spi_interrupts(struct qcaspi *qca)
{
	uint32_t old_intr_enable = qcaspi_read_register(qca, SPI_REG_INTR_ENABLE);
	qcaspi_write_register(qca, SPI_REG_INTR_ENABLE, 0);
	return old_intr_enable;
}

/*====================================================================*
 *
 * Enables only the SPI interrupts passed in the intr argument.
 * All others are disabled.
 * Returns the old value of the interrupt enable register.
 *
 *--------------------------------------------------------------------*/

uint32_t
enable_spi_interrupts(struct qcaspi *qca, uint32_t intr_enable)
{
	uint32_t old_intr_enable = qcaspi_read_register(qca, SPI_REG_INTR_ENABLE);
	qcaspi_write_register(qca, SPI_REG_INTR_ENABLE, intr_enable);
	return old_intr_enable;
}

/*====================================================================*
 *
 * Transmits a write command and len bytes of data
 * from src buffer in a single burst.
 *
 * Returns 0 if not all data could be transmitted,
 * and len if all data was transmitted.
 *
 *--------------------------------------------------------------------*/

extern SemaphoreHandle_t xGreenPHY_DMASemaphore;
extern void GreenPHY_DMA_IRQHandler (portBASE_TYPE * xHigherPriorityTaskWoken);

uint16_t
qcaspi_write_burst(struct qcaspi *qca, uint8_t* src, uint16_t len)
{
	Chip_SSP_DATA_SETUP_T dataConfig;
	uint8_t localBuffer[QCAFRM_HEADER_LEN];

	qcaspi_tx_cmd(qca, QCA7K_SPI_WRITE | QCA7K_SPI_EXTERNAL);

	/* send QCA header */
	QcaFrmCreateHeader(localBuffer, len, 0);
	dataConfig.length = QCAFRM_HEADER_LEN;
	dataConfig.tx_data = localBuffer;
	dataConfig.rx_data = NULL;
	dataConfig.tx_cnt = 0;
	dataConfig.rx_cnt = 0;
	Chip_SSP_RWFrames_Blocking(qca->SSPx, &dataConfig);

	/* Get a free DMA channel and register the interrupt handler */
	uint8_t channel = Chip_GPDMA_GetFreeChannel(LPC_GPDMA, GPDMA_CONN_SSP0_Tx);
	registerInterruptHandlerDMA(channel, GreenPHY_DMA_IRQHandler);

	/* data Tx_Buf --> SSP */
	Chip_GPDMA_Transfer(LPC_GPDMA,
						channel,
						(uint32_t) src,
						GPDMA_CONN_SSP0_Tx,
						GPDMA_TRANSFERTYPE_M2P_CONTROLLER_DMA,
						len);

	Chip_SSP_DMA_Enable(qca->SSPx);
	/* wait for DMA to complete */
	xSemaphoreTake( xGreenPHY_DMASemaphore, portMAX_DELAY );
	Chip_SSP_DMA_Disable(qca->SSPx);

	/* stop DMA transfer and free the channel */
	Chip_GPDMA_Stop(LPC_GPDMA, channel);

	/* send QCA footer */
	QcaFrmCreateFooter(localBuffer);
	dataConfig.length = QCAFRM_FOOTER_LEN;
	dataConfig.tx_data = localBuffer;
	dataConfig.rx_data = NULL;
	dataConfig.tx_cnt = 0;
	dataConfig.rx_cnt = 0;
	Chip_SSP_RWFrames_Blocking(qca->SSPx, &dataConfig);

	return len;
}

/*====================================================================*
 *
 * Sends a read command and then receives len
 * bytes of data from the external SPI slave into
 * the buffer at dst.
 *
 * Returns 0 if not all data could be received,
 * and len if all data was received.
 *
 *--------------------------------------------------------------------*/

uint16_t
qcaspi_read_burst(struct qcaspi *qca, uint8_t *dst, uint16_t len)
{
	uint16_t cmd = (QCA7K_SPI_READ | QCA7K_SPI_EXTERNAL);

	int status = Board_SSP_AssertSSEL(qca->SSPx);

	qcaspi_tx_cmd(qca,cmd);

	/* The SSP interface needs to write data to drive the clock, thus
	 * we need a dummy TX transfer, where we just send arbitrary data
	 * of the same length as RX */
	uint8_t channelRX = Chip_GPDMA_GetFreeChannel(LPC_GPDMA, GPDMA_CONN_SSP0_Tx);
	uint8_t channelTX = Chip_GPDMA_GetFreeChannel(LPC_GPDMA, GPDMA_CONN_SSP0_Tx);
	registerInterruptHandlerDMA(channelTX, GreenPHY_DMA_IRQHandler);
	registerInterruptHandlerDMA(channelRX, GreenPHY_DMA_IRQHandler);

	/* data dummy data --> SSP */
	Chip_GPDMA_Transfer(LPC_GPDMA,
						channelTX,
						(uint32_t) dst,
						GPDMA_CONN_SSP0_Tx,
						GPDMA_TRANSFERTYPE_M2P_CONTROLLER_DMA,
						len);

	/* data SSP --> RxBuf */
	Chip_GPDMA_Transfer(LPC_GPDMA,
						channelRX,
						GPDMA_CONN_SSP0_Rx,
						(uint32_t) dst,
						GPDMA_TRANSFERTYPE_P2M_CONTROLLER_DMA,
						len);

	Chip_SSP_DMA_Enable(qca->SSPx);
	/* wait for DMA to complete, take one semaphore for RX and TX each */
	xSemaphoreTake( xGreenPHY_DMASemaphore, portMAX_DELAY );
	xSemaphoreTake( xGreenPHY_DMASemaphore, portMAX_DELAY );
	Chip_SSP_DMA_Disable(qca->SSPx);

	/* stop DMA transfer and free the channels */
	Chip_GPDMA_Stop(LPC_GPDMA, channelTX);
	Chip_GPDMA_Stop(LPC_GPDMA, channelRX);

	if (status) Board_SSP_DeassertSSEL(qca->SSPx);

	return len;
}

/*====================================================================*
 *
 * Transmits an buffer in burst mode.
 *
 * Returns number of transmitted bytes, 0 on failure.
 *
 *--------------------------------------------------------------------*/

uint16_t
qcaspi_tx_frame(struct qcaspi *qca, NetworkBufferDescriptor_t * txBuffer)
{
	uint16_t writtenBytes = 0;

	uint8_t* pucData = txBuffer->pucEthernetBuffer;
	uint16_t len = txBuffer->xDataLength;
	uint16_t pad_len;
	if (len < QCAFRM_ETHMINLEN) {
		pad_len = QCAFRM_ETHMINLEN - len;
		memset(pucData+len, 0, pad_len);
		len += pad_len;
	}

	int status = Board_SSP_AssertSSEL(qca->SSPx);
	qcaspi_write_register(qca, SPI_REG_BFR_SIZE, len + QCAFRM_FRAME_OVERHEAD);
	if (status) Board_SSP_DeassertSSEL(qca->SSPx);

	status = Board_SSP_AssertSSEL(qca->SSPx);

	/* send ethernet packet via DMA to SPI */
	writtenBytes = qcaspi_write_burst(qca, pucData, len);

	if (status) Board_SSP_DeassertSSEL(qca->SSPx);

	return writtenBytes;
}

/*====================================================================*
 *
 * Transmits as many sk_buff's that will fit in
 * the SPI slave write buffer.
 *
 * Returns -1 on failure, 0 on success.
 *
 *--------------------------------------------------------------------*/
int
qcaspi_transmit(struct qcaspi *qca)
{
	uint16_t available;
	NetworkBufferDescriptor_t *txBuffer;

	/* read the available space in bytes from QCA7k */
	available = qcaspi_read_register(qca, SPI_REG_WRBUF_SPC_AVA);

	while( xQueuePeek(qca->txQueue, &txBuffer, 0) )
	{
		/* check whether there is enough space in the QCA7k buffer to hold
		 * the next packet */
		if ( available < (txBuffer->xDataLength + QCAFRM_FRAME_OVERHEAD) )
		{
			return -1;
		}

		/* receive and process the next packet */
		xQueueReceive(qca->txQueue, &txBuffer, 0);

		uint16_t writtenBytes = qcaspi_tx_frame(qca, txBuffer);

		vReleaseNetworkBufferAndDescriptor(txBuffer);

		available -= (writtenBytes + QCAFRM_FRAME_OVERHEAD);
		qca->stats.tx_packets++;
		qca->stats.tx_bytes += writtenBytes;
	}

	return 0;
}

/*====================================================================*
 *
 * Read the number of bytes available in the SPI slave and
 * then read and process the data from the slave.
 *
 * Returns -1 on error, 0 on success.
 *
 *--------------------------------------------------------------------*/

int
qcaspi_receive(struct qcaspi *qca)
{
	uint32_t available;
	uint32_t bytes_read;
	uint32_t bytes_proc;
	uint32_t count;
	uint8_t *rx_buffer;
	uint16_t len;

	IPStackEvent_t xRxEvent = { eNetworkRxEvent, NULL };

	/* Allocate rx buffer if we don't have one available. */
	if (qca->rx_desc == NULL) {
		qca->rx_desc = pxGetNetworkBufferWithDescriptor(ipTOTAL_ETHERNET_FRAME_SIZE, 0);
		if (qca->rx_desc == NULL) {
			qca->stats.rx_dropped++;
			return -1;
		}
	}

	/* Read the packet size. */
	available = qcaspi_read_register(qca, SPI_REG_RDBUF_BYTE_AVA);

	if (available == 0) {
		return -1;
	}

	qcaspi_write_register(qca, SPI_REG_BFR_SIZE, available);

	while (available) {
		count = available;
		if (count > QCASPI_BURST_LEN) {
			count = QCASPI_BURST_LEN;
		}

		bytes_read = qcaspi_read_burst(qca, qca->rx_buffer, count);
		rx_buffer = qca->rx_buffer;

		DEBUGOUT("qcaspi: available: %d, byte read: %d\n", available, bytes_read);

		available -= bytes_read;

		while (bytes_read && (qca->rx_desc)) {
			int32_t retcode = QcaFrmFsmDecode(&qca->lFrmHdl, qca->rx_desc->pucEthernetBuffer, ipTOTAL_ETHERNET_FRAME_SIZE,
											  rx_buffer, bytes_read, &bytes_proc);
			bytes_read -= bytes_proc;
			rx_buffer  += bytes_proc;
			switch (retcode) {
			case QCAFRM_GATHER:
				break;

			case QCAFRM_NOHEAD:
				break;

			case QCAFRM_NOTAIL:
				qca->stats.rx_errors++;
				qca->stats.rx_dropped++;
				break;

			case QCAFRM_INVLEN:
				qca->stats.rx_errors++;
				qca->stats.rx_dropped++;
				break;

			default:
				qca->stats.rx_packets++;
				qca->stats.rx_bytes += retcode;

				/* Data was received and stored.  Send a message to the IP
				task to let it know. */
				qca->rx_desc->xDataLength = retcode;

			#if configREAD_MAC_FROM_GREENPHY
				if( filter_rx_mme( qca->rx_desc ) )
				{
					/* Data was a MME which is handled elsewhere */
				}
				else
			#endif
				{
					/* Set the receiving interface */
					qca->rx_desc->pxInterface = qca->pxInterface;
				#if( ipconfig_USE_NETWORK_BRIDGE != 0 )
					if( qca->pxInterface->bits.bIsBridged )
					{
						xReturn = xBridge_Process( qca->rx_desc );
						if( xReturn == pdFAIL )
						{
							if( bReleaseAfterSend == pdTRUE )
							{
								/* The Bridge could not process the descriptor,
								it must be released. */
								vReleaseNetworkBufferAndDescriptor( qca->rx_desc );
								qca->stats.rx_dropped++;
								iptraceETHERNET_RX_EVENT_LOST();
							}
						}
					}
					else
				#endif
					{
						/* Pass data up to the IP Task */
						xRxEvent.pvData = ( void * ) qca->rx_desc;
						if( xSendEventStructToIPTask( &xRxEvent, ( TickType_t ) 0 ) == pdFAIL )
						{
							/* Could not send the descriptor into the TCP/IP
							stack, it must be released. */
							vReleaseNetworkBufferAndDescriptor( qca->rx_desc );
							qca->stats.rx_dropped++;
							iptraceETHERNET_RX_EVENT_LOST();
						}
					}
				}

				iptraceNETWORK_INTERFACE_RECEIVE();

				qca->rx_desc = pxGetNetworkBufferWithDescriptor(ipTOTAL_ETHERNET_FRAME_SIZE, 0);
				if (!qca->rx_desc) {
					qca->stats.rx_dropped++;
					break;
				}
				break;
			}

		}
	}

	return 0;
}

/*====================================================================*
 *
 * Flush the tx queue. This function is only safe to
 * call from the qcaspi_spi_thread.
 *
 *--------------------------------------------------------------------*/

void
qcaspi_flush_txq(struct qcaspi *qca)
{
NetworkBufferDescriptor_t * txBuffer = NULL;

#if GREEN_PHY_SIMPLE_QOS == ON
	int i;
	for(i=0; i<QCAGP_NO_OF_QUEUES; i+=1)
	{
		while(xQueueReceive(qca->txQueues[i],&txBuffer,0))
		{
			vReleaseNetworkBufferAndDescriptor(txBuffer);
		}
	}
#else
	while(xQueueReceive(qca->txQueue,&txBuffer,0))
	{
		vReleaseNetworkBufferAndDescriptor(txBuffer);
	}
#endif
}

/*====================================================================*
 *
 * Manage synchronization with the external SPI slave.
 *
 *--------------------------------------------------------------------*/
void
qcaspi_qca7k_sync(struct qcaspi *qca, int event)
{
	uint32_t signature;
	uint32_t spi_config;
	uint32_t wrbuf_space;
	static uint32_t reset_count;

	if( event != QCASPI_SYNC_UPDATE )
		qca->sync = event;

	while( 1 ) {
		switch( qca->sync ) {
		case QCASPI_SYNC_CPUON:
			/* Read signature twice, if not valid go back to unknown state. */
			signature = qcaspi_read_register(qca, SPI_REG_SIGNATURE);
			signature = qcaspi_read_register(qca, SPI_REG_SIGNATURE);
			if (signature != QCASPI_GOOD_SIGNATURE) {
				qca->sync = QCASPI_SYNC_HARD_RESET;
			} else {
				/* ensure that the WRBUF is empty */
				wrbuf_space = qcaspi_read_register(qca, SPI_REG_WRBUF_SPC_AVA);
				if (wrbuf_space != QCASPI_HW_BUF_LEN) {
					qca->sync = QCASPI_SYNC_SOFT_RESET;
				} else {
					qca->sync = QCASPI_SYNC_READY;
					return;
				}
			}
			break;

		case QCASPI_SYNC_UNKNOWN:
		case QCASPI_SYNC_RESET:
			signature = qcaspi_read_register(qca, SPI_REG_SIGNATURE);
			if (signature == QCASPI_GOOD_SIGNATURE)
			{
				/* signature correct, do a soft reset*/
				qca->sync = QCASPI_SYNC_SOFT_RESET;
			}
			else
			{
				/* could not read signature, do a hard reset */
				qca->sync = QCASPI_SYNC_HARD_RESET;
			}
			break;

		case QCASPI_SYNC_SOFT_RESET:
			spi_config = qcaspi_read_register(qca, SPI_REG_SPI_CONFIG);
			qcaspi_write_register(qca, SPI_REG_SPI_CONFIG, spi_config | QCASPI_SLAVE_RESET_BIT);

			qca->sync = QCASPI_SYNC_WAIT_RESET;
			reset_count = 0;
			return;

		case QCASPI_SYNC_HARD_RESET:
			/* reset is normally active low, so reset ... */
			Chip_GPIO_SetPinOutLow(LPC_GPIO, GREENPHY_RESET_GPIO_PORT, GREENPHY_RESET_GPIO_PIN);
			/*  ... for 100 ms ... */
			vTaskDelay( 100 * portTICK_RATE_MS);
			/* ... and release QCA7k from reset */
			Chip_GPIO_SetPinOutHigh(LPC_GPIO, GREENPHY_RESET_GPIO_PORT, GREENPHY_RESET_GPIO_PIN);

			qca->sync = QCASPI_SYNC_WAIT_RESET;
			reset_count = 0;
			return;

		case QCASPI_SYNC_WAIT_RESET:
			/* still awaiting reset, increase counter */
			++reset_count;
			if (reset_count >= QCASPI_RESET_TIMEOUT)
			{
				/* reset did not seem to take place, try again */
				qca->sync = QCASPI_SYNC_RESET;
			}
			break;

		case QCASPI_SYNC_READY:
		default:
			signature = qcaspi_read_register(qca, SPI_REG_SIGNATURE);
			/* if signature is correct, sync is still ready*/
			if (signature == QCASPI_GOOD_SIGNATURE)
			{
				return;
			}
			/* could not read signature, do a hard reset */
			qca->sync = QCASPI_SYNC_HARD_RESET;
			break;
		}
	}

}

/*====================================================================*
 *
 * qcaspi_spi_thread - SPI worker thread.
 *
 * Handle interrupts and transmit requests on the SPI interface.
 *
 * Return:   0 on success, else failure
 *
 *--------------------------------------------------------------------*/

#ifndef	GREENPHY_SYNC_HIGH_CHECK_TIME_MS
	/* Check if the Sync Status of the GREENPHY is still ready after 15 seconds of not
	receiving packets. */
	#define GREENPHY_SYNC_HIGH_CHECK_TIME_MS	15000
#endif

#ifndef	GREENPHY_SYNC_LOW_CHECK_TIME_MS
	/* Check if the Sync Status of the GREENPHY is still not ready every second. */
	#define GREENPHY_SYNC_LOW_CHECK_TIME_MS	1000
#endif

void
qcaspi_spi_thread(void *data)
{
	struct qcaspi *qca = (struct qcaspi *) data;
	uint32_t ulInterruptCause;
	uint32_t intr_enable;
	uint32_t ulNotificationValue;
	TickType_t xSyncRemTime = pdMS_TO_TICKS( GREENPHY_SYNC_LOW_CHECK_TIME_MS );

	for ( ;; )
	{
		/* Take notification
		 * 0 timeout
		 * 1 interrupt (including receive)
		 * 2 receive (not used, handled by interrupt)
		 * 4 transmit
		 * */
		ulNotificationValue = ulTaskNotifyTake( pdTRUE, xSyncRemTime );

		if ( !ulNotificationValue )
		{
			/* We got a timeout, check if we need to restart sync */
			qcaspi_qca7k_sync(qca, QCASPI_SYNC_UPDATE);
			/* not synced, awaiting reset, or unknown */
			if (qca->sync != QCASPI_SYNC_READY)
			{
				qcaspi_flush_txq(qca);
				continue;
			}
		}

		if ( ulNotificationValue & QCAGP_INT_FLAG )
		{
			/* We got an interrupt */
			intr_enable = disable_spi_interrupts(qca);
			ulInterruptCause = qcaspi_read_register(qca, SPI_REG_INTR_CAUSE);

			if (ulInterruptCause & SPI_INT_CPU_ON)
			{
				qcaspi_qca7k_sync(qca, QCASPI_SYNC_CPUON);
				/* if not synced, wait reset */
				if (qca->sync != QCASPI_SYNC_READY)
					continue;

				xSyncRemTime = pdMS_TO_TICKS( GREENPHY_SYNC_HIGH_CHECK_TIME_MS );
				intr_enable = (SPI_INT_CPU_ON | SPI_INT_PKT_AVLBL | SPI_INT_RDBUF_ERR | SPI_INT_WRBUF_ERR);
			}

			if (ulInterruptCause & SPI_INT_RDBUF_ERR)
			{
				/* restart sync */
				qcaspi_qca7k_sync(qca, QCASPI_SYNC_RESET);
				xSyncRemTime = pdMS_TO_TICKS( GREENPHY_SYNC_LOW_CHECK_TIME_MS );
				continue;
			}

			if (ulInterruptCause & SPI_INT_WRBUF_ERR)
			{
				/* restart sync */
				qcaspi_qca7k_sync(qca, QCASPI_SYNC_RESET);
				xSyncRemTime = pdMS_TO_TICKS( GREENPHY_SYNC_LOW_CHECK_TIME_MS );
				continue;
			}

			if (ulInterruptCause & SPI_INT_WRBUF_BELOW_WM)
			{
				/* transmit is handled later */
				/* disable write watermark interrupt */
				intr_enable &= ~SPI_INT_WRBUF_BELOW_WM;
			}

			qcaspi_write_register(qca, SPI_REG_INTR_CAUSE, ulInterruptCause);
			enable_spi_interrupts(qca, intr_enable);

			/* can only handle other interrupts if sync has occured */
			if (qca->sync == QCASPI_SYNC_READY)
			{
				if (ulInterruptCause & SPI_INT_PKT_AVLBL)
				{
					qcaspi_receive(qca);
				}
			}
		}

		if (qca->sync == QCASPI_SYNC_READY)
		{
			if( uxQueueMessagesWaiting(qca->txQueue) )
			{
				if( qcaspi_transmit(qca) != 0 )
				{
					/* QCA7k write buffer is full, but we need to send more packets
					 * so set watermark interrupt */
					uint32_t old_intr_enable = qcaspi_read_register(qca, SPI_REG_INTR_ENABLE);
					qcaspi_write_register(qca, SPI_REG_INTR_ENABLE, old_intr_enable | SPI_INT_WRBUF_BELOW_WM);
				}
			}
		}
	}

}
