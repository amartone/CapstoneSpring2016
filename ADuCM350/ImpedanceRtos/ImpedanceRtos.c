/*********************************************************************************

Copyright (c) 2013-2014 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.  By using 
this software you agree to the terms of the associated Analog Devices Software 
License Agreement.

*********************************************************************************/

/*****************************************************************************
 * @file:    RtosTest.c
 * @brief:   Verify that drivers build in RTOS mode
 * @version: $Revision: 28525 $
 * @date:    $Date: 2014-11-12 14:51:26 -0500 (Wed, 12 Nov 2014) $
 *****************************************************************************/

#include <time.h>
#include <stddef.h>  // for 'NULL'
#include <stdio.h>   // for scanf
#include <string.h>  // for strncmp

#include <ucos_ii.h>

#include "dma.h"
#include "system.h"
#include "rtc.h"
#include "wut.h"
#include "gpio.h"
#include "afe.h"
#include "beep.h"
#include "captouch.h"
#include "flash.h"
#include "i2c.h"
#include "i2s.h"
#include "lcd.h"
#include "rng.h"
#include "rtc.h"
#include "spi.h"
#include "uart.h"
#include "wdt.h"
#include "wut.h"
#include "gpio.h"


#include "test_common.h"



ADI_AFE_RESULT_TYPE  rafe;
ADI_AFE_DEV_HANDLE   hafe;
ADI_BEEP_RESULT_TYPE rbeep;
ADI_BEEP_HANDLE      hbeep;
ADI_CT_RESULT_TYPE   rct;
ADI_CT_DEV_HANDLE    hct;
ADI_FEE_RESULT_TYPE  rfee;
ADI_FEE_DEV_HANDLE   hfee;
ADI_I2S_RESULT_TYPE  ri2s;
ADI_I2S_DEV_HANDLE   hi2s;
ADI_LCD_RESULT_TYPE  rlcd;
ADI_LCD_DEV_HANDLE   hlcd;
ADI_I2C_RESULT_TYPE  ri2c;
ADI_I2C_DEV_HANDLE   hi2c;
ADI_RNG_RESULT_TYPE  rrng;
ADI_RNG_HANDLE       hrng;
ADI_RTC_RESULT_TYPE  rrtc;
ADI_RTC_HANDLE       hrtc;
ADI_SPI_RESULT_TYPE  rspi;
ADI_SPI_DEV_HANDLE   hspi;
ADI_UART_RESULT_TYPE ruart;
ADI_UART_HANDLE      huart;
ADI_WDT_RESULT_TYPE  rwdt;
ADI_WDT_DEV_HANDLE   hwdt;
ADI_WUT_RESULT_TYPE  rwut;
ADI_WUT_DEV_HANDLE   hwut;
                                   

uint32_t buffer[1];
uint32_t length;


/*
 * RTOS definitions
 */
#define THREAD1_STK_SIZE 200
#define THREAD1_PRIO 5
#define NUM_MSG 10
char    g_Thread1Stack[THREAD1_STK_SIZE];
OS_TCB  g_Thread1_TCB;

typedef struct {
    ADI_GPIO_PORT_TYPE Port;
    ADI_GPIO_DATA_TYPE Pins;
} PinMap;

PinMap led_DISPLAY  = {ADI_GPIO_PORT_0, ADI_GPIO_PIN_11};
PinMap led_DISPLAY1 = {ADI_GPIO_PORT_4, ADI_GPIO_PIN_2};


void Thread1Run(void* arg)
{
  
    volatile uint32_t   i;
    uint32_t            count = 0;
  
#if 0
    /* Change the system clock source to HFXTAL and change clock frequency to 16MHz     */
    /* Requirement for AFE (ACLK)                                                       */
    SystemTransitionClocks(ADI_SYS_CLOCK_TRIGGER_MEASUREMENT_ON);
    
    rafe = adi_AFE_Init(&hafe);
    if( rafe != ADI_AFE_SUCCESS)
    {
      FAIL("adi_AFE_Init failed");
    }
#endif
    
    /* Initialize GPIO */
    if (ADI_GPIO_SUCCESS != adi_GPIO_Init())
    {
        FAIL("adi_GPIO_Init");
    }

    /* Enable GPIO output drivers */
    if (ADI_GPIO_SUCCESS != adi_GPIO_SetOutputEnable(led_DISPLAY.Port, led_DISPLAY.Pins, true))
    {
        FAIL("adi_GPIO_SetOutputEnable (led_DISPLAY)");
    }
    if (ADI_GPIO_SUCCESS != adi_GPIO_SetOutputEnable(led_DISPLAY1.Port, led_DISPLAY1.Pins, true))
    {
        FAIL("adi_GPIO_SetOutputEnable (led_DISPLAY1)");
    }

	/* Loop indefinitely */
    while (1)
    {
		/* Delay */
		for (i = 0; i < 50000; i++)
            ;

		/* Flash the count using the LEDs */
		switch (count++)
        {
			case 3:
				if (ADI_GPIO_SUCCESS != adi_GPIO_SetLow(led_DISPLAY.Port,  led_DISPLAY.Pins))
                {
                    FAIL("adi_GPIO_SetLow (led_DISPLAY)");
                }
				if (ADI_GPIO_SUCCESS != adi_GPIO_SetLow(led_DISPLAY1.Port, led_DISPLAY1.Pins))
                {
                    FAIL("adi_GPIO_SetLow (led_DISPLAY1)");
                }
				break;

			case 2:
				if (ADI_GPIO_SUCCESS != adi_GPIO_SetHigh(led_DISPLAY.Port,  led_DISPLAY.Pins))
                {
                    FAIL("adi_GPIO_SetHigh (led_DISPLAY)");
                }
				if (ADI_GPIO_SUCCESS != adi_GPIO_SetLow(led_DISPLAY1.Port, led_DISPLAY1.Pins))
                {
                    FAIL("adi_GPIO_SetLow (led_DISPLAY1)");
                }
				break;

			case 1:
				if (ADI_GPIO_SUCCESS != adi_GPIO_SetLow(led_DISPLAY.Port,  led_DISPLAY.Pins))
                {
                    FAIL("adi_GPIO_SetLow (led_DISPLAY)");
                }
				if (ADI_GPIO_SUCCESS != adi_GPIO_SetHigh(led_DISPLAY1.Port, led_DISPLAY1.Pins))
                {
                    FAIL("adi_GPIO_SetHigh (led_DISPLAY1)");
                }
				break;

			case 0:
				if (ADI_GPIO_SUCCESS != adi_GPIO_SetHigh(led_DISPLAY.Port,  led_DISPLAY.Pins))
                {
                    FAIL("adi_GPIO_SetHigh (led_DISPLAY)");
                }
				if (ADI_GPIO_SUCCESS != adi_GPIO_SetHigh (led_DISPLAY1.Port, led_DISPLAY1.Pins))
                {
                    FAIL("adi_GPIO_SetHigh (led_DISPLAY1)");
                }
				break;

		}

		/* Apply module */
		count %= 4;
	}
}



int main (void)
{
    INT8U    OSRetVal;
  
  /* Initialize system */
    SystemInit();

    /* test system initialization */
    test_Init();
    
    
    OSInit();

    OSRetVal = OSTaskCreate (Thread1Run,
                NULL,
               (void *)(g_Thread1Stack + THREAD1_STK_SIZE),
               THREAD1_PRIO);
    
    if (OSRetVal != OS_ERR_NONE)
    {
    	FAIL("Error creating thread1\n");
    	return 1;
    }

    OSStart();
    
	/* should never get here... */
    FAIL("Error starting the RTOS\n");
    
    return 0;

}


