#include "ImpedanceRtos.h"

#define HIGH_PRESSURE 159
#define CUFF_DEFLATE_DELAY_S 2
#define CUFF_NORMALIZE_AFTER_INFLATE_DELAY_S 3

uint16_t mmhg_to_transducer(uint32_t pressure) {
  // Cortex M3 has no hardware FP (?), so use an approximation instead.
  // analog_value = 2.4935 (pressure) + 53.516
  return (uint16_t) ((pressure * 192u / 77) + 54);
}

uint32_t transducer_to_mmhg(uint16_t transducer) {
  // Cortex M3 has no hardware FP (?), so use an approximation instead.
  // pressure = 0.4010425 (analog) - 21.46219043
  return (((uint32_t) transducer) * 77 / 192) - 21;
}

uint8_t i2c_pump_rx[I2C_BUFFER_SIZE];
uint8_t i2c_pump_tx[I2C_BUFFER_SIZE];

uint16_t targetValue;
ADI_I2C_RESULT_TYPE i2cResult;

void SetTargetPressure(int32_t targetPressure) {
  targetValue = targetPressure ? mmhg_to_transducer(targetPressure) : 0;
  i2c_pump_tx[0] = SET_PRESSURE_COMMAND;
  i2c_pump_tx[1] = targetValue & 0xFFu;
  i2c_pump_tx[2] = (targetValue >> 8) & 0x3u;
  i2cResult = adi_I2C_MasterTransmit(i2cDevice, I2C_PUMP_SLAVE_ADDRESS, 0x0,
                                     ADI_I2C_8_BIT_DATA_ADDRESS_WIDTH,
                                     i2c_pump_tx, 3, false);
  if (i2cResult != ADI_I2C_SUCCESS) {
    FAIL("adi_I2C_MasterTransmit: send pressure command to Arduino");
  }
}

void PumpTask(void* arg) {
  uint8_t err;

  while (true) {
    // Suspend this task, as it will be resumed by the main one.
    printf("PumpTask: suspending to wait for main task.\n");
    err = OSTaskSuspend(OS_PRIO_SELF);
    if (err != OS_ERR_NONE) {
      FAIL("OSTaskResume: OSTaskSuspend (1)");
    }

    // Set the target pressure to the high pressure.
    printf("PumpTask: inflating to %d.\n", HIGH_PRESSURE);
    SetTargetPressure(HIGH_PRESSURE);
    
    // Wait until the Arduino pumps all the way up.
    while (true) {
      printf("PumpTask: checking transducer value via I2C...\n");
      err = OSTimeDlyHMSM(0, 0, 1, 0);
      if (err != OS_ERR_NONE) {
        FAIL("OSTimeDlyHMSM: PumpTaskRun (3)");
      }

      i2cResult = adi_I2C_MasterReceive(i2cDevice, I2C_PUMP_SLAVE_ADDRESS, 0x0,
                                        ADI_I2C_8_BIT_DATA_ADDRESS_WIDTH,
                                        i2c_pump_rx, 3, false);
      if (i2cResult != ADI_I2C_SUCCESS) {
        FAIL("adi_I2C_MasterReceive: get pressure from Arduino");
      }

      // Break once the Arduino is no longer inflating.
      if (i2c_pump_rx[0] == ARDUINO_STILL_INFLATING) {
        continue;
      } else if (i2c_pump_rx[0] == ARDUINO_PRESSURE_AVAILABLE) {
        printf("PumpTask: Arduino has finished inflating.\n");
        break;
      } else {
        FAIL("adi_I2C_MasterReceive: unknown result");
      }
    }
    
    // Resume the main task.
    printf("PumpTask: resuming main task.\n");
    err = OSTaskResume(TASK_MAIN_PRIO);
    if (err != OS_ERR_NONE) {
      FAIL("OSTaskResume: PumpTaskRun");
    }
    
    // Suspend this task until the main task finishes measuring. At that point,
    // the main task will resume this task (and suspend itself) so that we
    // can fully deflate the cuff.
    printf("PumpTask: suspending self.\n");
    err = OSTaskSuspend(OS_PRIO_SELF);
    if (err != OS_ERR_NONE) {
      FAIL("OSTaskResume: OSTaskSuspend (2)");
    }

    // Once we reach the low value, send "0" to the Arduino to deflate the cuff.
    printf("PumpTask: fully deflating cuff.\n");
    SetTargetPressure(0);

    // Wait a couple seconds for the cuff to deflate.
    printf("PumpTask: waiting a few seconds for the cuff to deflate.\n");
    err = OSTimeDlyHMSM(0, 0, CUFF_DEFLATE_DELAY_S, 0);
    if (err != OS_ERR_NONE) {
      FAIL("OSTimeDlyHMSM: PumpTaskRun (2)");
    }
    
    // Resume the main task and exit.
    err = OSTaskResume(TASK_MAIN_PRIO);
    if (err != OS_ERR_NONE) {
      FAIL("OSTaskResume: PumpTaskRun");
    }
  }
}