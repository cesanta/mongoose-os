/*
 * Copyright (c) 2014-2019 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once  // no_extern_c_check

#define configTICK_RATE_HZ ((TickType_t) 100)
#define configUSE_NEWLIB_REENTRANT 0
#define configMINIMAL_STACK_SIZE 256 /* x 4 = 1024 */
#define configMAX_PRIORITIES 25
#define configUSE_PREEMPTION 1
#define configUSE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0
#define configUSE_16_BIT_TICKS 0

// We enable everything, unused functions will be GC'd.
#define configUSE_CO_ROUTINES 1
#define configMAX_CO_ROUTINE_PRIORITIES 10
#define INCLUDE_vTaskPrioritySet 1
#define INCLUDE_uxTaskPriorityGet 1
#define INCLUDE_vTaskDelete 1
#define INCLUDE_vTaskSuspend 1
#define INCLUDE_vTaskDelayUntil 1
#define INCLUDE_vTaskDelay 1
#define INCLUDE_xTaskGetIdleTaskHandle 1
#define INCLUDE_xTaskAbortDelay 1
#define INCLUDE_xQueueGetMutexHolder 1
#define INCLUDE_xTaskGetHandle 1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define INCLUDE_uxTaskGetStackHighWaterMark2 1
#define INCLUDE_eTaskGetState 1
#define INCLUDE_xTaskResumeFromISR 1
#define INCLUDE_xTimerPendFunctionCall 1
#define INCLUDE_xTaskGetSchedulerState 1
#define INCLUDE_xTaskGetCurrentTaskHandle 1

#define configUSE_DAEMON_TASK_STARTUP_HOOK 0
#define configUSE_APPLICATION_TASK_TAG 0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 0
#define configUSE_RECURSIVE_MUTEXES 1
#define configUSE_MUTEXES 1
#define configUSE_TIMERS 1
#define configUSE_COUNTING_SEMAPHORES 1
#define portCRITICAL_NESTING_IN_TCB 0
#define configMAX_TASK_NAME_LEN 16

// Less than MGOS task (5)
#define configTIMER_TASK_PRIORITY 2
#define configTIMER_QUEUE_LENGTH 10
#define configTIMER_TASK_STACK_DEPTH configMINIMAL_STACK_SIZE

#define configUSE_MALLOC_FAILED_HOOK 0
#define configUSE_TICKLESS_IDLE 0
#define configUSE_TRACE_FACILITY 1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#define configAPPLICATION_ALLOCATED_HEAP 0
#define configUSE_TASK_NOTIFICATIONS 1
#define configUSE_POSIX_ERRNO 0
#define configSUPPORT_STATIC_ALLOCATION 1
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configSTACK_DEPTH_TYPE uint16_t
#define configENABLE_BACKWARD_COMPATIBILITY 1
#define configUSE_TASK_FPU_SUPPORT 1
#define configENABLE_MPU 0
#define configENABLE_FPU 1
#define configENABLE_TRUSTZONE 0
#define configRUN_FREERTOS_SECURE_ONLY 0

// For ARM ports
#ifdef configPRIO_BITS
#define configKERNEL_INTERRUPT_PRIORITY \
  (((1 << configPRIO_BITS) - 1) << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
  (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#endif

extern void vPortSVCHandler(void);
extern void xPortPendSVHandler(void);
extern void xPortSysTickHandler(void);
