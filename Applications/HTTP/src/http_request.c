/* Standard includes. */
#include <string.h>
#include <stdio.h>

/* FreeRTOS includes. */
#include <FreeRTOS.h>
#include <task.h>

/* FreeRTOS+TCP includes. */
#include <FreeRTOS_IP.h>

#include "http_query_parser.h"
#include "http_request.h"

extern BaseType_t xRequestHandler_Status( char *pcBuffer, size_t uxBufferLength, QueryParam_t *pxParams, BaseType_t xParamCount );

static HTTPRequestHandler_t pxHTTPRequestHandlers[ HTTP_MAX_REQUEST_HANDLERS ] =
{
	{ "status", xRequestHandler_Status },
};

HTTPRequestHandler_t *prvFindRequestHandler( const char *pcName )
{
BaseType_t x;
HTTPRequestHandler_t *pxRequestHandler = NULL;

	if( pcName != NULL )
	{
		for( x = 0; x < HTTP_MAX_REQUEST_HANDLERS; x++ )
		{
			if( pxHTTPRequestHandlers[ x ].pcName != NULL )
			{
				if( strcmp( pcName, pxHTTPRequestHandlers[ x ].pcName ) == 0 )
				{
					pxRequestHandler = &pxHTTPRequestHandlers[ x ];
					break;
				}
			}
		}
	}

	return pxRequestHandler;
}

BaseType_t xAddRequestHandler( const char *pcName, FHTTPRequestHandler fRequestHandler )
{
BaseType_t x, xSuccess = pdFALSE;

	if( prvFindRequestHandler( pcName ) == NULL )
	{
		for( x = 0; x < HTTP_MAX_REQUEST_HANDLERS; x++ )
		{
			if( pxHTTPRequestHandlers[ x ].pcName == NULL )
			{
				pxHTTPRequestHandlers[ x ].pcName = pcName;
				pxHTTPRequestHandlers[ x ].fRequestHandler = fRequestHandler;

				xSuccess = pdTRUE;
				break;
			}
		}
	}

	return xSuccess;
}

BaseType_t xRemoveRequestHandler( char *pcName )
{
BaseType_t xSuccess = pdFALSE;
HTTPRequestHandler_t *pxRequestHandler;

	pxRequestHandler = prvFindRequestHandler( pcName );
	if( pxRequestHandler != NULL )
	{
		pxRequestHandler->pcName = NULL;
		pxRequestHandler->fRequestHandler = NULL;

		xSuccess = pdTRUE;
	}

	return xSuccess;
}

size_t uxApplicationHTTPHandleRequestHook( const char *pcURLData, char *pcBuffer, size_t uxBufferLength )
{
char *pcQuery, *pcExt;
QueryParam_t pxParams[ HTTP_MAX_QUERY_PARAMS ];
BaseType_t xParamCount, uxResult = 0;
HTTPRequestHandler_t *pxRequestHandler;

	/* Split request query from the filename. */
	pcQuery = strchr( pcURLData, ipconfigHTTP_REQUEST_CHARACTER );
	*pcQuery++ = '\0';

	/* Strip extension from the filename. */
	pcExt = strchr( pcURLData, '.' );
	*pcExt++ = '\0';

	/* Find Handler with the files basename without leading '/'. */
	pxRequestHandler = prvFindRequestHandler( pcURLData + 1 );
	if( ( pxRequestHandler != NULL ) && ( pxRequestHandler->fRequestHandler != NULL ) )
	{
		xParamCount = xParseQuery( pcQuery, pxParams, HTTP_MAX_QUERY_PARAMS );

		if( pxRequestHandler->fRequestHandler != NULL )
		{
			uxResult = pxRequestHandler->fRequestHandler( pcBuffer, uxBufferLength, pxParams, xParamCount );
		}
	}

	return ( size_t ) uxResult;
}
/*-----------------------------------------------------------*/
