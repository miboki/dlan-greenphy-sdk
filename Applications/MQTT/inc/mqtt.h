/*
 * mqtt.h
 *
 *  Created on: 31.08.2017
 *      Author: christoph.domnik
 */

#ifndef MQTT_INC_MQTT_H_
#define MQTT_INC_MQTT_H_

#define MQTTQUEUE_LENGTH 20 /* NUber of Items could bestored in the Queue */
#define MQTTQUEUE_ITEMSIZE sizeof(MqttJob_t) /* Size of each Queue Item in Byte */

#define MQTTTASK_SIZE 420 /* Reserved stack-size for the MQTT Task in words */
#define MQTTTASK_DELAY 100 /* Delay for the task in ms */

#define MQTTRECV_BUFFER_SIZE 32 /* Size of MQTT Receive Buffer in Bytes */
#define MQTTSEND_BUFFER_SIZE 192 /* Size of MQTT Send Buffer in Bytes */
#define MQTTCLIENT_TIMEOUT 2000 /* Timeout of the client in ms */
#define MQTTRECEIVE_TIMEOUT 1000 /* Timeout for MQTTYield, will wait full time, if to low will cause errors */
//#define MQTTCREDENTIALS_INIT { {'M', 'Q', 'T', 'C'}, 0, 4, \
			{"TCdqKfhZeQriWr/7PNmy4mw", {0, NULL}}, \
			120, 1, 0, MQTTPacket_willOptions_initializer, \
			{"09da8a7e-165e-42b8-96af-fecf366cb89b", {0, NULL}}, \
			{"wy5t6n.SQBQx", {0, NULL}} }
#define MQTTCREDENTIALS_INIT { {'M', 'Q', 'T', 'C'}, 0, 4, \
			{"GreenPHY-2de", {0, NULL}}, \
			120, 1, 0, MQTTPacket_willOptions_initializer, \
			{NULL, {0, NULL}}, \
			{NULL, {0, NULL}} }
#define netconfigMQTT_BROKER "broker.hivemq.com"
#define netconfigMQTT_PORT 1883

#include "MQTTClient.h"

typedef enum
{
	eInitialize,
	eSet,
	eGet
} eVarAction;

typedef enum
{
	eNoEvent = -1,
	eConnect,		/* 0: Connect to Broker with stored Credentials */
	eDisconnect,	/* 1: Disconnect current connection */
	ePublish,		/* 2: Publish the message the data pointer points to*/
	eSubscribe,		/* 3: Subscribe to channel given via data pointer */
	eRecieve,		/* 4: Perform MQTT Yield to receive and proceed Packages */
	eKill			/* 5: Kill the Task, empty queue */
} eMqttJobType_t;

typedef struct {
	eMqttJobType_t eJobType;
	void *data;
} MqttJob_t;

typedef struct {
	unsigned char *pucTopic;
	MQTTMessage xMessage;
} MqttPublishMsg_t;


QueueHandle_t xGetMQTTQueueHandle( void );
QueueHandle_t xInitMQTT(void);
void vDeinitMQTT( void );

#endif /* MQTT_INC_MQTT_H_ */
