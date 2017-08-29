/* Standard includes. */
#include <string.h>

/* FreeRTOS includes. */
#include <FreeRTOS.h>
#include "task.h"

/* FreeRTOS +TCP includes. */
#include "FreeRTOS_IP.h"

#include "board.h"

#include "http_query_parser.h"

BaseType_t xRequestHandler_Status( char *pcBuffer, size_t uxBufferLength, QueryParam_t *pxParams, BaseType_t xParamCount )
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
	}

	xCount = snprintf( pcBuffer, uxBufferLength,
			"{"
				"\"uptime\":"    "%d,"
				"\"free_heap\":" "%d,"
				"\"led\":"       "%s,"
				"\"button\":"    "%s,"
				"\"hostname\":"  "\"%s\""
			"}",
			( portGET_RUN_TIME_COUNTER_VALUE() / 10000UL ),
			xPortGetFreeHeapSize(),
			( Board_LED_Test( LEDS_LED0 ) ? "true" : "false" ),
			( ( Buttons_GetStatus() != 0 ) ? "true" : "false" ),
			pcApplicationHostnameHook()
	);

	return xCount;
}
