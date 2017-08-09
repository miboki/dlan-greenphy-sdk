/*
 * Expand2Click.c
 *
 *  Created on: 29.12.2016
 *      Author: devolo AG / mikroelektronika
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
#include "Expand2Click.h"



//#include "resources.h"

#define EXPAND_ADDR (0x00)   /*jumper A0;A1;A2 on expand2click board*/

//sbit EXPAND_RST at GPIOC_ODR.B2;

unsigned char i = 0, old_res = 0, res;

//extern sfr sbit EXPAND_RST;




char Expander_Read_Byte(char ModuleAddress, char RegAddress){
  I2C_XFER_T xfer;
  char temp;
  xfer.slaveAddr = AddressCode|ModuleAddress;
  uint8_t tmp_data[2];

  tmp_data[0] = RegAddress;
  xfer.txBuff = tmp_data;
  xfer.rxBuff = tmp_data;
  xfer.txSz = 1;
  xfer.rxSz = 1;

  Chip_I2C_MasterSend(I2C1,xfer.slaveAddr,  xfer.txBuff, xfer.txSz);
  Chip_I2C_MasterRead(I2C1,xfer.slaveAddr,  xfer.rxBuff, xfer.rxSz);

  temp   = tmp_data[0];
  return temp;
}

void Expander_Write_Byte(char ModuleAddress,char RegAddress, char Data_) {
	  I2C_XFER_T xfer;
	  xfer.slaveAddr = AddressCode|ModuleAddress;
	  uint8_t tmp_data[2];
	  tmp_data[0] = RegAddress;
	  tmp_data[1] = Data_;
	  xfer.txBuff = tmp_data;
	  //xfer.rxBuff = tmp_data;
	  xfer.txSz = 2;
	  //xfer.rxSz = 2;
	  Chip_I2C_MasterSend(I2C1,xfer.slaveAddr, xfer.txBuff, xfer.txSz);
}


//Global Variable of the Task Handle
static TaskHandle_t xClickTaskHandleExpand = NULL;
char oBits = 0;
char iBits = 0;
int amountOfWater = 0;

char get_expand2click(void){
	return (Expander_Read_Byte(EXPAND_ADDR, GPIOA_BANK0));       // Read expander's PORTA
}

void set_expand2click(char data){
	Expander_Write_Byte(EXPAND_ADDR, OLATB_BANK0, data); // Write PORTD to expander's PORTB
}

void vExpand2Click_Task(void *pvParameters)
{
	char swap, lastBits;
	/*Task Work*/
	while (1){
		//Check if iBits changed since last Task call
		iBits = get_expand2click(); // read input Bits
		if ( lastBits != ( iBits & MASK_GET2Bit ))
		{
			lastBits = iBits & MASK_GET2Bit;
			// TODO: SEND MQTT message
			//for now, generate Debug Message and count
			DEBUGOUT("Saw Tick\n\r");
			amountOfWater += 2;
		}

		//oBits++;
		//toggle obits each time the Task is called
		if ( swap == 1)
		{
			oBits &= MASK_SWAPAND1;
			oBits |= MASK_SWAPOR2;
		}
		else
		{
			oBits &= MASK_SWAPAND2;
			oBits |= MASK_SWAPOR1;
		}
		set_expand2click(oBits);
		swap = 1 - swap;

		//DEBUGOUT("Expand2Click - Input: %x, Output: %x", iBits, oBits );
		//DEBUGOUT("\r\n");

		vTaskDelay(1000);
	}
}

#if( includeHTTP_DEMO != 0 )
	BaseType_t xExpand2Click_RequestHandler( char *pcBuffer, size_t uxBufferLength, QueryParam_t *pxParams, BaseType_t xParamCount )
	{
		BaseType_t xCount = 0;

		xCount += sprintf( pcBuffer, "{ \"bits\":%d, \"amount\":%d }", iBits, amountOfWater );
		return xCount;
	}
#endif
/*******************************************************************/

/********************************************************************
 *  Initialtisiation and deinitialisation of the Thermo3Click Task
 *  modified 02-aug-2017 by cdomnik
 *******************************************************************/
BaseType_t xExpand2Click_Init ( const char *pcName, BaseType_t xPort )
{
	BaseType_t xReturn = pdFALSE;

	if( xClickTaskHandleExpand == NULL )
	{
		//Configure GPIO Ports knowing on witch Click Port the Module is
		if( xPort == eClickboardPort1 )
		{
			Chip_GPIO_SetPinDIRInput(LPC_GPIO, CLICKBOARD1_INT_GPIO_PORT_NUM, CLICKBOARD1_INT_GPIO_BIT_NUM);
			/*define reset pin in slot 1*/
			Chip_GPIO_SetPinDIROutput(LPC_GPIO, CLICKBOARD1_RST_GPIO_PORT_NUM, CLICKBOARD1_RST_GPIO_BIT_NUM);
			/*start  reset procedure*/
			Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD1_RST_GPIO_PORT_NUM, CLICKBOARD1_RST_GPIO_BIT_NUM, true);
			//vTaskDelay(5); //Delay_ms(5);
			Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD1_RST_GPIO_PORT_NUM, CLICKBOARD1_RST_GPIO_BIT_NUM, false);
			//vTaskDelay(5); //Delay_ms(5);
			Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD1_RST_GPIO_PORT_NUM,	CLICKBOARD1_RST_GPIO_BIT_NUM, true);
			//vTaskDelay(1); //Delay_ms(1);
		}
		else if( xPort == eClickboardPort2 )
		{
			Chip_GPIO_SetPinDIRInput(LPC_GPIO, CLICKBOARD2_INT_GPIO_PORT_NUM, CLICKBOARD2_INT_GPIO_BIT_NUM);
			/*define reset pin in slot 1*/
			Chip_GPIO_SetPinDIROutput(LPC_GPIO, CLICKBOARD2_RST_GPIO_PORT_NUM, CLICKBOARD2_RST_GPIO_BIT_NUM);
			/*start  reset procedure*/
			Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD2_RST_GPIO_PORT_NUM, CLICKBOARD2_RST_GPIO_BIT_NUM, true);
			//vTaskDelay(5); //Delay_ms(5);
			Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD2_RST_GPIO_PORT_NUM,	CLICKBOARD2_RST_GPIO_BIT_NUM, false);
			//vTaskDelay(5); //Delay_ms(5);
			Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD2_RST_GPIO_PORT_NUM,	CLICKBOARD2_RST_GPIO_BIT_NUM, true);
			//vTaskDelay(1); //Delay_ms(1);
		}
		//Initialise i2c
		Board_I2C_Init( I2C1 );
		// Initialize Expander board
		Expander_Write_Byte(EXPAND_ADDR, IODIRB_BANK0, 0x00);       // Set Expander's PORTA to be output
		Expander_Write_Byte(EXPAND_ADDR, IODIRA_BANK0, 0xFF);       // Set Expander's PORTB to be input
	  	Expander_Write_Byte(EXPAND_ADDR, GPPUA_BANK0, 0xFF);        // Set pull-ups to all of the Expander's PORTB pins


		//Create a Task for Expander2Click Board
		if( xTaskCreate( vExpand2Click_Task, /* Expand2Click Task is implemented above */
						 pcName,			 /*  */
						 240,
						 NULL,
						 ( tskIDLE_PRIORITY + 1 ),
						 xClickTaskHandleExpand )
			!= pdPASS )
		{
			DEBUGOUT("Fatal Error -> Unable to create Expand2Click Task\r\n");
			xReturn = pdFAIL;
		}
		else
		{
			#if( includeHTTP_DEMO != 0 )
			{
				//the graphical output is used for debugging purposes
				xAddRequestHandler( pcName, xExpand2Click_RequestHandler );
			}
			#endif

			xReturn = pdTRUE;
		}
	}
	return xReturn;
}


BaseType_t xExpand2Click_Deinit ( void )
{
	BaseType_t xReturn = pdFALSE;

	//If Task exists, kill it
	if( xClickTaskHandleExpand != NULL )
	{
		#if( includeHTTP_DEMO != 0 )
		{
			//Also kill the RequestHandler for the graphical output
			xRemoveRequestHandler( pcTaskGetName( xClickTaskHandleExpand ) );
		}
		#endif
		vTaskDelete( xClickTaskHandleExpand );

		/* TODO: Reset I2C and GPIOs. */
		xReturn = pdTRUE;
	}

	return xReturn;
}
/*******************************************************************/

