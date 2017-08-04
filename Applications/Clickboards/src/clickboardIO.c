/*
===============================================================================
 Name        : ClickboardIO.c
 Author      : devolo AG
 Version     : 1.0
 Copyright   : $(copyright)
 Description : IO interface for many clickboards (from MikroElektronika)
===============================================================================
*/

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include "debug.h"
#include "clickboardIO.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

bool I2C_Init_Done = false;

void i2c_init(void){
	if (I2C_Init_Done == false)
	{
	Board_I2C_Init(I2C1);  // <--- configures the I2C pins
	Chip_I2C_Init(I2C1);

#define SPEED_400KHZ 400000
	Chip_I2C_SetClockRate(I2C1, SPEED_400KHZ);
	Chip_I2C_SetMasterEventHandler(I2C1, Chip_I2C_EventHandlerPolling);
	I2C_Init_Done = true;
	}
}

void Clickboard_Init() {
#if THERMO3CLICK != BOARD_UNUSED
  #include "thermo3click.h"
	xTaskCreate(Thermo3Click_Task, (const char * )"Thermo3Click", 240, (int *)THERMO3CLICK,	sensor_TASK_PRIORITY, &xHandle_thermo3click);
#endif

#if DEVOLO_LCD_CLICK != BOARD_UNUSED
  #include "lcdDevoloClick.h"
	xTaskCreate(Devolo_Lcd_Click_Task, (const char * )"DevoloLcdClick", 240, (int *)DEVOLO_LCD_CLICK,	sensor_TASK_PRIORITY, &xHandle_lcdDevoloClick);
#endif

#if RELAY_CLICK != BOARD_UNUSED
#include "RelayClick.h"
	xTaskCreate(Relay_Click_Task, (const char * )"RelayClick", 240, (int *)RELAY_CLICK,	sensor_TASK_PRIORITY, &xHandle_RelayClick);
#endif


#if EXPAND2CLICK != BOARD_UNUSED
#include "Expand2Click.h"
	xTaskCreate(Expand2Click_Task, (const char * )"Expand2Click", 240, (int *)EXPAND2CLICK,	sensor_TASK_PRIORITY, &xHandle_Expand2Click);
#endif

#if COLOR2CLICK != BOARD_UNUSED
#include "Color2Click.h"
	xTaskCreate(Color2Click_Task, (const char * )"Color2Click", 240, (int *)COLOR2CLICK,	sensor_TASK_PRIORITY, &xHandle_Color2Click);
#endif

}

