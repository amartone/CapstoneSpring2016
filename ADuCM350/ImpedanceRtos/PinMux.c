#include "device.h"

#define UART0_TX_PORTP0_MUX ((uint16_t)((uint16_t)2 << 12))
#define UART0_RX_PORTP0_MUX ((uint16_t)((uint16_t)2 << 14))

#define I2C_ALT_SCL_PORTP4_MUX ((uint16_t)((uint16_t)1 << 0))
#define I2C_ALT_SDA_PORTP4_MUX ((uint16_t)((uint16_t)1 << 2))

int32_t adi_initpinmux(void);

/*
 * Initialize the Port Control MUX Registers.
 */
int32_t adi_initpinmux(void) {
  /* Port Control MUX registers */
  *((volatile uint32_t *)REG_GPIO0_GPCON) =
      UART0_TX_PORTP0_MUX | UART0_RX_PORTP0_MUX;

  /* Port Control MUX registers */
  *((volatile uint32_t *)REG_GPIO4_GPCON) =
      I2C_ALT_SCL_PORTP4_MUX | I2C_ALT_SDA_PORTP4_MUX;

  return 0;
}
