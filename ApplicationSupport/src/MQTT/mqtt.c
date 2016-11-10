/*
 * mqtt.c
 *
 *  Created on: 25.11.2015
 *      Author: Sebastian Sura
 */

#include "mqtt.h"
#include "string.h"
#include "stdbool.h"
#include "FreeRTOS.h"
#include "task.h"
#include "uip.h"
#include "uipopt.h"
#include "psock.h"
#include "timer.h"
#include "queue.h"
#include "MQTTClient-C/src/MQTTClient.h"
#include "types.h"
#include "lpc17xx_pinsel.h"
#include "jansson.h"

#define mqtt_TASK_PRIORITY          ( tskIDLE_PRIORITY + 2 )

#define STATE_CONNECTING 0
#define STATE_CONNECTED_TCP 1
#define STATE_CONNECTED_MQTT 2
#define STATE_MQTT_SUBSCRIBED 3
#define STATE_CLOSED 4
#define STATE_AVAILABLE 5
#define STATE_POLLED 6
#define STATE_HEARTBEAT 7

#define MQTT_HEADER_CON 0x10
#define MQTT_HEADER_PUB 0x30
#define MQTT_HEADER_CONACK 0x20
#define MQTT_HEADER_SUB 0x80
#define MQTT_HEADER_SUBACK 0x90

xTaskHandle xHandle_mqtt = NULL;
char mqtt_buffer[150];
int mqtt_buffer_len;
int subscribe;
static struct mqtt_state mqtt;
struct MQTT_Queue_struct mqtt_data;
config_t *gpconfig;
uint8_t USRLEDstate = 0;
uip_ipaddr_t mqtt_addr;
extern xQueueHandle MQTT_Queue;

/********************************************************************//**
 * @brief       Creates the payload for the MQTT Pubslish Packet using
 *              JSON and writes it to payload
 * @param[in]   Pointer to payload
 * @return      None
 *********************************************************************/
void createPublishString(char *payload) {

    taskENTER_CRITICAL();
    float ttemp2, temperature = 0;
    int d1, d2;
    char temp[10];
    char *result;

    json_t *root = json_object();
    json_object_set_new(root, "meaning", json_string(mqtt_data.meaning));
    if (strstr(mqtt_data.meaning, "temperature")) {
        temperature = mqtt_data.value;
        d1 = temperature;
        ttemp2 = temperature - d1;
        d2 = (int)(ttemp2  * 10);
        sprintf(temp, "%i.%i", d1, d2);
    } else {
        sprintf(temp, "%i", (int)mqtt_data.value);
    }
    json_object_set_new(root, "value", json_string(temp));
    result = json_dumps(root, 0);
    sprintf(payload, result);
    vPortFree(result);
    json_decref(root);
    taskEXIT_CRITICAL();
}

/********************************************************************//**
 * @brief       Creates a MQTT Connect with provided credentials and
 *              writes it to mqtt_buffer
 * @param[in]   None
 * @return      None
 *********************************************************************/
void createMQTTConnect() {
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

    data.clientID.cstring = gpconfig->clientid;
    data.username.cstring = gpconfig->user;
    data.password.cstring = gpconfig->password;
    data.keepAliveInterval = 60;
    data.cleansession = 1;
    data.MQTTVersion = 4;
    mqtt_buffer_len = MQTTSerialize_connect(mqtt_buffer, sizeof(mqtt_buffer), &data);
}

/********************************************************************//**
 * @brief       Creates the payload for the MQTT Pubslish Packet using
 *              JSON and writes it to payload
 * @param[in]   Pointer to payload
 * @return      None
 *********************************************************************/
void createMQTTPublish() {

    MQTTString topicString = MQTTString_initializer;
    char payload[200];
    int payloadlen;
    topicString.cstring = gpconfig->topic;
    createPublishString(payload);
    payloadlen = strlen(payload);
    mqtt_buffer_len = MQTTSerialize_publish(mqtt_buffer, sizeof(mqtt_buffer), 0, 0, 0, 0, topicString, payload,payloadlen);
}

/********************************************************************//**
 * @brief       Creates a MQTTSubscribe Packet to subscribe to relayrs
 *              ./cmd topic
 * @param[in]   None
 * @return      None
 *********************************************************************/
void createMQTTSubscribe() {
    MQTTString topicString = MQTTString_initializer;
    char topic[45];
    strcpy(topic, "/v1/");
    strncat(topic, gpconfig->user, 36);
    strcat(topic, "/cmd");
    topic[44] = '\0';
    topicString.cstring = topic;
    mqtt_buffer_len = MQTTSerialize_subscribe(mqtt_buffer, sizeof(mqtt_buffer), 0 ,3 , 1, &topicString, 0);
}

/********************************************************************//**
 * @brief       Tell uIP stack to send a syn to remote host and establish
 *              new connection, IP is read from flash
 *              ./cmd topic
 * @param[in]   None
 * @return      None
 *********************************************************************/
void setup_mqtt(void) {
    //extern config_t gpconfig;
    //uip_ipaddr(&mqtt_addr, gpconfig.IP[0], gpconfig.IP[1],gpconfig.IP[2], gpconfig.IP[3]);
    //uip_ipaddr(&ipaddr, 52,17,197,41);
    //uip_ipaddr(&ipaddr, 54,154,53,255);
    //uip_ipaddr(&mqtt_addr, 54, 77, 125, 59);
    //uip_ipaddr(&ipaddr, 192,168,0,12);
    mqtt.conn = uip_connect(&mqtt_addr, HTONS(1883));
}

/********************************************************************//**
 * @brief       Toggles the USR LED on Eval Board
 * @param[in]   None
 * @return      None
 *********************************************************************/
void toogleUSRLED() {
    taskENTER_CRITICAL();
    PINSEL_CFG_Type PinCfg;
    PinCfg.Pinmode = 0;
    PinCfg.Funcnum = 0;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);
    GPIO_SetDir(0, (1 << 6), 1);
    if (USRLEDstate == 0) {
        GPIO_ClearValue(0, (1 << 6));
        USRLEDstate = 1;
    } else {
        GPIO_SetValue(0, (1 << 6));
        USRLEDstate = 0;
    }
    //Reset to Func3 for I2C
    PinCfg.Funcnum = 3;
    PINSEL_ConfigPin(&PinCfg);
    taskEXIT_CRITICAL();
}

/********************************************************************//**
 * @brief       Looks in the received MQTTPublish Packet for the status of
 *              the LED  and checks if correct deviceID is given
 * @param[in]   Pointer to the payload to look in, Length of payload
 * @return      None
 *********************************************************************/
void checkpayload(unsigned char *payload_in, int payloadlen_in) {
    taskENTER_CRITICAL();
    json_t *root, *jsonData;
    json_error_t error;
    const char *jsonClientID;
    int jsonValue;

    payload_in[payloadlen_in] = '\0';
    root = json_loads(payload_in, 0, &error);
    if (root) {
        jsonData = json_object_get(root, "deviceId");
        if (jsonData)
            jsonClientID = json_string_value(jsonData);
        if (!strcmp(jsonClientID, gpconfig->user)) {
            json_decref(jsonData);
            jsonData = json_object_get(root, "value");
            if (jsonData)
                jsonValue = json_integer_value(jsonData);
            if (jsonValue)
                toogleUSRLED();
        }
    }
    json_decref(root);
    json_decref(jsonData);
    taskEXIT_CRITICAL();
}

/********************************************************************//**
 * @brief       Checks is incomming packet is a correct MQTTPublish packet
 *              ./cmd topic
 * @param[in]   Pointer to incomming data on current connection
 * @return      None
 *********************************************************************/
void checkMQTTPublish(void *appdata) {
    MQTTString topicString;
    unsigned char* payload_in;
    int payloadlen_in;
    unsigned char dup, retained;
    int qos;
    unsigned short packetid;
    if (MQTTDeserialize_publish(&dup,&qos,&retained,&packetid, &topicString, &payload_in, &payloadlen_in, appdata, uip_len)) {
        if (payloadlen_in > 1)
            checkpayload(payload_in, payloadlen_in);
    }
}

/********************************************************************//**
 * @brief       Checks if valid credentials are given, since relayr
 *              generates random data, we can only check for length
 * @param[in]   None
 * @return      None
 *********************************************************************/
int checkcredentials() {

    if (strlen(gpconfig->user) != 36)
        return 0;
    if (strlen(gpconfig->password) != 12)
        return 0;
    if (strlen(gpconfig->clientid) != 23)
        return 0;
    if (strlen(gpconfig->topic) != 45 )
        return 0;

    return 1;
}

/********************************************************************//**
 * @brief       Creates an empty MQTTPubish Packet to keep connection alive
 *              Although the MQTT standard accepts empty Packets, the broker
 *              may disconnect you anyway
 * @param[in]   None
 * @return      None
 *********************************************************************/
void create_Heartbeat() {
    MQTTString topicString = MQTTString_initializer;
    memset(mqtt_buffer, '\0', sizeof(mqtt_buffer));
    mqtt_buffer_len = MQTTSerialize_publish(mqtt_buffer, sizeof(mqtt_buffer), 0, 0, 0, 0, topicString, NULL,0);
}

/********************************************************************//**
 * @brief       Checks incoming packet for MQTTHeader Type and returns
 *              the type
 * @param[in]   Pointer to the packet
 * @return      Type of MQTT Header
 *********************************************************************/
int check_header(unsigned *buf) {

    char c = *buf;

    if (c == MQTT_HEADER_CON)
        return 1;
    if (c == MQTT_HEADER_CONACK)
            return 2;
    if (c == MQTT_HEADER_PUB)
            return 3;
    if (c == MQTT_HEADER_SUB)
            return 8;
    if (c == MQTT_HEADER_SUBACK)
            return 9;
    return 0;
}

/********************************************************************//**
 * @brief       Controls MQTT Connection, is called everytime we send
 *              or receive a Packet to/on Port 1883
 * @param[in]   None
 * @return      None
 *********************************************************************/
void mqtt_appcall() {

    if (uip_aborted()) {
        mqtt.state = STATE_CLOSED;
        uip_abort();
    }

    if (uip_timedout()) {
        mqtt.state = STATE_CLOSED;
        uip_close();
    }

    if (uip_closed()) {
        mqtt.state = STATE_CLOSED;
    }

    if(uip_connected()) {
        mqtt.state = STATE_CONNECTED_TCP;
        createMQTTConnect();
        uip_send(mqtt_buffer, mqtt_buffer_len);
    }

    if (uip_acked()) {
        }

    if (uip_rexmit()) {
        uip_send(mqtt_buffer, mqtt_buffer_len);
    }

    if (uip_newdata()) {
            switch (check_header(uip_appdata)) {
                case 2:
                    mqtt.state = STATE_CONNECTED_MQTT;
                    createMQTTSubscribe();
                    uip_send(mqtt_buffer, mqtt_buffer_len);
                    break;
                case 3:
                    checkMQTTPublish(uip_appdata);
                    break;
                case 9:
                    mqtt.state = STATE_MQTT_SUBSCRIBED;
                default:
                    break;
            }
            return;
        }

    if (uip_poll() && mqtt.state == STATE_POLLED) {
            createMQTTPublish();
            uip_send(mqtt_buffer, mqtt_buffer_len);
            mqtt.state = STATE_MQTT_SUBSCRIBED;
    }

    if (uip_poll() && mqtt.state == STATE_HEARTBEAT) {
                uip_send(mqtt_buffer, mqtt_buffer_len);
                mqtt.state = STATE_MQTT_SUBSCRIBED;
        }
}

/********************************************************************//**
 * @brief       Activate/deactivate MQTT Task
 * @param[in]   Pointer to Input String from webserver task
 * @return      None
 *********************************************************************/
void mqtt_activate(char *c) {

    if (strstr(c, "?activate_mqtt=yes")) {
        if (!gpconfig->active) {
            init_mqtt();
            writeflash();
        }

    } else if(strstr(c, "?activate_mqtt=no")) {
        if (gpconfig->active) {
            deinit_mqtt();
            writeflash();
        }
    }
}

/********************************************************************//**
 * @brief       MQTT_Task waits i seconds for new sensor and receives it
 *              using a Queue. If data is available it changes
 *              the status of the connection to tell it to send data.
 *              If connection is closed, it reconnects automatically
 * @param[in]   None
 * @return      None
 *********************************************************************/
void MQTT_Task(void *pvParameters) {
    createMQTTConnect();
    mqtt.state == STATE_CLOSED;
    setup_mqtt();
    while (mqtt.state != STATE_MQTT_SUBSCRIBED) {
        vTaskDelay(1000);
    }

       for (;;) {
           if (xQueueReceive(MQTT_Queue, &mqtt_data, 30000)) {
               if (checkcredentials()) {
                   if (mqtt.state == STATE_CLOSED) {
                       mqtt.state = STATE_CONNECTING;
                       setup_mqtt();
                       vTaskDelay(2000);
                   } else if (mqtt.state == STATE_MQTT_SUBSCRIBED) {
                       createMQTTPublish();
                       mqtt.state = STATE_POLLED;
                       uip_poll_conn(mqtt.conn);
                   }
               }

           } else {
               if (checkcredentials()) {
                   //send heartbeat
                   if (mqtt.state == STATE_CLOSED) {
                       mqtt.state = STATE_CONNECTING;
                       setup_mqtt();
                   } else if (mqtt.state == STATE_MQTT_SUBSCRIBED) {
                       create_Heartbeat();
                       mqtt.state = STATE_HEARTBEAT;
                       uip_poll_conn(mqtt.conn);
                   }
               }
           }
       }
   }

/********************************************************************//**
 * @brief       Start MQTT Task
 * @param[in]   None
 * @return      None
 *********************************************************************/
void init_mqtt() {

    gpconfig->active = 1;
    xTaskCreate(MQTT_Task, (signed char *) "MQTT", 300, NULL, 4,
                &xHandle_mqtt);

}

/********************************************************************//**
 * @brief       Stop MQTT Task
 * @param[in]   None
 * @return      None
 *********************************************************************/
void deinit_mqtt() {
    gpconfig->active = 0;
    mqtt.state = STATE_CLOSED;
    vTaskDelete(xHandle_mqtt);
}
