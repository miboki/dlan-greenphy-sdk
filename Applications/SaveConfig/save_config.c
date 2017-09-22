/*
 * Copyright (c) 2017, devolo AG, Aachen, Germany.
 * All rights reserved.
 *
 * This Software is part of the devolo GreenPHY-SDK.
 *
 * Usage in source form and redistribution in binary form, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Usage in source form is subject to a current end user license agreement
 *    with the devolo AG.
 * 2. Neither the name of the devolo AG nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 3. Redistribution in binary form is limited to the usage on the GreenPHY
 *    module of the devolo AG.
 * 4. Redistribution in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* Standard includes. */
#include <stdio.h>
#include <string.h>

/* LPCOpen Includes. */
#include "board.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"

/* GreenPHY SDK includes. */
#include "GreenPhySDKConfig.h"

/* Project includes. */
#include "save_config.h"

#define member_size(type, member) sizeof(((type *)0)->member)

/*********************************************************************//**
 * @brief		Get Sector Number
 *
 * @param[in] adr	           Sector Address
 *
 * @return 	Sector Number.
 *
 **********************************************************************/
 uint32_t GetSecNum (uint32_t adr)
{
    uint32_t n;

    n = adr >> 12;                               //  4kB Sector
    if (n >= 0x10) {
      n = 0x0E + (n >> 3);                       // 32kB Sector
    }

    return (n);                                  // Sector Number
}
 /*-----------------------------------------------------------*/

typedef enum eCONFIG_VERSIONS
{
	eConfigVersion1 = 1
} eConfigVersion_t;

typedef struct __attribute__((__packed__)) xCONFIG_TLV
{
	uint8_t ucTag;        /* eConfigTag_t. */
	uint8_t reserved;     /* Reserved for alignment. */
	uint16_t usLength;    /* Length of pucValue. */
	uint8_t pucValue[1];  /* Data. Actual space needs to be allocated. */
} ConfigTLV_t;

typedef struct __attribute__((__packed__)) xCONFIG
{
	uint8_t ucVersion;			/* Config version. */
	uint8_t ucCount;            /* Count config rewrites. */
	uint16_t usLength;			/* Total length in bytes of TLV list. */
	uint32_t ulSignature;       /* Signature 0xAA55AA55 to validate config. */
	uint8_t pucTLVList[1];       /* List of TLV elements. */
} Config_t;

typedef enum eCONFIG_TAG_INDEXES {
	eConfigTagNotFound = -1,
#define X( id, name ) name##Index,
	LIST_OF_CONFIG_TAGS
#undef X
	eConfigNumberOfTags
} eConfigTagIndex_t;

/*-----------------------------------------------------------*/

static const ConfigTLV_t *ppxConfigCacheTLVList[ eConfigNumberOfTags ];
static const Config_t *pxConfig = NULL;

/*-----------------------------------------------------------*/

/*
 * Calculates the total size of a ConfigTLV from its data length.
 */
static uint16_t prvSizeOfTLV( uint16_t usLength )
{
uint16_t usSize;

	/* Calculate size needed for TLV. pucValue already includes one data byte. */
	usSize = ( sizeof( ConfigTLV_t ) - member_size( ConfigTLV_t, pucValue ) + usLength );
	/* Add padding to TLV so all TLVs are aligned properly. */
	usSize += ( ( 4 - ( usSize % 4 ) ) % 4 );
	return usSize;
}
/*-----------------------------------------------------------*/

/*
 * Calculates the total size of the config from the data length of all TLVs.
 */
static uint16_t prvSizeOfConfig( uint16_t usLength )
{
uint16_t usSize;

	/* Calculate size needed for config. Config_t already includes one ConfigTLV_t. */
	usSize = ( sizeof( Config_t ) - member_size( Config_t, pucTLVList ) + usLength );
	/* Add padding to config so config meets 256 byte alignment needed for flash. */
	usSize += ( ( 256 - ( usSize % 256 ) ) % 256 );
	return usSize;
}
/*-----------------------------------------------------------*/

/*
 * Returns the corresponding cache index to a tag.
 */
static eConfigTagIndex_t prvFindTLVInCache( eConfigTag_t xTag )
{
eConfigTagIndex_t eReturn;

	switch( xTag )
	{
#define X( id, name ) case name: eReturn = name##Index; break;
	LIST_OF_CONFIG_TAGS
#undef X
	default: eReturn = eConfigTagNotFound; break;
	}

	return eReturn;
}
/*-----------------------------------------------------------*/

/*
 * Searches the most recent config in flash.
 */
static const Config_t *prvFindConfig( void )
{
const uint8_t *pucConfig;
const Config_t *pxCurrentConfig, *pxReturn = NULL;

	for( pucConfig = (uint8_t *) CONFIG_FLASH_AREA_START;
		 pucConfig < (uint8_t *) CONFIG_FLASH_AREA_END;
		 pucConfig += prvSizeOfConfig( pxCurrentConfig->usLength ) )
	{
		pxCurrentConfig = (Config_t *) pucConfig;
		if( pxCurrentConfig->ulSignature != CONFIG_SIGNATURE )
		{
			/* Not a valid config. */
			break;
		}

		if( ( pucConfig > (uint8_t *) CONFIG_FLASH_AREA_START ) && ( pxCurrentConfig->ucCount != ( pxReturn->ucCount + 1 ) ) )
		{
			/* Last config is more recent. */
			break;
		}

		pxReturn = pxCurrentConfig;
	}

	return pxReturn;
}
/*-----------------------------------------------------------*/

static void prvCleanCache( void ) {
BaseType_t x;

	for( x = 0; x < eConfigNumberOfTags; x++ )
	{
		/* Check if there is a cached TLV in RAM and free it. */
		if( (void *) ppxConfigCacheTLVList[ x ] > (void *) LPC_FLASH_SIZE_512KB )
		{
			vPortFree( (void *) ppxConfigCacheTLVList[ x ] );
		}

		ppxConfigCacheTLVList[ x ] = NULL;
	}
}
/*-----------------------------------------------------------*/

BaseType_t xReadConfig( void )
{
const uint8_t *pucTLV;
const ConfigTLV_t *pxTLV;
eConfigTagIndex_t eCacheIndex;
BaseType_t xReturn = pdFAIL;

	/* Clean cache before (re-)reading config. */
	prvCleanCache();

	if( pxConfig == NULL )
	{
		pxConfig = prvFindConfig();
	}

	if( ( pxConfig != NULL ) && ( pxConfig->ulSignature == CONFIG_SIGNATURE ) )
	{
		if( pxConfig->ucVersion == eConfigVersion1 )
		{
			/* Iterate through all TLVs in config. */
			for( pucTLV = pxConfig->pucTLVList;
				 pucTLV < pxConfig->pucTLVList + pxConfig->usLength;
				 pucTLV += prvSizeOfTLV( pxTLV->usLength ) )
			{
				pxTLV = (ConfigTLV_t *) pucTLV;

				eCacheIndex = prvFindTLVInCache( pxTLV->ucTag );
				if( eCacheIndex != eConfigTagNotFound )
				{
					/* Store a pointer to the flash TLV in cache. */
					ppxConfigCacheTLVList[ eCacheIndex ] = pxTLV;
				}
				else
				{
					DEBUGOUT( "ReadConfig: unknown tag %d", pxTLV->ucTag );
				}
			}

			/* Verify that end of config matches with end of last TLV. */
			if( pxConfig->usLength == ( pucTLV - pxConfig->pucTLVList ) )
			{
				xReturn = pdPASS;
			}
			else
			{
				prvCleanCache();
				pxConfig = NULL;
			}
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

BaseType_t xWriteConfig( void )
{
const uint32_t usBufferSize = 256; /* 256, 512, 1024, 2048, 4096 */
const uint32_t ulSector = GetSecNum( CONFIG_FLASH_AREA_START );
uint32_t ulDestination;
uint8_t *pucBuffer;
uint16_t usBufferPos, usBytesAvailable, usBytesCopied, usBytesToCopy;
uint16_t usLength = 0;
uint8_t ucCount;
BaseType_t x;

	/* Iterate once over all TLVs to determine total size of config. */
	for( x = 0; x < eConfigNumberOfTags; x++ )
	{
		if( ( ppxConfigCacheTLVList[ x ] != NULL ) && ( ppxConfigCacheTLVList[ x ]->usLength > 0 ) )
		{
			usLength += prvSizeOfTLV( ppxConfigCacheTLVList[ x ]->usLength );
		}
	}

	/* Determine destination in flash and new config counter. */
	if( pxConfig == NULL )
	{
		/* No valid config found, write a new one at the beginning of flash. */
		ulDestination = (uint32_t) CONFIG_FLASH_AREA_START;
		ucCount = 0;
		/* Erase sector in case config was malformed. */
		vEraseConfig();
	}
	else if( ( (uint8_t *) pxConfig + prvSizeOfConfig( pxConfig->usLength ) + prvSizeOfConfig( usLength ) ) > (uint8_t *) CONFIG_FLASH_AREA_END )
	{
		/* Config does not fit into trailing space of flash, write to beginning. */
		ulDestination = (uint32_t) CONFIG_FLASH_AREA_START;
		ucCount = pxConfig->ucCount + 1;
	}
	else
	{
		/* Write config behind last one. */
		ulDestination = (uint32_t) ( (uint8_t *) pxConfig + prvSizeOfConfig( pxConfig->usLength ) );
		ucCount = pxConfig->ucCount + 1;
	}

	pucBuffer = pvPortMalloc( usBufferSize );
	( (Config_t *) pucBuffer )->ucVersion = eConfigVersion1;
	( (Config_t *) pucBuffer )->ucCount = ucCount;
	( (Config_t *) pucBuffer )->usLength = usLength;
	( (Config_t *) pucBuffer )->ulSignature = CONFIG_SIGNATURE;
	usBufferPos = offsetof( Config_t, pucTLVList );

	/* Iterate again to write config into transfer buffer. */
	for( x = 0; x < eConfigNumberOfTags; x++ )
	{
		if( ( ppxConfigCacheTLVList[ x ] != NULL ) && ( ppxConfigCacheTLVList[ x ]->usLength > 0 ) )
		{
			usBytesCopied = 0;
			usBytesAvailable = prvSizeOfTLV( ppxConfigCacheTLVList[ x ]->usLength );

			while( usBytesCopied < usBytesAvailable )
			{
				usBytesToCopy = usBytesAvailable - usBytesCopied;
				if( usBytesToCopy > ( usBufferSize - usBufferPos ) )
				{
					usBytesToCopy = ( usBufferSize - usBufferPos );
				}

				/* Copy TLV to write buffer. */
				memcpy( ( pucBuffer + usBufferPos ),
						( ppxConfigCacheTLVList[ x ] + usBytesCopied ),
						usBytesToCopy );

				usBufferPos += usBytesToCopy;

				if( usBufferPos == usBufferSize )
				{
					/* Buffer is full. Write it to flash. */
					Chip_IAP_PreSectorForReadWrite( ulSector, ulSector );
					Chip_IAP_CopyRamToFlash( ulDestination, (uint32_t *) pucBuffer, usBufferSize );

					usBufferPos = 0;
				}

				usBytesCopied += usBytesToCopy;
			}
		}
	}

	/* Zero fill end of buffer. */
	memset( pucBuffer + usBufferPos, 0, usBufferSize - usBufferPos );

	/* Write buffer to flash */
	Chip_IAP_PreSectorForReadWrite( ulSector, ulSector );
	Chip_IAP_CopyRamToFlash( ulDestination, (uint32_t *) pucBuffer, usBufferSize );

	vPortFree( pucBuffer );

	/* Reread config from flash. */
	return xReadConfig();
}
/*-----------------------------------------------------------*/

void vEraseConfig( void )
{
const uint32_t ulSector = GetSecNum( CONFIG_FLASH_AREA_START );

	/* Reset config. */
	prvCleanCache();
	pxConfig = NULL;

	/* Erase config from flash. */
	Chip_IAP_PreSectorForReadWrite( ulSector, ulSector );
	Chip_IAP_EraseSector( ulSector, ulSector );
}
/*-----------------------------------------------------------*/

void *pvGetConfig( eConfigTag_t xTag, uint16_t *pusLength )
{
eConfigTagIndex_t eCacheIndex;
void *pvReturn = NULL;

	eCacheIndex = prvFindTLVInCache( xTag );
	if( eCacheIndex != eConfigTagNotFound )
	{
		if( ppxConfigCacheTLVList[ eCacheIndex ] != NULL )
		{
			if( pusLength != NULL )
			{
				/* Return length, if wanted. */
				*pusLength = ppxConfigCacheTLVList[ eCacheIndex ]->usLength;
			}
			pvReturn = (void *) ppxConfigCacheTLVList[ eCacheIndex ]->pucValue;
		}
	}

	return pvReturn;
}
/*-----------------------------------------------------------*/

void *pvSetConfig( eConfigTag_t xTag, uint16_t usLength, const void * const pvValue )
{
eConfigTagIndex_t eCacheIndex;
ConfigTLV_t *pxTLV;
void *pvReturn = NULL;

	eCacheIndex = prvFindTLVInCache( xTag );

	/* Check if TLV is already in RAM and free it. */
	if( (void *) ppxConfigCacheTLVList[ eCacheIndex ] > (void *) 0x00080000 )
	{
		vPortFree( (void *) ppxConfigCacheTLVList[ eCacheIndex ] );
	}

	pxTLV = pvPortMalloc( prvSizeOfTLV( usLength ) );
	if( pxTLV != NULL )
	{
		pxTLV->ucTag = (uint8_t) xTag;
		pxTLV->usLength = usLength;
		memcpy( pxTLV->pucValue, pvValue, usLength );

		/* Store a pointer to the TLV in cache. */
		ppxConfigCacheTLVList[ eCacheIndex ] = pxTLV;
		pvReturn = (void *) pxTLV->pucValue;
	}

	return pvReturn;
}
/*-----------------------------------------------------------*/
