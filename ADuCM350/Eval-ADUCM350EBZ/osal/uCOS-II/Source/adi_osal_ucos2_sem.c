/* $Revision: 11977 $
 * $Date: 2011-11-04 07:23:10 -0400 (Fri, 04 Nov 2011) $
******************************************************************************
Copyright (c), 2008-2011 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************/
/*!
    @file adi_osal_ucos_sem.c

    Operating System Abstraction Layer - OSAL for uCOS-II - Semaphore
    functions

    This file contains the Semaphore APIs for the uCOS-II implementation of 
    OSAL

*/


/*=============  I N C L U D E S   =============*/

#include "adi_osal.h"
#include "osal_ucos.h"
#include "osal_common.h"
#include <stdlib.h>

#if defined (__ECC__)
/*Rule 14.7 indicates that a function shall have a single exit point */
#pragma diag(suppress:misra_rule_14_7:"Allowing several point of exit (mostly for handling parameter error checking) increases the code readability and therefore maintainability")


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
    @brief Creates a counting semaphore  with the memory which has already been
            provided.

    @param[in] nSemObjSize - Size of the memory passed for the creation of
                               the semaphore
    @param[in] pSemObject  - Area of memory provided to us for the semaphore

    @param[out] phSem      - Pointer to a location to write the returned 
                             semaphore ID
    @param[in]  nInitCount - Initial value for the creation of a counting 
                             semaphore.
                             Semaphore will be created "unavailable" state if 
                             "nInitCount" is equal to zero.

    @return ADI_OSAL_SUCCESS      - If semaphore is created successfully
    @return ADI_OSAL_FAILED       - If failed to create semaphore
    @return ADI_OSAL_BAD_COUNT    - The value specified in nInitCount is too 
                                    large for the RTOS
    @return ADI_OSAL_CALLER_ERROR - If the call is made from an invalid location
                                    (i.e an ISR)

  Note:
      phSem  set  to "ADI_OSAL_INVALID_SEM" if semaphore creation is failed.
 *****************************************************************************/
ADI_OSAL_STATUS  adi_osal_SemCreateStatic(void* const pSemObject, uint32_t nSemObjSize, ADI_OSAL_SEM_HANDLE *phSem, uint32_t nInitCount)
{
    ADI_OSAL_STATUS eRetStatus;

#ifdef OSAL_DEBUG
    if ( nSemObjSize < sizeof(OS_EVENT) )
    {
        *phSem = ADI_OSAL_INVALID_SEM;
        return (ADI_OSAL_MEM_TOO_SMALL);
    }

    if ((false == _adi_osal_IsMemoryAligned(pSemObject)) || (NULL == pSemObject) )
    {
        *phSem = ADI_OSAL_INVALID_SEM;
        return (ADI_OSAL_BAD_MEMORY);
    }
#endif /* OSAL_DEBUG */

    eRetStatus = adi_osal_SemCreate(phSem, nInitCount); 

    /* the memory came from uCOS so we don't really need the SemObject and size
     * for anything. There seems no point copying things there. */
     
    return(eRetStatus); 
}

/*!
  ****************************************************************************
    @brief Creates a counting semaphore.

    @param[out] phSem      - Pointer to a location to write the returned
                             semaphore ID
    @param[in]  nInitCount - Initial value for the creation of a counting
                             semaphore.
                             Semaphore will be created "unavailable" state if
                             "nInitCount" is equal to zero.

    @return ADI_OSAL_SUCCESS      - If semaphore is created successfully
    @return ADI_OSAL_FAILED       - If failed to create semaphore
    @return ADI_OSAL_BAD_COUNT    - The value specified in nInitCount is too
                                    large for the RTOS
    @return ADI_OSAL_CALLER_ERROR - If the call is made from an invalid location
                                    (i.e an ISR)

  Note:
      phSem  set  to "ADI_OSAL_INVALID_SEM" if semaphore creation is failed.
 *****************************************************************************/

ADI_OSAL_STATUS adi_osal_SemCreate(ADI_OSAL_SEM_HANDLE *phSem, uint32_t nInitCount)
{
    ADI_OSAL_STATUS eRetStatus;
    OS_EVENT *pSemaphore;

#ifdef OSAL_DEBUG
    if (NULL == phSem)
    {
        return(ADI_OSAL_FAILED);
    }

    if (CALLED_FROM_AN_ISR)
    {
        *phSem = ADI_OSAL_INVALID_SEM;
        return (ADI_OSAL_CALLER_ERROR);
    }

    if (nInitCount > ADI_OSAL_SEM_MAX_COUNT)
    {
        *phSem = ADI_OSAL_INVALID_SEM;
        return (ADI_OSAL_BAD_COUNT);
    }
#endif /* OSAL_DEBUG */

    /* the count of a ucos semaphore is 16-bit so we truncate it. The
     * expectation is that max count is a smaller value*/

    pSemaphore = OSSemCreate((INT16U) nInitCount);

    if(NULL != pSemaphore)
    {
        eRetStatus = ADI_OSAL_SUCCESS;
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the uCOS type into an OSAL type")
        *phSem = (ADI_OSAL_SEM_HANDLE) pSemaphore;
#pragma diag(pop)
    }
    else
    {
        *phSem = ADI_OSAL_INVALID_SEM;
        eRetStatus = ADI_OSAL_FAILED;
    }
    return(eRetStatus);
}


/*!
  ****************************************************************************
  @brief Returns the size of a semaphore object.

  This function can be used by the adi_osal_SemCreateStatic function to
  determine what the object size should be for this particular RTOS
  implementation.

  Parameters:
      None

    @return size of a semaphore object in bytes.

    @see adi_osal_SemCreateStatic

*****************************************************************************/

uint32_t adi_osal_SemGetObjSize(void)
{
    return ( sizeof(OS_EVENT) );
}


/*!
  ****************************************************************************
    @brief Destroys a specified semaphore without freeing memory.

    @param[in]  hSem      - The handle of the semaphore which need to be deleted

    @return ADI_OSAL_SUCCESS          - If semaphore is deleted successfully
    @return ADI_OSAL_FAILED           - If failed to delete semaphore
    @return ADI_OSAL_BAD_HANDLE       - If the specified semaphore handle is 
                                        invalid
    @return ADI_OSAL_CALLER_ERROR     - If the call is made from an invalid 
                                        location (i.e an ISR).
    @return ADI_OSAL_THREAD_PENDING   - If a thread is pending on the specified
                                        semaphore

 *****************************************************************************/
ADI_OSAL_STATUS  adi_osal_SemDestroyStatic(ADI_OSAL_SEM_HANDLE const hSem)
{
    ADI_OSAL_STATUS eRetValue;
    eRetValue = adi_osal_SemDestroy(hSem);
    /* we did not allocate any memory so there is nothing to free up */
    return (eRetValue);
}
/*!
  ****************************************************************************
    @brief Deletes a specified semaphore.

    @param[in]  hSem      - The handle of the semaphore which need to be deleted

    @return ADI_OSAL_SUCCESS          - If semaphore is deleted successfully
    @return ADI_OSAL_FAILED           - If failed to delete semaphore
    @return ADI_OSAL_BAD_HANDLE       - If the specified semaphore handle is
                                        invalid
    @return ADI_OSAL_CALLER_ERROR     - If the call is made from an invalid
                                        location (i.e an ISR).
    @return ADI_OSAL_THREAD_PENDING   - If a thread is pending on the specified
                                        semaphore

 *****************************************************************************/

ADI_OSAL_STATUS  adi_osal_SemDestroy(ADI_OSAL_SEM_HANDLE const hSem)
{
    INT8U nErr;
    ADI_OSAL_STATUS eRetStatus;

#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a pointer to a useful structure")
    OS_EVENT *hSemNative = (OS_EVENT *) hSem;
#pragma diag(pop)

#ifdef OSAL_DEBUG
    if (CALLED_FROM_AN_ISR)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if(ADI_OSAL_INVALID_SEM == hSem)
    {
        return (ADI_OSAL_BAD_HANDLE);
    }

#endif /* OSAL_DEBUG */

    OSSemDel( hSemNative, OS_DEL_NO_PEND, &nErr );
    switch ( nErr )
    {
        case OS_ERR_NONE:
            eRetStatus =  ADI_OSAL_SUCCESS;
            break;

#ifdef OSAL_DEBUG
        case OS_ERR_EVENT_TYPE:
        case OS_ERR_PEVENT_NULL:
            eRetStatus =  ADI_OSAL_BAD_HANDLE;
            break;

        case  OS_ERR_DEL_ISR:
            eRetStatus= ADI_OSAL_CALLER_ERROR;
            break;
        case OS_ERR_INVALID_OPT:
            eRetStatus= ADI_OSAL_FAILED;
            break;
#endif /* OSAL_DEBUG */

        case OS_ERR_TASK_WAITING:
            eRetStatus= ADI_OSAL_THREAD_PENDING;
            break;
        default:
            eRetStatus= ADI_OSAL_FAILED;
            break;

    } /*end of switch */

    return( eRetStatus );
}

/*!
  ****************************************************************************
    @brief Waits for access to a semaphore
  
    If the specified semaphore is acquired, its count will be decremented.


    @param[in]  hSem             - Handle of the semaphore to obtain
    @param[in]  nTimeoutInTicks  - Specify the number of system ticks after 
                                   which obtaining the semaphore will return.

            Valid timeouts are:

            ADI_OSAL_TIMEOUT_NONE     No wait. Results in an immediate return
                                      from this service regardless of whether
                                      or not it was successful
            ADI_OSAL_TIMEOUT_FOREVER  Wait option for calling task to suspend
                                      indefinitely  until a semaphore instance
                                      is obtained
            1....0xFFFFFFFE           Selecting a numeric value specifies the
                                      maximum time limit (in system ticks ) for
                                      obtaining a semaphore


    @return ADI_OSAL_SUCCESS    -  If semaphore acquired successfully
    @return ADI_OSAL_FAILED     -  If an error occured while acquiring the 
                                   semaphore
    @return ADI_OSAL_TIMEOUT    -  If the API failed to acquire the semaphore 
                                   within the specified time limit
    @return ADI_OSAL_BAD_HANDLE -  If the specified semaphore handle is invalid
    @return ADI_OSAL_BAD_TIME   -  If the specified time is invalid for the RTOS
    @return ADI_OSAL_CALLER_ERROR- If the function is invoked from an invalid 
                                   location

 *****************************************************************************/

ADI_OSAL_STATUS  adi_osal_SemPend(ADI_OSAL_SEM_HANDLE const hSem, ADI_OSAL_TICKS nTimeoutInTicks)
{
    INT8U nErr;
    ADI_OSAL_STATUS eRetStatus;
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a pointer to a useful structure")
    OS_EVENT* hSemNative = (OS_EVENT*) hSem;
#pragma diag(pop)


#ifdef OSAL_DEBUG
     if((nTimeoutInTicks > ADI_OSAL_MAX_TIMEOUT) && 
        (nTimeoutInTicks != ADI_OSAL_TIMEOUT_FOREVER))
     {
          return(ADI_OSAL_BAD_TIME);
     }

     if (CALLED_FROM_AN_ISR)
     {
         return (ADI_OSAL_CALLER_ERROR);
     }

    if(hSem == ADI_OSAL_INVALID_SEM)
    {
        return  (ADI_OSAL_BAD_HANDLE);
    }
#endif /* OSAL_DEBUG */

    if ( nTimeoutInTicks == ADI_OSAL_TIMEOUT_NONE )
    {
        if( 0u == OSSemAccept(hSemNative) )
        {
            eRetStatus = ADI_OSAL_FAILED;
        }
        else
        {
            eRetStatus = ADI_OSAL_SUCCESS;
        }

    }
    else
    {
        if(nTimeoutInTicks == ADI_OSAL_TIMEOUT_FOREVER)
        {
            nTimeoutInTicks = 0u;
        }

#pragma diag(push)
#pragma diag(suppress:misra_rule_10_1_c : "we have checked that the timeout is 16-bit or less in debug so we can cast it without losing information")
        OSSemPend(hSemNative, (INT16U) nTimeoutInTicks,  &nErr);
#pragma diag(pop)
        switch (nErr)
        {
            case OS_ERR_NONE:
                eRetStatus = ADI_OSAL_SUCCESS;
                break;

            case OS_ERR_TIMEOUT:
                eRetStatus =  ADI_OSAL_TIMEOUT;
                break;

#ifdef OSAL_DEBUG
            case OS_ERR_EVENT_TYPE:
            case OS_ERR_PEVENT_NULL:
                eRetStatus = ADI_OSAL_BAD_HANDLE;
                break;

            case OS_ERR_PEND_ISR:       /* FALLTHROUGH */
            case OS_ERR_PEND_LOCKED:

                eRetStatus = ADI_OSAL_CALLER_ERROR;
                break;
#endif /* OSAL_DEBUG */

            default:
                eRetStatus = ADI_OSAL_FAILED;
                break;
        }  /* end of switch */
    }

    return(eRetStatus);
}


/*!
  ****************************************************************************
    @brief Posts a semaphore

    The semaphore count will be incremented if the specified semaphore is
    posted successfully and there were no threads pending on it

    @param[in]  hSem - Handle of the semaphore to be posted

    @return ADI_OSAL_SUCCESS     - If semaphore was posted successfully
    @return ADI_OSAL_FAILED      - If an error occured while posting the 
                                   specified semaphore
    @return ADI_OSAL_BAD_HANDLE  - If the specified semaphore handle is invalid

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_SemPost(ADI_OSAL_SEM_HANDLE const hSem)
{
    INT8U nErrCode;
    ADI_OSAL_STATUS eRetStatus;
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a pointer to a useful structure")
    OS_EVENT* hSemNative = (OS_EVENT*) hSem;
#pragma diag(pop)

#ifdef OSAL_DEBUG
    if(hSem == ADI_OSAL_INVALID_SEM)
    {
        return (ADI_OSAL_BAD_HANDLE);
    }
#endif /* OSAL_DEBUG */

    nErrCode = OSSemPost(hSemNative);
    switch (nErrCode)
    {
        case OS_ERR_NONE:
            eRetStatus = ADI_OSAL_SUCCESS;
            break;

#ifdef OSAL_DEBUG
        case OS_ERR_EVENT_TYPE:
        case OS_ERR_PEVENT_NULL:
            eRetStatus = ADI_OSAL_BAD_HANDLE;
            break;

        case OS_ERR_SEM_OVF:
            eRetStatus = ADI_OSAL_COUNT_OVERFLOW;
            break;
#endif /* OSAL_DEBUG */

        default:
            eRetStatus  = ADI_OSAL_FAILED;
            break;

    }  /* end of switch */

  return(eRetStatus);
}

/*
**
** EOF:
**
*/
