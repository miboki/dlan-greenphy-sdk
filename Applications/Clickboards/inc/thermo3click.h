/*
 * thermo3click.h
 *
 *  Created on: 30.12.2016
 *      Author: mikroelektronika / devolo AG
 */

#ifndef INC_THERMO3CLICK_H_
#define INC_THERMO3CLICK_H_

xTaskHandle xHandle_thermo3click = NULL;
void Thermo3Click_Task(void *pvParameters);

#endif /* INC_THERMO3CLICK_H_ */
