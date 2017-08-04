#ifndef HTTP_QUERY_PARSER_H
#define HTTP_QUERY_PARSER_H

/* A structure to hold the query string parameter values. */
typedef struct query_param {
	char *pcKey;
	char *pcValue;
} QueryParam_t;

BaseType_t xParseQuery(char *pcQuery, QueryParam_t *pxParams, BaseType_t xMaxParams);
QueryParam_t *pxFindKeyInQueryParams( const char *pcKey, QueryParam_t *pxParams, BaseType_t xParamCount );

#endif /* HTTP_QUERY_PARSER_H */
