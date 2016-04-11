#ifndef __IMPEDANCE_RTOS_H__
#define __IMPEDANCE_RTOS_H__

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "arm_math.h"

#include "adi_global_config.h"
#include "lib_cfg.h"
#include "os_cfg.h"
#include "test_common.h"

#include "afe.h"
#include "afe_lib.h"
#include "i2c.h"
#include "uart.h"

#include "gpio.h"
#include "rtc.h"
#include "wut.h"

#include <ucos_ii.h>

#include "afe.h"
#include "beep.h"
#include "captouch.h"
#include "dma.h"
#include "flash.h"
#include "gpio.h"
#include "gpio.h"
#include "i2c.h"
#include "i2s.h"
#include "lcd.h"
#include "rng.h"
#include "rtc.h"
#include "rtc.h"
#include "spi.h"
#include "system.h"
#include "uart.h"
#include "wdt.h"
#include "wut.h"

////////////////////////////////////////////////////////////////////////////////
// General RTOS/initialization. ////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define M3_FREQ 16000000
#define SYSTICKS_PER_SECOND 1000

#define TASK_MAIN_STK_SIZE 1024u
#define TASK_MAIN_PRIO 5
#define TASK_PUMP_STK_SIZE 128u
#define TASK_PUMP_PRIO 6
#define TASK_UX_STK_SIZE 128u
#define TASK_UX_PRIO 8

extern ADI_I2C_DEV_HANDLE i2cDevice;
extern OS_EVENT *i2c_mutex;

extern int32_t adi_initpinmux(void);


////////////////////////////////////////////////////////////////////////////////
// User Experience (implemented in UX.c) ///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

extern void UX_LCD_Init();
extern void UX_LCD_ShowMessage(const uint8_t *message);
extern void UX_Task(void *arg);

////////////////////////////////////////////////////////////////////////////////
// Pump thread (implemented in PumpThread.c). //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// Anything faster than ~30 kHz is physically non-realizable with 1 MHz pclk,
// which is desirable for low-power concerns.  With a 16 MHz pclk, faster I2C
// serial clocks are possible, at the cost of higher power consumption.
#define I2C_MASTER_CLOCK 100000

// Maximum I2C TX/RX buffer size.
#define I2C_BUFFER_SIZE 3

// Slave address of the Arduino pump module.
#define I2C_PUMP_SLAVE_ADDRESS 0x77

// Command send to the Arduino to ask for pressure measurement.
#define READ_PRESSURE_COMMAND (char) 0x42

// First byte from the Arduino indicating that pressure is available.
#define ARDUINO_PRESSURE_AVAILABLE (char) 0x24

extern void PumpThreadRun(void *arg);

////////////////////////////////////////////////////////////////////////////////
// Data export thread (implemented in TransmitDataThread.c). ///////////////////
////////////////////////////////////////////////////////////////////////////////

extern void TransmitDataThreadRun(void *arg);

////////////////////////////////////////////////////////////////////////////////
// Impedance thread (implemented in MainThread.c). /////////////////////////////
////////////////////////////////////////////////////////////////////////////////

extern void MainThreadRun(void *arg);

////////////////////////////////////////////////////////////////////////////////
// Miscellaneous functions. ////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static void delay(uint32_t count) {
  while (count > 0) {
    count--;
  }
}

#endif  // __IMPEDANCE_RTOS_H__