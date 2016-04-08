/* $Revision: 21629 $
 * $Date: 2013-05-29 08:01:48 -0400 (Wed, 29 May 2013) $
******************************************************************************
Copyright (c), 2008-2011 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************/

/*!
    @file adi_osal_ucos_critical.c
    
    Operating System Abstraction Layer - OSAL for uCOS-II - Critical section
    functions

    This file contains the critical section & scheduler locking APIs for the
    uCOS-II implementation of OSAL

*/


/*=============  I N C L U D E S   =============*/

#include "adi_osal.h"
#include "osal_ucos.h"
#if defined __ECC__

/*Rule 14.7 indicates that a function shall have a single exit point */
#pragma diag(suppress:misra_rule_14_7:"Allowing several point of exits (mostly for handling parameter error checking) increases the code readability and therefore maintainability")
/* Rule-5.1 indicates that all identifiers shall not rely on more than 31 characters of significance */
#pragma diag(suppress:misra_rule_5_1:"prefixes added in front of identifiers quickly increases their size. In order to keep the code self explanatory, and since ADI tools are the main targeted tools, this restriction is not really a necessity")


/*=============  D E F I N E S  =============*/

#pragma file_attr(  "libGroup=adi_osal.h")
#pragma file_attr(  "libName=libosal")
#pragma file_attr(  "prefersMem=internal")
#pragma file_attr(  "prefersMemNum=30")
#endif


/*=============  D A T A  =============*/
#ifdef OSAL_DEBUG
/*!
    @internal
    @var _adi_osal_gnSchedulerLockCnt
         Indicates if the code is within a Scheduler lock section. It is only
         used in debug mode to check if Unlock is called with Lock being called
         first. This needs to be a counter to allow for nesting
         Initially the scheduler is not locked
    @endinternal
*/
uint32_t _adi_osal_gnSchedulerLockCnt = 0u ;

#endif /* OSAL_DEBUG */

/*!
    @internal
    @var _adi_osal_gnCriticalRegionNestingCnt
         This variable is a counter which is incremented when
         adi_osal_EnterCriticalRegion() is called and decremented when
         adi_osal_ExitCriticalRegion is called.
         Initially we are not in a critical region.
    @endinternal
*/
int32_t _adi_osal_gnCriticalRegionNestingCnt = 0;



/*=============  C O D E  =============*/



/*!
  ****************************************************************************
    @brief Determines whether the scheduler is running.

    @return true  - If the scheduler is running, 
    @return false - If the scheduler is not running
*****************************************************************************/

bool adi_osal_IsSchedulerActive(void)
{
    /* although the variable OSRunning is not part of the public
     * interface, we have confirmed with Micrium that it will remain
     * there in all versions of the OS 
     */
  return(OSRunning == (BOOLEAN) OS_TRUE);
}


/*!
  ****************************************************************************
    @brief Prevents rescheduling until adi_osal_SchedulerUnlock is called.

    After this function is called, the current thread does not become
    de-scheduled , even if a high-priority thread becomes ready to run. 
    
    Note that calls to adi_osal_SchedulerLock may be nested. A count is
    maintained to ensure that a matching number of calls to
    adi_osal_SchedulerUnlock are made before scheduling is re-enabled.

    @see adi_osal_SchedulerUnlock
*****************************************************************************/

void   adi_osal_SchedulerLock( void )
{
    OSSchedLock();

#ifdef OSAL_DEBUG
    _adi_osal_gnSchedulerLockCnt++;
#endif /* OSAL_DEBUG */
    return;
}


/*!
  ****************************************************************************
    @brief Re-enables thread scheduling.

    This function decrements the internal count which tracks how many times
    adi_osal_SchedulerLock was called. The API relies on the RTOS to
    enable scheduling when appropriate

    @return ADI_OSAL_SUCCESS - If thread scheduling was enabled successfully
    @return ADI_OSAL_FAILED  - If the function call does not match a call to 
                               adi_osal_SchedulerLock

    @see adi_osal_SchedulerLock
*****************************************************************************/

ADI_OSAL_STATUS adi_osal_SchedulerUnlock( void )
{
#ifdef OSAL_DEBUG

    if (0u == _adi_osal_gnSchedulerLockCnt)
    {
        return (ADI_OSAL_FAILED);       /* if the Unlock function is called before the lock, return an error */
    }
    _adi_osal_gnSchedulerLockCnt--;             /* it must be done before unlocking */
#endif /* OSAL_DEBUG */

    OSSchedUnlock();                    /* uCOS OSSchedUnlock function takes care of nesting itself */

    return (ADI_OSAL_SUCCESS);
}

/*
**
** EOF: 
**
*/
