/*
 **
 ** Source file generated on November 26, 2014 at 13:02:50.	
 **
 ** Copyright (C) 2014 Analog Devices Inc., All Rights Reserved.
 **
 ** This file is generated automatically based upon the options selected in 
 ** the Pin Multiplexing configuration editor. Changes to the Pin Multiplexing
 ** configuration should be made by changing the appropriate options rather
 ** than editing this file.
 **
 ** Selected Peripherals
 ** --------------------
 ** UART0 (Tx, Rx)
 **
 ** GPIO (unavailable)
 ** ------------------
 ** P0_06, P0_07
 */

#include "device.h"

#define UART0_TX_PORTP0_MUX  ((uint16_t) ((uint16_t) 2<<12))
#define UART0_RX_PORTP0_MUX  ((uint16_t) ((uint16_t) 2<<14))

#define I2C_ALT_SCL_PORTP4_MUX  ((uint16_t) ((uint16_t) 1<<0))
#define I2C_ALT_SDA_PORTP4_MUX  ((uint16_t) ((uint16_t) 1<<2))

int32_t adi_initpinmux(void);

/*
 * Initialize the Port Control MUX Registers
 */
int32_t adi_initpinmux(void) {
    /* Port Control MUX registers */
    *((volatile uint32_t *)REG_GPIO0_GPCON) = UART0_TX_PORTP0_MUX | UART0_RX_PORTP0_MUX;

    /* Port Control MUX registers */
    *((volatile uint32_t *)REG_GPIO4_GPCON) = I2C_ALT_SCL_PORTP4_MUX | I2C_ALT_SDA_PORTP4_MUX;

    return 0;
}

