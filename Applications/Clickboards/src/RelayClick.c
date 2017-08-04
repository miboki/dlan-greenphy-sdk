/*
 RelayClick.c
 *
 *  Created on: 23.01.2017
 *      Author: devolo AG
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
#include "RelayClick.h"






void Relay_Click_Task(void *pvParameters){

	int iValue = 0;


	/* Init */
	if ((int)pvParameters == SLOT1)
		{
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD1_CS_GPIO_PORT_NUM, CLICKBOARD1_CS_GPIO_BIT_NUM, GPIO_OUTPUT);
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD1_PWM_GPIO_PORT_NUM, CLICKBOARD1_PWM_GPIO_BIT_NUM, GPIO_OUTPUT);
	}
	else if ((int)pvParameters == SLOT2)
		{
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD2_CS_GPIO_PORT_NUM, CLICKBOARD2_CS_GPIO_BIT_NUM, GPIO_OUTPUT);
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD2_PWM_GPIO_PORT_NUM, CLICKBOARD2_PWM_GPIO_BIT_NUM, GPIO_OUTPUT);
	}


	/* Task Work */
	while (1){


		if ((int)pvParameters == SLOT1)
					{
					Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD1_CS_GPIO_PORT_NUM, CLICKBOARD1_CS_GPIO_BIT_NUM, iValue & 1);
					Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD1_PWM_GPIO_PORT_NUM, CLICKBOARD1_PWM_GPIO_BIT_NUM, iValue & 2);
					}
				else if ((int)pvParameters == SLOT2)
					{
					Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD2_CS_GPIO_PORT_NUM, CLICKBOARD2_CS_GPIO_BIT_NUM, iValue & 1);
					Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD2_PWM_GPIO_PORT_NUM, CLICKBOARD2_PWM_GPIO_BIT_NUM, iValue & 2);
					}
				vTaskDelay(1000);

	}
}






