/*
 * thermo3click.c
 *
 *  Created on: 30.12.2016
 *      Author: mikroelektronika / devolo AG
 *
 *  Last Modified on: 07.08.2017
 *  			  By: cdomnik @ devolo AG
 */

#include "chip.h"

#include "ClickboardConfig.h"
#include "board.h"

#include <string.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "clickboard_config.h"
#include "http_query_parser.h"
#include "http_request.h"
#include "thermo3click.h"



float Get_Temperature(void)
{
#define TEMP_OFFSET  -2.0
	uint8_t tmp_data[2];
	int TemperatureSum;
	float Temperature;

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

	  Temperature = (float)TemperatureSum * 0.0625;              // Multiply temperature value with 0.0625 (value per bit)

	  return (Temperature + TEMP_OFFSET);                                        // Return temperature data
}


//Global Variable of the Task Handle
static TaskHandle_t xClickTaskHandleThermo = NULL;
float temp;
//high have to be lower than temp could be - low have to be higher than temp could be - mean is initialized with a realistic value
int temp_cur, temp_high = -10000, temp_low = 10000;

/********************************************************************
 * Task and Request Handler of the Thermo3Click Board
 * added 03-aug-2017 by cdomnik
 *******************************************************************/
void vThermo3Click_Task(void *pvParameters)
{
	//Calculate Delay for 1 min = 60 * 1000ms
 	const TickType_t xDelay = 60000 / portTICK_PERIOD_MS;
	// Task run in infinite loops
	while( 1 )
	{
		temp = Get_Temperature(); //Pull Temperature from Clickboard with function above
		temp_cur = ( int ) ( 100 * temp );
		if( temp_cur < temp_low ) { temp_low = temp_cur; } // If temp is lower than low, temp is new low Value
		if( temp_cur > temp_high ) { temp_high = temp_cur; } // If temp is higher than high, temp is new high value

		DEBUGOUT("Thermo3Click - Temperature: %4.2f,Current: %d, High: %d, Low: %d", temp, temp_cur, temp_high, temp_low );
		//PrintStatus();
		DEBUGOUT("\r\n");

		vTaskDelay( xDelay );
	}
}

#if( includeHTTP_DEMO != 0 )
	BaseType_t xThermo3Click_RequestHandler( char *pcBuffer, size_t uxBufferLength, QueryParam_t *pxParams, BaseType_t xParamCount )
	{
		BaseType_t xCount = 0;

		xCount += sprintf( pcBuffer, "{\"temp_cur\":%d,\"temp_high\":%d,\"temp_low\":%d}", temp_cur, temp_high, temp_low );
		return xCount;
	}
#endif
/*******************************************************************/




/********************************************************************
 *  Initialtisiation and deinitialisation of the Thermo3Click Task
 *  added 02-oct-2017 by cdomnik
 *******************************************************************/
BaseType_t xThermo3Click_Init ( const char *pcName, BaseType_t xPort )
{
	BaseType_t xReturn = pdFALSE;

	if( xClickTaskHandleThermo == NULL )
	{
		//Configure GPIO Ports knowing on witch Click Port the Module is
		if( xPort == eClickboardPort1 )
		{
			Chip_GPIO_SetPinDIRInput(LPC_GPIO, CLICKBOARD1_INT_GPIO_PORT_NUM, CLICKBOARD1_INT_GPIO_BIT_NUM );
		}
		else if( xPort == eClickboardPort2 )
		{
			Chip_GPIO_SetPinDIRInput(LPC_GPIO, CLICKBOARD2_INT_GPIO_PORT_NUM, CLICKBOARD2_INT_GPIO_BIT_NUM );
		}
		//Initialise i2c
		Board_I2C_Init( I2C1 );
		// TODO: Initialize termo board from color2click.c actually don't know if necessary

		//Create a Task for Thermo3Click Board
		if( xTaskCreate( vThermo3Click_Task, /* Termo3Click Task is implemented above */
						 pcName,			 /*  */
						 300,
						 NULL,
						 ( tskIDLE_PRIORITY + 1 ),
						 xClickTaskHandleThermo )
			!= pdPASS )
		{
			DEBUGOUT("Fatal Error -> Unable to create Thermo3Click Task\r\n");
			xReturn = pdFAIL;
		}
		else
		{
			#if( includeHTTP_DEMO != 0 )
			{
				//the graphical output is used for debugging purposes
				xAddRequestHandler( pcName, xThermo3Click_RequestHandler );
			}
			#endif

			xReturn = pdTRUE;
		}
	}
	return xReturn;
}

BaseType_t xThermo3Click_Deinit ( void )
{
	BaseType_t xReturn = pdFALSE;

	//If Task exists, kill it
	if( xClickTaskHandleThermo != NULL )
	{
		#if( includeHTTP_DEMO != 0 )
		{
			//Also kill the RequestHandler for the graphical output
			xRemoveRequestHandler( pcTaskGetName( xClickTaskHandleThermo ) );
		}
		#endif
		vTaskDelete( xClickTaskHandleThermo );

		/* TODO: Reset I2C and GPIOs. */
		xReturn = pdTRUE;
	}

	return xReturn;
}
/*******************************************************************/
