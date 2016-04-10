#include "ImpedanceRtos.h"

OS_STK g_TaskMainStack[TASK_MAIN_STK_SIZE];
OS_STK g_TaskUxStack[TASK_UX_STK_SIZE];

int main(void) {
  INT8U OSRetVal;

  SystemInit();

  // Change the system clock source to HFXTAL and change clock frequency to
  // 16MHz. Requirement for AFE (ACLK).
  SystemTransitionClocks(ADI_SYS_CLOCK_TRIGGER_MEASUREMENT_ON);

  // SPLL with 32MHz used, need to divide by 2.
  SetSystemClockDivider(ADI_SYS_CLOCK_UART, 2);

  test_Init();
  adi_initpinmux();

  OSInit();

  // Create the main thread.
  OSRetVal = OSTaskCreate(MainThreadRun, NULL,
                          (OS_STK*)&g_TaskMainStack[TASK_MAIN_STK_SIZE - 1],
                          TASK_MAIN_PRIO);
  if (OSRetVal != OS_ERR_NONE) {
    FAIL("Error creating the main thread.\n");
    return 1;
  }

  // Create the low-priority UX task.
  OSRetVal =
      OSTaskCreate(UX_Task, NULL, (OS_STK*)&g_TaskUxStack[TASK_UX_STK_SIZE - 1],
                   TASK_UX_PRIO);
  if (OSRetVal != OS_ERR_NONE) {
    FAIL("Error creating the blink thread.\n");
    return 1;
  }

  SysTick_Config(M3_FREQ / SYSTICKS_PER_SECOND);
  OSStart();

  // Should never get here.
  FAIL("Error starting the RTOS.\n");
  return 0;
}
