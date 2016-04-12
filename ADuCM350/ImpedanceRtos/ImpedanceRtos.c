#include "ImpedanceRtos.h"

OS_STK g_TaskMainStack[TASK_MAIN_STK_SIZE];
OS_STK g_TaskPumpStack[TASK_PUMP_STK_SIZE];
OS_STK g_TaskUxStack[TASK_UX_STK_SIZE];

OS_EVENT *i2c_mutex;
OS_EVENT *ux_button_semaphore;

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

  // Create a mutex indicating that I2C is initialized.
  i2c_mutex = OSMutexCreate(1, &OSRetVal);
  if (OSRetVal != OS_ERR_NONE) {
    FAIL("Error creating the I2C mutex\n");
  }
  
  // Create a semaphore indicating that the button was pressed.
  ux_button_semaphore = OSSemCreate(0);
  if (ux_button_semaphore == (void *) 0) {
    FAIL("Error creating the button semaphore\n");
  }

  // Create the main task.
  OSRetVal = OSTaskCreate(MainTask, NULL,
                          (OS_STK*)&g_TaskMainStack[TASK_MAIN_STK_SIZE - 1],
                          TASK_MAIN_PRIO);
  if (OSRetVal != OS_ERR_NONE) {
    FAIL("Error creating the main task.\n");
  }

  // Create the task in charge of interfacing with the pump.
  OSRetVal = OSTaskCreate(PumpTask, NULL,
                          (OS_STK*)&g_TaskPumpStack[TASK_PUMP_STK_SIZE - 1],
                          TASK_PUMP_PRIO);
  if (OSRetVal != OS_ERR_NONE) {
    FAIL("Error creating the pump task.\n");
  }

  // Create the low-priority UX task.
  OSRetVal =
      OSTaskCreate(UX_Task, NULL, (OS_STK*)&g_TaskUxStack[TASK_UX_STK_SIZE - 1],
                   TASK_UX_PRIO);
  if (OSRetVal != OS_ERR_NONE) {
    FAIL("Error creating the UX task.\n");
  }

  SysTick_Config(M3_FREQ / SYSTICKS_PER_SECOND);
  OSStart();

  // Should never get here.
  FAIL("Error starting the RTOS.\n");
  return 0;
}
