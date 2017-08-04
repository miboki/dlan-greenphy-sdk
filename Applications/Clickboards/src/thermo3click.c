/*
 * thermo3click.c
 *
 *  Created on: 30.12.2016
 *      Author: mikroelektronika / devolo AG
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
#include "thermo3click.h"




float Get_Temperature(void){
#define TEMP_OFFSET  -2.0
	int TemperatureSum;
	float Temperature;

	I2C_XFER_T xfer;
	xfer.slaveAddr = 0x48;                    // TMP102 I2C address (ADD0 pin is connected to ground)
  //xfer.slaveAddr = 0x49;                    // TMP102 I2C address (ADD0 pin is connected to ground)

  uint8_t tmp_data[2];
  tmp_data[0] = 0;
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



void Thermo3Click_Task(void *pvParameters){
	float fTemperature  = 0.0;
	float oldTemperature = 0.0;

	if ((int)pvParameters == SLOT1)  		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD1_INT_GPIO_PORT_NUM, CLICKBOARD1_INT_GPIO_BIT_NUM, GPIO_INPUT);
	else if ((int)pvParameters == SLOT2)  Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD2_INT_GPIO_PORT_NUM, CLICKBOARD2_INT_GPIO_BIT_NUM, GPIO_INPUT);
	i2c_init();

	while (1){
		fTemperature  = Get_Temperature();

		if (oldTemperature != fTemperature)
		{
			printToUart("Thermo3click: %d.%d deg.\r\n",(int)fTemperature,(int)(fTemperature*10)%10);
			oldTemperature = fTemperature;
		}

		vTaskDelay(1000);
	}
}






