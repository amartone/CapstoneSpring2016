/* $Revision: 13692 $
 * $Date: 2012-04-18 11:35:43 -0400 (Wed, 18 Apr 2012) $
******************************************************************************
Copyright (c), 2008-2011 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************/
/*!
    @file adi_osal_ucos_mutex.c

    Operating System Abstraction Layer - OSAL for uCOS-II - Mutex functions

    This file contains the Mutex APIs for the uCOS-II implementation of OSAL

    Note that OSAL cannot support the case where a task which owns a mutex
    changes priority because uCOS-II uses the priority as the task ID which
    will no longer match.

*/


/*=============  I N C L U D E S   =============*/

#include "adi_osal.h"
#include "osal_ucos.h"
#include "osal_common.h"
#include <limits.h>
#include <stdlib.h>

#if defined (__ECC__)
/*Rule 14.7 indicates that a function shall have a single exit point */
#pragma diag(suppress:misra_rule_14_7:"Allowing several point of exit (mostly for handling parameter error checking) increases the code readability and therefore maintainability")
/* Rule-11.3 indicates that typecast of a integer value into pointer is invalid */
#pragma diag(suppress:misra_rule_11_3 : "typecasting is necessary everytimes a predefine value is written to a return pointer during error conditions")

/*=============  D E F I N E S  =============*/

#pragma file_attr(  "libGroup=adi_osal.h")
#pragma file_attr(  "libName=libosal")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")
#endif



/*=============  D A T A  =============*/




/*=============  C O D E  =============*/



/******

NOTE: PRIORITY INHERITANCE (CEILING) IS ALWAYS CONSIDERED TURNED OFF IN UCOS-II

******/

/*!
  ****************************************************************************
    @brief Creates a mutex.

    @param[out] phMutex  - Pointer to a location to write the returned mutex 
                           handle

    @return ADI_OSAL_SUCCESS      - If mutex is created successfully
    @return ADI_OSAL_FAILED       - If failed to create mutex
    @return ADI_OSAL_MEM_ALLOC_FAILED - If memory could not be allocated
    @return ADI_OSAL_CALLER_ERROR - If the function is invoked from an invalid 
                                    location

     "phMutex" points to NULL if mutex creation is failed.

 *****************************************************************************/

/*  @Note:
    uCOS-II needs different priority levels for each mutex. Since the OSAL does
    not have a knowledge of each task in the system, its only option would be
    to reserve the top levels for usage by the mutex. Because of this
    requirement, there may be a case where the highest priority task may not
    get CPU control due to priority inheritance between two lower priority
    tasks. To avoid this situation, mutex functionality is replaced with a
    binary semaphore. When a Mutex is shared by more than one task, It is the
    responsibility of the application to ensure that the task with the highest
    priority should "acquire the mutex successfully" (Since it is a  binary
    semaphore).  This helps to prevent the highest priority task from getting
    starved."
 *****************************************************************************/

ADI_OSAL_STATUS adi_osal_MutexCreate(ADI_OSAL_MUTEX_HANDLE *phMutex)
{
    ADI_OSAL_STATUS eRetStatus;
    ADI_OSAL_MUTEX_INFO_PTR pMutexObj;

#ifdef OSAL_DEBUG
    if (NULL == phMutex)
    {
        return(ADI_OSAL_FAILED);
    }

    if (CALLED_FROM_AN_ISR)
    {
        *phMutex = ADI_OSAL_INVALID_MUTEX;
        return (ADI_OSAL_CALLER_ERROR);
    }
#endif /* OSAL_DEBUG */

#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the ADI_OSAL_MUTEX_INFO_PTR type in to a generic void** type required by malloc")
    /* a new mutex is created, we need to allocate some memory for it */
    if (ADI_OSAL_SUCCESS != _adi_osal_MemAlloc((void**) &pMutexObj, sizeof(ADI_OSAL_MUTEX_INFO)))
    {
        *phMutex = ADI_OSAL_INVALID_MUTEX;
        return (ADI_OSAL_MEM_ALLOC_FAILED);
    }
#pragma diag(pop)

    eRetStatus = adi_osal_MutexCreateStatic(pMutexObj, sizeof(ADI_OSAL_MUTEX_INFO), phMutex);

    if (eRetStatus != ADI_OSAL_SUCCESS)
    {
        _adi_osal_MemFree(pMutexObj);
    }

    return (eRetStatus);
}


/*!
  ****************************************************************************
    @brief Creates a mutex using user allocated memory for the mutex

    THIS FUNCTION IS THE SAME AS adi_osal_MutexCreate except that memory is 
    passed to it instead of relying on dynamic memory.

    @param[out] phMutex      - Pointer to a location to write the returned 
                               mutex handle
    @param[in] nMutexObjSize - Size of the memory passed for the creation of 
                               the mutex
    @param[in] pMutexObject  - Area of memory provided to us for the mutex

    @return ADI_OSAL_SUCCESS      - If mutex is created successfully
    @return ADI_OSAL_FAILED       - If failed to create mutex
    @return ADI_OSAL_MEM_TOO_SMALL- If there isn't sufficient memory to create 
                                    the mutex
    @return ADI_OSAL_CALLER_ERROR - If the function is invoked from an invalid 
                                    location

    @see adi_osal_MutexCreate
    @see adi_osal_MutexDestroyStatic
 *****************************************************************************/

ADI_OSAL_STATUS adi_osal_MutexCreateStatic(void* const pMutexObject, uint32_t nMutexObjSize, ADI_OSAL_MUTEX_HANDLE *phMutex)
{
    OS_EVENT*       hMtxNative;
 
#ifdef OSAL_DEBUG
    if (NULL == phMutex)
    {
        return(ADI_OSAL_FAILED);
    }

    if (CALLED_FROM_AN_ISR)
    {
        *phMutex = ADI_OSAL_INVALID_MUTEX;
        return (ADI_OSAL_CALLER_ERROR);
    }

    if ( nMutexObjSize < sizeof(ADI_OSAL_MUTEX_INFO) )
    {
        *phMutex = ADI_OSAL_INVALID_MUTEX;
        return (ADI_OSAL_MEM_ALLOC_FAILED);
    }

    if (!_adi_osal_IsMemoryAligned(pMutexObject))
    {
        *phMutex = ADI_OSAL_INVALID_MUTEX;
        return (ADI_OSAL_BAD_MEMORY);
    }

#endif /* OSAL_DEBUG */

    hMtxNative = OSSemCreate(INIT_COUNT_FOR_EMULATED_MUTEX);
    if ( hMtxNative != NULL)
    {
        ADI_OSAL_MUTEX_INFO_PTR pMutex = (ADI_OSAL_MUTEX_INFO_PTR) pMutexObject;

        pMutex->hMutexID = hMtxNative;
        pMutex->hOwnerThread = NULL; /* the mutex is not owned by the thread */
                                     /* creating it but by the thread        */
                                     /* acquiring it (MutexPend)             */
        pMutex->nAcquisitionCount = 0u; /* initially a mutex is not acquired */

#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the uCOS type in to an OSAL type")
        *phMutex = (ADI_OSAL_MUTEX_HANDLE) pMutex;
#pragma diag(pop)

        return (ADI_OSAL_SUCCESS);
    }
    else
    {
        *phMutex = ADI_OSAL_INVALID_MUTEX;
        return (ADI_OSAL_FAILED);
    }
}

/*!
  ****************************************************************************
  @brief Returns the size of a mutex object. 
  
  This function can be used by the adi_osal_MutexCreateStatic function to
  determine what the object size should be for this particular RTOS
  implementation.

  Parameters:
      None

    @return size of a mutex object in bytes.

    @see adi_osal_MutexCreateStatic

*****************************************************************************/

uint32_t adi_osal_MutexGetObjSize(void)
{
    return ( sizeof(ADI_OSAL_MUTEX_INFO) );
}
/*!
  ****************************************************************************
    @brief Deletes a mutex without freeing memory

    This API is designed to destroy a mutex that has been allocated with
    adi_osal_MutexCreateStatic().

    @param[in] hMutex      - Handle of the mutex to be deleted

    @return ADI_OSAL_SUCCESS      - If mutex is deleted successfully
    @return ADI_OSAL_FAILED       - If failed to delete mutex
    @return ADI_OSAL_BAD_HANDLE   - If the specified mutex handle is invalid
    @return ADI_OSAL_CALLER_ERROR - If function is invoked from an invalid 
                                    location
    @return ADI_OSAL_THREAD_PENDING-If one or more threads are pending on the 
                                    specified mutex.

  Notes:
      Only owner is authorised to release the acquired mutex. But it
      can "destroyed" by  other task.

    @see adi_osal_MutexCreateStatic                                
    @see adi_osal_MutexDestroy

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_MutexDestroyStatic(ADI_OSAL_MUTEX_HANDLE const hMutex)
{
    OS_EVENT        *pRetValue;
    INT8U           nErr;
    ADI_OSAL_STATUS eRetStatus = ADI_OSAL_BAD_HANDLE;

#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle into a pointer to a useful structure")
    ADI_OSAL_MUTEX_INFO_PTR  pMutexInfo  = (ADI_OSAL_MUTEX_INFO_PTR) hMutex;
#pragma diag(pop)


#ifdef OSAL_DEBUG
    if(CALLED_FROM_AN_ISR)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if ((NULL == hMutex) || (ADI_OSAL_INVALID_MUTEX == hMutex))
    {
        return (ADI_OSAL_BAD_HANDLE);
    }
#endif  /* OSAL_DEBUG */

    /* check whether index is with in range and points to a valid mutex */
    if(pMutexInfo->hMutexID != NULL)
    {
        return (ADI_OSAL_BAD_HANDLE);
    }
    pRetValue = OSSemDel(pMutexInfo->hMutexID, OS_DEL_NO_PEND, &nErr);
    switch (nErr)
    {
        case OS_ERR_NONE:
            if(NULL == pRetValue)
            {
                eRetStatus = ADI_OSAL_SUCCESS;
            }
            else
            {
                eRetStatus  = ADI_OSAL_FAILED;
            }
            break;

#ifdef OSAL_DEBUG
        case OS_ERR_EVENT_TYPE:
            eRetStatus= ADI_OSAL_BAD_HANDLE;
            break;

        case  OS_ERR_DEL_ISR:
            eRetStatus= ADI_OSAL_CALLER_ERROR;
            break;

        case OS_ERR_TASK_WAITING:
            eRetStatus= ADI_OSAL_THREAD_PENDING;
            break;

        case OS_ERR_INVALID_OPT:       /* FALLTHROUGH */
        case OS_ERR_PEVENT_NULL:
            eRetStatus  = ADI_OSAL_FAILED;
            break;
#endif
        default:
            eRetStatus  = ADI_OSAL_FAILED;
            break;
    } /* end of switch */

    return(eRetStatus);
}

/*!
  ****************************************************************************
    @brief This function is used to delete a mutex.

    @param[in] hMutex      - Handle of the mutex to be deleted

    @return ADI_OSAL_SUCCESS      - If mutex is deleted successfully
    @return ADI_OSAL_FAILED       - If failed to delete mutex
    @return ADI_OSAL_BAD_HANDLE   - If the specified mutex handle is invalid
    @return ADI_OSAL_CALLER_ERROR - If function is invoked from an invalid 
                                    location
    @return ADI_OSAL_THREAD_PENDING - one or more thread is pending on the 
                                    specified mutex.

  Notes:
      Only owner is authorised to release the acquired mutex. But it
      can "destroyed" by  other task.

*****************************************************************************/
ADI_OSAL_STATUS adi_osal_MutexDestroy(ADI_OSAL_MUTEX_HANDLE const hMutex)
{
    ADI_OSAL_STATUS eRetStatus;
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle into a pointer to a useful structure")
    ADI_OSAL_MUTEX_INFO_PTR  pMutexInfo  = (ADI_OSAL_MUTEX_INFO_PTR) hMutex;
#pragma diag(pop)

    eRetStatus = adi_osal_MutexDestroyStatic(hMutex);
    if (eRetStatus == ADI_OSAL_SUCCESS )
    {
        _adi_osal_MemFree(pMutexInfo);
    }
    
    return (eRetStatus);    
}


/*!
  ****************************************************************************
  @brief Acquires a mutex with a timeout

  This function is used to lock a mutex (acquire a resource)

    @param[in] hMutex            - Handle of the mutex which need to be acquired
    @param[in] nTimeoutInTicks   - Specify the number of system ticks for 
                                   acquiring the mutex

      Valid timeouts are:

        ADI_OSAL_TIMEOUT_NONE       -  No wait. Results in an immediate return
                                       from this service  regardless of whether
                                       or not it was successful

        ADI_OSAL_TIMEOUT_FOREVER    -  Wait option for calling task to suspend
                                       indefinitely until a specified  mutex is
                                       obtained

        1 ....0xFFFFFFFE            -  Selecting a numeric value specifies the
                                       maximum time limit (in system ticks) for
                                       obtaining specified mutex

    @return ADI_OSAL_SUCCESS      - If the specified mutex is locked 
                                    successfully
    @return ADI_OSAL_FAILED       - If failed to lock the specified mutex
    @return ADI_OSAL_BAD_HANDLE   - If the specified mutex ID is invalid
    @return ADI_OSAL_TIMEOUT      - If the specified time limit expired.
    @return ADI_OSAL_CALLER_ERROR - If the function is invoked from an invalid 
                                    location

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_MutexPend(ADI_OSAL_MUTEX_HANDLE const hMutex, ADI_OSAL_TICKS nTimeoutInTicks)
{
    INT8U    nErr;
    ADI_OSAL_STATUS  eRetStatus;
    OS_TCB  *hCurrThread;

#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle into a pointer to a useful structure")
    ADI_OSAL_MUTEX_INFO_PTR  pMutexInfo = (ADI_OSAL_MUTEX_INFO_PTR) hMutex;
#pragma diag(pop)

#ifdef OSAL_DEBUG
    if((nTimeoutInTicks > ADI_OSAL_MAX_TIMEOUT) && 
       (nTimeoutInTicks != ADI_OSAL_TIMEOUT_FOREVER))
    {
        return(ADI_OSAL_BAD_TIME);
    }

    /* the scheduler locked region case is returned by the RTOS so we 
       don't need to check it twice */
    if (CALLED_FROM_AN_ISR) 
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if ((NULL == hMutex) || (ADI_OSAL_INVALID_MUTEX == hMutex))
    {
        return (ADI_OSAL_BAD_HANDLE);
    }

    /* check whether index is with in range and points to a valid mutex */
    if(pMutexInfo->hMutexID == NULL)
    {
        return (ADI_OSAL_BAD_HANDLE);
    }
#endif /* OSAL_DEBUG */


    /* Current thread handle is necessary if all following conditionals paths */
    /* The mutex owner is a uCOS thread pointer because all threads in the
     * application are uCOS threads but not all threads are OSAL threads 
     * OSAL cannot support the case where a task which owns a mutex
     * changes priority because uCOS-II uses the priority as the task ID which
     * will no longer match.
     */

    hCurrThread = OSTCBCur;


    /* Nesting: check if the mutex is already acquired by the same owner */
    if ((pMutexInfo->nAcquisitionCount > 0u) &&
        (pMutexInfo->hOwnerThread == hCurrThread))  /* already owned by curr thread*/
    {
#ifdef OSAL_DEBUG
        /* check for count overflow */
        if (pMutexInfo->nAcquisitionCount == UINT_MAX)
        {
            return (ADI_OSAL_COUNT_OVERFLOW);
        }
#endif /* OSAL_DEBUG */

        pMutexInfo->nAcquisitionCount += 1u;
        return (ADI_OSAL_SUCCESS);
    }

    if(ADI_OSAL_TIMEOUT_NONE == nTimeoutInTicks)
    {
        INT16U  nErrVal;
        nErrVal = OSSemAccept(pMutexInfo->hMutexID);
        if (0u == nErrVal)
        {
            eRetStatus = ADI_OSAL_FAILED;
        }
        else
        {
            /* A binary semaphore is acquired successfully. Store the thread
             * handle of the current thread. This information is needed to find
             * the owner for subsequent operations.  
             * OSAL cannot support the case where a task which owns a mutex
             * changes priority because uCOS-II uses the priority as the task
             * ID which will no longer match.
             */

            pMutexInfo->hOwnerThread = hCurrThread;
            pMutexInfo->nAcquisitionCount += 1u;
            eRetStatus  = ADI_OSAL_SUCCESS;
        }
    }
    else
    {
        if(nTimeoutInTicks == ADI_OSAL_TIMEOUT_FOREVER)
        {
            nTimeoutInTicks =0u;
        }
#pragma diag(push)
#pragma diag(suppress:misra_rule_10_1_c : "we have checked that the timeout is 16-bit or less in debug so we can cast it without losing information")
        OSSemPend( pMutexInfo->hMutexID, (INT16U) nTimeoutInTicks,  &nErr);
#pragma diag(pop)
        switch (nErr)
        {
            case OS_ERR_NONE:
                /* A binary semaphore is acquired successfully. Store the
                 * taskID of the current task.This information is needed to
                 * find the owner when  releasing it. 
                 * OSAL cannot support the case where a task which owns a mutex
                 * changes priority because uCOS-II uses the priority as the
                 * task ID which will no longer match.
                 */
                pMutexInfo->hOwnerThread = hCurrThread;
                pMutexInfo->nAcquisitionCount += 1u;
                eRetStatus = ADI_OSAL_SUCCESS;
                break;

            case OS_ERR_TIMEOUT:
                eRetStatus = ADI_OSAL_TIMEOUT;
                break;

#ifdef OSAL_DEBUG
            case OS_ERR_EVENT_TYPE:
                eRetStatus = ADI_OSAL_BAD_HANDLE;
                break;

            case OS_ERR_PEND_ISR:       /* FALLTHROUGH */
            case OS_ERR_PEND_LOCKED:
                eRetStatus = ADI_OSAL_CALLER_ERROR;
                break;
            case OS_ERR_PEVENT_NULL:
                eRetStatus = ADI_OSAL_FAILED;
                break;
#endif /* OSAL_DEBUG */

            default:
                eRetStatus = ADI_OSAL_FAILED;
                break;
        }
    }

    return (eRetStatus);
}


/*!
  ****************************************************************************

    @brief Unlocks a mutex.

    @param[in] hMutex      - Handle of the mutex which needs to be unlocked

    @return ADI_OSAL_SUCCESS          - If mutex is un locked successfully
    @return ADI_OSAL_FAILED           - If failed unlock mutex
    @return ADI_OSAL_BAD_HANDLE       - If the specified mutex ID is invalid
    @return ADI_OSAL_NOT_MUTEX_OWNER  - If it is called from a task which does 
                                        not own it.
    @return ADI_OSAL_CALLER_ERROR     - The function is invoked from an invalid 
                                        location

    @note
         Mutex can be successfully released by its owner : Only the task which 
         acquired it can release it. Any attempt to release it by non-owner will
         result in error.
         
    @note 
         In uCOS-II OSAL cannot support the case where a task which owns a
         mutex changes priority because uCOS-II uses the priority as the task
         ID which will no longer match.

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_MutexPost(ADI_OSAL_MUTEX_HANDLE const hMutex)
{
    INT8U nErrCode;
    ADI_OSAL_STATUS eRetStatus;
    OS_TCB *hCurrThread;

#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle into a pointer to a useful structure")
    ADI_OSAL_MUTEX_INFO_PTR  pMutexInfo  = (ADI_OSAL_MUTEX_INFO_PTR) hMutex;
#pragma diag(pop)

#ifdef OSAL_DEBUG
    /* Even though semaphore posting is valid within an ISR, Mutex posting is NOT.
       So we artificially restrict it here since we use binary semaphores to emulate
       mutexes */
    if (CALLED_FROM_AN_ISR)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    /* check whether "pMutexInfo" is a valid "mutex" and that it is called from
       the thread which acquired it.
       Irrespective of priority, A mutex can be "released" only by the thread
       which owns it.
       This condition check is implemented in OSAL since mutex is replaced by the binary
       semaphore.
    */
    if ((NULL == hMutex) || (ADI_OSAL_INVALID_MUTEX == hMutex))
    {
        return (ADI_OSAL_BAD_HANDLE);
    }

    if (NULL == pMutexInfo->hMutexID)
    {
        return (ADI_OSAL_BAD_HANDLE);
    }

#endif /* OSAL_DEBUG */


    /* Check if the current thread is the mutex owner */
    /* The mutex owner is a uCOS thread pointer because all threads in the
     * application are uCOS threads but not all threads are OSAL threads 
     * In uCOS-II OSAL cannot support the case where a task which owns a mutex
     * changes priority because uCOS-II uses the priority as the task ID which
     * will no longer match.
     */ 
    hCurrThread = OSTCBCur;

    if(pMutexInfo->hOwnerThread != hCurrThread)
    {
        return (ADI_OSAL_NOT_MUTEX_OWNER);
    }

    /* Scheduler lock is necessary to keep the pMutexInfo structure in sync
     * with the OS Post so that there isn't a case when the OSSemPost creates a
     * reschedule with the pMutexInfo structure not yet updated*/
    OSSchedLock();

    /* The current thread is the owner. Nesting need to be checked now */
    if (pMutexInfo->nAcquisitionCount > 1u)
    {
        pMutexInfo->nAcquisitionCount--;
        eRetStatus = ADI_OSAL_SUCCESS;
    }
    else
    {
        /* current thread is the owner and nAcquisitionCount is 1 */
        nErrCode =  OSSemPost(pMutexInfo->hMutexID);
        switch (nErrCode)
        {
            case OS_ERR_NONE:
                pMutexInfo->hOwnerThread = NULL;
                pMutexInfo->nAcquisitionCount = 0u;
                eRetStatus = ADI_OSAL_SUCCESS;
                break;

    #ifdef OSAL_DEBUG
            case OS_ERR_EVENT_TYPE:
                eRetStatus = ADI_OSAL_BAD_HANDLE;
                break;
    #endif /* OSAL_DEBUG */
            case OS_ERR_PEVENT_NULL:           /* FALLTHROUGH */
            case OS_ERR_SEM_OVF:
            default :
                eRetStatus = ADI_OSAL_FAILED;
                break;
        } /*end of switch */
    }

    OSSchedUnlock();

    return(eRetStatus);
}

/*
**
** EOF: 
**
*/
