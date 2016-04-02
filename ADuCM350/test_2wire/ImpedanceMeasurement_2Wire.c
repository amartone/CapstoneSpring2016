/*********************************************************************************

Copyright (c) 2013-2014 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.  By
using
this software you agree to the terms of the associated Analog Devices Software
License Agreement.

*********************************************************************************/

/*****************************************************************************
 * @file:    ImpedanceMeasurement_2Wire.c
 * @brief:   Impedance measurement example for ADuCM350 in 2-wire configuration
 * @version: $Revision: 29403 $
 * @date:    $Date: 2015-01-08 10:34:56 -0500 (Thu, 08 Jan 2015) $
 *****************************************************************************/

#include <stddef.h>  // for 'NULL'
#include <stdio.h>
#include <string.h>  // for strncmp
#include <time.h>

#include "arm_math.h"

#include "test_common.h"

#include "afe.h"
#include "afe_lib.h"
#include "uart.h"

#include "gpio.h"
#include "rtc.h"
#include "wut.h"

/* Macro to enable the returning of AFE data using the UART */
/*      1 = return AFE data on UART                         */
/*      0 = return AFE data on SW (Std Output)              */
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

static void Ext_Int8_Callback(void *pCBParam, uint32_t Event, void *pArg);

ADI_UART_HANDLE hUartDevice = NULL;

/* Function prototypes */
q15_t arctan(q15_t imag, q15_t real);
fixed32_t calculate_magnitude(q31_t magnitude_rcal, q31_t magnitude_z);
fixed32_t calculate_phase(q15_t phase_rcal, q15_t phase_z);
void convert_dft_results(int16_t *dft_results, q15_t *dft_results_q15,
                         q31_t *dft_results_q31);
void sprintf_fixed32(char *out, fixed32_t in);
void print_MagnitudePhase(char *text, fixed32_t magnitude, fixed32_t phase);
void test_print(char *pBuffer);
ADI_UART_RESULT_TYPE uart_Init(void);
ADI_UART_RESULT_TYPE uart_UnInit(void);
extern int32_t adi_initpinmux(void);

/* Sequence for AC measurement, performs 4 DFTs:        */
/*     RCAL, AFE3-AFE4, AFE4-AFE5, AFE3-AFE5            */
uint32_t
    seq_afe_acmeas2wire[] =
        {
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

uint32_t nummeasurements;
uint8_t done;

int main(void) {
  ADI_AFE_DEV_HANDLE hDevice;
  int16_t dft_results[DFT_RESULTS_COUNT];
  q15_t dft_results_q15[DFT_RESULTS_COUNT];
  q31_t dft_results_q31[DFT_RESULTS_COUNT];
  q31_t magnitude[DFT_RESULTS_COUNT / 2];
  q15_t phase[DFT_RESULTS_COUNT / 2];
  fixed32_t magnitude_result[DFT_RESULTS_COUNT / 2 - 1];
  fixed32_t phase_result[DFT_RESULTS_COUNT / 2 - 1];
  char msg[MSG_MAXLEN];
  int8_t i;
  done = 0;

  nummeasurements = 0;

  /* Initialize system */
  SystemInit();

  /* Change the system clock source to HFXTAL and change clock frequency to
   * 16MHz     */
  /* Requirement for AFE (ACLK) */
  SystemTransitionClocks(ADI_SYS_CLOCK_TRIGGER_MEASUREMENT_ON);

  /* SPLL with 32MHz used, need to divide by 2 */
  SetSystemClockDivider(ADI_SYS_CLOCK_UART, 2);

  ADI_WUT_DEV_HANDLE hWUT;
  uint32_t rtcCount;

  /* initialize driver */
  rtc_Init();

  /* calibrate */
  rtc_Calibrate();

  /* initialize flags */
  bRtcAlarmFlag = bRtcInterrupt = bWdtInterrupt = false;

  /* get the current count */
  if (ADI_RTC_SUCCESS != adi_RTC_GetCount(hRTC, &rtcCount))
    FAIL("adi_RTC_GetCount failed");

  /* Test initialization */
  test_Init();

  /* initialize static pinmuxing */
  adi_initpinmux();

  /* Initialize the UART for transferring measurement data out */
  if (ADI_UART_SUCCESS != uart_Init()) {
    FAIL("uart_Init");
  }

  /* Initialize the AFE API */
  if (adi_AFE_Init(&hDevice)) {
    FAIL("adi_AFE_Init");
  }

  /* AFE power up */
  if (adi_AFE_PowerUp(hDevice)) {
    FAIL("adi_AFE_PowerUp");
  }

  /* Excitation Channel Power-Up */
  if (adi_AFE_ExciteChanPowerUp(hDevice)) {
    FAIL("adi_AFE_ExciteChanPowerUp");
  }

  /* TIA Channel Calibration */
  if (adi_AFE_TiaChanCal(hDevice)) {
    FAIL("adi_AFE_TiaChanCal");
  }

  /* Excitation Channel Calibration (Attenuation Enabled) */
  if (adi_AFE_ExciteChanCalAtten(hDevice)) {
    FAIL("adi_AFE_ExciteChanCalAtten");
  }

  /* Update FCW in the sequence */
  seq_afe_acmeas2wire[3] = SEQ_MMR_WRITE(REG_AFE_AFE_WG_FCW, FCW);
  /* Update sine amplitude in the sequence */
  seq_afe_acmeas2wire[4] =
      SEQ_MMR_WRITE(REG_AFE_AFE_WG_AMPLITUDE, SINE_AMPLITUDE);

  /* Recalculate CRC in software for the AC measurement, because we changed   */
  /* FCW and sine amplitude settings                                          */
  adi_AFE_EnableSoftwareCRC(hDevice, true);

  /* Perform the Impedance measurement */
  if (adi_AFE_RunSequence(hDevice, seq_afe_acmeas2wire, (uint16_t *)dft_results,
                          DFT_RESULTS_COUNT)) {
    FAIL("Impedance Measurement");
  }

  /* set RTC alarm */
  printf("rtcCount: %d\r\n", rtcCount);
  if (ADI_RTC_SUCCESS != adi_RTC_SetAlarm(hRTC, rtcCount + 120))
    FAIL("adi_RTC_SetAlarm failed");

  /* enable RTC alarm */
  if (ADI_RTC_SUCCESS != adi_RTC_EnableAlarm(hRTC, true))
    FAIL("adi_RTC_EnableAlarm failed");

  /* enable alarm interrupting */
  if (ADI_RTC_SUCCESS !=
      adi_RTC_EnableInterrupts(hRTC, ADI_RTC_INT_ENA_ALARM, true))
    FAIL("adi_RTC_EnableInterrupts failed");

  q31_t magnitudecal;
  q15_t phasecal;

  convert_dft_results(dft_results, dft_results_q15, dft_results_q31);
  arm_cmplx_mag_q31(dft_results_q31, &magnitudecal, 2);

  phasecal = arctan(dft_results[1], dft_results[0]);

  printf("raw rcal data: %d, %d\r\n", dft_results[1], dft_results[0]);
  printf("rcal (magnitude, phase) = (%d, %d)\r\n", magnitudecal, phasecal);

  ADI_AFE_TypeDef *pAFE = pADI_AFE;
  while (true /*!done*/) {
    // delay(2000000);
    // printf("%8X : %8X\r\n", &(pADI_AFE->AFE_DFT_RESULT_REAL),
    // (pADI_AFE->AFE_DFT_RESULT_REAL));
    // printf("%8X : %d\r\n", &(pADI_AFE->AFE_ADC_RESULT),
    // (pADI_AFE->AFE_ADC_RESULT));
    //        printf("DFT results (real, imaginary) : (%6d, %6d)\r\n",
    //        pADI_AFE->AFE_DFT_RESULT_REAL, pADI_AFE->AFE_DFT_RESULT_IMAG );
    dft_results[0] = pADI_AFE->AFE_DFT_RESULT_REAL;
    dft_results[1] = pADI_AFE->AFE_DFT_RESULT_IMAG;

    // Convert DFT results to 1.15 and 1.31 formats.
    convert_dft_results(dft_results, dft_results_q15, dft_results_q31);

    // Magnitude calculation
    // Use CMSIS function
    arm_cmplx_mag_q31(dft_results_q31, magnitude, DFT_RESULTS_COUNT / 2);

    // Calculate final magnitude values, calibrated with RCAL.
    // for (i = 0; i < DFT_RESULTS_COUNT / 2 - 1; i++)
    //{
    fixed32_t magnituderesult;
    magnituderesult = calculate_magnitude(magnitudecal, magnitude[0]);
    //}

    // phase[0] = arctan(dft_results[1], dft_results[0]);
    q15_t phaseresult;
    phaseresult = arctan(dft_results[1], dft_results[0]);
    fixed32_t phasecalibrated;

    // calibrate with phase from rcal
    phasecalibrated = calculate_phase(phasecal, phaseresult);
    // printf("final results (magnitude, phase) : (%6d, %6d)\r\n",
    // magnitude_result[0], phasecalibrated );
    print_MagnitudePhase("", magnituderesult, phasecalibrated);
    nummeasurements++;
  }

  /* Restore to using default CRC stored with the sequence */
  adi_AFE_EnableSoftwareCRC(hDevice, false);

  /* Print DFT complex results */
  /*sprintf(msg, "DFT results (real, imaginary):\r\n");
  PRINT(msg);
  sprintf(msg, "    RCAL        = (%6d, %6d)\r\n", dft_results[0],
  dft_results[1]);
  PRINT(msg);
  sprintf(msg, "    AFE3 - AFE4 = (%6d, %6d)\r\n", dft_results[2],
  dft_results[3]);
  PRINT(msg);
  sprintf(msg, "    AFE4 - AFE5 = (%6d, %6d)\r\n", dft_results[4],
  dft_results[5]);
  PRINT(msg);
  sprintf(msg, "    AFE3 - AFE5 = (%6d, %6d)\r\n", dft_results[6],
  dft_results[7]);
  PRINT(msg);*/

  /* Convert DFT results to 1.15 and 1.31 formats.  */
  convert_dft_results(dft_results, dft_results_q15, dft_results_q31);

  /* Magnitude calculation */
  /* Use CMSIS function */
  arm_cmplx_mag_q31(dft_results_q31, magnitude, DFT_RESULTS_COUNT / 2);

  /* Calculate final magnitude values, calibrated with RCAL. */
  for (i = 0; i < DFT_RESULTS_COUNT / 2 - 1; i++) {
    magnitude_result[i] = calculate_magnitude(magnitude[0], magnitude[i + 1]);
  }

  /* Phase calculation */
  /* RCAL first */
  phase[0] = arctan(dft_results[1], dft_results[0]);
  for (i = 0; i < DFT_RESULTS_COUNT / 2 - 1; i++) {
    /* No need to calculate the phase if magnitude is 0 (open circuit) */
    if (magnitude_result[i].full) {
      /* First the measured phase. */
      phase[i + 1] =
          arctan(dft_results[2 * (i + 1) + 1], dft_results[2 * (i + 1)]);
      /* Then the phase calibrated with RCAL. */
      phase_result[i] = calculate_phase(phase[0], phase[i + 1]);
    } else {
      phase[i + 1] = 0;
      phase_result[i].full = 0;
    }
  }

  /* Print final results */
  /* PRINT("Final results (magnitude, phase):\r\n");
   print_MagnitudePhase("AFE3 - AFE4", magnitude_result[0], phase_result[0]);
   print_MagnitudePhase("AFE4 - AFE5", magnitude_result[1], phase_result[1]);
   print_MagnitudePhase("AFE3 - AFE5", magnitude_result[2], phase_result[2]);
*/
  /* AFE Power Down */
  if (adi_AFE_PowerDown(hDevice)) {
    FAIL("adi_AFE_PowerDown");
  }

  /* Uninitialize the AFE API */
  if (adi_AFE_UnInit(hDevice)) {
    FAIL("adi_AFE_UnInit");
  }

  /* Uninitialize the UART */
  adi_UART_UnInit(hUartDevice);

  PASS();
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

/* Helper function for printing fixed32_t (magnitude & phase) results */
void print_MagnitudePhase(char *text, fixed32_t magnitude, fixed32_t phase) {
  char msg[MSG_MAXLEN];
  char tmp[MSG_MAXLEN];
  sprintf(msg, "%s", text);
  // sprintf(msg, "    %s = (", text);
  /* Magnitude */
  sprintf_fixed32(tmp, magnitude);
  strcat(msg, tmp);
  // strcat(msg, ", ");
  /* Phase */
  // sprintf_fixed32(tmp, phase);
  //    strcat(msg, tmp);
  // strcat(msg, ")\r\n");
  strcat(msg, "\r\n");
  PRINT(msg);
}

/* Helper function for printing a string to UART or Std. Output */
void test_print(char *pBuffer) {
#if (1 == USE_UART_FOR_DATA)
  int16_t size;
  /* Print to UART */
  size = strlen(pBuffer);
  adi_UART_BufTx(hUartDevice, pBuffer, &size);

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
  /* Set UART baud rate to 9600 */
  if (ADI_UART_SUCCESS !=
      (result = adi_UART_SetBaudRate(hUartDevice, ADI_UART_BAUD_9600))) {
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

  /* program GPIO to capture P0.10 pushbutton interrupt... */

  /* initialize GPIO driver */
  if (adi_GPIO_Init()) {
    FAIL("adi_GPIO_Init failed");
  }

  /* enable P0.10 input */
  if (adi_GPIO_SetInputEnable(ADI_GPIO_PORT_0, ADI_GPIO_PIN_10, true)) {
    FAIL("Initialise_GPIO: adi_GPIO_SetInputEnable failed");
  }

  /* disable P0.10 output */
  if (adi_GPIO_SetOutputEnable(ADI_GPIO_PORT_0, ADI_GPIO_PIN_10, false)) {
    FAIL("Initialise_GPIO: adi_GPIO_SetOutputEnable failed");
  }

  /* set P0.10 pullep enable */
  if (adi_GPIO_SetPullUpEnable(ADI_GPIO_PORT_0, ADI_GPIO_PIN_10, true)) {
    FAIL("Initialise_GPIO: adi_GPIO_SetOutputEnable failed");
  }

  /* Register the external interrupt callback */
  if (adi_GPIO_RegisterCallback(EINT8_IRQn, Ext_Int8_Callback, NULL)) {
    FAIL("Initialise_GPIO: adi_GPIO_RegisterCallbackExtInt failed");
  }

  /* enable P0.10 as external interrupt */
  if (adi_GPIO_EnableIRQ(EINT8_IRQn, ADI_GPIO_IRQ_EITHER_EDGE)) {
    FAIL("Initialise_GPIO: adi_GPIO_EnableIRQ failed");
  }

  /* release GPIO driver */
  if (adi_GPIO_UnInit()) {
    FAIL("adi_GPIO_Init failed");
  }
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

/* External interrupt callback */
static void Ext_Int8_Callback(void *pCBParam, uint32_t Event, void *pArg) {
  /* clear the interrupt */
  NVIC_ClearPendingIRQ(EINT8_IRQn);

  /* also call hibernation exit API */
  SystemExitLowPowerMode(&bHibernateExitFlag);
}
