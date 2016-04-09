#include "ImpedanceRtos.h"

char g_Thread1Stack[THREAD1_STK_SIZE];
OS_TCB g_Thread1_TCB;

typedef struct {
  ADI_GPIO_PORT_TYPE Port;
  ADI_GPIO_DATA_TYPE Pins;
} PinMap;

uint32_t buffer[1];
uint32_t length;

PinMap led_DISPLAY = {ADI_GPIO_PORT_0, ADI_GPIO_PIN_11};
PinMap led_DISPLAY1 = {ADI_GPIO_PORT_4, ADI_GPIO_PIN_2};

void BlinkThreadRun(void* arg) {
  volatile uint32_t i;
  uint32_t count = 0;


  /* Initialize GPIO */
  if (ADI_GPIO_SUCCESS != adi_GPIO_Init()) {
    FAIL("adi_GPIO_Init");
  }

  /* Enable GPIO output drivers */
  if (ADI_GPIO_SUCCESS !=
      adi_GPIO_SetOutputEnable(led_DISPLAY.Port, led_DISPLAY.Pins, true)) {
    FAIL("adi_GPIO_SetOutputEnable (led_DISPLAY)");
  }

  /* Loop indefinitely */
  while (1) {
    /* Delay */
    for (i = 0; i < 50000; i++)
      ;
    
    if (++count % 2) {
      if (ADI_GPIO_SUCCESS !=
          adi_GPIO_SetLow(led_DISPLAY.Port, led_DISPLAY.Pins)) {
        FAIL("adi_GPIO_SetLow (led_DISPLAY)");
      }
    } else {
      if (ADI_GPIO_SUCCESS !=
          adi_GPIO_SetHigh(led_DISPLAY.Port, led_DISPLAY.Pins)) {
        FAIL("adi_GPIO_SetHigh (led_DISPLAY)");
      }
    }
  }
}

int main(void) {
  INT8U OSRetVal;

  SystemInit();
  test_Init();
  OSInit();

  // Create each of the threads.
  OSRetVal = OSTaskCreate(BlinkThreadRun, NULL,
                          (void*)(g_Thread1Stack + THREAD1_STK_SIZE),
                          THREAD1_PRIO);

  if (OSRetVal != OS_ERR_NONE) {
    FAIL("Error creating a thread.\n");
    return 1;
  }

  OSStart();

  /* Should never get here. */
  FAIL("Error starting the RTOS.\n");
  return 0;
}
