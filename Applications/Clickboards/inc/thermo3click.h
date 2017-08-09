/*
 * thermo3click.h
 *
 *  Created on: 30.12.2016
 *      Author: mikroelektronika / devolo AG
 *
 *  Last Modified on: 07.08.2017
 *  			  By: cdomnik @ devolo AG
 */

#ifndef INC_THERMO3CLICK_H_
#define INC_THERMO3CLICK_H_

#define TMP102_I2C_ADDR   0x48 //TMP102 I2C address (ADD0 pin is connected to ground) -> otherwise it is 0x49

float Get_Temperature(void);
//void Thermo3Click_Task(void *pvParameters);


#endif /* INC_THERMO3CLICK_H_ */
