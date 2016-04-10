#include "ImpedanceRtos.h"

char g_ThreadMainStack[THREAD_MAIN_STK_SIZE];
char g_ThreadUxStack[THREAD_UX_STK_SIZE];

int main(void) {
  INT8U OSRetVal;

  SystemInit();
  test_Init();
  adi_initpinmux();

  OSInit();

  // Create the main impedance thread.
  OSRetVal = OSTaskCreate(MainThreadRun, NULL,
                          (void*)(g_ThreadMainStack + THREAD_MAIN_STK_SIZE),
                          THREAD_MAIN_PRIO);
  if (OSRetVal != OS_ERR_NONE) {
    FAIL("Error creating the main thread.\n");
    return 1;
  }


  // Create the low-priority UX thread.
  OSRetVal = OSTaskCreate(UxThreadRun, NULL,
                          (void*)(g_ThreadUxStack + THREAD_UX_STK_SIZE),
                          THREAD_UX_PRIO);
  if (OSRetVal != OS_ERR_NONE) {
    FAIL("Error creating the blink thread.\n");
    return 1;
  }

  SysTick_Config(M3_FREQ / SYSTICKS_PER_SECOND);
  OSStart();

  /* Should never get here. */
  FAIL("Error starting the RTOS.\n");
  return 0;
}
