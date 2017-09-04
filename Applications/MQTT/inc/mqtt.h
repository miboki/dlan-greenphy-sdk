/*
 * mqtt.h
 *
 *  Created on: 31.08.2017
 *      Author: christoph.domnik
 */

#ifndef MQTT_INC_MQTT_H_
#define MQTT_INC_MQTT_H_

#include "MQTTClient.h"
#include "MQTTFreeRTOS.h"

/*********************************************************************************
 *	Defines
 * *******************************************************************************/

#define mqttRECV_BUFFER_SIZE 32 /* Size of MQTT Receive Buffer in Bytes */
#define mqttSEND_BUFFER_SIZE 192 /* Size of MQTT Send Buffer in Bytes */

#define mqttRECEIVE_DELAY 1000 /* Delay of MQTT Task in milliseconds */

#define CLIENT_BLOCKED 1
#define CLIENT_FREE 0


/*********************************************************************************
 *	Structs and typedefs
 * *******************************************************************************/
/*typedef struct{
	char *pcBrokername;
	int port;
	char *pcClientID;
	char *pcUsername;
	char *pcPasswort;
	uint8_t ucWillFlag;
	struct{
		char *pcWillMessage;
		char *pcWillTopic;
		uint8_t
			willqos : 2,
			willretain : 1;
	}xWillParam;
}xMqttCredentials_t;
*/

/*********************************************************************************
 *	function prototypes
 * *******************************************************************************/
void vInitMqttTask();
BaseType_t xDeinitMqtt();
BaseType_t xPublishMessage( char *pucMessage, char *pucTopic, unsigned char ucQos, unsigned char ucRetained );


#endif /* MQTT_INC_MQTT_H_ */
