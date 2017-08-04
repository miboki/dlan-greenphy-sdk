#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#define HTTP_MAX_REQUEST_HANDLERS    5
#define HTTP_MAX_QUERY_PARAMS       3

typedef BaseType_t ( * FHTTPRequestHandler ) ( char *pcBuffer, size_t uxBufferLength, QueryParam_t *pxParams, BaseType_t xParamCount );

typedef struct xHTTP_REQUEST_HANDLER
{
	const char *pcName;
	FHTTPRequestHandler fRequestHandler;
} HTTPRequestHandler_t;


BaseType_t xAddRequestHandler( char *pcName, FHTTPRequestHandler fRequestHandler );
BaseType_t xRemoveRequestHandler( char *pcName );

#endif /* HTTP_REQUEST_H */
