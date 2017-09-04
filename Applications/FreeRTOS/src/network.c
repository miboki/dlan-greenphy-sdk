/* LPCOpen includes. */
#include "board.h"

/* FreeRTOS includes. */
#include <FreeRTOS.h>
#include "task.h"

/* FreeRTOS +TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_Routing.h"
#include "FreeRTOS_TCP_server.h"

/* GreenPHY SDK includes. */
#include "GreenPhySDKNetConfig.h"
#include "network.h"
#include "mqtt.h"


/* Verify network configuration is sane. */
#if( ( netconfigUSE_BRIDGE == 0 ) && ( netconfigUSE_IP != 0 ) && ( netconfigIP_INTERFACE == netconfigBRIDGE_INTERFACE ) )
	#error "netconfigBRIDGE_INTERFACE used for IP stack, but netconfigUSE_BRIDGE not defined"
#endif

#if( ( netconfigUSE_IP != 0 ) && ( netconfigIP_INTERFACE != netconfigETH_INTERFACE ) \
                              && ( netconfigIP_INTERFACE != netconfigPLC_INTERFACE ) \
                              && ( netconfigIP_INTERFACE != netconfigBRIDGE_INTERFACE ) )
	#error "netconfigUSE_IP defined, but no netconfigIP_INTERFACE"
#endif

/* Storage for the network interfaces in use. */
#if( ( netconfigUSE_BRIDGE != 0 ) || ( ( netconfigUSE_IP != 0 ) && ( netconfigIP_INTERFACE == netconfigETH_INTERFACE ) ) )
	static NetworkInterface_t xEthInterface    = { 0 };
#endif
#if( ( netconfigUSE_BRIDGE != 0 ) || ( ( netconfigUSE_IP != 0 ) && ( netconfigIP_INTERFACE == netconfigPLC_INTERFACE ) ) )
	static NetworkInterface_t xPlcInterface    = { 0 };
#endif
#if( netconfigUSE_BRIDGE != 0 )
	static NetworkInterface_t xBridgeInterface = { 0 };
#endif

#if( netconfigUSE_IP != 0 )
	static NetworkEndPoint_t  xEndPoint        = { 0 };

	static const uint8_t ucIPAddress[ 4 ]        = { netconfigIP_ADDR0,         netconfigIP_ADDR1,         netconfigIP_ADDR2,         netconfigIP_ADDR3 };
	static const uint8_t ucNetMask[ 4 ]          = { netconfigNET_MASK0,        netconfigNET_MASK1,        netconfigNET_MASK2,        netconfigNET_MASK3 };
	static const uint8_t ucGatewayAddress[ 4 ]   = { netconfigGATEWAY_ADDR0,    netconfigGATEWAY_ADDR1,    netconfigGATEWAY_ADDR2,    netconfigGATEWAY_ADDR3 };
	static const uint8_t ucDNSServerAddress[ 4 ] = { netconfigDNS_SERVER_ADDR0, netconfigDNS_SERVER_ADDR1, netconfigDNS_SERVER_ADDR2, netconfigDNS_SERVER_ADDR3 };

	const uint8_t ucMACAddress[ 6 ] = { netconfigMAC_ADDR0, netconfigMAC_ADDR1, netconfigMAC_ADDR2, netconfigMAC_ADDR3, netconfigMAC_ADDR4, netconfigMAC_ADDR5 };
#endif

/*-----------------------------------------------------------*/

#if( netconfigUSE_IP != 0 )
	static void prvServerWorkTask( void *pvParameters )
	{
	const TickType_t xInitialBlockTime = pdMS_TO_TICKS( 200UL );
	TCPServer_t *pxTCPServer = NULL;

	/* A structure that defines the servers to be created.  Which servers are
	included in the structure depends on the mainCREATE_HTTP_SERVER and
	mainCREATE_FTP_SERVER settings at the top of this file. */
	static const struct xSERVER_CONFIG xServerConfiguration[] =
	{
		/* Server type,		port number,	backlog, 	root dir. */
		{ eSERVER_HTTP, 	80, 			0, 		"" }
	};

		/* Remove compiler warning about unused parameter. */
		( void ) pvParameters;

		/* Create the servers defined by the xServerConfiguration array above. */
		pxTCPServer = FreeRTOS_CreateTCPServer( xServerConfiguration, sizeof( xServerConfiguration ) / sizeof( xServerConfiguration[ 0 ] ) );
		configASSERT( pxTCPServer );

		for( ;; )
		{
			FreeRTOS_TCPServerWork( pxTCPServer, xInitialBlockTime );
		}
	}
#endif
/*-----------------------------------------------------------*/

void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent, NetworkEndPoint_t *pxEndPoint  ) {
	#if( netconfigUSE_IP != 0 )
	{
	uint32_t ulIPAddress, ulNetMask, ulGatewayAddress, ulDNSServerAddress;
	static BaseType_t xTasksAlreadyCreated = pdFALSE;
	int8_t cBuffer[ 16 ];

		/* Check this was a network up event, as opposed to a network down event. */
		DEBUGOUT("Network hook\r\n");
		if( eNetworkEvent == eNetworkUp )
		{
			DEBUGOUT("Network up\r\n");
			/* Create the tasks that use the TCP/IP stack if they have not already been
			created. */
			if( xTasksAlreadyCreated == pdFALSE )
			{
				/*
				 * Create the tasks here.
				 */

			#if( netconfigUSEMQTT != 0 )
				/* _CD_ Initialize MQTT:
				 * 	-> Get TCP socket,
				 * 	-> establish connection to broker,
				 * 	-> start task to receive MQTT packages. */
				xTaskCreate( vInitMqttTask,
							 "MqttStarup",
							 240,
							 NULL,
							 ( tskIDLE_PRIORITY + 1 ),
							 NULL);
			#endif /* #if( netconfigUSEMQTT != 0 ) */


				#define	mainTCP_SERVER_STACK_SIZE						240 /* Not used in the Win32 simulator. */

				xTaskCreate( prvServerWorkTask, "SvrWork", mainTCP_SERVER_STACK_SIZE, NULL, ipconfigIP_TASK_PRIORITY - 1, NULL );


				xTasksAlreadyCreated = pdTRUE;
			}

			/* The network is up and configured.  Print out the configuration,
			which may have been obtained from a DHCP server. */
			FreeRTOS_GetAddressConfiguration( pxEndPoint,
											  &ulIPAddress,
											  &ulNetMask,
											  &ulGatewayAddress,
											  &ulDNSServerAddress );

			/* Convert the IP address to a string then print it out. */
			FreeRTOS_inet_ntoa( ulIPAddress, cBuffer );
			DEBUGOUT( "IP Address: %s\r\n", cBuffer );

			/* Convert the net mask to a string then print it out. */
			FreeRTOS_inet_ntoa( ulNetMask, cBuffer );
			DEBUGOUT( "Subnet Mask: %s\r\n", cBuffer );

			/* Convert the IP address of the gateway to a string then print it out. */
			FreeRTOS_inet_ntoa( ulGatewayAddress, cBuffer );
			DEBUGOUT( "Gateway IP Address: %s\r\n", cBuffer );

			/* Convert the IP address of the DNS server to a string then print it out. */
			FreeRTOS_inet_ntoa( ulDNSServerAddress, cBuffer );
			DEBUGOUT( "DNS server IP Address: %s\r\n", cBuffer );
		}
	}
	#endif /* netconfigUSE_IP */
}
/*-----------------------------------------------------------*/

const char *pcApplicationHostnameHook( void )
{
	return netconfigHOSTNAME;
}
/*-----------------------------------------------------------*/

/*_HT_ introduced this memory-check temporarily for debugging. */
BaseType_t xApplicationMemoryPermissions( uint32_t aAddress )
{
	return 0x03;
}
/*-----------------------------------------------------------*/

void *vNetworkInit( void )
{
	#if( netconfigUSE_BRIDGE != 0 )
	{
		extern NetworkInterface_t *pxBridge_FillInterfaceDescriptor( BaseType_t xIndex, NetworkInterface_t *pxInterface );
		pxBridge_FillInterfaceDescriptor(0, &xBridgeInterface);
		FreeRTOS_AddNetworkInterface(&xBridgeInterface);
	}
	#endif

	#if( ( netconfigUSE_BRIDGE != 0 ) || ( ( netconfigUSE_IP != 0 ) && ( netconfigIP_INTERFACE == netconfigETH_INTERFACE ) ) )
		extern NetworkInterface_t *pxLPC1758_FillInterfaceDescriptor( BaseType_t xIndex, NetworkInterface_t *pxInterface );
		pxLPC1758_FillInterfaceDescriptor(0, &xEthInterface);
		#if( netconfigUSE_BRIDGE != 0 )
		{
			xEthInterface.bits.bIsBridged = 1;
		}
		#endif
		FreeRTOS_AddNetworkInterface(&xEthInterface);
	#endif

	#if( ( netconfigUSE_BRIDGE != 0 ) || ( ( netconfigUSE_IP != 0 ) && ( netconfigIP_INTERFACE == netconfigPLC_INTERFACE ) ) )
		extern NetworkInterface_t *pxQCA7000_FillInterfaceDescriptor( BaseType_t xIndex, NetworkInterface_t *pxInterface );
		pxQCA7000_FillInterfaceDescriptor(0, &xPlcInterface);
		#if( netconfigUSE_BRIDGE != 0)
		{
			xPlcInterface.bits.bIsBridged = 1;
		}
		#endif
		FreeRTOS_AddNetworkInterface(&xPlcInterface);
	#endif

	#if( netconfigUSE_IP != 0 )
		FreeRTOS_FillEndPoint(&xEndPoint, ucIPAddress, ucNetMask, ucGatewayAddress, ucDNSServerAddress, ucMACAddress);
		xEndPoint.bits.bIsDefault = pdTRUE_UNSIGNED;
		#if( netconfigUSE_DHCP != 0 )
		{
			xEndPoint.bits.bWantDHCP = pdTRUE_UNSIGNED;
		}
		#endif
		#if( netconfigIP_INTERFACE == netconfigETH_INTERFACE )
		{
			FreeRTOS_AddEndPoint(&xEthInterface, &xEndPoint);
		}
		#elif( netconfigIP_INTERFACE == netconfigPLC_INTERFACE )
		{
			FreeRTOS_AddEndPoint(&xPlcInterface, &xEndPoint);
		}
		#elif( netconfigIP_INTERFACE == netconfigBRIDGE_INTERFACE )
		{
			FreeRTOS_AddEndPoint(&xBridgeInterface, &xEndPoint);
		}
		#endif /* netconfigIP_INTERFACE */
	#endif /* netconfigUSE_IP */

	FreeRTOS_IPStart();
}
/*-----------------------------------------------------------*/
