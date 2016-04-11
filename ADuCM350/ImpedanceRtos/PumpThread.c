#include "ImpedanceRtos.h"

#define HIGH_PRESSURE 159
#define LOW_PRESSURE 0
#define INTERVAL 3
#define INTERVAL_DELAY_S 1
#define DELAY_TO_GET_TO_HIGH_S 10

uint16_t mmhg_to_transducer(int32_t pressure) {
  // Cortex M3 has no hardware FP (?), so use an approximation instead.
  // return (uint16_t) (2.4728 * pressure + 36.989);
  return (uint16_t) (pressure * 591 / 239 + 37);
}

uint8_t i2c_tx[I2C_BUFFER_SIZE];

uint16_t targetValue;
ADI_I2C_RESULT_TYPE i2cResult;

void SetTargetPressure(int32_t targetPressure) {
  targetValue = mmhg_to_transducer(targetPressure);
  i2c_tx[0] = READ_PRESSURE_COMMAND;
  i2c_tx[1] = targetValue & 0xFFu;
  i2c_tx[2] = (targetValue >> 8) & 0x3u;
  i2cResult = adi_I2C_MasterTransmit(i2cDevice, I2C_PUMP_SLAVE_ADDRESS, 0x0,
                                     ADI_I2C_8_BIT_DATA_ADDRESS_WIDTH, i2c_tx,
                                     3, false);
  if (i2cResult != ADI_I2C_SUCCESS) {
    FAIL("adi_I2C_MasterTransmit: send pressure command to Arduino");
  }
}

void PumpThreadRun(void* arg) {
  uint8_t err;

  // Wait for the configuration to finish.
  OSMutexPend(i2c_mutex, 0, &err);
  if (err) {
    FAIL("OSMutexPend PumpThread");
  }

  int32_t targetMmhg;
  while (true) {
    // Wait two seconds to collect just some data.
    OSTimeDlyHMSM(0, 0, 2, 0);

    // Set the target pressure to the high pressure and sleep a bit.
    targetMmhg = HIGH_PRESSURE;
    SetTargetPressure(targetMmhg);
    OSTimeDlyHMSM(0, 0, DELAY_TO_GET_TO_HIGH_S, 0);
    
    for (targetMmhg = HIGH_PRESSURE - INTERVAL; targetMmhg >= LOW_PRESSURE;
         targetMmhg -= INTERVAL) {
      // Set the target pressure to be a bit lower.
      SetTargetPressure(targetMmhg);

      // Wait some time for the pressure to decrease.
      OSTimeDlyHMSM(0, 0, INTERVAL_DELAY_S, 0);
    }
  }
}