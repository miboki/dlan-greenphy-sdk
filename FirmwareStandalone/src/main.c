/* LPCOpen includes. */
#include "board.h"
#include <cr_section_macros.h>

/* FreeRTOS includes. */
#include <FreeRTOS.h>
#include "task.h"

/* FreeRTOS +TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_Routing.h"
#include "FreeRTOS_TCP_server.h"

/* GreenPHY SDK includes. */
#include "netConfig.h"
#include "greenPhyModuleConfig.h"

/* The default IP and MAC address used by the demo.  The address configuration
defined here will be used if ipconfigUSE_DHCP is 0, or if ipconfigUSE_DHCP is
1 but a DHCP server could not be contacted.  See the online documentation for
more information. */
static const uint8_t ucIPAddress[ 4 ] = { configIP_ADDR0, configIP_ADDR1, configIP_ADDR2, configIP_ADDR3 };
static const uint8_t ucNetMask[ 4 ] = { configNET_MASK0, configNET_MASK1, configNET_MASK2, configNET_MASK3 };
static const uint8_t ucGatewayAddress[ 4 ] = { configGATEWAY_ADDR0, configGATEWAY_ADDR1, configGATEWAY_ADDR2, configGATEWAY_ADDR3 };
static const uint8_t ucDNSServerAddress[ 4 ] = { configDNS_SERVER_ADDR0, configDNS_SERVER_ADDR1, configDNS_SERVER_ADDR2, configDNS_SERVER_ADDR3 };

/* Default MAC address configuration.  The demo creates a virtual network
connection that uses this MAC address by accessing the raw Ethernet data
to and from a real network connection on the host PC.  See the
configNETWORK_INTERFACE_TO_USE definition for information on how to configure
the real network connection to use. */
const uint8_t ucMACAddress[ 6 ] = { configMAC_ADDR0, configMAC_ADDR1, configMAC_ADDR2, configMAC_ADDR3, configMAC_ADDR4, configMAC_ADDR5 };

static NetworkInterface_t xEthInterface = { 0 };
static NetworkInterface_t xPlcInterface = { 0 };
static NetworkInterface_t xBridgeInterface = { 0 };
static NetworkEndPoint_t  xEndPoint =  { 0 };

/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook(void) {
//	printToUart("Malloc failed!\r\n");
	for (;;)
		;
}

/*-----------------------------------------------------------*/

void vApplicationTickHook(void) {
	/* Called from every tick interrupt */

//	DEBUG_EXECUTE(
//	{
//		static size_t old_mem = 0;
//		size_t mem = xPortGetFreeHeapSize();
//		if(old_mem != mem)
//		{
//			DEBUG_PRINT(DEBUG_INFO,"application free heap: %d(0x%x)\r\n",mem,mem);
//			old_mem = mem;
//		}
//	}
//	);
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

static void prvServerWorkTask( void *pvParameters );

/*-----------------------------------------------------------*/
void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent, NetworkEndPoint_t *pxEndPoint  ) {
	uint32_t ulIPAddress, ulNetMask, ulGatewayAddress, ulDNSServerAddress;
	static BaseType_t xTasksAlreadyCreated = pdFALSE;
	int8_t cBuffer[ 16 ];

	    /* Check this was a network up event, as opposed to a network down event. */
	    DEBUGOUT("Network hook\r\n");
	    if( eNetworkEvent == eNetworkUp )
	    {
		    DEBUGOUT("Network up\r\n");
	        /* Create the tasks that use the TCP/IP stack if they have not already been
	        created. */
	        if( xTasksAlreadyCreated == pdFALSE )
	        {
	            /*
	             * Create the tasks here.
	             */

	        	#define	mainTCP_SERVER_STACK_SIZE						240 /* Not used in the Win32 simulator. */

	    		xTaskCreate( prvServerWorkTask, "SvrWork", mainTCP_SERVER_STACK_SIZE, NULL, ipconfigIP_TASK_PRIORITY - 1, NULL );


	            xTasksAlreadyCreated = pdTRUE;
	        }

	        /* The network is up and configured.  Print out the configuration,
	        which may have been obtained from a DHCP server. */
	        FreeRTOS_GetAddressConfiguration( pxEndPoint,
	                                          &ulIPAddress,
	                                          &ulNetMask,
	                                          &ulGatewayAddress,
	                                          &ulDNSServerAddress );

	        /* Convert the IP address to a string then print it out. */
	        FreeRTOS_inet_ntoa( ulIPAddress, cBuffer );
	        DEBUGOUT( "IP Address: %s\r\n", cBuffer );

	        /* Convert the net mask to a string then print it out. */
	        FreeRTOS_inet_ntoa( ulNetMask, cBuffer );
	        DEBUGOUT( "Subnet Mask: %s\r\n", cBuffer );

	        /* Convert the IP address of the gateway to a string then print it out. */
	        FreeRTOS_inet_ntoa( ulGatewayAddress, cBuffer );
	        DEBUGOUT( "Gateway IP Address: %s\r\n", cBuffer );

	        /* Convert the IP address of the DNS server to a string then print it out. */
	        FreeRTOS_inet_ntoa( ulDNSServerAddress, cBuffer );
	        DEBUGOUT( "DNS server IP Address: %s\r\n", cBuffer );
	    }
}
/*-----------------------------------------------------------*/

const char *pcApplicationHostnameHook( void )
{
	return "GreenPHY evalboard II";
}
/*-----------------------------------------------------------*/

/*_HT_ introduced this memory-check temporarily for debugging. */
BaseType_t xApplicationMemoryPermissions( uint32_t aAddress )
{
	return 0x03;
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

static void prvServerWorkTask( void *pvParameters )
{
const TickType_t xInitialBlockTime = pdMS_TO_TICKS( 200UL );
TCPServer_t *pxTCPServer = NULL;

/* A structure that defines the servers to be created.  Which servers are
included in the structure depends on the mainCREATE_HTTP_SERVER and
mainCREATE_FTP_SERVER settings at the top of this file. */
static const struct xSERVER_CONFIG xServerConfiguration[] =
{
	/* Server type,		port number,	backlog, 	root dir. */
	{ eSERVER_HTTP, 	80, 			0, 		"" }
};

	/* Remove compiler warning about unused parameter. */
	( void ) pvParameters;

	/* Create the servers defined by the xServerConfiguration array above. */
	pxTCPServer = FreeRTOS_CreateTCPServer( xServerConfiguration, sizeof( xServerConfiguration ) / sizeof( xServerConfiguration[ 0 ] ) );
	configASSERT( pxTCPServer );

	for( ;; )
	{
		FreeRTOS_TCPServerWork( pxTCPServer, xInitialBlockTime );
	}
}
/*-----------------------------------------------------------*/

/* A structure to hold the query string parameter values. */
typedef struct query_param {
	char *pcKey;
	char *pcValue;
} QueryParam_t;

BaseType_t xParseQuery(char *pcQuery, QueryParam_t *pxParams, BaseType_t xMaxParams)
{
BaseType_t x = 0;

	if( ( pcQuery != NULL ) && ( *pcQuery != '\0' ) )
	{
		pxParams[x++].pcKey = pcQuery;             /* First key is at begin of query. */
		while ( ( x < xMaxParams ) && ( (pcQuery = strchr(pcQuery, ipconfigHTTP_REQUEST_DELIMITER)) != NULL ) )
		{
			*pcQuery = '\0';                     /* Replace delimiter with '\0'. */
			pxParams[x].pcKey = ++pcQuery;         /* Set next parameter key. */

			/* Look for previous parameter value. */
			if ((pxParams[x - 1].pcValue = strchr(pxParams[x - 1].pcKey, '=')) != NULL) {
				*(pxParams[x - 1].pcValue)++ = '\0'; /* Replace '=' with '\0'. */
			}
			x++;
		}

		/* Look for last parameter value. */
		if ((pxParams[x - 1].pcValue = strchr(pxParams[x - 1].pcKey, '=')) != NULL) {
			*(pxParams[x - 1].pcValue)++ = '\0';     /* Replace '=' with '\0'. */
		}
	}

	return x;
}


QueryParam_t *pxFindKeyInQueryParams( char *pcKey, QueryParam_t *pxParams, BaseType_t xParamCount )
{
BaseType_t x;
QueryParam_t *pxParam = NULL;

	for( x = 0; x < xParamCount; x++ )
	{
		if( strcmp( pxParams[ x ].pcKey, pcKey ) == 0 )
		{
			pxParam = &pxParams[ x ];
			break;
		}
	}

	return pxParam;
}

BaseType_t xIsStringOneOf( char *pcString, char *ppcStringList[], BaseType_t xStringListSize )
{
BaseType_t x;

	for( x = 0; x < xStringListSize; x++ )
	{
		if( strcmp( pcString, ppcStringList[x] ) == 0 )
		{
			break;
		}
	}

	return x;
}

typedef UBaseType_t ( * FHTTPFileHandleFunction ) ( char *pcBuffer, size_t uxBufferLength, QueryParam_t *pxParams, BaseType_t xParamCount );

typedef struct xHTTP_FILE_HANDLE
{
	const char *pcFilename;
	FHTTPFileHandleFunction fFileHandleFunction_Get;
	FHTTPFileHandleFunction fFileHandleFunction_Set;
} HTTPFileHandle_t;


//TaskStatus_t *pxTaskStatusArray;
//volatile UBaseType_t uxArraySize, x;
//uint32_t ulTotalTime, ulStatsAsPercentage;
//
//uxArraySize = uxCurrentNumberOfTasks;
//pxTaskStatusArray = pvPortMalloc( uxCurrentNumberOfTasks * sizeof( TaskStatus_t ) );
//
//if( pxTaskStatusArray != NULL )
//{
//	/* Generate the (binary) data. */
//	uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, &ulTotalTime );
//

enum eActions {
	eACTION_GET,
	eACTION_SET,
	eACTION_UNK,
};

UBaseType_t uxFileHandle_StatusGet( char *pcBuffer, size_t uxBufferLength, QueryParam_t *pxParams, BaseType_t xParamCount )
{
BaseType_t xCount = 0;

	xCount = snprintf( pcBuffer, uxBufferLength,
			"{\"status\":\"success\",\"data\":{\"uptime\":\"%ds\",\"free_heap\":\"%d bytes\",\"led\":%s}}",
			( portGET_RUN_TIME_COUNTER_VALUE() / 10000UL ), xPortGetFreeHeapSize(),
			(Board_LED_Test( LEDS_LED0 ) ? "true" : "false" ) );

	return xCount;
}

UBaseType_t uxFileHandle_StatusSet( char *pcBuffer, size_t uxBufferLength, QueryParam_t *pxParams, BaseType_t xParamCount )
{
BaseType_t xCount = 0;
QueryParam_t *pxParam;

	pxParam = pxFindKeyInQueryParams( "led", pxParams, xParamCount );
	if( pxParam != NULL )
	{
		if( strcmp(pxParam->pcValue, "on") == 0 )
		{
			Board_LED_Set( LEDS_LED0, TRUE );
		}
		else if( strcmp(pxParam->pcValue, "off") == 0 )
		{
			Board_LED_Set( LEDS_LED0, FALSE );
		}
		else if( strcmp(pxParam->pcValue, "toggle") == 0 )
		{
			Board_LED_Toggle( LEDS_LED0 );
		}
		else
		{
			xCount = -1;
		}
	}

	return xCount;
}

UBaseType_t uxFileHandle_ConfigGet( char *pcBuffer, size_t uxBufferLength, QueryParam_t *pxParams, BaseType_t xParamCount )
{
BaseType_t xCount = 0;

	return xCount;
}

static HTTPFileHandle_t pxHTTPFileHandles[ ] =
{
	{ "/status.json", uxFileHandle_StatusGet, uxFileHandle_StatusSet },
	{ "/config.json", uxFileHandle_ConfigGet, NULL }
};

#define ARRAY_SIZE(x) ( BaseType_t ) (sizeof( x ) / sizeof( x )[ 0 ] )

HTTPFileHandle_t *pxFindFileHandle( const char *pcFilename )
{
BaseType_t x;
HTTPFileHandle_t *pxFileHandle = NULL;

	for( x = 0; x < ARRAY_SIZE( pxHTTPFileHandles ); x++ )
	{
		if( strcmp( pcFilename, pxHTTPFileHandles[ x ].pcFilename ) == 0 )
		{
			pxFileHandle = &pxHTTPFileHandles[ x ];
			break;
		}
	}

	return pxFileHandle;
}

/*
 * A GET request is received containing a special character,
 * usually a question mark.
 * const char *pcURLData;	// A request, e.g. "/request?limit=75"
 * char *pcBuffer;			// Here the answer can be written
 * size_t uxBufferLength;	// Size of the buffer
 *
 */
size_t uxApplicationHTTPHandleRequestHook( const char *pcURLData, char *pcBuffer, size_t uxBufferLength )
{
char *pcQuery;
QueryParam_t pxParams[3];
BaseType_t xParamCount;
HTTPFileHandle_t *pxFileHandle = NULL;
QueryParam_t *pxParam;
size_t uxResult = 0;

	pcQuery = strchr( pcURLData, ipconfigHTTP_REQUEST_CHARACTER );
	*pcQuery++ = '\0';

	pxFileHandle = pxFindFileHandle( pcURLData );
	if( pxFileHandle != NULL )
	{
		xParamCount = xParseQuery( pcQuery, pxParams, ARRAY_SIZE( pxParams ) );

		pxParam = pxFindKeyInQueryParams( "action", pxParams, xParamCount );
		if( pxParam != NULL )
		{
			if( strcmp(pxParam->pcValue, "get") == 0 )
			{
				uxResult = pxFileHandle->fFileHandleFunction_Get(pcBuffer, uxBufferLength, pxParams, xParamCount);
			}
			else if( strcmp(pxParam->pcValue, "set") == 0 )
			{
				uxResult = pxFileHandle->fFileHandleFunction_Set(pcBuffer, uxBufferLength, pxParams, xParamCount);
			}

			if( uxResult <= 0 )
			{
				uxResult = snprintf( pcBuffer, uxBufferLength,
						"{\"status\":%s}",
						( ( uxResult == 0 ) ? "\"success\"" : "\"fail\"" ) );
			}
		}
	}

	return uxResult;
}
/*-----------------------------------------------------------*/

int main(void) {
	SystemCoreClockUpdate();
	Board_Init();

	DEBUGSTR("\r\n\r\nSTANDALONE ");
	{
		uint32_t reset_reason = LPC_SYSCTL->RSID;
		DEBUGOUT("RSID:0x%x", reset_reason);
		if (!reset_reason)
			DEBUGOUT("->Bootloader");
		if (reset_reason & 0x1)
			DEBUGOUT("->Power On");
		if (reset_reason & 0x2)
			DEBUGOUT("->Reset");
		if (reset_reason & 0x4)
			DEBUGOUT("->Watchdog");
		if (reset_reason & 0x8)
			DEBUGOUT("->BrownOut Detection");
		if (reset_reason & 0x10)
			DEBUGOUT("->JTAG/restart");
		DEBUGOUT("\r\n");
		LPC_SYSCTL->RSID = reset_reason;
	}

//	DEBUGOUT("UART0 %s(%s)\r\n", features, version);

	extern NetworkInterface_t *pxBridge_FillInterfaceDescriptor( BaseType_t xIndex, NetworkInterface_t *pxInterface );
	pxBridge_FillInterfaceDescriptor(0, &xBridgeInterface);
	FreeRTOS_AddNetworkInterface(&xBridgeInterface);

	extern NetworkInterface_t *pxLPC1758_FillInterfaceDescriptor( BaseType_t xIndex, NetworkInterface_t *pxInterface );
	pxLPC1758_FillInterfaceDescriptor(0, &xEthInterface);
	xEthInterface.bits.bIsBridged = 1;
	FreeRTOS_AddNetworkInterface(&xEthInterface);

	extern NetworkInterface_t *pxQCA7000_FillInterfaceDescriptor( BaseType_t xIndex, NetworkInterface_t *pxInterface );
	pxQCA7000_FillInterfaceDescriptor(0, &xPlcInterface);
	xPlcInterface.bits.bIsBridged = 1;
	FreeRTOS_AddNetworkInterface(&xPlcInterface);

	FreeRTOS_FillEndPoint(&xEndPoint, ucIPAddress, ucNetMask, ucGatewayAddress, ucDNSServerAddress, ucMACAddress);
	xEndPoint.bits.bIsDefault = pdTRUE_UNSIGNED;
	xEndPoint.bits.bWantDHCP = pdTRUE_UNSIGNED;
	FreeRTOS_AddEndPoint(&xBridgeInterface, &xEndPoint);

	FreeRTOS_IPStart();

	vTaskStartScheduler();

	return 0;
}
