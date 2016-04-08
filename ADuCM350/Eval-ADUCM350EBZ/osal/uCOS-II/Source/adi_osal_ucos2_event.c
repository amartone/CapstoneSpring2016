/* $Revision: 21629 $
 * $Date: 2013-05-29 08:01:48 -0400 (Wed, 29 May 2013) $
******************************************************************************
Copyright (c), 2008-2011 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************/
/*!
    @file adi_osal_ucos_event.c

    Operating System Abstraction Layer - OSAL for uCOS-II - Events
    functions

    This file contains the Events APIs for the uCOS-II implementation of
    OSAL

    OSAL requires the events groups to be 32 bits, uCOS-II allows several
    options: 8, 16, or 32 bits.  Users should have chosen the right option (32
    bits) but we cannot check because the information is only available when
    uCOS debug is selected.


*/


/*=============  I N C L U D E S   =============*/

#include <string.h>                                                             /* for strncpy */
#include "adi_osal.h"
#include "osal_ucos.h"
#include "osal_common.h"
#include <os_cpu.h>

#if defined (__ECC__)
/* Rule-11.3 indicates that typecast of a integer value into pointer is invalid */
#pragma diag(suppress:misra_rule_11_3 : "typecasting is necessary everytimes a predefine value is written to a return pointer during error conditions")
/*Rule 14.7 indicates that a function shall have a single exit point */
#pragma diag(suppress:misra_rule_14_7:"Allowing several point of exit (mostly for handling parameter error checking) increases the code readability and therefore maintainability")
/* Rule-16.2 indicates that  Functions shall not call themselves,either directly or indirectly */
#pragma diag(suppress:misra_rule_16_2 : "Since the OSAL is reentrant by nature (several thread could call the API) the compiler MISRA checker mistakes sometimes the reentrancy for recurrence")
/* Rule-2.1 indicates that Assembly language shall be encapsulated and isolated */
#pragma diag(suppress:misra_rule_2_1 : "In this case we use macros to isolate an assembly function, for readability reasons, it's been applied to the whole file and not around each macro call")
/* Rule-5.1 indicates that all identifiers shall not rely on more than 31 characters of significance */
#pragma diag(suppress:misra_rule_5_1:"prefixes added in front of identifiers quickly increases their size. In order to keep the code self explanatory, and since ADI tools are the main targeted tools, this restriction is not really a necessity")


/*=============  D E F I N E S  =============*/

#pragma file_attr(  "libGroup=adi_osal.h")
#pragma file_attr(  "libName=libosal")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")
#endif


/*=============  D A T A  =============*/




/*=============  C O D E  =============*/

static ADI_OSAL_STATUS osal_ModifyFlags( ADI_OSAL_EVENT_HANDLE const hEventGroup,
                                      ADI_OSAL_EVENT_FLAGS nEventFlags,
                                      uint32_t nOption );



/*!
  ****************************************************************************
    @brief Creates an event group with the memory which has already been
            provided.

    @param[in] nEventObjSize - Size of the memory passed for the creation of
                               the event group
    @param[in] pEventObject  - Area of memory provided to us for the event group
 
    This is typically used to synchronize threads with events that happen in
    the system.

    @param[out] phEventGroup - Pointer to a location to write the returned 
                               event group handle

    @return ADI_OSAL_SUCCESS      - If event group is created successfully
    @return ADI_OSAL_FAILED       - If failed to create event group
    @return ADI_OSAL_CALLER_ERROR - If function is invoked from an invalid 
                                    location

*****************************************************************************/
ADI_OSAL_STATUS adi_osal_EventGroupCreateStatic(void* const pEventObject, uint32_t nEventObjSize, ADI_OSAL_EVENT_HANDLE *phEventGroup)
{
    ADI_OSAL_STATUS eRetStatus;
#ifdef OSAL_DEBUG
    if ( nEventObjSize < sizeof(OS_EVENT) )
    {
        *phEventGroup = ADI_OSAL_INVALID_EVENT_GROUP;
        return (ADI_OSAL_MEM_TOO_SMALL);
    }

    if ((false == _adi_osal_IsMemoryAligned(pEventObject)) || (NULL == pEventObject) )
    {
        *phEventGroup = ADI_OSAL_INVALID_EVENT_GROUP;
        return (ADI_OSAL_BAD_MEMORY);
    }
#endif /* OSAL_DEBUG */

    eRetStatus =  adi_osal_EventGroupCreate(phEventGroup);

    /* the memory came from uCOS so we don't really need the SemObject and size
     * for anything. There seems no point copying things there. */
    return (eRetStatus);
}
/*!
  ****************************************************************************
    @brief Creates a event group.

    @param[out] phEventGroup    - Pointer to a location to write the returned
                             event group ID

    @return ADI_OSAL_SUCCESS      - If event is created successfully
    @return ADI_OSAL_FAILED       - If failed to create event
    @return ADI_OSAL_CALLER_ERROR - If the call is made from an invalid location
                                    (i.e an ISR)

  Note:
      phEventGroup set to "ADI_OSAL_INVALID_EVENT_GROUP" if event creation is failed.
 *****************************************************************************/
ADI_OSAL_STATUS adi_osal_EventGroupCreate(ADI_OSAL_EVENT_HANDLE *phEventGroup)
{
    OS_FLAG_GRP* pEventNative;
    INT8U nErr;
    ADI_OSAL_STATUS eRetStatus;

#ifdef OSAL_DEBUG
    if (NULL == phEventGroup)
    {
        return(ADI_OSAL_FAILED);
    }


    if (CALLED_FROM_AN_ISR)
    {
        *phEventGroup = ADI_OSAL_INVALID_EVENT_GROUP;
        return (ADI_OSAL_CALLER_ERROR);
    }
#endif /* OSAL_DEBUG */

    /* Flags are initially all created as unset (0x0) */
    pEventNative = OSFlagCreate(0x0u, &nErr);
    switch (nErr)
    {
        case OS_ERR_NONE:
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a pointer to a useful structure")
            *phEventGroup = (ADI_OSAL_EVENT_HANDLE) pEventNative;
#pragma diag(pop)
            eRetStatus = ADI_OSAL_SUCCESS;
            break;
#ifdef OSAL_DEBUG
        case OS_ERR_CREATE_ISR:
            *phEventGroup = ADI_OSAL_INVALID_EVENT_GROUP;
            eRetStatus = ADI_OSAL_CALLER_ERROR;
            break;

        case OS_ERR_FLAG_GRP_DEPLETED:
            *phEventGroup = ADI_OSAL_INVALID_EVENT_GROUP;
            eRetStatus = ADI_OSAL_MEM_ALLOC_FAILED;
            break;
#endif
        default:
            *phEventGroup = ADI_OSAL_INVALID_EVENT_GROUP;
            eRetStatus = ADI_OSAL_FAILED;
            break;
    } /* end of switch */
    return (eRetStatus);
}


/*!
  ****************************************************************************
  @brief Returns the size of a event group object.

  This function can be used by the adi_osal_EventGroupCreateStatic function to
  determine what the object size should be for this particular RTOS
  implementation.

  Parameters:
      None

    @return size of a event group object in bytes.

    @see adi_osal_EventGroupCreateStatic

*****************************************************************************/
uint32_t adi_osal_EventGroupGetObjSize(void)
{
    return ( sizeof(OS_FLAG_GRP) );
}

/*!
  ****************************************************************************
    @brief Destroys the specified Event group without freeing memory

    @param[in] hEventGroup - handle of the event group to be destroyed

    @return ADI_OSAL_SUCCESS         - If event group is destroyed
                                       successfully
    @return ADI_OSAL_FAILED          - If failed to create event group
    @return ADI_OSAL_CALLER_ERROR    - If function is invoked from an 
                                       invalid location
    @return ADI_OSAL_THREAD_PENDING  - If the destruction of the Event Group 
                                       is not possible because threads are 
                                       pending on events from that group
    @return ADI_OSAL_BAD_HANDLE      - Invalid event flag group ID

*****************************************************************************/
ADI_OSAL_STATUS adi_osal_EventGroupDestroyStatic(ADI_OSAL_EVENT_HANDLE const hEventGroup)
{
    ADI_OSAL_STATUS eRetValue;
    eRetValue = adi_osal_EventGroupDestroy(hEventGroup);
    /* we did not allocate any memory so there is nothing to free up */
    return (eRetValue);
}
/*!
  ****************************************************************************
    @brief Deletes a specified event group.

    @param[in]  hEvent      - The handle of the event group which need to be deleted

    @return ADI_OSAL_SUCCESS          - If event group is deleted successfully
    @return ADI_OSAL_FAILED           - If failed to delete event group
    @return ADI_OSAL_BAD_HANDLE       - If the specified event group handle is
                                        invalid
    @return ADI_OSAL_CALLER_ERROR     - If the call is made from an invalid
                                        location (i.e an ISR).
    @return ADI_OSAL_THREAD_PENDING   - If a thread is pending on the specified
                                        event group

 *****************************************************************************/

ADI_OSAL_STATUS adi_osal_EventGroupDestroy(ADI_OSAL_EVENT_HANDLE const hEventGroup)
{

    INT8U nErr;
    ADI_OSAL_STATUS eRetStatus;

#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a native uCOS pointer")
    OS_FLAG_GRP* hEventNative = (OS_FLAG_GRP*) hEventGroup;
#pragma diag(pop)


#ifdef OSAL_DEBUG
    if (CALLED_FROM_AN_ISR)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if ((NULL == hEventGroup) || (ADI_OSAL_INVALID_EVENT_GROUP == ADI_OSAL_INVALID_EVENT_GROUP))
    {
        return(ADI_OSAL_BAD_HANDLE);
    }
#endif /* OSAL_DEBUG */

    OSFlagDel(hEventNative, 
              OS_DEL_NO_PEND, /* delete if no threads are pending */
              &nErr);
    switch(nErr)
    {
        case OS_ERR_NONE:
            eRetStatus = ADI_OSAL_SUCCESS;
            break;
        case OS_ERR_TASK_WAITING:
            eRetStatus = ADI_OSAL_THREAD_PENDING;
            break;
#ifdef OSAL_DEBUG
        case OS_ERR_DEL_ISR:
            eRetStatus = ADI_OSAL_CALLER_ERROR;
            break;
        case OS_ERR_FLAG_INVALID_PGRP:
        case OS_ERR_EVENT_TYPE:
            eRetStatus = ADI_OSAL_BAD_HANDLE;
            break;
        case OS_ERR_INVALID_OPT:
            eRetStatus = ADI_OSAL_FAILED;
            break;
#endif /* OSAL_DEBUG */
        default:
            eRetStatus = ADI_OSAL_FAILED;
            break;
    }

    return (eRetStatus);
}

/*!
  ****************************************************************************
    @brief Waits for event flags

    @param[in] hEventGroup      - handle of the event group to use
    @param[in] nRequestedEvents - Specifies requested event flags.
    @param[in] eGetOption       - Specifies whether all bits need to be 
                                  set/cleared OR any of the bits to be set.
    @param[in] nTimeoutInTicks  - Timeout for the event flag in system ticks.
    @param[in] pnReceivedEvents - Pointer to destination of where the retrieved 
                                  event flags are placed.


         The following options are valid for setting flag eGetOption.

            ADI_OSAL_EVENT_FLAG_SET_ANY  - check any of the bits specified by
                                           the nRequestedEvents is set
            ADI_OSAL_EVENT_FLAG_SET_ALL  - check all the bits specified by the
                                           nRequestedEvents are set.

         Valid options for nTimeoutInTicks  are:

           ADI_OSAL_TIMEOUT_NONE     -  No wait. Results in an immediate return
                                        from this service  regardless of whether
                                        or not it was successful
           ADI_OSAL_TIMEOUT_FOREVER  -  Wait option for calling task to suspend
                                        indefinitely until the required flags are
                                        set.
           1 ... 0XFFFFFFFE          -  Selecting a numeric value specifies the
                                        maximum time limit (in system ticks) for
                                        set required event flags

    @return ADI_OSAL_SUCCESS      -  If there is no error while retrieving the 
                                     event flags. This does not indicate event 
                                     flag condition - the user must read the 
                                     flags separately.
    @return ADI_OSAL_FAILED       -  If an error occurred while retrieving event
                                     flags.
    @return ADI_OSAL_BAD_HANDLE   -  If the specified event group is invalid. 
    @return ADI_OSAL_TIMEOUT      -  If the function failed to get the specified
                                     event flag(s) due to timeout.
    @return ADI_OSAL_CALLER_ERROR -  If the function is invoked from an invalid
                                     location (i.e an ISR)
    @return ADI_OSAL_BAD_OPTION   -  If "eGetOption" specifies a wrong option.
*****************************************************************************/


ADI_OSAL_STATUS adi_osal_EventPend (ADI_OSAL_EVENT_HANDLE const hEventGroup,
                                    ADI_OSAL_EVENT_FLAGS        nRequestedEvents,
                                    ADI_OSAL_EVENT_FLAG_OPTION  eGetOption,
                                    ADI_OSAL_TICKS              nTimeoutInTicks,
                                    ADI_OSAL_EVENT_FLAGS        *pnReceivedEvents)
{
    INT8U           nErr;
    INT8U           nWaitOption = OS_FLAG_WAIT_SET_ALL;
    OS_FLAGS        nRetValue;
    ADI_OSAL_STATUS eRetStatus;
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a native uCOS pointer")
    OS_FLAG_GRP     *hEventNative = (OS_FLAG_GRP*) hEventGroup;
#pragma diag(pop)


#ifdef OSAL_DEBUG
    if ( (nTimeoutInTicks > ADI_OSAL_MAX_TIMEOUT) &&
         (nTimeoutInTicks != ADI_OSAL_TIMEOUT_FOREVER) )
    {
         return (ADI_OSAL_BAD_TIME);
    }

    if ((CALLED_FROM_AN_ISR) || (CALLED_IN_SCHED_LOCK_REGION))
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if ( (eGetOption != ADI_OSAL_EVENT_FLAG_ANY) &&
         (eGetOption != ADI_OSAL_EVENT_FLAG_ALL) )
    {
        return (ADI_OSAL_BAD_OPTION);
    }

    if ((NULL == hEventGroup) || (ADI_OSAL_INVALID_EVENT_GROUP == hEventGroup))
    {
        return(ADI_OSAL_BAD_HANDLE);
    }
#endif /* OSAL_DEBUG */

    if(eGetOption == ADI_OSAL_EVENT_FLAG_ANY)
    {
        nWaitOption = OS_FLAG_WAIT_SET_ANY;
    }

    switch (nTimeoutInTicks)
    {
        case ADI_OSAL_TIMEOUT_NONE:
            nRetValue = OSFlagAccept(hEventNative, (OS_FLAGS) nRequestedEvents, nWaitOption, &nErr);
            break;
        case ADI_OSAL_TIMEOUT_FOREVER:
            nRetValue = OSFlagPend(hEventNative, (OS_FLAGS) nRequestedEvents, nWaitOption, 0u, &nErr);
            break;
        default:
#pragma diag(push)
#pragma diag(suppress:misra_rule_10_1_c : "we have checked that the timeout is 16-bit or less in debug so we can cast it without losing information")
            nRetValue = OSFlagPend(hEventNative, (OS_FLAGS) nRequestedEvents, nWaitOption, (INT16U) nTimeoutInTicks, &nErr);
            break;
#pragma diag(pop)
    }

    switch (nErr)
    {
        case OS_ERR_NONE:
            eRetStatus = ADI_OSAL_SUCCESS;
            break;

        case OS_ERR_TIMEOUT:
            eRetStatus = ADI_OSAL_TIMEOUT;
            break;

#ifdef OSAL_DEBUG
        case OS_ERR_EVENT_TYPE:                     /* FALLTHROUGH */
        case OS_ERR_FLAG_INVALID_PGRP:
            eRetStatus = ADI_OSAL_BAD_HANDLE;
            break;

        case OS_ERR_PEND_ISR:
            eRetStatus = ADI_OSAL_CALLER_ERROR;
            break;

        case OS_ERR_FLAG_WAIT_TYPE:
            eRetStatus = ADI_OSAL_BAD_OPTION;
            break;

#endif /* OSAL_DEBUG */

        default:
            eRetStatus = ADI_OSAL_FAILED;
            break;
    }

    /* uC/OS also sets its return value (nRetValue) to be 0 in case of error */
    *pnReceivedEvents = (uint32_t) nRetValue;

    return( eRetStatus );
}

/*****************************************************************************
  @internal

  @brief This internal function is used by the adi_osal_EventSet and adi_osal_EventClear APIs
  It is a separate function in order to avoid duplication of code

   Parameters:
     @param[in] hEventGroup      - handle of the event group to use
     @param[in] nEventFlags      - Specifies the event flags to set.
                         'ORed' into the current event flags.
     @param[in] nOption          - uCOS option OS_FLAG_SET or OS_FLAG_CLR

     @return same as adi_osal_EventSet and adi_osal_EventClear APIs
   @endinternal

*****************************************************************************/


static ADI_OSAL_STATUS osal_ModifyFlags( ADI_OSAL_EVENT_HANDLE const hEventGroup,
                                      ADI_OSAL_EVENT_FLAGS nEventFlags,
                                      uint32_t nOption )
{
    INT8U nErr;
    ADI_OSAL_STATUS eRetStatus;
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a native uCOS pointer")
    OS_FLAG_GRP *hEventNative = (OS_FLAG_GRP*) hEventGroup;
#pragma diag(pop)

#ifdef OSAL_DEBUG
   if ((NULL == hEventGroup) || (ADI_OSAL_INVALID_EVENT_GROUP == hEventGroup))
    {
        return(ADI_OSAL_BAD_HANDLE);
    }

#endif /* OSAL_DEBUG */

    /* Only SET operations are allowed in the adi_osal_EventPost API */
    OSFlagPost(hEventNative, 
               (OS_FLAGS) nEventFlags, 
               (INT8U) nOption, 
               &nErr);

    switch (nErr)
    {
        case OS_ERR_NONE:
            eRetStatus = ADI_OSAL_SUCCESS;
            break;

#ifdef OSAL_DEBUG
        case OS_ERR_EVENT_TYPE: /* FALLTHROUGH */
        case OS_ERR_FLAG_INVALID_PGRP:
            eRetStatus = ADI_OSAL_BAD_HANDLE;
            break;
        case OS_ERR_FLAG_INVALID_OPT:
            eRetStatus = ADI_OSAL_FAILED;
            break;
#endif /* OSAL_DEBUG */

        default:
            eRetStatus = ADI_OSAL_FAILED;
            break;
    } /* end of switch */

    return( eRetStatus );
}


/*!
  ****************************************************************************
    @brief Sets one or more event flags.

    @param[in] hEventGroup      - handle of the event group to use
    @param[in] nEventFlags      - Specifies the event flags to set.
                                  'ORed' into the current event flags.

    @return ADI_OSAL_SUCCESS    - If the event flag(s) are posted successfully.
    @return ADI_OSAL_FAILED     - If the function failed to post the event 
                                  flags.
    @return ADI_OSAL_BAD_HANDLE - If the specified event group is invalid 
    @return ADI_OSAL_BAD_EVENT  - If the events presented in the nEventFlags 
                                  variable are not valid

*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_EventSet( ADI_OSAL_EVENT_HANDLE const hEventGroup,
                                     ADI_OSAL_EVENT_FLAGS nEventFlags)
{
    return (osal_ModifyFlags(hEventGroup, nEventFlags, OS_FLAG_SET));
}

/*!
  ****************************************************************************
    @brief Clears one or more event flags.

    @param[in] hEventGroup      - Handle of the event group to use
    @param[in] nEventFlags      - Specifies the event flags to cleared.

    @return ADI_OSAL_SUCCESS    - If the event flag(s) are cleared successfully.
    @return ADI_OSAL_FAILED     - If the function failed to clear the event 
                                  flags.
    @return ADI_OSAL_BAD_HANDLE - If the specified event group is invalid
    @return ADI_OSAL_BAD_EVENT  - If the events presented in the nEventFlags 
                                  variable are not valid

*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_EventClear(ADI_OSAL_EVENT_HANDLE const hEventGroup, ADI_OSAL_EVENT_FLAGS nEventFlags)
{
    return (osal_ModifyFlags(hEventGroup, nEventFlags, OS_FLAG_CLR));
}


/*
**
** EOF: 
**
*/
