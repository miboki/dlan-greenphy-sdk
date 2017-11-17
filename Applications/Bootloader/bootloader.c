/* LPCOpen includes. */
#include "board.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_TFTP_client.h"

/* GreenPHY SDK includes. */
#include "GreenPhySDKConfig.h"

/* Project includes. */
#include "bootloader.h"

typedef struct xTFTP_CALLBACK_HANDLE
{
	uint32_t ulBytesWritten; /* Bytes written to flash. */
} TFTPCallbackHandle_t;

static BaseType_t prvTFTPReceiveCallback( char *pcData, BaseType_t xDataLength, void *pvCallbackHandle )
{
BaseType_t x, xReturn = pdFAIL;
uint32_t ulChecksum = 0, *pulChecksum, ulDestination, ulSector, ulReturn;
TFTPCallbackHandle_t *pxCallbackHandle = pvCallbackHandle;

	/* Verify checksum in first block. */
	if( pxCallbackHandle->ulBytesWritten == 0 )
	{
		/* The eighth double word is the two's complement
		of the sum of the first seven double words. */
		pulChecksum = (uint32_t *) pcData;
		for (x = 0; x < 8; x++) {
			ulChecksum += *pulChecksum++;
		}
		DEBUGOUT("Bootloader: receiving ");
	}

	if( ulChecksum == 0 )
	{
		DEBUGOUT(".");

		/* Calculate current address and sector in flash. */
		ulDestination = sdkconfigUSER_PROGRAM_START + pxCallbackHandle->ulBytesWritten;
		ulSector = Chip_IAP_GetSectorNumber( ulDestination );

		/* Check if it's the first block written into this sector. */
		if( ulSector != Chip_IAP_GetSectorNumber( ulDestination - tftpBLOCK_LENGTH ) )
		{
			/* Erase sector so new data can be written to it. */
			Chip_IAP_PreSectorForReadWrite( ulSector, ulSector );
			Chip_IAP_EraseSector( ulSector, ulSector );
		}

		configASSERT( ( ulDestination & 0xFF ) == 0 );           /* 256 byte alignment required. */
		configASSERT( ( ( ( uint32_t ) pcData ) & 0x01 ) == 0 ); /* Word alignment required. */

		/* Copy data to flash. */
		__disable_irq();
		ulReturn = Chip_IAP_PreSectorForReadWrite( ulSector, ulSector );
		if( ulReturn == IAP_CMD_SUCCESS )
		{
			ulReturn = Chip_IAP_CopyRamToFlash( ulDestination, (uint32_t *) pcData, tftpBLOCK_LENGTH );
			if( ulReturn == IAP_CMD_SUCCESS )
			{
				pxCallbackHandle->ulBytesWritten += tftpBLOCK_LENGTH;
				xReturn = pdPASS;
			}
		}
		__enable_irq();
	}

	return xReturn;
}

void prvInvalidateCode()
{
uint32_t ulSector = Chip_IAP_GetSectorNumber( sdkconfigUSER_PROGRAM_START );

	/* Erase first sector of user program from flash. */
	Chip_IAP_PreSectorForReadWrite( ulSector, ulSector );
	Chip_IAP_EraseSector( ulSector, ulSector );
}

BaseType_t prvVerifyCode()
{
uint32_t ulSector = Chip_IAP_GetSectorNumber( sdkconfigUSER_PROGRAM_START );
uint32_t ulChecksum = 0, *pulChecksum;
BaseType_t x, xReturn = pdFAIL;

	if( Chip_IAP_BlankCheckSector( ulSector, ulSector ) != 0 )
	{
		/* Flash sector is not blank. */

		/* The eighth double word is the two's complement
		of the sum of the first seven double words. */
		pulChecksum = ( uint32_t * ) sdkconfigUSER_PROGRAM_START;
		for (x = 0; x < 8; x++) {
			ulChecksum += *pulChecksum++;
		}

		if( ulChecksum == 0 )
		{
			xReturn = pdPASS;
		}
	}

	return xReturn;
}

void vStartApplication()
{
void (*user_code_entry)(void);
uint32_t *pul;	/* Used to load address of reset handler from user flash. */

	DEBUGOUT("Bootloader: starting user application now.\r\n");

	/* Load contents of second word of user flash - the reset handler address
	in the applications vector table. */
	pul = (uint32_t *)( sdkconfigUSER_PROGRAM_START + 4 );

	/* Set user_code_entry to be the address contained in that second word
	of user flash. */
	user_code_entry = (void *) *pul;

	/* Change the Vector Table to the start of the user application
	in case it uses interrupts */

	SCB->VTOR = ( sdkconfigUSER_PROGRAM_START & 0x1FFFFF80 );

	/* Jump to user application. */
    user_code_entry();

    /* Should not get here. */
    for( ;; );
}

static void vBootloaderTask(void *pvParameters)
{
const TickType_t xDelay = 3000 / portTICK_PERIOD_MS;
const char *pcFilename = "greenphy.dvl";
struct freertos_sockaddr xAddress;
TFTPCallbackHandle_t xCallbackHandle = { 0 };
BaseType_t xReturn;

	do {
		/* _ML_ TODO: read TFTP server address from configuration. */
		xAddress.sin_addr = FreeRTOS_inet_addr_quick( 172, 16, 200, 40 );
		xAddress.sin_port = FreeRTOS_htons( 69 );
		xCallbackHandle.ulBytesWritten = 0;

		DEBUGOUT( "Bootloader: looking for new firmware.\r\n" );
		xReturn = xTFTPReceiveFile( &xAddress, pcFilename, prvTFTPReceiveCallback, &xCallbackHandle );
		if( xReturn != pdPASS )
		{
			DEBUGOUT( "Bootloader: an error occurred.\r\n" );

			if( xCallbackHandle.ulBytesWritten != 0 )
			{
				/* An error occurred, but old firmware was overwritten. */
				prvInvalidateCode();
			}
		}
		else
		{
			DEBUGOUT( "\nBootloader: successfully received new firmware.\r\n" );
		}

		if( prvVerifyCode() == pdPASS )
		{
			/* We have a valid application, disable interrupts, deinitialize EMAC
			and end the scheduler to switch into the user application. */
			__disable_irq();
			Chip_ENET_DeInit( LPC_ETHERNET );
			vTaskEndScheduler();
			break;
		}

		/* Wait before retry. */
		vTaskDelay( xDelay );
	} while( 1 );

}

void vBootloaderInit()
{

	xTaskCreate( vBootloaderTask, "bootloader", 240, NULL, ( tskIDLE_PRIORITY + 1 ), NULL );

}
