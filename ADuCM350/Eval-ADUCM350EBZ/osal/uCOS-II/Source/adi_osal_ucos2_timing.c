/* $Revision: 11977 $
 * $Date: 2011-11-04 07:23:10 -0400 (Fri, 04 Nov 2011) $
******************************************************************************
Copyright (c), 2008-2011 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************/
/*!
    @file adi_osal_ucos_timing.c

    Operating System Abstraction Layer - OSAL for uCOS-II - Timing
    functions

    This file contains the Timing APIs for the uCOS-II implementation of
    OSAL

*/


/*=============  I N C L U D E S   =============*/

#include "adi_osal.h"
#include "osal_ucos.h"
#include "osal_common.h"
#include <limits.h>

#if defined (__ECC__)
/*Rule 14.7 indicates that a function shall have a single exit point */
#pragma diag(suppress:misra_rule_14_7:"Allowing several point of exit (mostly for handling parameter error checking) increases the code readability and therefore maintainability")
/* Rule-11.3 indicates that typecast of a integer value into pointer is invalid */
#pragma diag(suppress:misra_rule_11_3 : "typecasting is necessary everytime a predefine value is written to a return pointer during error conditions")


/*=============  D E F I N E S  =============*/

#pragma file_attr(  "libGroup=adi_osal.h")
#pragma file_attr(  "libName=libosal")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")
#endif



/*=============  D A T A  =============*/




/*=============  C O D E  =============*/


/*!
  ****************************************************************************
    @brief Returns the duration of a tick period in microseconds.

    @param[out] pnTickPeriod - pointer to a location to write the tick period 
                               in microseconds.

    @return ADI_OSAL_SUCCESS - If the function successfully returned the 
                               duration of the tick period in microseconds.

  Notes:
      This function  helps to convert  time units to system ticks which is
      needed by the pend APIs of message-Q,semaphore,mutex,event  and to
      put the task in "sleep" mode.

                                                   No. Microsec in one second
      Duration of the tick period (in micro second) =  -------------------------
                                                   No of ticks in one second
*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_TickPeriodInMicroSec(uint32_t *pnTickPeriod)
{
	/* In uCOS-II the variable to transform the tick period to ms is
	 * only present in the micrium debug configuration. For this reason we
	 * cannot reliably give anything of any use to customers unless they set
	 * it themselves in adi_osal_Config.
	 */
	if (UINT_MAX == _adi_osal_gnTickPeriod)
	{
		*pnTickPeriod = UINT_MAX;
		return(ADI_OSAL_FAILED);
	}
	else
	{
		*pnTickPeriod = _adi_osal_gnTickPeriod;
		return(ADI_OSAL_SUCCESS);
	}
}


/*!
  ****************************************************************************
  @brief Processes a clock tick

  This indicates to the OS that a tick period is completed.

*****************************************************************************/

void adi_osal_TimeTick(void)
{
    OSTimeTick();

    return;
}



/*!
  ****************************************************************************
    @brief Returns the current value of the continuously incrementing timer 
           tick counter. 
  
    The counter increments once for every timer interrupt.

    @param[out] pnTicks - pointer to a location to write the current value of 
                          the tick counter.

    @return ADI_OSAL_SUCCESS - If the function successfully returned the tick 
                               counter value

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_GetCurrentTick(uint32_t *pnTicks )
{
    *pnTicks = OSTimeGet();

    return(ADI_OSAL_SUCCESS);
}

/*
**
** EOF: 
**
*/
