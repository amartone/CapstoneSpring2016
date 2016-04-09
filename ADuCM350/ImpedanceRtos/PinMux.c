#include "device.h"

// UART.
#define UART0_TX_PORTP0_MUX ((uint16_t)((uint16_t)2 << 12))
#define UART0_RX_PORTP0_MUX ((uint16_t)((uint16_t)2 << 14))

// I2C to Arduino pump.
#define I2C_ALT_SCL_PORTP4_MUX ((uint16_t)((uint16_t)1 << 0))
#define I2C_ALT_SDA_PORTP4_MUX ((uint16_t)((uint16_t)1 << 2))

// LCD display.
#define UART0_ALT_TX_PORTP3_MUX  ((uint16_t) ((uint16_t) 1<<12))
#define UART0_ALT_RX_PORTP3_MUX  ((uint16_t) ((uint16_t) 1<<14))
#define LCD_S1_PORTP2_MUX  ((uint16_t) ((uint16_t) 1<<8))
#define LCD_S2_PORTP2_MUX  ((uint16_t) ((uint16_t) 1<<10))
#define LCD_S3_PORTP1_MUX  ((uint16_t) ((uint16_t) 1<<0))
#define LCD_S4_PORTP1_MUX  ((uint16_t) ((uint16_t) 1<<2))
#define LCD_S5_PORTP1_MUX  ((uint16_t) ((uint16_t) 1<<4))
#define LCD_S6_PORTP1_MUX  ((uint16_t) ((uint16_t) 1<<6))
#define LCD_S7_PORTP1_MUX  ((uint16_t) ((uint16_t) 1<<8))
#define LCD_S8_PORTP1_MUX  ((uint16_t) ((uint16_t) 1<<10))
#define LCD_S9_PORTP1_MUX  ((uint16_t) ((uint16_t) 1<<12))
#define LCD_S10_PORTP1_MUX  ((uint16_t) ((uint16_t) 1<<14))
#define LCD_S11_PORTP1_MUX  ((uint32_t) ((uint32_t) 1<<16))
#define LCD_S12_PORTP1_MUX  ((uint32_t) ((uint32_t) 1<<18))
#define LCD_S13_PORTP1_MUX  ((uint32_t) ((uint32_t) 1<<20))
#define LCD_S14_PORTP1_MUX  ((uint32_t) ((uint32_t) 1<<22))
#define LCD_S15_PORTP1_MUX  ((uint32_t) ((uint32_t) 1<<24))
#define LCD_S16_PORTP1_MUX  ((uint32_t) ((uint32_t) 1<<26))
#define LCD_S17_PORTP1_MUX  ((uint32_t) ((uint32_t) 1<<28))
#define LCD_S18_PORTP1_MUX  ((uint32_t) ((uint32_t) 1<<30))
#define LCD_S19_PORTP2_MUX  ((uint16_t) ((uint16_t) 1<<12))
#define LCD_S20_PORTP2_MUX  ((uint16_t) ((uint16_t) 1<<14))
#define LCD_S21_PORTP2_MUX  ((uint32_t) ((uint32_t) 1<<16))
#define LCD_S22_PORTP2_MUX  ((uint32_t) ((uint32_t) 1<<18))
#define LCD_S23_PORTP2_MUX  ((uint32_t) ((uint32_t) 1<<20))
#define LCD_S24_PORTP2_MUX  ((uint32_t) ((uint32_t) 1<<22))
#define LCD_S25_PORTP2_MUX  ((uint32_t) ((uint32_t) 1<<24))
#define LCD_S26_PORTP2_MUX  ((uint32_t) ((uint32_t) 1<<26))
#define LCD_S27_PORTP2_MUX  ((uint32_t) ((uint32_t) 1<<28))
#define LCD_S28_PORTP2_MUX  ((uint32_t) ((uint32_t) 1<<30))
#define LCD_S29_PORTP3_MUX  ((uint32_t) ((uint32_t) 1<<16))
#define LCD_S30_PORTP3_MUX  ((uint32_t) ((uint32_t) 1<<18))
#define LCD_S31_PORTP3_MUX  ((uint32_t) ((uint32_t) 1<<20))
#define LCD_S32_PORTP3_MUX  ((uint32_t) ((uint32_t) 1<<22))
#define LCD_BPLANE0_PORTP2_MUX  ((uint16_t) ((uint16_t) 1<<0))
#define LCD_BPLANE1_PORTP2_MUX  ((uint16_t) ((uint16_t) 1<<2))
#define LCD_BPLANE2_PORTP2_MUX  ((uint16_t) ((uint16_t) 1<<4))
#define LCD_BPLANE3_PORTP2_MUX  ((uint16_t) ((uint16_t) 1<<6))

int32_t adi_initpinmux(void);

/*
 * Initialize the Port Control MUX Registers.
 */
int32_t adi_initpinmux(void) {
  /* Port Control MUX registers for UART connection. */
  *((volatile uint32_t *)REG_GPIO0_GPCON) =
      UART0_TX_PORTP0_MUX | UART0_RX_PORTP0_MUX;

  /* Port Control MUX registers for I2C connection to Arduino. */
  *((volatile uint32_t *)REG_GPIO4_GPCON) =
      I2C_ALT_SCL_PORTP4_MUX | I2C_ALT_SDA_PORTP4_MUX;

  /* Port Control MUX registers for LCD display. */
  *((volatile uint32_t *)REG_GPIO1_GPCON) = LCD_S3_PORTP1_MUX | LCD_S4_PORTP1_MUX
   | LCD_S5_PORTP1_MUX | LCD_S6_PORTP1_MUX | LCD_S7_PORTP1_MUX
   | LCD_S8_PORTP1_MUX | LCD_S9_PORTP1_MUX | LCD_S10_PORTP1_MUX
   | LCD_S11_PORTP1_MUX | LCD_S12_PORTP1_MUX | LCD_S13_PORTP1_MUX
   | LCD_S14_PORTP1_MUX | LCD_S15_PORTP1_MUX | LCD_S16_PORTP1_MUX
   | LCD_S17_PORTP1_MUX | LCD_S18_PORTP1_MUX;
  *((volatile uint32_t *)REG_GPIO2_GPCON) = LCD_S1_PORTP2_MUX | LCD_S2_PORTP2_MUX
   | LCD_S19_PORTP2_MUX | LCD_S20_PORTP2_MUX | LCD_S21_PORTP2_MUX
   | LCD_S22_PORTP2_MUX | LCD_S23_PORTP2_MUX | LCD_S24_PORTP2_MUX
   | LCD_S25_PORTP2_MUX | LCD_S26_PORTP2_MUX | LCD_S27_PORTP2_MUX
   | LCD_S28_PORTP2_MUX | LCD_BPLANE0_PORTP2_MUX | LCD_BPLANE1_PORTP2_MUX
   | LCD_BPLANE2_PORTP2_MUX | LCD_BPLANE3_PORTP2_MUX;
  *((volatile uint32_t *)REG_GPIO3_GPCON) = UART0_ALT_TX_PORTP3_MUX | UART0_ALT_RX_PORTP3_MUX
   | LCD_S29_PORTP3_MUX | LCD_S30_PORTP3_MUX | LCD_S31_PORTP3_MUX
   | LCD_S32_PORTP3_MUX;
  return 0;
}
