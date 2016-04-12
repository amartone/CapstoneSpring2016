#include "ImpedanceRtos.h"

/* Macro to enable the returning of AFE data using the UART. */
/*      1 = return AFE data on UART                          */
/*      0 = return AFE data on SW (Std Output)               */
#define USE_UART_FOR_DATA (1)

/* Excitation frequency in Hz */
#define FREQ (50000)
/* Peak voltage in mV */
#define VPEAK (599)  //(12.73)
/* RCAL value, in ohms */
#define RCAL (1000)

/* FCW = FREQ * 2^26 / 16e6 */
#define FCW ((uint32_t)(((uint64_t)FREQ << 26) / 16000000 + 0.5))

/* DAC LSB size in mV, before attenuator (1.6V / (2^12 - 1)) */
#define DAC_LSB_SIZE (0.39072)

/* Sine amplitude in DAC codes */
#define SINE_AMPLITUDE ((uint16_t)((VPEAK) / DAC_LSB_SIZE + 0.5))

/* If both real and imaginary result are within the interval
 * (DFT_RESULTS_OPEN_MIN_THR, DFT_RESULTS_OPEN_MAX_THR),  */
/* it is considered an open circuit and results for both magnitude and phase
 * will be 0.                             */
#define DFT_RESULTS_OPEN_MAX_THR (10)
#define DFT_RESULTS_OPEN_MIN_THR (-10)

/* The number of results expected from the DFT, in this case 8 for 4 complex
 * results */
#define DFT_RESULTS_COUNT (8)

/* Fractional LSB size for the fixed32_t type defined below, used for printing
 * only. */
#define FIXED32_LSB_SIZE (625)
#define MSG_MAXLEN (50)

/* Helper macro for printing strings to UART or Std. Output */
#define PRINT(s) test_print(s)

/* Custom fixed-point type used for final results,              */
/* to keep track of the decimal point position.                 */
/* Signed number with 28 integer bits and 4 fractional bits.    */
typedef union {
  int32_t full;
  struct {
    uint8_t fpart : 4;
    int32_t ipart : 28;
  } parts;
} fixed32_t;

// Type used for passing impedance results through the message queue.
typedef union {
  int32_t full;
  struct {
    int16_t magnitude : 16;
    int16_t phase : 16;
  } parts;
} packed32_t;

// Time stuff

// leap-year compute macro (ignores leap-seconds)
#define LEAP_YEAR(x) (((0 == x % 4) && (0 != x % 100)) || (0 == x % 400))

/* device and board specific values selected according to computed trim
 * measurement */
/* THESE VALUES ARE UNIQUE TO THE EVAL-ADUCM350EBZ REV. 0 BOARD, SERIAL#: AVAS
 * 35070 */
#define ADI_RTC_TRIM_INTERVAL (uint32_t) ADI_RTC_TRIM_INTERVAL_14
#define ADI_RTC_TRIM_DIRECTION (uint32_t) ADI_RTC_TRIM_SUB
#define ADI_RTC_TRIM_VALUE (uint32_t) ADI_RTC_TRIM_1

/* Device handle */
ADI_RTC_HANDLE hRTC = NULL;
volatile bool_t bRtcAlarmFlag;
volatile bool_t bRtcInterrupt;
volatile bool_t bWdtInterrupt;
volatile bool_t bHibernateExitFlag;

/* prototypes */
void rtc_Init(void);
void rtc_Calibrate(void);
void rtc_ReportTime(void);
uint32_t BuildSeconds(void);

/* callbacks */
void rtcCallback(void *pCBParam, uint32_t Event, void *EventArg);
void wutCallback(void *pCBParam, uint32_t Event, void *EventArg);

void AFE_DFT_Callback(void *pCBParam, uint32_t Event, void *pArg);

ADI_UART_HANDLE hUartDevice = NULL;
ADI_I2C_DEV_HANDLE i2cDevice = NULL;

/* Function prototypes */
q15_t arctan(q15_t imag, q15_t real);
fixed32_t calculate_magnitude(q31_t magnitude_rcal, q31_t magnitude_z);
fixed32_t calculate_phase(q15_t phase_rcal, q15_t phase_z);
void convert_dft_results(int16_t *dft_results, q15_t *dft_results_q15,
                         q31_t *dft_results_q31);
void sprintf_fixed32(char *out, fixed32_t in);
void print_PressureMagnitudePhase(char *text, uint16_t pressure,
                                  fixed32_t magnitude, fixed32_t phase,
                                  int queue_size);
void test_print(char *pBuffer);
ADI_UART_RESULT_TYPE uart_Init(void);
ADI_UART_RESULT_TYPE uart_UnInit(void);
void i2c_Init(ADI_I2C_DEV_HANDLE *i2cDevice);
//void i2c_Init(ADI_I2C_DEV_HANDLE *i2cDevice);

/* Sequence for AC measurement, performs 4 DFTs:        */
/*     RCAL, AFE3-AFE4, AFE4-AFE5, AFE3-AFE5            */
uint32_t seq_afe_acmeas2wire[] = {
    0x00120043, /* Safety word: bits 31:16 = command count, bits 7:0 =
                   CRC */
    0x84005818, /* AFE_FIFO_CFG: DATA_FIFO_SOURCE_SEL = 10 */
    0x8A000034, /* AFE_WG_CFG: TYPE_SEL = 10 */
    0x98000000, /* AFE_WG_CFG: SINE_FCW = 0 (placeholder, user
                   programmable) */
    0x9E000000, /* AFE_WG_AMPLITUDE: SINE_AMPLITUDE = 0 (placeholder,
                   user programmable) */
    0x88000F00, /* AFE_DAC_CFG: DAC_ATTEN_EN = 0 */
    0xA0000002, /* AFE_ADC_CFG: MUX_SEL = 00010, GAIN_OFFS_SEL = 00 */
    /* RCAL */
    0x86008811, /* DMUX_STATE = 1, PMUX_STATE = 1, NMUX_STATE = 8,
                   TMUX_STATE = 8 */
    0x00000640, /* Wait 100us */
    0x80024EF0, /* AFE_CFG: WAVEGEN_EN = 1 */
    0x00000C80, /* Wait 200us */
    0x8002CFF0, /* AFE_CFG: ADC_CONV_EN = 1, DFT_EN = 1 */
    0x00032340, /* Wait 13ms */
    0x80024EF0, /* AFE_CFG: ADC_CONV_EN = 0, DFT_EN = 0 */
    /* AFE3 - AFE4 */
    0x86003344, /* DMUX_STATE = 3, PMUX_STATE = 3, NMUX_STATE = 4,
                   TMUX_STATE = 4 */
    0x00000640, /* Wait 100us */
    0x8002CFF0, /* AFE_CFG: ADC_CONV_EN = 1, DFT_EN = 1 */
    0x00032340, /* Wait 13ms */

    0x82000002, /* AFE_SEQ_CFG: SEQ_EN = 0 */
};

#define DFT_QUEUE_SIZE 20

uint32_t nummeasurements;
uint8_t done;

OS_EVENT *dft_queue;
void *dft_queue_msg[DFT_QUEUE_SIZE];

uint8_t i2c_rx[I2C_BUFFER_SIZE];

void MainTaskRun(void *arg) {
  ADI_AFE_DEV_HANDLE hDevice;
  int16_t dft_results[DFT_RESULTS_COUNT];
  q15_t dft_results_q15[DFT_RESULTS_COUNT];
  q31_t dft_results_q31[DFT_RESULTS_COUNT];
  q31_t magnitude[DFT_RESULTS_COUNT / 2];
  q15_t phase[DFT_RESULTS_COUNT / 2];
  fixed32_t magnitude_result[DFT_RESULTS_COUNT / 2 - 1];
  fixed32_t phase_result[DFT_RESULTS_COUNT / 2 - 1];
  char msg[MSG_MAXLEN];
  uint8_t err;
  done = 0;
  uint16_t pressure_analog;
  uint32_t pressure;
  nummeasurements = 0;
  uint32_t rtcCount;
  ADI_I2C_RESULT_TYPE i2cResult;

  // Initialize driver.
  rtc_Init();

  // Calibrate.
  rtc_Calibrate();

  // Initialize UART.
  if (uart_Init()) {
    FAIL("ADI_UART_SUCCESS");
  }

  // Initialize I2C.
  i2c_Init(&i2cDevice);
  
  // Initialize flags.
  bRtcAlarmFlag = bRtcInterrupt = bWdtInterrupt = false;

  // Get the current count.
  if (adi_RTC_GetCount(hRTC, &rtcCount)) {
    FAIL("adi_RTC_GetCount failed");
  }

  // Initialize the AFE API.
  if (adi_AFE_Init(&hDevice)) {
    FAIL("adi_AFE_Init");
  }

  // AFE power up.
  if (adi_AFE_PowerUp(hDevice)) {
    FAIL("adi_AFE_PowerUp");
  }

  // Excitation Channel Power-up.
  if (adi_AFE_ExciteChanPowerUp(hDevice)) {
    FAIL("adi_AFE_ExciteChanPowerUp");
  }

  // TIA Channel Calibration.
  if (adi_AFE_TiaChanCal(hDevice)) {
    FAIL("adi_AFE_TiaChanCal");
  }

  // Excitation Channel Calibration (Attenuation Enabled).
  if (adi_AFE_ExciteChanCalAtten(hDevice)) {
    FAIL("adi_AFE_ExciteChanCalAtten");
  }

  // Update FCW in the sequence.
  seq_afe_acmeas2wire[3] = SEQ_MMR_WRITE(REG_AFE_AFE_WG_FCW, FCW);
  // Update sine amplitude in the sequence.
  seq_afe_acmeas2wire[4] =
      SEQ_MMR_WRITE(REG_AFE_AFE_WG_AMPLITUDE, SINE_AMPLITUDE);

  // Recalculate CRC in software for the AC measurement, because we changed.
  // FCW and sine amplitude settings.
  adi_AFE_EnableSoftwareCRC(hDevice, true);

  // Perform the impedance measurement.
  if (adi_AFE_RunSequence(hDevice, seq_afe_acmeas2wire, (uint16_t *)dft_results,
                          DFT_RESULTS_COUNT)) {
    FAIL("Impedance Measurement");
  }

  // Set RTC alarm.
  printf("rtcCount: %d\r\n", rtcCount);
  if (ADI_RTC_SUCCESS != adi_RTC_SetAlarm(hRTC, rtcCount + 120)) {
    FAIL("adi_RTC_SetAlarm failed");
  }

  // Enable RTC alarm.
  if (ADI_RTC_SUCCESS != adi_RTC_EnableAlarm(hRTC, true)) {
    FAIL("adi_RTC_EnableAlarm failed");
  }

  // Read the initial impedance.
  q31_t magnitudecal;
  q15_t phasecal;

  convert_dft_results(dft_results, dft_results_q15, dft_results_q31);
  arm_cmplx_mag_q31(dft_results_q31, &magnitudecal, 2);

  phasecal = arctan(dft_results[1], dft_results[0]);

  printf("raw rcal data: %d, %d\r\n", dft_results[1], dft_results[0]);
  printf("rcal (magnitude, phase) = (%d, %d)\r\n", magnitudecal, phasecal);

  // Create the message queue for communicating between the ISR and this task.
  dft_queue = OSQCreate(&dft_queue_msg[0], DFT_QUEUE_SIZE);

  // Hook into the DFT interrupt.
  if (ADI_AFE_SUCCESS !=
      adi_AFE_RegisterAfeCallback(
          hDevice, ADI_AFE_INT_GROUP_CAPTURE, AFE_DFT_Callback,
          BITM_AFE_AFE_ANALOG_CAPTURE_IEN_DFT_RESULT_READY_IEN)) {
    FAIL("adi_AFE_RegisterAfeCallback");
  }
  if (ADI_AFE_SUCCESS !=
      adi_AFE_ClearInterruptSource(
          hDevice, ADI_AFE_INT_GROUP_CAPTURE,
          BITM_AFE_AFE_ANALOG_CAPTURE_IEN_DFT_RESULT_READY_IEN)) {
    FAIL("adi_AFE_ClearInterruptSource (1)");
  }

  packed32_t q_result;
  void *q_result_void;
  OS_Q_DATA q_data;
  uint16_t q_size;
  while (true) {
    // Wait for the user to press the button.
    printf("MainTask: waiting for button.\n");
    OSSemPend(ux_button_semaphore, 0, &err);
    if (err != OS_ERR_NONE) {
      FAIL("OSSemPend: MainTask");
    }
    
    // Have the pump task inflate the cuff.
    printf("MainTask: button detected. Resuming pump task.\n");
    err = OSTaskResume(TASK_PUMP_PRIO);
    if (err != OS_ERR_NONE) {
      FAIL("OSTaskResume: MainTask (1)");
    }
    
    // Suspend this task until the pump task finishes inflating.
    printf("MainTask: suspending self.\n");
    err = OSTaskSuspend(OS_PRIO_SELF);
    if (err != OS_ERR_NONE) {
      FAIL("OSTaskResume: MainTask (1)");
    }
    
    // Enable the DFT interrupt.
    printf("MainTask: enabling DFT interrupt.\n");
    if (ADI_AFE_SUCCESS !=
        adi_AFE_EnableInterruptSource(
            hDevice, ADI_AFE_INT_GROUP_CAPTURE,
            BITM_AFE_AFE_ANALOG_CAPTURE_IEN_DFT_RESULT_READY_IEN, true)) {
      FAIL("adi_AFE_EnableInterruptSource");
    }

    packed32_t pend_dft_results;
    while (true) {
      // Wait on the queue to get DFT data from the ISR (~76 Hz).
      printf("MainTask: pending on DFT queue.\n");
      q_result_void = OSQPend(dft_queue, 0, &err);
      q_result = *((packed32_t *)&q_result_void);
      if (err != OS_ERR_NONE) {
        FAIL("OSQPend: dft_queue");
      }
      OSQQuery(dft_queue, &q_data);
      q_size = q_data.OSNMsgs;
    
      // Right after we get this data, get the transducer's value from the
      // Arduino.
      printf("MainTask: getting transducer value via I2C.\n");
      i2cResult = adi_I2C_MasterReceive(i2cDevice, I2C_PUMP_SLAVE_ADDRESS, 0x0,
                                        ADI_I2C_8_BIT_DATA_ADDRESS_WIDTH,
                                        i2c_rx, 3, false);
      if (i2cResult != ADI_I2C_SUCCESS) {
        FAIL("adi_I2C_MasterReceive: get pressure from Arduino");
      }

      // Get the analog pressure value from the Arduino.
      if (i2c_rx[0] == ARDUINO_PRESSURE_AVAILABLE) {
        pressure_analog = i2c_rx[1] | (i2c_rx[2] << 8);
      } else {
        FAIL("Corrupted or unexpected data from Arduino.");
      }
      
      // Convert the analog value to mmHg.
      pressure = transducer_to_mmhg(pressure_analog);
      printf("MainTask: got pressure value: %d mmHg.\n", pressure);
      
      // If the pressure is below the threshold, we're done; break the loop.
      if (pressure < LOWEST_PRESSURE_THRESHOLD_MMHG) {
        break;
      }

      // Convert DFT results to 1.15 and 1.31 formats.
      dft_results[0] = q_result.parts.magnitude;
      dft_results[1] = q_result.parts.phase;
      convert_dft_results(dft_results, dft_results_q15, dft_results_q31);

      // Compute the magnitude using CMSIS.
      arm_cmplx_mag_q31(dft_results_q31, magnitude, DFT_RESULTS_COUNT / 2);

      // Calculate final magnitude values, calibrated with RCAL.
      fixed32_t magnituderesult;
      magnituderesult = calculate_magnitude(magnitudecal, magnitude[0]);
      q15_t phaseresult;
      phaseresult = arctan(dft_results[1], dft_results[0]);
      fixed32_t phasecalibrated;

      // Calibrate with phase from rcal.
      phasecalibrated = calculate_phase(phasecal, phaseresult);
      
      // TODO: dispatch to other thread?
      printf("MainTask: sending data via UART.\n");;
      print_PressureMagnitudePhase("", pressure, magnituderesult, phasecalibrated,
                                   q_size);
      nummeasurements++;
    }

    // We're done measuring, for now. Disable the DFT interrupts.
    printf("MainTask: disabling DFT interrupts.\n");
    if (ADI_AFE_SUCCESS !=
        adi_AFE_EnableInterruptSource(
            hDevice, ADI_AFE_INT_GROUP_CAPTURE,
            BITM_AFE_AFE_ANALOG_CAPTURE_IEN_DFT_RESULT_READY_IEN, false)) {
      FAIL("adi_AFE_EnableInterruptSource (false)");
    }
    
    // Tell the pump task to deflate the cuff.
    printf("MainTask: resuming pump task to deflate the cuff.\n");
    err = OSTaskResume(TASK_PUMP_PRIO);
    if (err != OS_ERR_NONE) {
      FAIL("OSTaskResume: MainTask (2)");
    }

    // Suspend until the pump finishes deflating. We can then go back to
    // listening for the user input.
    printf("MainTask: suspending to wait for pump task.\n");
    err = OSTaskSuspend(OS_PRIO_SELF);
    if (err != OS_ERR_NONE) {
      FAIL("OSTaskSuspend: MainTask (3)");
    }

    // Tell the UX that we're done for now.
    UX_Disengage();
  }
}

packed32_t pend_dft_results;

static void AFE_DFT_Callback(void *pCBParam, uint32_t Event, void *pArg) {
  OSIntEnter();

  pend_dft_results.parts.magnitude = pADI_AFE->AFE_DFT_RESULT_REAL;
  pend_dft_results.parts.phase = pADI_AFE->AFE_DFT_RESULT_IMAG;
  if (OS_ERR_NONE != OSQPost(dft_queue, *((void **)&pend_dft_results))) {
    FAIL("AFE_DFT_Callback: OSQPost");
  }

  OSIntExit();
}

/* Arctan Implementation */
/* ===================== */
/* Arctan is calculated using the formula: */
/*                                                                                                          */
/*      y = arctan(x) = 0.318253 * x + 0.003314 * x^2 - 0.130908 * x^3 +
 * 0.068542 * x^4 - 0.009159 * x^5    */
/*                                                                                                          */
/* The angle in radians is given by (y * pi) */
/*                                                                                                          */
/* For the fixed-point implementation below, the coefficients are quantized to
 * 16-bit and                   */
/* represented as 1.15 */
/* The input vector is rotated until positioned between 0 and pi/4. After the
 * arctan                        */
/* is calculated for the rotated vector, the initial angle is restored. */
/* The format of the output is 1.15 and scaled by PI. To find the angle value in
 * radians from the output    */
/* of this function, a multiplication by PI is needed. */

const q15_t coeff[5] = {
    (q15_t)0x28BD, /*  0.318253 */
    (q15_t)0x006D, /*  0.003314 */
    (q15_t)0xEF3E, /* -0.130908 */
    (q15_t)0x08C6, /*  0.068542 */
    (q15_t)0xFED4, /* -0.009159 */
};

q15_t arctan(q15_t imag, q15_t real) {
  q15_t t;
  q15_t out;
  uint8_t rotation; /* Clockwise, multiples of PI/4 */
  int8_t i;

  if ((q15_t)0 == imag) {
    /* Check the sign*/
    if (real & (q15_t)0x8000) {
      /* Negative, return -PI */
      return (q15_t)0x8000;
    } else {
      return (q15_t)0;
    }
  } else {
    rotation = 0;
    /* Rotate the vector until it's placed in the first octant (0..PI/4) */
    if (imag < 0) {
      imag = -imag;
      real = -real;
      rotation += 4;
    }
    if (real <= 0) {
      /* Using 't' as temporary storage before its normal usage */
      t = real;
      real = imag;
      imag = -t;
      rotation += 2;
    }
    if (real <= imag) {
      /* The addition below may overflow, drop 1 LSB precision if needed. */
      /* The subtraction cannot underflow.                                */
      t = real + imag;
      if (t < 0) {
        /* Overflow */
        t = imag - real;
        real = (q15_t)(((q31_t)real + (q31_t)imag) >> 1);
        imag = t >> 1;
      } else {
        t = imag - real;
        real = (real + imag);
        imag = t;
      }
      rotation += 1;
    }

    /* Calculate tangent value */
    t = (q15_t)((q31_t)(imag << 15) / real);

    out = (q15_t)0;

    for (i = 4; i >= 0; i--) {
      out += coeff[i];
      arm_mult_q15(&out, &t, &out, 1);
    }

    /* Rotate back to original position, in multiples of pi/4 */
    /* We're using 1.15 representation, scaled by pi, so pi/4 = 0x2000 */
    out += (rotation << 13);

    return out;
  }
}

/* This function performs dual functionality: */
/* - open circuit check: the real and imaginary parts can be non-zero but very
 * small    */
/*   due to noise. If they are within the defined thresholds, overwrite them
 * with 0s,   */
/*   this will indicate an open. */
/* - convert the int16_t to q15_t and q31_t formats, needed for the magnitude
 * and phase */
/*   calculations. */
void convert_dft_results(int16_t *dft_results, q15_t *dft_results_q15,
                         q31_t *dft_results_q31) {
  int8_t i;

  for (i = 0; i < (DFT_RESULTS_COUNT / 2); i++) {
    if ((dft_results[i] < DFT_RESULTS_OPEN_MAX_THR) &&
        (dft_results[i] > DFT_RESULTS_OPEN_MIN_THR) && /* real part */
        (dft_results[2 * i + 1] < DFT_RESULTS_OPEN_MAX_THR) &&
        (dft_results[2 * i + 1] >
         DFT_RESULTS_OPEN_MIN_THR)) { /* imaginary part */

      /* Open circuit, force both real and imaginary parts to 0 */
      dft_results[i] = 0;
      dft_results[2 * i + 1] = 0;
    }
  }

  /*  Convert to 1.15 format */
  for (i = 0; i < DFT_RESULTS_COUNT; i++) {
    dft_results_q15[i] = (q15_t)dft_results[i];
  }

  /*  Convert to 1.31 format */
  arm_q15_to_q31(dft_results_q15, dft_results_q31, DFT_RESULTS_COUNT);
}

/* Calculates calibrated magnitude.                                     */
/* The input values are the measured RCAL magnitude (magnitude_rcal)    */
/* and the measured magnitude of the unknown impedance (magnitude_z).   */
/* Performs the calculation:                                            */
/*      magnitude = magnitude_rcal / magnitude_z * RCAL                 */
/* Output in custom fixed-point format (28.4).                          */
fixed32_t calculate_magnitude(q31_t magnitude_rcal, q31_t magnitude_z) {
  q63_t magnitude;
  fixed32_t out;

  magnitude = (q63_t)0;
  if ((q63_t)0 != magnitude_z) {
    magnitude = (q63_t)magnitude_rcal * (q63_t)RCAL;
    /* Shift up for additional precision and rounding */
    magnitude = (magnitude << 5) / (q63_t)magnitude_z;
    /* Rounding */
    magnitude = (magnitude + 1) >> 1;
  }

  /* Saturate if needed */
  if (magnitude & 0xFFFFFFFF00000000) {
    /* Cannot be negative */
    out.full = 0x7FFFFFFF;
  } else {
    out.full = magnitude & 0xFFFFFFFF;
  }

  return out;
}

/* Calculates calibrated phase.                                     */
/* The input values are the measured RCAL phase (phase_rcal)        */
/* and the measured phase of the unknown impedance (magnitude_z).   */
/* Performs the calculation:                                        */
/*      phase = (phase_z - phase_rcal) * PI / (2 * PI) * 180        */
/*            = (phase_z - phase_rcal) * 180                        */
/* Output in custom fixed-point format (28.4).                      */
fixed32_t calculate_phase(q15_t phase_rcal, q15_t phase_z) {
  q63_t phase;
  fixed32_t out;

  /* Multiply by 180 to convert to degrees */
  phase = ((q63_t)(phase_z - phase_rcal) * (q63_t)180);
  /* Round and convert to fixed32_t */
  out.full = ((phase + (q63_t)0x400) >> 11) & 0xFFFFFFFF;

  return out;
}

/* Simple conversion of a fixed32_t variable to string format. */
void sprintf_fixed32(char *out, fixed32_t in) {
  fixed32_t tmp;

  if (in.full < 0) {
    tmp.parts.fpart = (16 - in.parts.fpart) & 0x0F;
    tmp.parts.ipart = in.parts.ipart;
    if (0 != in.parts.fpart) {
      tmp.parts.ipart++;
    }
    if (0 == tmp.parts.ipart) {
      sprintf(out, "      -0.%04d", tmp.parts.fpart * FIXED32_LSB_SIZE);
    } else {
      sprintf(out, "%8d.%04d", tmp.parts.ipart,
              tmp.parts.fpart * FIXED32_LSB_SIZE);
    }
  } else {
    sprintf(out, "%8d.%04d", in.parts.ipart, in.parts.fpart * FIXED32_LSB_SIZE);
  }
}

/* Helper function for printing fixed32_t (magnitude & phase) and uint15_t
 * (pressure) results. */
void print_PressureMagnitudePhase(char *text, uint16_t pressure,
                                  fixed32_t magnitude, fixed32_t phase,
                                  int queue_size) {
  char msg[MSG_MAXLEN];
  char tmp[MSG_MAXLEN];
  sprintf(msg, "%s %d: %d %d (%d/%d)\r\n", text, pressure, magnitude.full,
          phase.full, queue_size, DFT_QUEUE_SIZE);

  PRINT(msg);
}

/* Helper function for printing a string to UART or Std. Output */
void test_print(char *pBuffer) {
#if (1 == USE_UART_FOR_DATA)
  int16_t size;
  /* Print to UART */
  size = strlen(pBuffer);
  if (ADI_UART_SUCCESS != adi_UART_BufTx(hUartDevice, pBuffer, &size)) {
    FAIL("test_print: ADI_UART_SUCCESS");
  }

#elif (0 == USE_UART_FOR_DATA)
  /* Print  to console */
  printf(pBuffer);

#endif /* USE_UART_FOR_DATA */
}

/* Initialize the UART, set the baud rate and enable */
ADI_UART_RESULT_TYPE uart_Init(void) {
  ADI_UART_RESULT_TYPE result = ADI_UART_SUCCESS;

  /* Open UART in blocking, non-intrrpt mode by supplying no internal buffs */
  if (ADI_UART_SUCCESS !=
      (result = adi_UART_Init(ADI_UART_DEVID_0, &hUartDevice, NULL))) {
    return result;
  }

  /* Set UART baud rate to 115200 */
  // if (ADI_UART_SUCCESS != (result = adi_UART_SetBaudRate(hUartDevice,
  // ADI_UART_BAUD_115200)))
  //{
  //    return result;
  // }
  /* Set UART baud rate to 115200 */
  if (ADI_UART_SUCCESS !=
      (result = adi_UART_SetBaudRate(hUartDevice, ADI_UART_BAUD_115200))) {
    return result;
  }
  /* Enable UART */
  if (ADI_UART_SUCCESS != (result = adi_UART_Enable(hUartDevice, true))) {
    return result;
  }

  return result;
}

/* Uninitialize the UART */
ADI_UART_RESULT_TYPE uart_UnInit(void) {
  ADI_UART_RESULT_TYPE result = ADI_UART_SUCCESS;

  /* Uninitialize the UART API */
  if (ADI_UART_SUCCESS != (result = adi_UART_UnInit(hUartDevice))) {
    return result;
  }

  return result;
}

void i2c_Init(ADI_I2C_DEV_HANDLE *i2cDevice) {
  /* Take HCLK/PCLK down to 1MHz for better power utilization */
  /* Need to set PCLK frequency first, because HCLK frequency */
  /* needs to be greater than or equal to the PCLK frequency  */
  /* at all times.                                            */
  // SetSystemClockDivider(ADI_SYS_CLOCK_PCLK, 16);
  // SetSystemClockDivider(ADI_SYS_CLOCK_CORE, 16);

  // Initialize I2C driver.
  if (ADI_I2C_SUCCESS != adi_I2C_MasterInit(ADI_I2C_DEVID_0, i2cDevice)) {
    FAIL("adi_I2C_MasterInit");
  }

  // Select serial bit rate (~100 kHz max).
  if (ADI_I2C_SUCCESS != adi_I2C_SetMasterClock(*i2cDevice, I2C_MASTER_CLOCK)) {
    FAIL("adi_I2C_SetMasterClock");
  }

  // Disable blocking mode... i.e., poll for completion.
  //if (ADI_I2C_SUCCESS != adi_I2C_SetBlockingMode(*i2cDevice, false)) {
  //  FAIL("adi_I2C_SetBlockingMode");
  //}
}

void rtc_Init(void) {
  /* callbacks */
  ADI_RTC_INT_SOURCE_TYPE callbacks = (ADI_RTC_INT_SOURCE_TYPE)(
      ADI_RTC_INT_SOURCE_WRITE_PEND | ADI_RTC_INT_SOURCE_WRITE_SYNC |
      ADI_RTC_INT_SOURCE_WRITE_PENDERR | ADI_RTC_INT_SOURCE_ISO_DONE |
      ADI_RTC_INT_SOURCE_LCD_UPDATE | ADI_RTC_INT_SOURCE_ALARM |
      ADI_RTC_INT_SOURCE_FAIL);
  uint32_t buildTime = BuildSeconds();
  ADI_RTC_RESULT_TYPE result;

  result = adi_RTC_Init(ADI_RTC_DEVID_0, &hRTC);

  /* retry on failsafe */
  if (ADI_RTC_ERR_CLOCK_FAILSAFE == result) {
    /* clear the failsafe */
    adi_RTC_ClearFailSafe();

    /* un-init RTC for a clean restart, but ignore failure */
    adi_RTC_UnInit(hRTC);

    /* re-init RTC */
    if (ADI_RTC_SUCCESS != adi_RTC_Init(ADI_RTC_DEVID_0, &hRTC))
      FAIL("Double fault on adi_RTC_Init");

    PERF("Resetting clock and trim values after init failure");

    /* set clock to latest build time */
    if (ADI_RTC_SUCCESS != adi_RTC_SetCount(hRTC, buildTime))
      FAIL("adi_RTC_SetCount failed");

    /* apply pre-computed calibration BOARD-SPECIFIC trim values */
    if (adi_RTC_SetTrim(hRTC, ADI_RTC_TRIM_INTERVAL | ADI_RTC_TRIM_DIRECTION |
                                  ADI_RTC_TRIM_VALUE))
      FAIL("adi_RTC_SetTrim failed");

    /* enable trimming */
    if (adi_RTC_EnableTrim(hRTC, true)) FAIL("adi_RTC_EnableTrim failed");

    /* catch all other open failures */
  } else {
    if (result != ADI_RTC_SUCCESS)
      FAIL("Generic failure to initialize the RTC");
  }

  /* RTC opened successfully... */

  /* disable alarm */
  if (ADI_RTC_SUCCESS != adi_RTC_EnableAlarm(hRTC, false))
    FAIL("adi_RTC_EnableAlarm failed");

#ifdef ADI_RTC_RESET
  /* force a reset to the latest build timestamp */
  PERF("Resetting clock");
  if (ADI_RTC_SUCCESS != adi_RTC_SetCount(hRTC, buildTime))
    FAIL("adi_RTC_SetCount failed");
  PERF("New time is:");
  rtc_ReportTime();
#endif

  /* register callback handler for all interrupts */
  if (ADI_RTC_SUCCESS !=
      adi_RTC_RegisterCallback(hRTC, rtcCallback, callbacks)) {
    FAIL("adi_RTC_RegisterCallback failed");
  }

  /* enable RTC */
  if (ADI_RTC_SUCCESS != adi_RTC_EnableDevice(hRTC, true))
    FAIL("adi_RTC_EnableDevice failed");
}

void rtc_Calibrate(void) {
#ifdef ADI_RTC_CALIBRATE

  /*

  Compute the LF crystal trim values to compensate the RTC.  This can
  come from a static measure (a frequency counter), a real-time drift measure
  based on a USB transaction, Ethernet NTP or PTP protocol, or some other
  external reference.

  Commercial crystals typically run between 20-100 ppm.  As an exercise, we
  demonstrate trimming a particular crystal and board configuration in which
  we measure an untrimmed error of about +58.6ppm (0.00586%).  This corresponds
  to a raw clock about 35.5 seconds/week fast (30 minutes/year).

  Available Trim Corrections:
      X axis: trim interval (seconds)
      Y axis: trim value (seconds)
      Table content: trim correction (ppm)
    Value     16384    32768    65536   131072 (Interval)
      0        0.00     0.00     0.00     0.00
      1       61.04    30.52    15.26     7.63
      2      122.07    61.04    30.52    15.26
      3      183.11    91.55    45.78    22.89
      4      244.14   122.07    61.04    30.52
      5      305.18   152.59    76.29    38.15
      6      366.21   183.11    91.55    45.78
      7      427.25   213.62   106.81    53.41

  Referencing the trim table, we see the closest matching ppm correction for
  our example is 61.04.  In case there are different combinations yielding
  the same desired correction, we prefer the shortest trim interval (and
  smallest trim value) so as to minimize instantaneous drift.

  So we choose a trim interval of 2^14 seconds with a negative trim value of 1
  second, subtracting 1 second every 4.5 hours to "slow" the fast crystal down
  to a more reasonable rate.  This particular trim leaves a residual error of
  negative 2.44ppm (0.000244%), making the trimmed clock a tad slow (less than
  1.5 seconds/week or about 1.3 minutes/year), but much better than the
  untrimmed accuracy of 30 minutes/year.

  */

  /* dial-up external LF crystal to clockout pin (P1.7) for measurement */
  if (adi_GPIO_Init()) FAIL("adi_GPIO_Init failed");
  SetSystemClockMux(ADI_SYS_CLOCK_MUX_LFCLK_LFXTAL);   // select LF crystal
  SetSystemClockMux(ADI_SYS_CLOCK_MUX_OUTPUT_LF_CLK);  // route output

  /* Use static pinmuxing */
  adi_initpinmux();

  if (adi_GPIO_UnInit()) FAIL("adi_GPIO_UnInit failed");

  PERF("RTC clockout programmed to P1.7 for calibration...");

  /* program the BOARD-SPECIFIC computed trim value, as described above */
  if (adi_RTC_SetTrim(hRTC, ADI_RTC_TRIM_INTERVAL | ADI_RTC_TRIM_DIRECTION |
                                ADI_RTC_TRIM_VALUE))
    FAIL("adi_RTC_SetTrim failed");

  /* enable trim */
  if (adi_RTC_EnableTrim(hRTC, true)) FAIL("adi_RTC_EnableTrim failed");

#endif
}

/* test standard ctime (time.h) constructs */
void rtc_ReportTime(void) {
  char buffer[128];

  time_t rawtime;

  // get the RTC count through the "time" CRTL function
  time(&rawtime);

  // print raw count
  sprintf(buffer, "Raw time: %d", rawtime);
  PERF(buffer);

  // convert to UTC string and print that too
  sprintf(buffer, "UTC/GMT time: %s", ctime(&rawtime));
  PERF(buffer);
}

uint32_t BuildSeconds(void) {
  /* count up seconds from the epoc (1/1/70) to the most recient build time */

  char timestamp[] = __DATE__ " " __TIME__;
  int month_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  uint32_t days, month, date, year, hours, minutes, seconds;
  char Month[4];

  // parse the build timestamp
  sscanf(timestamp, "%s %d %d %d:%d:%d", Month, &date, &year, &hours, &minutes,
         &seconds);

  // parse ASCII month to a value
  if (!strncmp(Month, "Jan", 3))
    month = 1;
  else if (!strncmp(Month, "Feb", 3))
    month = 2;
  else if (!strncmp(Month, "Mar", 3))
    month = 3;
  else if (!strncmp(Month, "Apr", 3))
    month = 4;
  else if (!strncmp(Month, "May", 3))
    month = 5;
  else if (!strncmp(Month, "Jun", 3))
    month = 6;
  else if (!strncmp(Month, "Jul", 3))
    month = 7;
  else if (!strncmp(Month, "Aug", 3))
    month = 8;
  else if (!strncmp(Month, "Sep", 3))
    month = 9;
  else if (!strncmp(Month, "Oct", 3))
    month = 10;
  else if (!strncmp(Month, "Nov", 3))
    month = 11;
  else if (!strncmp(Month, "Dec", 3))
    month = 12;

  // count days from prior years
  days = 0;
  for (int y = 1970; y < year; y++) {
    days += 365;
    if (LEAP_YEAR(y)) days += 1;
  }

  // add days for current year
  for (int m = 1; m < month; m++) days += month_days[m - 1];

  // adjust if current year is a leap year
  if ((LEAP_YEAR(year) && ((month > 2) || ((month == 2) && (date == 29)))))
    days += 1;

  // add days this month (not including current day)
  days += date - 1;

  return (days * 24 * 60 * 60 + hours * 60 * 60 + minutes * 60 + seconds);
}

/* RTC Callback handler */
void rtcCallback(void *pCBParam, uint32_t nEvent, void *EventArg) {
  bRtcInterrupt = true;

  printf("number of measurements: %d\r\n", nummeasurements);
  done = 1;

  /* process RTC interrupts (cleared by driver) */
  if (ADI_RTC_INT_SOURCE_WRITE_PEND & (ADI_RTC_INT_SOURCE_TYPE)nEvent) {
    PERF(
        "got RTC interrupt callback with ADI_RTC_INT_SOURCE_WRITE_PEND status");
  }

  if (ADI_RTC_INT_SOURCE_WRITE_SYNC & (ADI_RTC_INT_SOURCE_TYPE)nEvent) {
    PERF(
        "got RTC interrupt callback with ADI_RTC_INT_SOURCE_WRITE_SYNC status");
  }

  if (ADI_RTC_INT_SOURCE_WRITE_PENDERR & (ADI_RTC_INT_SOURCE_TYPE)nEvent) {
    PERF(
        "got RTC interrupt callback with ADI_RTC_INT_SOURCE_WRITE_PENDERR "
        "status");
  }

  if (ADI_RTC_INT_SOURCE_ISO_DONE & (ADI_RTC_INT_SOURCE_TYPE)nEvent) {
    PERF("got RTC interrupt callback with ADI_RTC_INT_SOURCE_ISO_DONE status");
  }

  if (ADI_RTC_INT_SOURCE_LCD_UPDATE & (ADI_RTC_INT_SOURCE_TYPE)nEvent) {
    PERF(
        "got RTC interrupt callbackwithon ADI_RTC_INT_SOURCE_LCD_UPDATE "
        "status");
  }

  if (ADI_RTC_INT_SOURCE_ALARM & (ADI_RTC_INT_SOURCE_TYPE)nEvent) {
    PERF("got RTC interrupt callback with ADI_RTC_INT_SOURCE_ALARM status");
    bRtcAlarmFlag = true;       // note alarm flag
    bHibernateExitFlag = true;  // exit hibernation on return from interrupt
  }

  if (ADI_RTC_INT_SOURCE_FAIL & (ADI_RTC_INT_SOURCE_TYPE)nEvent) {
    PERF("got RTC interrupt callback with ADI_RTC_INT_SOURCE_FAIL status");
  }
}

/* WUT callback */
static void wutCallback(void *hWut, uint32_t Event, void *pArg) {
  pADI_WUT->T2CLRI |= T2CLRI_WUFB_CLR;
  bHibernateExitFlag = true;
  bWdtInterrupt = true;
  FAIL("RTC test failed: Got failsafe WUT interrupt");
}