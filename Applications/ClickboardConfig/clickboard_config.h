#ifndef CLICKBOARD_CONFIG_H
#define CLICKBOARD_CONFIG_H

#define NUM_CLICKBOARD_PORTS   2

/*
 * Clickboard identifier. Numerical value is PID from MicroElektronika.
 */
typedef enum eCLICKBOARD_IDS {
	eClickboardIdExpand2 = 1838,
	eClickboardIdThermo3 = 1885,
	eClickboardIdColor2  = 1988
} eClickboardId_t;

/*
 * Clickboard initializer function. Called on activation.
 * Returns pdTRUE on success, otherwise pdFALSE.
 */
typedef BaseType_t ( * FClickboardInit ) ( const char *pcName, BaseType_t xPort );

/*
 * Clickboard deinitializer function. Called on deactivation.
 * Returns pdTRUE on success, otherwise pdFALSE.
 */
typedef BaseType_t ( * FClickboardDeinit ) ( void );

/*
 * A struct to hold information about a clickboard.
 */
typedef struct xCLICKBOARD
{
	eClickboardId_t xClickboardId; /* MikroE PID to identify the clickboard. */
	const char *pcName;         /* Lowercase clickboard name, without 'Click'
								suffix, e.g. 'Color2Click' -> 'color2' */
	FClickboardInit fClickboardInit;
	FClickboardDeinit fClickboardDeinit;
	uint8_t xPortsAvailable;    /* Ports on which a clickboard can be activated. */
	uint8_t xPortsActive;       /* Ports on which a clickboard is activated. */
} Clickboard_t;

/*
 * Initializer for the clickboard config interface. It activates any
 * preconfigured clickboard and adds a request handler to the HTTP server.
 */
void xClickboardsInit();

/*
 * Returns a clickboard with given name if found, otherwise NULL.
 */
Clickboard_t *pxFindClickboard( char *pcName );

/*
 * Returns a clickboard on a given port if found, otherwise NULL.
 */
Clickboard_t *pxFindClickboardOnPort( BaseType_t xPort );

/*
 * Activate a clickboard on a given port. Any active clickboard on the port is
 * deactivated first. To get a clickboard use pxFindClickboard first.
 * Return pdTRUE on success, otherwise pdFALSE.
 */
BaseType_t xClickboardActivate( Clickboard_t *pxClickboard, BaseType_t xPort );

/*
 * Deactivate a clickboard. To get a clickboard use pxFindClickboard first.
 * Return pdTRUE on success, otherwise pdFALSE.
 */
BaseType_t xClickboardDeactivate( Clickboard_t *pxClickboard );

#endif /* CLICKBOARD_CONFIG_H */
