/*
 * Expand2Click.c
 *
 *  Created on: 29.12.2016
 *      Author: devolo AG / mikroelektronika
 */


#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include <string.h>
#include <stdlib.h>
#include "clickboardIO.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "debug.h"
#include "Expand2Click.h"
#include "clickboardIO.h"



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





char get_expand2click(void){
	return (Expander_Read_Byte(EXPAND_ADDR, GPIOA_BANK0));       // Read expander's PORTA
}

void set_expand2click(char data){
	Expander_Write_Byte(EXPAND_ADDR, OLATB_BANK0, data); // Write PORTD to expander's PORTB
}

void Expand2Click_Task(void *pvParameters){
/*Init*/

	if ((int)pvParameters == SLOT1)  	{
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD1_INT_GPIO_PORT_NUM, CLICKBOARD1_INT_GPIO_BIT_NUM, GPIO_INPUT);
		/*define reset pin in slot 1*/
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD1_RST_GPIO_PORT_NUM, CLICKBOARD1_RST_GPIO_BIT_NUM, GPIO_OUTPUT);
		/*start  reset procedure*/
		Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD1_RST_GPIO_PORT_NUM, CLICKBOARD1_RST_GPIO_BIT_NUM, true);
		vTaskDelay(5); //Delay_ms(5);
		Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD1_RST_GPIO_PORT_NUM, CLICKBOARD1_RST_GPIO_BIT_NUM, false);
		vTaskDelay(5); //Delay_ms(5);
		Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD1_RST_GPIO_PORT_NUM,	CLICKBOARD1_RST_GPIO_BIT_NUM, true);
		vTaskDelay(1); //Delay_ms(1);
	} else if ((int) pvParameters == SLOT2) {
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD2_INT_GPIO_PORT_NUM, CLICKBOARD2_INT_GPIO_BIT_NUM, GPIO_INPUT);
		/*define reset pin in slot 1*/
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD2_RST_GPIO_PORT_NUM, CLICKBOARD2_RST_GPIO_BIT_NUM, GPIO_OUTPUT);
		/*start  reset procedure*/
		Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD2_RST_GPIO_PORT_NUM, CLICKBOARD2_RST_GPIO_BIT_NUM, true);
		vTaskDelay(5); //Delay_ms(5);
		Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD2_RST_GPIO_PORT_NUM,	CLICKBOARD2_RST_GPIO_BIT_NUM, false);
		vTaskDelay(5); //Delay_ms(5);
		Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD2_RST_GPIO_PORT_NUM,	CLICKBOARD2_RST_GPIO_BIT_NUM, true);
		vTaskDelay(1); //Delay_ms(1);
	}

	i2c_init();

  	Expander_Write_Byte(EXPAND_ADDR, IODIRB_BANK0, 0x00);       // Set Expander's PORTA to be output
  	Expander_Write_Byte(EXPAND_ADDR, IODIRA_BANK0, 0xFF);       // Set Expander's PORTB to be input
  	Expander_Write_Byte(EXPAND_ADDR, GPPUA_BANK0, 0xFF);        // Set pull-ups to all of the Expander's PORTB pins

	/*Task Work*/
	int i = 0;
	char cBits_temp = 0;
	char cBits = 0;

	while (1){
		cBits = get_expand2click();
		if (cBits != cBits_temp){
			cBits_temp = cBits;
			i = 0;
		}

		set_expand2click(i);
		i++;
		vTaskDelay(100);
	}
}


