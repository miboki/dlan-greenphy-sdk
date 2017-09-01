/***********************************************
 *            CONFIG VARIABLES
 *
 * Add new variables like below with:
 *     X( ID, NAME ) \
 *
 * Keep IDs backwards compatible!
 *
 * IDs 0 - 127 are reserved for use by devolo.
 * IDs 128-255 are free for customer's use.
 ***********************************************/
#define LIST_OF_CONFIG_TAGS           \
	X(   0, eConfigNetworkIp )        \
	X(   1, eConfigNetworkNetmask )   \
	X(   2, eConfigNetworkGateway )   \
	X(   3, eConfigNetworkHostname )  \
	X(  10, eConfigClickConfPort1 )   \
	X(  11, eConfigClickConfPort2 )   \
	/* ADD YOUR OWN TAGS BELOW */     \
	// X( 128, eConfigCustom )

/* Storage location of the config in flash.
Config must not exceed boundaries of a 32kb sector. */
#define CONFIG_FLASH_AREA_START 	 ( 0x78000 ) /* Sector 29 */
#define CONFIG_FLASH_AREA_SIZE       ( 0x08000 ) /* 32kb */
#define CONFIG_FLASH_AREA_END        ( CONFIG_FLASH_AREA_START + CONFIG_FLASH_AREA_SIZE )

#define CONFIG_SIGNATURE             ( 0xAAAA5555 )
#define LPC_FLASH_SIZE_512KB         ( 0x00080000 )

/* X-Macro definition of config tags.
 * Use LIST_OF_CONFIG_TAGS macro above to add new tags. */
typedef enum eCONFIG_TAGS
{
#define X( id, name ) name = id,
	LIST_OF_CONFIG_TAGS
#undef X
} eConfigTag_t;

/* Read and verify latest config and store pointers to the flash location of
 * the found config variables. */
BaseType_t xReadConfig( void );

/* (Re-)writes config to flash. Afterwards config is reread from flash, all cached
 * config variables are freed and set to the new flash location. */
BaseType_t xWriteConfig( void );

/* Erases the whole config sector. */
void vEraseConfig( void );

/* Returns pointer to a config variable matching xTag with returned length pusLength. */
void *pvGetConfig( eConfigTag_t xTag, uint16_t *pusLength );

/* Stores pvValue of length usLength in config cache. Use xWriteConfig() to save permanently.
 * Previously cached config variables are freed. Returns pointer to config variable in cache. */
void *pvSetConfig( eConfigTag_t xTag, uint16_t usLength, const void * const pvValue );
