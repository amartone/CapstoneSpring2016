#include "ImpedanceRtos.h"

#define HIGH_PRESSURE 159
#define LOW_PRESSURE 0
#define INTERVAL 3
#define INTERVAL_MS 1000
#define DELAY_TO_GET_TO_HIGH_MS 10000

#define MMHG_TO_TRANSDUCER(x) ((uint16_t) (2.4728x + 36.989 * x))

uint8_t i2c_tx[I2C_BUFFER_SIZE];

uint16_t targetValue;
ADI_I2C_RESULT_TYPE i2cResult;

void SetTargetPressure(uint16_t targetPressure) {
  printf("Setting target pressure to %d mmHg...\n", targetPressure);
  //targetValue = MMHG_TO_TRANSDUCER(targetPressure);
  i2c_tx[0] = READ_PRESSURE_COMMAND;
  i2c_tx[1] = targetPressure & 0xFFu;
  i2c_tx[2] = (targetPressure >> 8) & 0x3u;
  i2cResult = adi_I2C_MasterTransmit(i2cDevice, I2C_PUMP_SLAVE_ADDRESS, 0x0,
                                     ADI_I2C_8_BIT_DATA_ADDRESS_WIDTH, i2c_tx,
                                     3, false);
  if (i2cResult != ADI_I2C_SUCCESS) {
    FAIL("adi_I2C_MasterTransmit: send pressure command to Arduino");
  }
}

void PumpThreadRun(void* arg) {
  // Wait for the configuration to finish.
  OSMutexPost(i2c_mutex);

  int16_t targetMmhg;
  while (true) {
    // Wait two seconds to collect just some data.
    OSTimeDlyHMSM(0, 0, 2, 0);
    
    // Set the target pressure to the high pressure and sleep a bit.
    targetMmhg = HIGH_PRESSURE;
    SetTargetPressure(targetMmhg);
    OSTimeDlyHMSM(0, 0, 0, DELAY_TO_GET_TO_HIGH_MS);
    
    for (targetMmhg = HIGH_PRESSURE; targetMmhg >= LOW_PRESSURE;
         targetMmhg -= INTERVAL) {
      // Set the target pressure to be a bit lower.
      SetTargetPressure(targetMmhg);

      // Wait some time for the pressure to decrease.
      OSTimeDlyHMSM(0, 0, 0, INTERVAL_MS);
    }
  }
}