/*
 * dali_click.h
 *
 *  Created on: 20.08.2015
 *      Author: Sebastian Sura
 */

#ifndef INC_DALI_CLICK_H_
#define INC_DALI_CLICK_H_

#include "stdint.h"
#include <stdbool.h>

#define TX_PORT 1
#define RX_PORT 2
#define BUTTON_PORT 2

//#if DALIPORT == M1
//#define TX_PIN 26
//#define RX_PIN 3
//#define BUTTON_PIN 2
//#else
//#define TX_PIN 28
//#define RX_PIN 6
//#define BUTTON_PIN 7
//#endif

/*DALI COMMANDS*/
#define TERMINATE 0xA100 //256
#define INITIALISE 0xA500 //258
#define RANDOMISE 0xA700 //259
#define COMPARE 0xA900 //260
#define WITHDRAW 0xab00 //261
#define SEARCHADDRH 0xB100 //264
#define SEARCHADDRM 0xB300 //265
#define SEARCHADDRL 0xB500 //266
#define PROGRAMSHORTADDRESS 0xB701 //267
#define VERIFYSHORTADDRESS 0xB901 //268
#define STOREDTR 0xFF80 //128




static volatile uint8_t usbBackwardFrame;
static volatile bool earlyAnswer;
//static volatile capturedFrameType capturedFrame;

extern int getmasterState();
extern void setmasterState(int s);
//extern int getBackwardFrame();
extern int getDALI_slaves();

#endif /* INC_DALI_CLICK_H_ */
