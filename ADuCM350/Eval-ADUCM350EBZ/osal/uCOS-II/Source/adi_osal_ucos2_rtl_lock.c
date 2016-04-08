/* $Revision: 11977 $
 * $Date: 2011-11-04 07:23:10 -0400 (Fri, 04 Nov 2011) $
******************************************************************************
Copyright (c), 2008-2011 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
*****************************************************************************/

/*!
    @file adi_osal_ucos3_rtl_lock.c

    Operating System Abstraction Layer - OSAL for uCOS-II 

    This file contains the APIs designed to abstract the runtime library
    locking mechanism so each RTOS can adapt them according to its needs.

*/

/*=============  I N C L U D E S   =============*/

#include "adi_osal.h"
#include "osal_ucos.h"
#include "osal_common.h"



/* Rule-5.1 indicates that all identifiers shall not rely on more than 31 characters of significance */
#pragma diag(suppress:misra_rule_5_1:"prefixes added in front of identifiers quickly increases their size. In order to keep the code self explanatory, and since ADI tools are the main targeted tools, this restriction is not really a necessity")


/*=============  D E F I N E S  =============*/

#pragma file_attr(  "libGroup=adi_osal.h")
#pragma file_attr(  "libName=libosal")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")


/*!
  ****************************************************************************
    @brief Acquires the RTL global lock

    @details
    API used by the runtime library for its internal locking mechanism. In the
    case of uCOS-II this is implemented as a scheduler region lock because 
    functions like heap_free must be allowed in the task destuction hook which
    is called with the scheduler lock held.

    Other RTOS might implement this API as a mutex pend.
    
    @return ADI_OSAL_SUCCESS - In all cases

    @see adi_osal_SchedulerLock
    @see adi_osal_RTLGlobalsUnlock

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_RTLGlobalsLock( void)
{
    adi_osal_SchedulerLock();

    return( ADI_OSAL_SUCCESS);
}


/*!
  ****************************************************************************
    @brief Releases the RTL global lock

    @details
    API used by the runtime library for its internal locking mechanism. In the
    case of uCOS-II this is implemented as a scheduler region unlock because 
    functions like heap_free must be allowed in the task destuction hook which
    is called with the scheduler lock held.

    @return ADI_OSAL_SUCCESS - If the lock was released successfully
    @return ADI_OSAL_FAILED  - If the function call does not match a call to
                               adi_osal_RTLGlobalsLock/adi_osal_SchedulerLock

    @see adi_osal_SchedulerUnlock
    @see adi_osal_RTLGlobalsLock
*****************************************************************************/


ADI_OSAL_STATUS adi_osal_RTLGlobalsUnlock( void)
{
    return (adi_osal_SchedulerUnlock());

}
/*
**
** EOF: 
**
*/
