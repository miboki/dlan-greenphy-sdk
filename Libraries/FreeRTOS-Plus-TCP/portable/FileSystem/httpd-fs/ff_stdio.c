
/* Standard includes. */
#include <stdio.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"

/* httpd-fs includes. */
#include "ff_stdio.h"
#include "httpd-fs.h"

FF_FILE *httpdfs_fopen( const char *pcFile, const char *pcMode)
{
	struct httpd_fs_file pxFileHandle;
	FF_FILE *pxFile = NULL;

	/* pcMode not used */
	(void) pcMode;

	if( httpd_fs_open( pcFile, &pxFileHandle ) )
	{
		pxFile = pvPortMalloc( sizeof( FF_FILE) );
		if( pxFile != NULL )
		{
			pxFile->pucBuffer = pxFileHandle.data;
			pxFile->ulFileSize = pxFileHandle.len;
			pxFile->ulFilePointer = 0;
		}
	}

	return pxFile;
}

int httpdfs_fclose(FF_FILE *pxStream)
{
	vPortFree(pxStream);
	return 0;
}

size_t httpdfs_fread(void *pvBuffer, size_t xSize, size_t xItems, FF_FILE *pxStream)
{
	size_t count = 0;
	if( pxStream->ulFilePointer < pxStream->ulFileSize )
	{
		count = xSize * xItems;
	}
	if( count > (pxStream->ulFileSize - pxStream->ulFilePointer) )
	{
		count = pxStream->ulFileSize - pxStream->ulFilePointer;
	}

	memcpy(pvBuffer, pxStream->pucBuffer + pxStream->ulFilePointer, count);
	pxStream->ulFilePointer += count;
	return count;
}
