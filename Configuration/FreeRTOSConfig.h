#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "chip.h"
//#include "netConfig.h"
//
///*-----------------------------------------------------------
// * Application specific definitions.
// *
// * These definitions should be adjusted for your particular hardware and
// * application requirements.
// *
// * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
// * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
// *----------------------------------------------------------*/
//
//#define configUSE_PREEMPTION		1
//#define configUSE_IDLE_HOOK			1
//#define configMAX_PRIORITIES		5
//#define configUSE_TICK_HOOK			1
//#define configCPU_CLOCK_HZ			( ( unsigned long ) 100000000 )
//#define configTICK_RATE_HZ			1000
//#define configMINIMAL_STACK_SIZE	( ( unsigned short ) 80 )
//#define configTOTAL_HEAP_SIZE		( ( size_t ) ( 19 * 1024 ) )
//#define configMAX_TASK_NAME_LEN		( 12 )
//#define configUSE_TRACE_FACILITY	1
//#define configUSE_16_BIT_TICKS		0
//#define configIDLE_SHOULD_YIELD		1
//#define configUSE_CO_ROUTINES 		0
//#define configUSE_MUTEXES			1
//
//#define configUSE_MALLOC_FAILED_HOOK 1
//
///* added defines for FreeRTOS timer usage */
//#define configUSE_TIMERS				1
//#define configTIMER_TASK_PRIORITY		configTIMER_PRIORITY
//#define configTIMER_QUEUE_LENGTH		3
//#define configTIMER_TASK_STACK_DEPTH	240
//
//#define configMAX_CO_ROUTINE_PRIORITIES ( 2 )
//
//#define configUSE_COUNTING_SEMAPHORES 	1
//#define configUSE_ALTERNATIVE_API 		0
//#define configCHECK_FOR_STACK_OVERFLOW	2
//#define configUSE_RECURSIVE_MUTEXES		1
//#define configQUEUE_REGISTRY_SIZE		10
//
///* Run time stats  */
//#define configGENERATE_RUN_TIME_STATS	1
//#define configUSE_STATS_FORMATTING_FUNCTIONS 1
//
///* Set the following definitions to 1 to include the API function, or zero
//to exclude the API function. */
//
//#define INCLUDE_vTaskPrioritySet			1
//#define INCLUDE_uxTaskPriorityGet			1
//#define INCLUDE_vTaskDelete					1
//#define INCLUDE_vTaskCleanUpResources		0
//#define INCLUDE_vTaskSuspend				1
//#define INCLUDE_vTaskDelayUntil				1
//#define INCLUDE_vTaskDelay					1
//#define INCLUDE_uxTaskGetStackHighWaterMark	1
//#define INCLUDE_xTaskGetSchedulerState      1
//
///*-----------------------------------------------------------
// * Ethernet configuration.
// *-----------------------------------------------------------*/
//
///* Use the system definition, if there is one */
//#ifdef __NVIC_PRIO_BITS
//	#define configPRIO_BITS       __NVIC_PRIO_BITS
//#else
//	#define configPRIO_BITS       5        /* 32 priority levels */
//#endif
//
///* The lowest priority. */
//#define configKERNEL_INTERRUPT_PRIORITY 	( 31 << (8 - configPRIO_BITS) )
///* Priority 5, or 160 as only the top three bits are implemented. */
//#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	( 5 << (8 - configPRIO_BITS) )
//
/* Priorities passed to NVIC_SetPriority() do not require shifting as the
function does the shifting itself.  Note these priorities need to be equal to
or lower than configMAX_SYSCALL_INTERRUPT_PRIORITY - therefore the numeric
value needs to be equal to or greater than 5 (on the Cortex-M3 the lower the
numeric value the higher the interrupt priority). */

#define configGPIO_INTERRUPT_PRIORITY		6
#define configDMA_INTERRUPT_PRIORITY		7
#define configSPI_INTERRUPT_PRIORITY		8
#define configSPI_INTERRUPT_TASK_PRIORITY	9
#define configETHERNET_INTERRUPT_PRIORITY	10
#define configUART0_INTERRUPT_PRIORITY		12
#define configUART1_INTERRUPT_PRIORITY		13
#define configUART2_INTERRUPT_PRIORITY		14
#define configUART3_INTERRUPT_PRIORITY		15
#define configMAIN_IDLE_TASK_PRIORITY		29
#define configTIMER_PRIORITY				3
//
///*-----------------------------------------------------------
// * Macros required to setup the timer for the run time stats.
// *-----------------------------------------------------------*/
//extern void vConfigureTimerForRunTimeStats( void );
//#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() vConfigureTimerForRunTimeStats()
////#define portGET_RUN_TIME_COUNTER_VALUE() LPC_TIM0->TC
//#define portGET_RUN_TIME_COUNTER_VALUE() Chip_TIMER_ReadCount(LPC_TIMER0)
//#endif /* FREERTOS_CONFIG_H */
//

/* Definitions that map the FreeRTOS port interrupt handlers to their CMSIS
standard names - or at least those used in the unmodified vector table. */
#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler
#define xPortSysTickHandler SysTick_Handler



/* Use the system definition, if there is one */
#ifdef __NVIC_PRIO_BITS
	#define configPRIO_BITS       __NVIC_PRIO_BITS
#else
	#define configPRIO_BITS       5        /* 32 priority levels */
#endif

#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      ( ( unsigned long ) 100000000 )/*TF was 60000000 2017-02-09*/
#define configTICK_RATE_HZ                      1000 /*TF was 250 2017-02-09*/
#define configMAX_PRIORITIES                    5
#define configMINIMAL_STACK_SIZE                ( ( unsigned short ) 80 )
#define configMAX_TASK_NAME_LEN                 12 /*TF was 16 2017-02-09*/
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS            1
#define configUSE_MUTEXES                       1 /*TF was 0 2017-02-09*/
#define configUSE_RECURSIVE_MUTEXES             1 /*TF was 0 2017-02-09*/
#define configUSE_COUNTING_SEMAPHORES           1 /*TF was 0*/
#define configUSE_ALTERNATIVE_API               0 /* Deprecated! */
#define configQUEUE_REGISTRY_SIZE               10
#define configUSE_QUEUE_SETS                    0
#define configUSE_TIME_SLICING                  0
#define configUSE_NEWLIB_REENTRANT              0
#define configENABLE_BACKWARD_COMPATIBILITY     1
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5

/* Memory allocation related definitions. */
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 19 * 1024 ) )
#define configAPPLICATION_ALLOCATED_HEAP        0

/* Hook function related definitions. */
#define configUSE_IDLE_HOOK                     1 /*TF was 0 2017-02-09*/
#define configUSE_TICK_HOOK                     1 /*TF was 0 2017-02-09*/
#define configCHECK_FOR_STACK_OVERFLOW          2 /*TF was 0 2017-02-09*/
#define configUSE_MALLOC_FAILED_HOOK            1 /*TF was 0 2017-02-09*/
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

/* Run time and task stats gathering related definitions. */
#define configGENERATE_RUN_TIME_STATS           1 /*TF was 0 2017-02-09*/
#define configUSE_TRACE_FACILITY                1 /*TF was 0 2017-02-09*/
#define configUSE_STATS_FORMATTING_FUNCTIONS    1 /*TF was 0 2017-02-09*/

/* Co-routine related definitions. */
#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES         2 /*TF was 1 2017-02-09*/

/* Software timer related definitions. */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               3
#define configTIMER_QUEUE_LENGTH                10 /*TF was 10 2017-02-09*/
#define configTIMER_TASK_STACK_DEPTH            240 /*TF was configMINIMAL_STACK_SIZE 2017-02-09*/

/* Interrupt nesting behaviour configuration. */
#define configKERNEL_INTERRUPT_PRIORITY         ( 31 << (8 - configPRIO_BITS) ) /* [dependent of processor] */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    ( 5 << (8 - configPRIO_BITS) ) /*[dependent on processor and application]*/
/*configMAX_API_CALL_INTERRUPT_PRIORITY is a new name for configMAX_SYSCALL_INTERRUPT_PRIORITY that is used by newer ports only.*/
#define configMAX_API_CALL_INTERRUPT_PRIORITY   configMAX_SYSCALL_INTERRUPT_PRIORITY /*[dependent on processor and application]*/

/* Define to trap errors during development. */
//#define configASSERT( ( x ) ) if( ( x ) == 0 ) vAssertCalled( __FILE__, __LINE__ )

/* FreeRTOS MPU specific definitions. */
#define configINCLUDE_APPLICATION_DEFINED_PRIVILEGED_FUNCTIONS 0

/* Optional functions - most linkers will remove unused functions anyway. */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xResumeFromISR                  1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetIdleTaskHandle          0
#define INCLUDE_eTaskGetState                   0
#define INCLUDE_xEventGroupSetBitFromISR        1
#define INCLUDE_xTimerPendFunctionCall          0
#define INCLUDE_xTaskAbortDelay                 0
#define INCLUDE_xTaskGetHandle                  0
#define INCLUDE_xTaskResumeFromISR              1


extern void vConfigureTimerForRunTimeStats( void );
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() vConfigureTimerForRunTimeStats()
#define portGET_RUN_TIME_COUNTER_VALUE() Chip_TIMER_ReadCount(LPC_TIMER0)

/* A header file that defines trace macro can be included here. */

/* MCUXpresso FreeRTOS Task Aware Debugger header.
See "MCUXpresso_IDE_FreeRTOS_Debug_Guide.pdf". */
#define configINCLUDE_FREERTOS_TASK_C_ADDITIONS_H 1
#define configFRTOS_MEMORY_SCHEME 4
#define configRECORD_STACK_HIGH_ADDRESS 1

#endif /* FREERTOS_CONFIG_H */
