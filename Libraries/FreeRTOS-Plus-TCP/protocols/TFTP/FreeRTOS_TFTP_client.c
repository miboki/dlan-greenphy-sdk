/* Standard includes. */

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_TFTP_client.h"

typedef enum
{
	eTFTPOpcodeRRQ = 1,
	eTFTPOpcodeWRQ,
	eTFTPOpcodeDATA,
	eTFTPOpcodeACK,
	eTFTPOpcodeERROR
} eTFTPOpcode_t;

/* Error codes from the RFC. */
typedef enum
{
	eTFTPErrorFileNotFound = 1,
	eTFTPErrorAccessViolation,
	eTFTPErrorDiskFull,
	eTFTPErrorIllegalTFTPOperation,
	eTFTPErrorUnknownTransferID,
	eTFTPErrorFileAlreadyExists
} eTFTPErrorCode_t;

#include "pack_struct_start.h"
struct xTFTP_REQ_PACKET
{
	uint16_t usOpcode;
	char  pcFilename[1];
}
#include "pack_struct_end.h"
; /* Suppress IDE syntax error warning. */
typedef struct xTFTP_REQ_PACKET TFTPReqPacket_t;

#include "pack_struct_start.h"
struct xTFTP_DATA_PACKET
{
	uint16_t usOpcode;
	uint16_t usBlockNumber;
	char  pcData[1];
}
#include "pack_struct_end.h"
; /* Suppress IDE syntax error warning. */
typedef struct xTFTP_DATA_PACKET TFTPDataPacket_t;

#include "pack_struct_start.h"
struct xTFTP_ACK_PACKET
{
	uint16_t usOpcode;
	uint16_t usBlockNumber;
}
#include "pack_struct_end.h"
; /* Suppress IDE syntax error warning. */
typedef struct xTFTP_ACK_PACKET TFTPAckPacket_t;

#include "pack_struct_start.h"
struct xTFTP_ERROR_PACKET
{
	uint16_t usOpcode;
	uint16_t usErrorCode;
	char  pcErrorMsg[1];
}
#include "pack_struct_end.h"
; /* Suppress IDE syntax error warning. */
typedef struct xTFTP_ERROR_PACKET TFTPErrorPacket_t;

struct xTFTP_CLIENT
{
	Socket_t xSocket;
	struct freertos_sockaddr *pxAddress;
	const char *pcFilename;
	FTFTPReceiveCallback fReceiveCallback;
	void *pvCallbackHandle;
	uint16_t usBlockNumber;
};

typedef struct xTFTP_CLIENT TFTPClient_t;

/* The index for the error string below MUST match the value of the applicable
eTFTPErrorCode_t error code value. */
static const char *ppcErrorStrings[] =
{
	NULL, /* Not valid. */
	"File not found.",
	"Access violation.",
	"Disk full or allocation exceeded.",
	"Illegal TFTP operation.",
	"Unknown transfer ID.",
	"File already exists.",
	"No such user."
};

/*-----------------------------------------------------------*/

static BaseType_t prvSendRequest( TFTPClient_t *pxClient, uint16_t usOpcode )
{
const char pcOctedMode[] = "octet";
TFTPReqPacket_t *pxPacket;
BaseType_t xFilenameLength, uxCount, xReturn = pdFAIL;

	xFilenameLength = ( strlen( pxClient->pcFilename ) + 1 ); /* Include terminating null byte. */
	uxCount = sizeof( usOpcode ) + xFilenameLength + sizeof( pcOctedMode );

	pxPacket = ( TFTPReqPacket_t * ) FreeRTOS_GetUDPPayloadBuffer( uxCount, portMAX_DELAY );

	if( pxPacket != NULL )
	{
		pxPacket->usOpcode = FreeRTOS_htons( usOpcode );

		strcpy( pxPacket->pcFilename, pxClient->pcFilename );
		strcpy( pxPacket->pcFilename + xFilenameLength, pcOctedMode );

		if( FreeRTOS_sendto( pxClient->xSocket, pxPacket, uxCount, FREERTOS_ZERO_COPY,
		 					 pxClient->pxAddress, sizeof( *(pxClient->pxAddress) ) ) == 0 )
		{
		   /* The send operation failed, release the buffer. */
		   FreeRTOS_ReleaseUDPPayloadBuffer( ( void * ) pxPacket );
		}
		else
		{
			xReturn = pdPASS;
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

static BaseType_t prvSendAck( TFTPClient_t *pxClient )
{
TFTPAckPacket_t *pxPacket;
BaseType_t xCount, xReturn = pdFAIL;

	xCount = sizeof( TFTPAckPacket_t );
	pxPacket = ( TFTPAckPacket_t * ) FreeRTOS_GetUDPPayloadBuffer( xCount, portMAX_DELAY );

	if( pxPacket != NULL )
	{
		pxPacket->usOpcode = FreeRTOS_htons( eTFTPOpcodeACK );
		pxPacket->usBlockNumber = FreeRTOS_htons( pxClient->usBlockNumber );

		if( FreeRTOS_sendto( pxClient->xSocket, pxPacket, xCount, FREERTOS_ZERO_COPY,
		 					 pxClient->pxAddress, sizeof( *(pxClient->pxAddress) ) ) == 0 )
		{
		   /* The send operation failed, release the buffer. */
		   FreeRTOS_ReleaseUDPPayloadBuffer( ( void * ) pxPacket );
		}
		else
		{
			xReturn = pdPASS;
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

static BaseType_t prvSendError( TFTPClient_t *pxClient, uint16_t usErrorCode )
{
TFTPErrorPacket_t *pxPacket;
BaseType_t xErrorMsgLength, xCount, xReturn = pdFAIL;

	xErrorMsgLength = strlen( ppcErrorStrings[ usErrorCode ] ) + 1; /* Include terminating null byte. */
	xCount = sizeof( TFTPErrorPacket_t ) - 1 + xErrorMsgLength;

	pxPacket = ( TFTPErrorPacket_t * ) FreeRTOS_GetUDPPayloadBuffer( xCount, portMAX_DELAY );

	if( pxPacket != NULL )
	{
		pxPacket->usOpcode = FreeRTOS_htons( eTFTPOpcodeERROR );
		pxPacket->usErrorCode = FreeRTOS_htons( usErrorCode );
		strcpy( pxPacket->pcErrorMsg, ppcErrorStrings[ usErrorCode ] );

		if( FreeRTOS_sendto( pxClient->xSocket, pxPacket, xCount, FREERTOS_ZERO_COPY,
		 					 pxClient->pxAddress, sizeof( *(pxClient->pxAddress) ) ) == 0 )
		{
		   /* The send operation failed, release the buffer. */
		   FreeRTOS_ReleaseUDPPayloadBuffer( ( void * ) pxPacket );
		}
		else
		{
			xReturn = pdPASS;
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

static BaseType_t prvReceiveFile( TFTPClient_t *pxClient )
{
BaseType_t xBytesReceived, xDataLength = tftpBLOCK_LENGTH, xReturn = pdPASS, xRetriesLeft = tftpMAX_RETRIES;
uint8_t *pucBuffer;
TFTPDataPacket_t *pxPacket;

	/* Send initial read request. */
	prvSendRequest( pxClient, eTFTPOpcodeRRQ );

	/* xDataLength is TFTP block length (512B) until last packet is received. Dallying is not supported.
	xReturn is set to pdFAIL on error or exceeding retry limit. */
	while( ( xReturn != pdFAIL ) && xDataLength == tftpBLOCK_LENGTH )
	{
		/* Receive packet from socket and update it's sockaddr, as the TFTP server will change it's port from 69 to a new one. */
		xBytesReceived = FreeRTOS_recvfrom( pxClient->xSocket, &pucBuffer, 0, FREERTOS_ZERO_COPY, pxClient->pxAddress, NULL );

		if( xBytesReceived > 0 )
		{
			/* Packet received. */
			pxPacket = ( TFTPDataPacket_t * ) pucBuffer;
			if( pxPacket->usOpcode == FreeRTOS_htons( eTFTPOpcodeDATA ) )
			{
				if ( pxPacket->usBlockNumber == FreeRTOS_htons( pxClient->usBlockNumber + 1 ) )
				{
					pxClient->usBlockNumber++;
					xRetriesLeft = tftpMAX_RETRIES;

					/* Expected block number was received, process data. */
					xDataLength = xBytesReceived - ( sizeof( pxPacket->usOpcode ) + sizeof( pxPacket->usBlockNumber) );
					if( pxClient->fReceiveCallback( pxPacket->pcData, xDataLength, pxClient->pvCallbackHandle ) != pdPASS )
					{
						prvSendError( pxClient, eTFTPErrorDiskFull );
						xReturn = pdFAIL;
					}
				}
				else
				{
					/* Wrong block number. */
					xRetriesLeft--;
				}
			}
			else if( pxPacket->usOpcode == FreeRTOS_htons( eTFTPOpcodeERROR ) )
			{
				/* Error received. */
				FreeRTOS_printf( ( "TFTP: error code %d received: %s\r\n",
									( ( TFTPErrorPacket_t * ) pxPacket )->usErrorCode,
									( ( TFTPErrorPacket_t * ) pxPacket )->pcErrorMsg ) );
				xReturn = pdFAIL;
			}
		}
		else if( xBytesReceived < 0 )
		{
			/* Timeout occurred. */
			xRetriesLeft--;
		}

		if( xBytesReceived >= 0 )
		{
			/* A zero-copy payload buffer was received, release it. */
			FreeRTOS_ReleaseUDPPayloadBuffer( pucBuffer );
		}

		if( xRetriesLeft == 0 )
		{
			FreeRTOS_printf( ( "TFTP: max retries exceeded.\r\n" ) );
			xReturn = pdFAIL;
		}

		if( xReturn != pdFAIL )
		{
			if( pxClient->usBlockNumber == 0 )
			{
				/* Read request was not acknowledged, resend it. */
				prvSendRequest( pxClient, eTFTPOpcodeRRQ );
			}
			else
			{
				/*(Re-)acknowledge last received data packet. */
				prvSendAck( pxClient );
			}
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

BaseType_t xTFTPReceiveFile(struct freertos_sockaddr *pxAddress, const char *pcFilename, FTFTPReceiveCallback fReceiveCallback, void *pvCallbackHandle )
{
TFTPClient_t *pxClient;
TickType_t xRxTimeout = pdMS_TO_TICKS( ipconfigTFTP_TIMEOUT_MS );
BaseType_t xReturn = pdFAIL;

	pxClient = pvPortMalloc( sizeof( TFTPClient_t ) );

	if( pxClient != NULL )
	{
		pxClient->xSocket = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP );
		configASSERT( pxClient->xSocket != FREERTOS_INVALID_SOCKET );
		pxClient->pxAddress = pxAddress;
		pxClient->pcFilename = pcFilename;
		pxClient->fReceiveCallback = fReceiveCallback;
		pxClient->pvCallbackHandle = pvCallbackHandle;
		pxClient->usBlockNumber = 0;

		FreeRTOS_setsockopt( pxClient->xSocket, 0, FREERTOS_SO_RCVTIMEO, &xRxTimeout, sizeof( xRxTimeout ) );

		xReturn = prvReceiveFile( pxClient );

		FreeRTOS_closesocket( pxClient->xSocket );
		vPortFree( pxClient );
	}

	return xReturn;
}
/*-----------------------------------------------------------*/
