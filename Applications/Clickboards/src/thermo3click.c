/*
 * thermo3click.c
 *
 *  Created on: 30.12.2016
 *      Author: mikroelektronika / devolo AG
 *
 *  Last Modified on: 07.08.2017
 *  			  By: cdomnik @ devolo AG
 */

/* Standard includes. */
#include <string.h>
#include <stdlib.h>

/* LPCOpen Includes. */
#include "board.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* GreenPHY SDK includes. */
#include "GreenPhySDKConfig.h"
#include "http_query_parser.h"
#include "http_request.h"
#include "clickboard_config.h"
#include "thermo3click.h"

/* Task-Delay in ms, change to your preference */
#define TASKWAIT_THERMO3 10000 /* 10s */

/* Temperature offset used to calibrate the sensor. */
#define TEMP_OFFSET  -2.0

/*****************************************************************************/

int Get_Temperature(void)
{
	uint8_t tmp_data[2];
	int TemperatureSum;
	int Temperature;

	tmp_data[0] = 0;

	I2C_XFER_T xfer;
	xfer.slaveAddr = TMP102_I2C_ADDR;
	xfer.txBuff = tmp_data;
	xfer.rxBuff = tmp_data;
	xfer.txSz = 1;
	xfer.rxSz = 2;

	/*transfer data to T-chip via I2C and read new temperature data, check if error has occurred*/
	  if (Chip_I2C_MasterTransfer(I2C1, &xfer) != I2C_STATUS_DONE) return -450.0;

	  TemperatureSum = ((tmp_data[0] << 8) | tmp_data[1]) >> 4;  // Justify temperature values
	  if(TemperatureSum & (1 << 11))                             // Test negative bit
	    TemperatureSum |= 0xF800;                                // Set bits 11 to 15 to logic 1 to get this reading into real two complement

	  // Temperature = (float)TemperatureSum * 0.0625;              // Multiply temperature value with 0.0625 (value per bit)
	  Temperature = TemperatureSum * 625 / 100 + (TEMP_OFFSET * 100);    /* _ML_: Use an integer in hundredth of a degree instead of float. Also introduce an offset. */

	  return Temperature;                                        // Return temperature data
}

/*****************************************************************************/

/* Task handle used to identify the clickboard's task and check if the
clickboard is activated. */
static TaskHandle_t xClickTaskHandle = NULL;

/* Holds current temperature in hundredth of a degree. */
static int temp_cur;

/* Holds lowest measured temperature in hundredth of a degree.
Must be initialized to a high value.*/
static int temp_low = 0x7fffffff;

/* Holds highest measured temperature in hundredth of a degree.
Must be initialized to a low value. */
static int temp_high = (~0x7fffffff);

/*-----------------------------------------------------------*/

static void vClickTask(void *pvParameters)
{
const TickType_t xDelay = TASKWAIT_THERMO3 / portTICK_PERIOD_MS;

	while( 1 )
	{
		/* Read temperature in hundredth of a degree. */
		temp_cur = Get_Temperature();

		/* Check for lowest and highest temperatures. */
		if( temp_cur < temp_low )  temp_low = temp_cur;
		if( temp_cur > temp_high ) temp_high = temp_cur;

		DEBUGOUT("Thermo3Click - Temperature Current: %d, High: %d, Low: %d\r\n", temp_cur, temp_high, temp_low );

		vTaskDelay( xDelay );
	}
}
/*-----------------------------------------------------------*/

#if( includeHTTP_DEMO != 0 )
	static BaseType_t xClickHTTPRequestHandler( char *pcBuffer, size_t uxBufferLength, QueryParam_t *pxParams, BaseType_t xParamCount )
	{
	BaseType_t xCount = 0;

		xCount += sprintf( pcBuffer, "{\"temp_cur\":%d,\"temp_high\":%d,\"temp_low\":%d}", temp_cur, temp_high, temp_low );
		return xCount;
	}
#endif
/*-----------------------------------------------------------*/

BaseType_t xThermo3Click_Init ( const char *pcName, BaseType_t xPort )
{
BaseType_t xReturn = pdFALSE;

	/* Use the task handle to guard against multiple initialization. */
	if( xClickTaskHandle == NULL )
	{
		/* Configure GPIOs depending on the microbus port. */
		if( xPort == eClickboardPort1 )
		{
			/* Set interrupt pin. */
			Chip_GPIO_SetPinDIRInput(LPC_GPIO, CLICKBOARD1_INT_GPIO_PORT_NUM, CLICKBOARD1_INT_GPIO_BIT_NUM );
		}
		else if( xPort == eClickboardPort2 )
		{
			/* Set interrupt pin. */
			Chip_GPIO_SetPinDIRInput(LPC_GPIO, CLICKBOARD2_INT_GPIO_PORT_NUM, CLICKBOARD2_INT_GPIO_BIT_NUM );
		}

		/* Initialize I2C. Both microbus ports are connected to the same I2C bus. */
		Board_I2C_Init( I2C1 );

		/* Create task. */
		xTaskCreate( vClickTask, pcName, 300, NULL, ( tskIDLE_PRIORITY + 1 ), &xClickTaskHandle );
		if( xClickTaskHandle != NULL )
		{
			#if( includeHTTP_DEMO != 0 )
			{
				/* Add HTTP request handler. */
				xAddRequestHandler( pcName, xClickHTTPRequestHandler );
			}
			#endif

			xReturn = pdTRUE;
		}
	}
	return xReturn;
}
/*-----------------------------------------------------------*/

BaseType_t xThermo3Click_Deinit ( void )
{
BaseType_t xReturn = pdFALSE;

	if( xClickTaskHandle != NULL )
	{
		#if( includeHTTP_DEMO != 0 )
		{
			/* Use the task's name to remove the HTTP Request Handler. */
			xRemoveRequestHandler( pcTaskGetName( xClickTaskHandle ) );
		}
		#endif

		/* Delete the task. */
		vTaskDelete( xClickTaskHandle );
		/* Set the task handle to NULL, so the clickboard can be reactivated. */
		xClickTaskHandle = NULL;

		/* TODO: Reset I2C and GPIOs. */
		xReturn = pdTRUE;
	}

	return xReturn;
}
/*-----------------------------------------------------------*/
