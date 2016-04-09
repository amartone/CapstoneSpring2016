#include "ImpedanceRtos.h"

char g_ThreadBlinkStack[THREAD_BLINK_STK_SIZE];
char g_ThreadMainStack[THREAD_MAIN_STK_SIZE];

int main(void) {
  INT8U OSRetVal;

  SystemInit();
  test_Init();
  adi_initpinmux();

  UX_LCD_Init();
  UX_LCD_ShowMessage("CAPSTONE"); // Must be 8 characters long.
  OSInit();

  // Create the main impedance thread.
  OSRetVal = OSTaskCreate(MainThreadRun, NULL,
                          (void*)(g_ThreadMainStack + THREAD_MAIN_STK_SIZE),
                          THREAD_MAIN_PRIO);
  if (OSRetVal != OS_ERR_NONE) {
    FAIL("Error creating the main thread.\n");
    return 1;
  }


  // Create the blink thread (can be used for non-essential UX stuff).
  OSRetVal = OSTaskCreate(BlinkThreadRun, NULL,
                          (void*)(g_ThreadBlinkStack + THREAD_BLINK_STK_SIZE),
                          THREAD_BLINK_PRIO);
  if (OSRetVal != OS_ERR_NONE) {
    FAIL("Error creating the blink thread.\n");
    return 1;
  }

  OSStart();

  /* Should never get here. */
  FAIL("Error starting the RTOS.\n");
  return 0;
}
