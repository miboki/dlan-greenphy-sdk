/*
 * Color2Click.c
 *
 *  Created on: 25.01.2017
 *      Author: Jordan McConnell @ SparkFun Electronics / devolo AG
 */

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
#include "color2click.h"


uint8_t _addr = 0x44; //ISL29125 default ID;

struct Colors {
	int red;
	int green;
	int blue;
	int status;
} color;

uint8_t read8(uint8_t reg);
void write8(uint8_t reg, uint8_t data);

uint16_t read16(uint8_t reg);
void write16(uint8_t reg, uint16_t data);

/* Reset all registers - returns true if successful */
bool reset_ISL29125() {
	uint8_t data = 0x00;
	/* Reset registers */
	write8(DEVICE_ID, 0x46);
	/* Check reset */
	data = read8(CONFIG_1);
	data |= read8(CONFIG_2);
	data |= read8(CONFIG_3);
	data |= read8(STATUS);
	if (data != 0x00) {
		return false;
	}
	return true;
}

/* Setup Configuration registers (three registers) - returns true if successful
 // Use CONFIG1 variables from ISL29125_SOFT.h for first parameter config1, CONFIG2 for config2, 3 for 3
 // Use CFG_DEFAULT for default configuration for that register */
bool config_ISL29125(uint8_t config1, uint8_t config2, uint8_t config3) {
	bool ret = true;
	uint8_t data = 0x00;

	/* Set 1st configuration register */
	write8(CONFIG_1, config1);
	/* Set 2nd configuration register */
	write8(CONFIG_2, config2);
	/* Set 3rd configuration register */
	write8(CONFIG_3, config3);

	/* Check if configurations were set correctly */
	data = read8(CONFIG_1);
	if (data != config1) {
		ret &= false;
	}
	data = read8(CONFIG_2);
	if (data != config2) {
		ret &= false;
	}
	data = read8(CONFIG_3);
	if (data != config3) {
		ret &= false;
	}
	return ret;
}

/* Verifies sensor is there by checking its device ID
 / Resets all registers/configurations to factory default
 / Sets configuration registers for the common use case */
bool ISL29125_SOFT_init(void) {
	bool ret = true;
	uint8_t data = 0x00;
	data = read8(DEVICE_ID);
	if (data != 0x7D) {
		ret &= false;
	}
	/* Reset registers */
	ret &= reset_ISL29125();
	/* Set to RGB mode, 10k lux, and high IR compensation */
	ret &= config_ISL29125(CFG1_MODE_RGB | CFG1_10KLUX, CFG2_IR_ADJUST_HIGH,
	CFG_DEFAULT);
	return ret;
}

/* Sets upper threshold value for triggering interrupts */
void setUpperThreshold(uint16_t data) {
	write16(THRESHOLD_HL, data);
}

/* Sets lower threshold value for triggering interrupts */
void setLowerThreshold(uint16_t data) {
	write16(THRESHOLD_LL, data);
}

/* Check what the upper threshold is, 0xFFFF by default */
uint16_t readUpperThreshold() {
	return read16(THRESHOLD_HL);
}

/* Check what the upper threshold is, 0x0000 by default */
uint16_t readLowerThreshold() {
	return read16(THRESHOLD_LL);
}

/* Read the latest Sensor ADC reading for the color Red */
uint16_t readRed() {
	return read16(RED_L);
}

/* Read the latest Sensor ADC reading for the color Green */
uint16_t readGreen() {
	return read16(GREEN_L);
}

/* Read the latest Sensor ADC reading for the color Blue */
uint16_t readBlue() {
	return read16(BLUE_L);
}

/* Check status flag register that allows for checking for interrupts, brownouts, and ADC conversion completions */
uint8_t readStatus() {
	return read8(STATUS);
}

/* Generic I2C read register (single byte) */
uint8_t read8(uint8_t reg) {
	I2C_XFER_T xfer;
	xfer.slaveAddr = _addr;
	uint8_t tmp_data[2];
	tmp_data[0] = reg;
	xfer.txBuff = tmp_data;
	xfer.rxBuff = tmp_data;
	xfer.txSz = 1;
	xfer.rxSz = 2;
	/*transfer data to T-chip via I2C and read new data, check if error has occurred*/
	if (Chip_I2C_MasterTransfer(I2C1, &xfer) != I2C_STATUS_DONE)
		return 0xaf;
	return tmp_data[0];
}

/* Generic I2C write data to register (single byte) */
void write8(uint8_t reg, uint8_t data) {
	I2C_XFER_T xfer;
	xfer.slaveAddr = _addr;
	uint8_t tmp_data[2];
	tmp_data[0] = reg;
	tmp_data[1] = data;
	xfer.txBuff = tmp_data;
	xfer.txSz = 2;
	Chip_I2C_MasterSend(I2C1, xfer.slaveAddr, xfer.txBuff, xfer.txSz);
	return;
}

/* Generic I2C read registers (two bytes, LSB first) */
uint16_t read16(uint8_t reg) {
	I2C_XFER_T xfer;
	xfer.slaveAddr = _addr;
	uint8_t tmp_data[3];
	tmp_data[0] = reg;
	xfer.txBuff = tmp_data;
	xfer.rxBuff = tmp_data;
	xfer.txSz = 1;
	xfer.rxSz = 3;
	/*transfer data to T-chip via I2C and read new data, check if error has occurred*/
	if (Chip_I2C_MasterTransfer(I2C1, &xfer) != I2C_STATUS_DONE)
		return 0xaf;

	return ((tmp_data[1] << 8) | tmp_data[0]);

}

/* Generic I2C write data to registers (two bytes, LSB first) */
void write16(uint8_t reg, uint16_t data) {
	I2C_XFER_T xfer;
	xfer.slaveAddr = _addr;
	uint8_t tmp_data[3];
	tmp_data[0] = reg;
	tmp_data[1] = data;
	tmp_data[2] = (data >> 8);
	xfer.txBuff = tmp_data;
	xfer.txSz = 3;
	Chip_I2C_MasterSend(I2C1, xfer.slaveAddr, xfer.txBuff, xfer.txSz);
	return;
}

void PrintStatus(void) {
	DEBUGOUT(" Status: ");
	if (color.status & FLAG_INT)
		DEBUGOUT("FLAG_INT ");
	if (color.status & FLAG_CONV_DONE)
		DEBUGOUT("CONV_DONE ");
	if (color.status & FLAG_BROWNOUT)
		DEBUGOUT("FLAG_BROWNOUT ");
	if ((color.status & FLAG_CONV_B) == FLAG_CONV_B)
		DEBUGOUT("B");
	else if (color.status & FLAG_CONV_R)
		DEBUGOUT("R");
	else if (color.status & FLAG_CONV_G)
		DEBUGOUT("G");

}

void ReadColors(void) {
	color.red = readRed();
	color.green = readGreen();
	color.blue = readBlue();
	color.status = readStatus();
}

/*****************************************************************************/

static TaskHandle_t xClickTaskHandle = NULL;

void vColor2Click_Task(void *pvParameters) {
	while( 1 )
	{
		ReadColors();

		DEBUGOUT("Color2click - red: %d; green: %d; blue: %d", color.red,
				color.green, color.blue);
		PrintStatus();
		DEBUGOUT("\r\n");

		vTaskDelay( 1000 );
	}
}

#if( includeHTTP_DEMO != 0 )
	BaseType_t xColor2Click_RequestHandler( char *pcBuffer, size_t uxBufferLength, QueryParam_t *pxParams, BaseType_t xParamCount )
	{
		BaseType_t xCount = 0;
		int lColorMax = color.red;
		lColorMax = MAX( lColorMax, color.green );
		lColorMax = MAX( lColorMax, color.blue );

		xCount += sprintf( pcBuffer, "{\"r\":%d,\"g\":%d,\"b\":%d}",
//				( ( color.red * 255 ) / lColorMax ),
//				( ( color.green * 255 ) / lColorMax ),
//				( ( color.blue * 255 ) / lColorMax ) );
				color.red, color.green, color.blue );

		return xCount;
	}
#endif

BaseType_t xColor2Click_Init ( const char *pcName, BaseType_t xPort )
{
BaseType_t xReturn = pdFALSE;

	if( xClickTaskHandle == NULL )
	{
		if( xPort == eClickboardPort1 )
		{
			Chip_GPIO_SetPinDIRInput( LPC_GPIO, CLICKBOARD1_INT_GPIO_PORT_NUM, CLICKBOARD1_INT_GPIO_BIT_NUM );
		}
		else if( xPort == eClickboardPort2 )
		{
			Chip_GPIO_SetPinDIRInput( LPC_GPIO, CLICKBOARD2_INT_GPIO_PORT_NUM, CLICKBOARD2_INT_GPIO_BIT_NUM );
		}

		Board_I2C_Init( I2C1 );
		ISL29125_SOFT_init();

		xTaskCreate( vColor2Click_Task, pcName, 240, NULL,	( tskIDLE_PRIORITY + 1 ), &xClickTaskHandle );
		#if( includeHTTP_DEMO != 0 )
		{
			xAddRequestHandler( pcName, xColor2Click_RequestHandler );
		}
		#endif

		xReturn = pdTRUE;
	}

	return xReturn;
}

BaseType_t xColor2Click_Deinit ( void )
{
BaseType_t xReturn = pdFALSE;

	if( xClickTaskHandle != NULL )
	{
		#if( includeHTTP_DEMO != 0 )
		{
			xRemoveRequestHandler( pcTaskGetName( xClickTaskHandle ) );
		}
		#endif
		vTaskDelete( xClickTaskHandle );
		xClickTaskHandle = NULL;

		/* TODO: Reset I2C and GPIOs. */
		xReturn = pdTRUE;
	}

	return xReturn;
}
