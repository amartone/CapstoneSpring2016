#include "ImpedanceRtos.h"

/*  Anything faster than ~30 kHz is physically non-realizable with 1 MHz pclk,
    which is desirable for low-power concerns.  With a 16 MHz pclk, faster I2C
    serial clocks are possible, at the cost of higher power consumption.
*/
#define I2C_MASTER_CLOCK 100000

/* Maximum I2C TX/RX buffer size. */
#define I2C_BUFFER_SIZE 3

/* Slave address of the Arduino pump module. */
#define I2C_PUMP_SLAVE_ADDRESS 0x77

/* Command send to the Arduino to ask for pressure measurement. */
#define READ_PRESSURE_COMMAND (char)0x42

/* First byte from the Arduino indicating that pressure is available. */
#define ARDUINO_PRESSURE_AVAILABLE (char)0x24

/* I2C buffers. */
uint8_t i2c_tx[I2C_BUFFER_SIZE];
uint8_t i2c_rx[I2C_BUFFER_SIZE];

void PumpThreadRun(void* arg) {
  // TODO
}