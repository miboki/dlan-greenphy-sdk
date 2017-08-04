/* Standard includes. */
#include <string.h>

/* FreeRTOS includes. */
#include <FreeRTOS.h>
#include "task.h"

/* FreeRTOS+TCP includes. */
#include <FreeRTOS_IP.h>

#include "http_query_parser.h"

BaseType_t xParseQuery( char *pcQuery, QueryParam_t *pxParams, BaseType_t xMaxParams )
{
BaseType_t x = 0;

	if( ( pcQuery != NULL ) && ( *pcQuery != '\0' ) )
	{
		pxParams[x++].pcKey = pcQuery;             /* First key is at begin of query. */
		while ( ( x < xMaxParams ) && ( ( pcQuery = strchr( pcQuery, ipconfigHTTP_REQUEST_DELIMITER ) ) != NULL ) )
		{
			*pcQuery = '\0';                     /* Replace delimiter with '\0'. */
			pxParams[x].pcKey = ++pcQuery;         /* Set next parameter key. */

			/* Look for previous parameter value. */
			if( ( pxParams[x - 1].pcValue = strchr( pxParams[x - 1].pcKey, '=' ) ) != NULL) {
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

QueryParam_t *pxFindKeyInQueryParams( const char *pcKey, QueryParam_t *pxParams, BaseType_t xParamCount )
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
