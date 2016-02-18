
Impedance Measurement Example: 4-Wire Isolated BioImpedance Application Example

To target the IEC-60601 standards the ADuCM350 is used in conjunction with an 
external Instrumentation Amplifier (the AD8226 in this example) to complete high 
precision absolute measurements using a 4-Wire measurement technique.
 
Z(Magnitude) = (Voltage Magnitude / Current Magnitude) * (1.5/1.494) * RTIA 

- The Current Measurement value is actually converted to a voltage, using the 
  RTIA, for measurement purposes. This needs to be taken into account.
- The 1.5 Gain in the equation is the ratio between the gain of the ADuCM350 
  Current Measurement channel which is 1.5 versus the gain of the ADuCM350 
  Voltage Measurement channel which is 1.
- The gain of the In-Amp is determined by the selection of RG, For the AD8226 
  this is determined by the equation:
        RG = (49.4k) / (G-1)
  Choosing RG = 100k we get a gain of 1.494

For this example, the calibration of the Auxiliary Channel and TIA channel needs 
to take into account the gain through the system. 
- For the Voltage measurement channel the Auxiliary channel is calibrated.
- For the Current measurement channel the Temp sensor is calibrated and these 
  results are loaded to the offset and gain registers of the TIA channel. This 
  ensures that the difference between the voltage and current gain is exactly 1.5. 

After the initialization, power up and calibration steps, 2 DFT measurements are 
performed, returning the real and imaginary values for the current and the
voltage.
Note: There is a delay of ~125ms after powering up the AFE to allow VBIAS to settle.

The test then calculates the magnitude and phase of the DFT results and
reports the magnitude and phase of the impedance.
Note: ADI are currently reviewing the phase calculations and results.

The example doesn't use floating-point, all the arithmetic is performed using
fixed-point. The fixed-point types and functions defined in CMSIS DSP library
are used  whenever possible. A custom fixed-point type, with 28 integer bits
and 4 fractional bits, is used to store the final results.

The excitation voltage and amplitude, as well as RCAL and RTIA values, are 
programmable through macros. 
Note: there are no checks in the code that the values are within admissible ranges,
which needs to be ensured by the user.

When using the Eval-ADuCM350EBZ board, the test needs a daughter board attached
to the evaluation board, with the relevant impedances populated.

Once the test has finished, it sends a result message string to STDIO;
"PASS" for success and "FAIL" (plus failure message) for failures.

The raw DFT complex results (real and imaginary parts) are also reported
through the UART/STDIO, as well as the final calibrated unknown impedances,
represented in polar coordinates (magnitude and phase). The USE_UART_FOR_DATA 
macro determines whether the results are returned to the UART or STDIO.
To return data is using the UART, set the macro USE_UART_FOR_DATA = 1.
To return data is using STDIO, set the macro USE_UART_FOR_DATA = 0.

For Eval-ADUCM350EBZ boards, the results are returned to the PC/host via the 
UART-on-USB interface on the USB-SWD/UART-EMUZ (Rev.C) board to a listening 
PC based terminal application tuned to the corresponding virtual serial port. 
See the ADuCM350 Device Drivers Getting Started Guide for information on drivers 
and configuring the PC based terminal application.
