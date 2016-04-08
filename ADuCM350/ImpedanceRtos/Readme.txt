##########################################################
#
# Readme.txt  - RtosTest example
#
##########################################################

This example is used to verify that the build of the driver files when
the macro ADI_CFG_ENABLE_RTOS_SUPPORT=1 results in a clean build.

The example will actually run (simply calling the init function of each
driver). This will ensure that the OSAL related APIs for uC/OS-II are
successfully compiled and linked.

The default template version of lib_cfg.h (from Micrium) is used without change.
(Typically, applications may override this default file by providing
their own app_cfg.h and use that instead by switching the initial include
define at the top of lib_cfg.h, as needed.)
        
        
RUNNING THE TEST
    1. Build, download the test to an ADuCM350 target board.
    2. Run the program.


OUTPUT
Test output is printed to STDIO, which is reported to the debugger console via the
semi-hosting facility (see the "Capturing STDIO from Examples" section of the
"ADuCM350BBCZ Software Users Guide").  Note: semi-hosting is enabled by default in
most examples.  Typically, each example prints "PASS!" or "FAIL: <filename,
linenumber, message>" messages, depending on success or failure of the example run.
Some tests provide additional performance data or message reports during the test
run.  Performance messages are formatted as "PERF: <message>".


REAL-TIME CLOCK FAILURES
The RtosTest example depends on the on-chip Real-Time Clock (RTC) to be set to a
"good" time because RTC device driver interface is tested as part of RtosTest.
The RtosTest will fail if the RTC has never been powered or has been unpowered
without a battery for more than 24 hours (exceeding the SuperCap charge reserve);
in which case the RTC hardware reports an alarm indicating that the RTC internal
time is unreliable.  The RTC device driver detects this alarm state and reports
the ADI_RTC_ERR_CLOCK_FAILSAFE result code back to RtosTest, in turn causing it
to fail.

It is the application's responsiblity to clear this alarm (see the adi_RTC_ClearFailSafe()
API) and set the RTC clock (see the adi_RTC_SetCount() API) to a known value.  Please refer
to the RtcTest example and device driver documentation for details on setting the RTC count
to a meaningful time and date, as well as a discussion on trimming the RTC.


HARDWARE SETUP
No hardware setup is required.
