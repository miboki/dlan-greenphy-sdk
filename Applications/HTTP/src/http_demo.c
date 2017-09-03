/* Standard includes. */
#include <string.h>

/* LPCOpen Includes. */
#include "board.h"

/* FreeRTOS includes. */
#include <FreeRTOS.h>
#include "task.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_Routing.h"

/* GreenPHY SDK includes. */
#include "GreenPhySDKConfig.h"

/* Project includes. */
#include "http_query_parser.h"

BaseType_t xRequestHandler_Status( char *pcBuffer, size_t uxBufferLength, QueryParam_t *pxParams, BaseType_t xParamCount )
{
BaseType_t xCount = 0;
QueryParam_t *pxParam;
NetworkEndPoint_t *pxEndPoint;
uint32_t ulIPAddress, ulNetMask, ulGatewayAddress, ulDNSServerAddress;
int8_t cBuffer[ 16 ];

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
	}

	xCount = snprintf( pcBuffer, uxBufferLength,
			"{"
				"\"uptime\":"    "%d,"
				"\"free_heap\":" "%d,"
				"\"led\":"       "%s,"
				"\"button\":"    "%s,"
				"\"build\":"     "\"%s\","
				"\"hostname\":"  "\"%s\",",
			( portGET_RUN_TIME_COUNTER_VALUE() / 10000UL ),
			xPortGetFreeHeapSize(),
			( Board_LED_Test( LEDS_LED0 ) ? "true" : "false" ),
			( ( Buttons_GetStatus() != 0 ) ? "true" : "false" ),
			BUILD_STRING,
			pcApplicationHostnameHook()
	);

	pxEndPoint = FreeRTOS_FirstEndPoint( NULL );
	if( pxEndPoint != NULL )
	{
		xCount += snprintf( pcBuffer + xCount, uxBufferLength - xCount,
				"\"mac\":\"%02x:%02x:%02x:%02x:%02x:%02x\",",
				pxEndPoint->xMACAddress.ucBytes[0],
				pxEndPoint->xMACAddress.ucBytes[1],
				pxEndPoint->xMACAddress.ucBytes[2],
				pxEndPoint->xMACAddress.ucBytes[3],
				pxEndPoint->xMACAddress.ucBytes[4],
				pxEndPoint->xMACAddress.ucBytes[5]
		);

		FreeRTOS_GetAddressConfiguration( pxEndPoint,
										  &ulIPAddress,
										  &ulNetMask,
										  &ulGatewayAddress,
										  &ulDNSServerAddress );

		FreeRTOS_inet_ntoa( ulIPAddress, cBuffer );
		xCount += snprintf( pcBuffer + xCount, uxBufferLength - xCount, "\"ip\":\"%s\",", cBuffer);
		FreeRTOS_inet_ntoa( ulNetMask, cBuffer );
		xCount += snprintf( pcBuffer + xCount, uxBufferLength - xCount, "\"netmask\":\"%s\",", cBuffer);
		FreeRTOS_inet_ntoa( ulGatewayAddress, cBuffer );
		xCount += snprintf( pcBuffer + xCount, uxBufferLength - xCount, "\"gateway\":\"%s\",", cBuffer);
		FreeRTOS_inet_ntoa( ulDNSServerAddress, cBuffer );
		xCount += snprintf( pcBuffer + xCount, uxBufferLength - xCount, "\"dns\":\"%s\"", cBuffer);
	}

	xCount += snprintf( pcBuffer + xCount, uxBufferLength - xCount, "}" );


	return xCount;
}
