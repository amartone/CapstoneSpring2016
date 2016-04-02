/*********************************************************************************

Copyright (c) 2013-2014 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.  By
using
this software you agree to the terms of the associated Analog Devices Software
License Agreement.

*********************************************************************************/

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

