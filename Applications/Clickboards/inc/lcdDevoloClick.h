/*
 * lcdDevoloClick.h
 *
 *  Created on: 02.01.2017
 *      Author: devolo AG
 */

#ifndef INC_LCDDEVOLOCLICK_H_
#define INC_LCDDEVOLOCLICK_H_

int LCD_Init();
int LCD_Deinit();

//int LCD_Print(int, char*, enum ClickboardCounter);
//
//
//void fToLCD( int row, char *name, float f, char *unit, enum ClickboardCounter);
//void iToLCD( int row, char *name, int i, char *unit, enum ClickboardCounter);

xTaskHandle xHandle_lcdDevoloClick = NULL;
void Devolo_Lcd_Click_Task(void *pvParameters);

#endif /* INC_LCDDEVOLOCLICK_H_ */
