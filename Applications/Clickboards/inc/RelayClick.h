/*
 * RelayClick.h
 *
 *  Created on: 23.01.2017
 *      Author: devolo AG
 */

#ifndef INC_RELAYCLICK_H_
#define INC_RELAYCLICK_H_

int RelayClick_Init();
int RelayClick_Deinit();

xTaskHandle xHandle_RelayClick = NULL;
void Relay_Click_Task(void *pvParameters);

#endif /* INC_RELAYCLICK_H_ */
