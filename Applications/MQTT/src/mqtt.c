/*
 * mqtt.c
 *
 *  Created on: 20.09.2017
 *      Author: Christoph.Domnik
 *
 *  This is a Interface to the PAHO embedded MQTT Client.
 *  Its features contain: - connecting to a MQTT-Broker
 *  					  - publish messages to the connected MQTT-Broker
 *  					  - handle requests of the WebUI to show and change broker credentials
 *
 *  To Use the interface simply add jobs to the MQTT-Queue, an internal task will handle them.
 *
 */


/* ------------------------------
 * |          INCLUDES          |
 * ------------------------------ */
/* LPCOpen Includes. */
#include "board.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* GreenPHY SDK and Configuration includes */
#include "GreenPhySDKConfig.h"
#include "GreenPhySDKNetConfig.h"
#include "save_config.h"
#include "http_query_parser.h"
#include "http_request.h"

/* Include MQTT header files */
#include "mqtt.h"

/* ------------------------------
 * |      Global (Modul) Variables      |
 * ------------------------------ */
/* Handle for the Mqtt Queue, shared between xInitMQTT and xDeinitMQTT and xGetMQTTQueue */
static QueueHandle_t xMqttQueueHandle = NULL;

/* Handle for the Mqtt Task */
static TaskHandle_t xMqttTaskHandle = NULL;

/* ------------------------------
 * |          Functions         |
 * ------------------------------ */
/***********************************************************************************************************
 *   function: xManagePubCounter
 *   Purpose: Handle the pubcounter Variable, allow initialzising to 0, set to counter +1 and getting the counter
 *   Parameter: xAction - specify what the function will do
 *   Returns: pdPASS - Initializing / Setting done
 *   		  -44 - xAction has unsupported value
 *   		  xPubCounter (xAction = eGet)
 ***********************************************************************************************************/
BaseType_t xManagePubCounter( eVarAction xAction )
{
	static BaseType_t xPubCounter;
	switch(xAction){
	  case eInitialize:
		  xPubCounter = 0;
		break;
	  case eSet:
		  xPubCounter += 1;
		  break;
	  case eGet:
		return xPubCounter;
		break;
	  default:
		return -44;
	}
	return pdPASS;
}


/***********************************************************************************************************
 *   function: xManageUptime
 *   Purpose: Handle the uptime Variable, allow initialzising to actual uptime, setting to -1 and getting the uptime
 *   Parameter: xAction - specify what the function will do
 *   Returns: pdPASS - Initializing / Setting done
 *   		  -44 - xAction has unsupported value
 *   		  xUptime (xAction = eGet)
 ***********************************************************************************************************/
BaseType_t xManageUptime( eVarAction xAction )
{
	static BaseType_t xUptime = -1;
	switch(xAction){
	  case eInitialize:
		xUptime = ( portGET_RUN_TIME_COUNTER_VALUE() / 10000UL );
		break;
	  case eSet:
		xUptime = -1;
		break;
	  case eGet:
		if( xUptime < 0 )
			return xUptime;
		else
			return ( ( portGET_RUN_TIME_COUNTER_VALUE() / 10000UL ) - xUptime );
		break;
	  default:
		return -44;
	}
	return pdPASS;
}


/***********************************************************************************************************
 *   function: vCloseSocket
 *   Purpose: Close a tcp socket clean
 *   Parameter: Socket_t xSocket - the socket to be closed
 ***********************************************************************************************************/
void vCloseSocket( Socket_t xSocket )
{
	void *buffer;
	if( xSocket != NULL )
	{
		FreeRTOS_shutdown( xSocket, FREERTOS_SHUT_RDWR );
		while( FreeRTOS_recv( xSocket, buffer, 0, 0 ) >= 0 ){
			vTaskDelay( pdMS_TO_TICKS( 100 ) );
		}
		/* The socket has shut down and is safe to close. */
		FreeRTOS_closesocket( xSocket );
	}
}


/***********************************************************************************************************
 *   function: vGetCredentials
 *   Purpose: Read one parameter from the configuration, which parameter should be read is defined by the Config Tag
 *   Parameter: eConfigTag_t eConfigMqtt - Tag specifies which parameter should be read (same name as in config)
 *   			void *pvBuffer - return buffer for the read data
 ***********************************************************************************************************/
void vGetCredentials( eConfigTag_t eConfigMqtt, void *pvBuffer )
{
	void *pvValue = NULL;
	pvValue = pvGetConfig( eConfigMqtt, NULL );
	if( pvValue != NULL )
		pvBuffer = pvValue;
}



/***********************************************************************************************************
 *   function: xConnect
 *   Purpose: Connect the client given as parameter to a MQTT Broker. Credentials are in config or defines
 *   Parameter: MQTTCLIENT *pxClient - Pointer to the MQTT Client
 *   			Network *pxNetwork - Pointer to the Network Variable
 *   Returns:
 ***********************************************************************************************************/
BaseType_t xConnect( MQTTClient *pxClient, Network *pxNetwork )
{
	char *pcBroker = netconfigMQTT_BROKER;
	BaseType_t xPort = netconfigMQTT_PORT;
	MQTTPacket_connectData connectData = MQTTCREDENTIALS_INIT;

	vGetCredentials( eConfigMqttBroker, pcBroker );
	vGetCredentials( eConfigMqttPort, &xPort );

	/* Establish TCP connection to broker. No MQTT Connection at this point */
	if( NetworkConnect( pxNetwork, pcBroker, xPort ) != 0 )
	{
		DEBUGOUT("MQTT-Error 0x0111: Error while NetworkConnect. %s, Port %d", pcBroker, xPort);
		vCloseSocket( pxClient->ipstack->my_socket );
		pxClient->ipstack->my_socket = NULL;
		return pdFAIL;
	}

	/* Get the credentials */
	vGetCredentials( eConfigMqttClientID, connectData.clientID.cstring );
	vGetCredentials( eConfigMqttUser, connectData.username.cstring );
	vGetCredentials( eConfigMqttPassWD, connectData.password.cstring );
	vGetCredentials( eConfigMqttWill, &(connectData.willFlag) );
	if( connectData.willFlag )
	{
		vGetCredentials( eConfigMqttWillTopic, connectData.will.topicName.cstring );
		vGetCredentials( eConfigMqttWillMsg, connectData.will.message.cstring );
	}
	/* Connect to MQTT Broker */
	if( MQTTConnect( pxClient, &connectData ) != 0 )
	{
		DEBUGOUT("MQTT-Error 0x0112: MQTTConnect failed.");
		vCloseSocket( pxClient->ipstack->my_socket );
		pxClient->ipstack->my_socket = NULL;
		return pdFAIL;
	}
	xManageUptime( eInitialize );
	return pdPASS;
}



/***********************************************************************************************************
 *   function: vMQTTTask
 *   Purpose: Handles all of the Jobs comming from the MQTT queue.
 *   		  Connects, Disconnects, Publishes, Subscribe and Receive.
 *   		  This function is a adapter to the PAHO embedded MQTT Client.
 *   Global Variables used: xMqttQueueHandle, xMqttTaskHandle
 *   Parameter: void *pvParameters - not used currently
 ***********************************************************************************************************/
void vMQTTTask( void *pvParameters )
{
	/* Variables */
	BaseType_t cycles = 0;
	MQTTClient xClient;
	Network xNetwork;
	unsigned char pcSendBuf [MQTTSEND_BUFFER_SIZE];
	unsigned char pcRecvBuf [MQTTRECV_BUFFER_SIZE];
	MqttJob_t xJob;
	MqttPublishMsg_t *pxPubMsg;

	/* Initialize Variables for using */
	xManagePubCounter( eInitialize );
	NetworkInit( &xNetwork );
	MQTTClientInit( &xClient, &xNetwork, MQTTCLIENT_TIMEOUT, pcSendBuf, sizeof(pcSendBuf), pcRecvBuf, sizeof(pcRecvBuf) );

	/* Infinite Task Loop */
	for(;;)
	{
		/* -------------------- Check if task is working normal -------------------- */
		if(( xClient.isconnected != 1 ) && ( xClient.ipstack->my_socket != NULL ))
		{
			DEBUGOUT("MQTT-Error 0x0150: Connection lost. Will close Socket.\n");
			vCloseSocket( xClient.ipstack->my_socket );
			xClient.ipstack->my_socket = NULL;
		}

		/* -------------------- Get the next job from the queue -------------------- */
		if( xQueueReceive( xMqttQueueHandle, &xJob, 0 ) != pdPASS )
			xJob.eJobType = eNoEvent;

		/* ------------------------ Work on the current Job ------------------------ */
		switch( xJob.eJobType )
		{
			case eNoEvent:
				break;

			case eConnect:
				if( xClient.isconnected == 0 )
				{
					DEBUGOUT(" ** Info ** MQTT Connect\n");
					if( xConnect( &xClient, &xNetwork ) != pdPASS )
						DEBUGOUT("MQTT-Error 0x0110: Unable to connect to MQTT Broker.\n");
				}
				break;

			case eDisconnect:
				if( xClient.isconnected )
				{
					DEBUGOUT(" ** Info ** MQTT Disconnect\n");
					MQTTDisconnect( &xClient );
				}
				/* Clean up the tcp socket save */
				vCloseSocket( xClient.ipstack->my_socket );
				xClient.ipstack->my_socket = NULL;
				xManageUptime( eSet );
				break;

			case ePublish:
				if( xClient.isconnected )
				{
					DEBUGOUT(" ** Info ** MQTT Publish\n");
					pxPubMsg = (MqttPublishMsg_t *) xJob.data;
					if( MQTTPublish( &xClient, (pxPubMsg->pucTopic), &( pxPubMsg->xMessage ) ) != SUCCESS_MQTT)
					{
						DEBUGOUT("MQTT-Error 0x0130: Could not publish Message.\n");
					}
					xManagePubCounter( eSet );
					pxPubMsg->xMessage.payload = NULL;
				}
				else
					DEBUGOUT("MQTT-Error: 0x0131: Client lost connection. Publish will be discarded.\n");
				break;

			case eSubscribe:
				break;

			case eRecieve:
				if( xClient.isconnected )
				{
					DEBUGOUT(" ** Info ** MQTT Yield\n");
					if ( MQTTYield( &xClient, MQTTRECEIVE_TIMEOUT ) != SUCCESS_MQTT )
						DEBUGOUT("MQTT-Error 0x0140: MQTTYield failed.\n");
				}
				break;

			case eKill:
				DEBUGOUT(" ** Info ** MQTT Kill\n");
				/* Clean up Queue */
				vQueueDelete( xMqttQueueHandle );
				xMqttQueueHandle = NULL;
				xMqttTaskHandle = NULL;
				vTaskDelete( NULL );
				break;

			default:
				break;
		}

		/* Add Recive Job to Queue each 20 cycles (apox. 2s) */
		if( ( cycles++ ) > 20 )
		{
			xJob.eJobType = eRecieve;
			xQueueSendToBack( xMqttQueueHandle, &xJob, 0 );
			cycles = 0;
		}

		/* Delay until next call */
		vTaskDelay( pdMS_TO_TICKS( MQTTTASK_DELAY ) );
	}
}


#if( includeHTTP_DEMO != 0 )
	/***********************************************************************************************************
	 *   function: xMQTTRequestHandler
	 *   Purpose: Handle the HTTP requests for mqtt. Contains reading of new Credentials,
	 *   Global Variables used: xMqttQueueHandle, xMqttTaskHandle
	 *   Parameter: void *pvParameters - not used currently
	 ***********************************************************************************************************/
	static BaseType_t xMQTTRequestHandler( char *pcBuffer, size_t uxBufferLength, QueryParam_t *pxParams, BaseType_t xParamCount )
	{
		BaseType_t xCount = 0;
		QueryParam_t *pxParam;
		MqttJob_t xManageJob;

		pxParam = pxFindKeyInQueryParams( "toggle", pxParams, xParamCount );
		if( pxParam != NULL )
		{
			if( xManageUptime( eGet ) < 0 )
				xManageJob.eJobType = eConnect;
			else
				xManageJob.eJobType = eDisconnect;

			xQueueSendToBack( xMqttQueueHandle, &xManageJob, 0 );
		}


		char *pcBroker = netconfigMQTT_BROKER;
		BaseType_t xPort = netconfigMQTT_PORT;
		MQTTPacket_connectData connectData = MQTTCREDENTIALS_INIT;

		vGetCredentials( eConfigMqttBroker, pcBroker );
		vGetCredentials( eConfigMqttPort, &xPort );
		xCount += sprintf( pcBuffer, "{\"mqttUptime\":%d,\"mqttPubMsg\":%d,\"bad\":\"%s\",\"bpd\":%d", xManageUptime( eGet ), xManagePubCounter( eGet ), pcBroker, xPort );

		vGetCredentials( eConfigMqttClientID, connectData.clientID.cstring );
		if( connectData.clientID.cstring != NULL)
			xCount += sprintf( pcBuffer + xCount, ",\"cID\":\"%s\"", connectData.clientID.cstring );

		vGetCredentials( eConfigMqttUser, connectData.username.cstring );
		if( connectData.username.cstring != NULL )
			xCount += sprintf( pcBuffer + xCount, ",\"user\":\"%s\"", connectData.username.cstring );

		vGetCredentials( eConfigMqttPassWD, connectData.password.cstring );
		if( connectData.password.cstring != NULL )
			xCount += sprintf( pcBuffer + xCount, ",\"pwd\":\"%s\"", connectData.password.cstring );

		vGetCredentials( eConfigMqttWill, &(connectData.willFlag) );
		if( connectData.willFlag )
		{
			vGetCredentials( eConfigMqttWillTopic, connectData.will.topicName.cstring );
			vGetCredentials( eConfigMqttWillMsg, connectData.will.message.cstring );
			xCount += sprintf( pcBuffer + (xCount -1), ",\"will\":1,\"wtp\":\"%s\",\"wms\":\"%s\"", connectData.willFlag, connectData.will.message.cstring );
		}

		xCount += sprintf( pcBuffer + xCount, "}");

		return xCount;
	}
#endif


/***********************************************************************************************************
 *   function: xGetMQTTQueueHandle
 *   Purpose: Gives the handle to the MQTT queue
 *   Global Variables used: xMqttQueueHandle
 *   Returns: 	Pointer to the Mqtt Queue
 *   			NULL: MQTT is not initialized
 ***********************************************************************************************************/
QueueHandle_t xGetMQTTQueueHandle( void )
{
	return xMqttQueueHandle;
}


/***********************************************************************************************************
 *   function: xInitMQTT
 *   Purpose: Start MQTT-Queue and Task. Add RequestHandler. MQTT Connection will not be established at this point.
 *   Global Variables used: xMqttQueueHandle, xMqttTaskHandle
 *   Returns: 	Pointer to the Mqtt Queue
 *   			NULL - an Error occurred
 ***********************************************************************************************************/
QueueHandle_t xInitMQTT(void)
{
	BaseType_t xRet = 0;


	/* Bevore doing anything check if mqtt is already initialized */
	if(( xMqttQueueHandle != NULL ) || (xMqttTaskHandle != NULL) )
	{
		DEBUGOUT("MQTT-Error 0x0010: Initialization failed. Already initialized.\n");
		return NULL;
	}

	/* Init-Step 1 of 3: Initialise Queue */
	xMqttQueueHandle = xQueueCreate( MQTTQUEUE_LENGTH, MQTTQUEUE_ITEMSIZE );
	if( xMqttQueueHandle == NULL )
	{
		DEBUGOUT("MQTT-Error 0x0011: Initialization failed at getting queue.\n");
		return NULL;
	}

	/* Init-Step 2 of 3: Start Mqtt Task */
	xRet = xTaskCreate( vMQTTTask,					/* Task Code */
						"MqttTask",					/* Task Name */
						MQTTTASK_SIZE,				/* Task Stack Size */
						NULL,						/* Task Parameters */
						( tskIDLE_PRIORITY + 2 ),	/* Task Priority */
						&xMqttTaskHandle);			/* Task Handle */
	if(( xRet != pdPASS ) || ( xMqttTaskHandle == NULL ))
	{
		DEBUGOUT("MQTT-Error 0x0012: Initialization failed at starting task.\n");
		/* Do not forget to free the queue! Otherwise it a new initialization will not be able */
		vQueueDelete( xMqttQueueHandle );
		xMqttQueueHandle = NULL;
		return NULL;
	}

	#if( includeHTTP_DEMO != 0 )
		/* Init-Step 3 of 3: Start HTTP Request Handler */
		xRet = xAddRequestHandler( "mqtt", xMQTTRequestHandler );
		if( xRet != pdPASS )
			DEBUGOUT("MQTT-Warning: Failed to add HTTP-Request-Handler. Task will run with saved config.\n");
	#endif

	return xMqttQueueHandle;
}


/***********************************************************************************************************
 *   function: vDeinitMQTT
 *   Purpose: Stop the Mqtt Task, free the Queue, remove HTTP-Request-Handler.
 *   		  Use only to save memory, because you dont use MQTT at all, to end the connection use MQTT Task
 *   Global Variables used: xMqttQueueHandle, xMqttTaskHandle
 ***********************************************************************************************************/
void vDeinitMQTT( void )
{
	BaseType_t xRet = pdFAIL;
	MqttJob_t xKillJob;

	if(( xMqttQueueHandle != NULL ) || ( xMqttTaskHandle != NULL ))
	{
		/* Deinit-Step 1 of X: Send kill to MqttTask, set it to the top of the queue so no other Jobs will be done */
		xKillJob.eJobType = eKill;
		xKillJob.data = NULL;
		/* sent kill to queue, 2s timeout. _CD_ Task normally needs max 1s to do a Job, so assume it is unresponsive when 2s are passed */
		xRet = xQueueSendToBack( xMqttQueueHandle, &xKillJob, pdMS_TO_TICKS( 2000 ) );

		/* Deinit-Step 2 of X: Check if kill command was send to queue otherwise kill task manually */
		if( xRet == errQUEUE_FULL )
		{
			DEBUGOUT("MQTT-Error 0x0020: Send 'kill' to task failed. Task seems to be unresponsive, will delete it.\n");
			/* Task could not delete himself, so delete it */
			vTaskDelete( xMqttTaskHandle );
			xMqttTaskHandle = NULL;

			/* Clean up Queue */
			vQueueDelete( xMqttQueueHandle );
			xMqttQueueHandle = NULL;
		}
	}
	else if( xMqttTaskHandle != NULL )
	{
		/* No Queue available for mqtt task, so kill it. */
		vTaskDelete( xMqttTaskHandle );
		xMqttTaskHandle = NULL;
	}

	#if( includeHTTP_DEMO != 0 )
		/* Deinit-Step 3 of X: Remove HTTP request handler */
		xRet = xRemoveRequestHandler( "mqtt" );
		if( xRet != pdTRUE )
			DEBUGOUT("MQTT-Warning: Could not remove HTTP request handler from list.\n");
	#endif /* #if( includeHTTP_DEMO != 0 ) */

}
