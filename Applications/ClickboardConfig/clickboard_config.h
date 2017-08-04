#ifndef CLICKBOARD_CONFIG_H
#define CLICKBOARD_CONFIG_H

#define NUM_CLICKBOARD_PORTS   2

typedef BaseType_t ( * FClickboardInit ) ( const char *pcName, BaseType_t xPort );
typedef BaseType_t ( * FClickboardDeinit ) ( void );

typedef struct xCLICKBOARD
{
	const char *pcName;
	FClickboardInit fClickboardInit;
	FClickboardDeinit fClickboardDeinit;
	uint8_t xPortsAvailable;
	uint8_t xPortsActive;
} Clickboard_t;


void xClickboardsInit();
Clickboard_t *pxFindClickboard( char *pcName );
Clickboard_t *pxFindClickboardOnPort( BaseType_t xPort );
BaseType_t xClickboardActivate( Clickboard_t *pxClickboard, BaseType_t xPort );
BaseType_t xClickboardDeactivate( Clickboard_t *pxClickboard );

#endif /* CLICKBOARD_CONFIG_H */
