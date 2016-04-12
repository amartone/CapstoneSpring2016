#include "ImpedanceRtos.h"

#include "lcd.h"
#include "lcd_VIM828.h"

ADI_LCD_DEV_HANDLE hLCD;
volatile bool ux_is_engaged = false;

typedef struct {
  ADI_GPIO_PORT_TYPE Port;
  ADI_GPIO_DATA_TYPE Pins;
} PinMap;

uint32_t buffer[1];
uint32_t length;
PinMap led_DISPLAY = {ADI_GPIO_PORT_0, ADI_GPIO_PIN_11};

void UX_Button_Callback(void *pCBParam, uint32_t Event, void *pArg);
void UX_LCD_Callback(void *pCBParam, uint32_t nEvent, void *pArg);

// Capture P0.10 pushbutton interrupt.
void UX_Button_Init() {
  /* initialize GPIO driver */
  if (adi_GPIO_Init()) {
   FAIL("UX_Button_Init failed");
  }

  // Enable P0.10 input.
  if (adi_GPIO_SetInputEnable(ADI_GPIO_PORT_0, ADI_GPIO_PIN_10, true)) {
    FAIL("UX_Button_Init: adi_GPIO_SetInputEnable failed");
  }

  // Disable P0.10 output.
  if (adi_GPIO_SetOutputEnable(ADI_GPIO_PORT_0, ADI_GPIO_PIN_10, false)) {
    FAIL("UX_Button_Init: adi_GPIO_SetOutputEnable failed");
  }

  // Set P0.10 pullep enable.
  if (adi_GPIO_SetPullUpEnable(ADI_GPIO_PORT_0, ADI_GPIO_PIN_10, true)) {
    FAIL("UX_Button_Init: adi_GPIO_SetOutputEnable failed");
  }

  // Register the external interrupt callback.
  if(adi_GPIO_RegisterCallback(EINT8_IRQn, UX_Button_Callback, NULL)) {
    FAIL("UX_Button_Init: adi_GPIO_RegisterCallbackExtInt failed");
  }

  // Enable P0.10 as external interrupt.
  if (adi_GPIO_EnableIRQ(EINT8_IRQn,  ADI_GPIO_IRQ_RISING_EDGE)) {
    FAIL("UX_Button_Init: adi_GPIO_EnableIRQ failed");
  }

  // Release GPIO driver.
  if (adi_GPIO_UnInit()) {
    FAIL("adi_GPIO_Init failed");
  }
}

void UX_LCD_Init() {
  bool_t bVLCDState;

  // Initialise lcd driver, by passing the configuration structure.
  if (ADI_LCD_SUCCESS != adi_LCD_Init(ADI_LCD_DEVID_0, &hLCD)) {
    FAIL("adi_LCD_Init Failed");
  }

  // Install the callback handler.
  if (ADI_LCD_SUCCESS !=
      adi_LCD_RegisterCallback(hLCD, UX_LCD_Callback, NULL)) {
    FAIL("Install the callback handler failed (adi_LCD_RegisterCallback)");
  }

  // Enable the display.
  if (ADI_LCD_SUCCESS != adi_LCD_Enable(hLCD, true)) {
    FAIL("Enable LCD failed (adi_LCD_Enable)");
  }

  do {
    if (ADI_LCD_SUCCESS != adi_LCD_GetVLCDStatus(hLCD, &bVLCDState)) {
      FAIL("Charge pump failed to reach bias level (adi_LCD_GetVLCDStatus)");
    }
  } while (bVLCDState != true);
}

void UX_LCD_ShowMessage(const uint8_t *message) {
  if (ADI_LCDVIM828_SUCCESS !=
      adi_LCDVIM828_DisplayString(hLCD, ADI_LCD_SCREEN_0, message)) {
    FAIL("UX_LCD_ShowMessage: adi_LCDVIM828_DisplayString failed");
  }

  if (adi_LCD_SetContrast(hLCD, 15) != ADI_LCD_SUCCESS) {
    FAIL("Failed to set the LCD contrast");
  }
}

void UX_LCD_Callback(void *pCBParam, uint32_t nEvent, void *EventArg) {
  switch ((ADI_LCD_EVENT_TYPE)nEvent) {
    /* Charge pump good status went low. */
    case ADI_LCD_EVENT_CP_GD:
      FAIL("Charge pump good status went low");
      break;

    /* Frame boundary interrupt. */
    case ADI_LCD_EVENT_FRAME_BOUNDARY:
      break;
  }
}

void UX_Button_Callback(void *pCBParam, uint32_t nEvent, void *EventArg) {
  NVIC_ClearPendingIRQ(EINT8_IRQn);

  ux_is_engaged = true;
  printf("UX: button pressed. Signaling semaphore.\n");
  OSSemPost(ux_button_semaphore);
}

void UX_Disengage() {
  ux_is_engaged = false;
}

void UX_Task(void *arg) {
  volatile uint32_t i;
  uint32_t count = 0;

  // Initialize LCD display.
  UX_LCD_Init();
  UX_LCD_ShowMessage("CAPSTONE");  // Must be 8 characters long.

  // Initialize GPIO.
  if (ADI_GPIO_SUCCESS != adi_GPIO_Init()) {
    FAIL("adi_GPIO_Init");
  }

  // Enable GPIO output drivers.
  if (ADI_GPIO_SUCCESS !=
      adi_GPIO_SetOutputEnable(led_DISPLAY.Port, led_DISPLAY.Pins, true)) {
    FAIL("adi_GPIO_SetOutputEnable (led_DISPLAY)");
  }
  
  // Enable the pushbutton.
  UX_Button_Init();

  while (1) {
    if (++count % 2) {
      if (ADI_GPIO_SUCCESS !=
          adi_GPIO_SetLow(led_DISPLAY.Port, led_DISPLAY.Pins)) {
        FAIL("adi_GPIO_SetLow (led_DISPLAY)");
      }
      OSTimeDlyHMSM(0, 0, 0, 100);
    } else {
      if (ADI_GPIO_SUCCESS !=
          adi_GPIO_SetHigh(led_DISPLAY.Port, led_DISPLAY.Pins)) {
        FAIL("adi_GPIO_SetHigh (led_DISPLAY)");
      }
      OSTimeDlyHMSM(0, 0, 0, ux_is_engaged ? 200 : 1900);
    }
  }
}