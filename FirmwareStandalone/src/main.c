/* LPCOpen includes. */
#include "board.h"
#include <cr_section_macros.h>

/* FreeRTOS includes. */
#include <FreeRTOS.h>
#include "task.h"

/* FreeRTOS +TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

/* GreenPHY SDK includes. */
#include "GreenPhySDKConfig.h"
#include "network.h"
#include "save_config.h"
#include "clickboard_config.h"

/*-----------------------------------------------------------*/
static void prvTestTask( void *pvParameters )
{
char cBuffer[16];
uint32_t ip;

	vTaskDelay( 10000 );
	ip = FreeRTOS_gethostbyname("google.de");

	FreeRTOS_inet_ntoa( ip, cBuffer );
	DEBUGOUT( "google.de IP Address: %s\r\n", cBuffer );

	vTaskDelete( NULL );
}

int main(void) {
	SystemCoreClockUpdate();
	Board_Init();

	DEBUGSTR("\r\n\r\nSTANDALONE ");
	{
		uint32_t reset_reason = LPC_SYSCTL->RSID;
		DEBUGOUT("RSID:0x%x", reset_reason);
		if (!reset_reason)
			DEBUGSTR("->Bootloader");
		if (reset_reason & 0x1)
			DEBUGSTR("->Power On");
		if (reset_reason & 0x2)
			DEBUGSTR("->Reset");
		if (reset_reason & 0x4)
			DEBUGSTR("->Watchdog");
		if (reset_reason & 0x8)
			DEBUGSTR("->BrownOut Detection");
		if (reset_reason & 0x10)
			DEBUGSTR("->JTAG/restart");
		DEBUGSTR("\r\n");
		LPC_SYSCTL->RSID = reset_reason;
	}

	vNetworkInit();

	xReadConfig();

	xClickboardsInit();

	xTaskCreate( prvTestTask, "Test", 240, NULL,  1, NULL );

	vTaskStartScheduler();

	return 0;
}
