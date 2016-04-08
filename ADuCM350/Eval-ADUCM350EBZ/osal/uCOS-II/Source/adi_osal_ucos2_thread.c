/* $Revision: 21629 $
 * $Date: 2013-05-29 08:01:48 -0400 (Wed, 29 May 2013) $
******************************************************************************
Copyright (c), 2008-2011 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************/
/*!
    @file adi_osal_ucos_thread.c

    Operating System Abstraction Layer - OSAL for uCOS-II - Thread
    functions

    This file contains the Thread APIs for the uCOS-II implementation of
    OSAL.
    
    Note that uCOS-II restricts a lot what can be done with tasks. For example
    the only task ID is the task priority. Because OSAL needs to know the task
    ID, we cannot support changes to OSAL Task priorities outside OSAL. 
    If an application changes the priority of an OSAL task and then OSAL tries
    to use the same task again, the application will not work.

    Also note that to support tasks the ucos-II option OS_TASK_CREATE_EXT_EN
    must be greater than 0 because OSTaskCreateExt is used.

*/

/*=============  I N C L U D E S   =============*/

#include <string.h>                                                             /* for strncpy */
#include <stddef.h>
#include "adi_osal.h"
#include "osal_ucos.h"
#include "osal_common.h"
#include <os_cpu.h>

#if defined (__ECC__)
#pragma diag(suppress:misra_rule_17_4:"Array indexing shall be the only allowed form  of pointer arithmetic")
/*Rule 14.7 indicates that a function shall have a single exit point */
#pragma diag(suppress:misra_rule_14_7:"Allowing several point of exit (mostly for handling parameter error checking) increases the code readability and therefore maintainability")
/* Rule-5.1 indicates that all identifiers shall not rely on more than 31 characters of significance */
#pragma diag(suppress:misra_rule_5_1:"prefixes added in front of identifiers quickly increases their size. In order to keep the code self explanatory, and since ADI tools are the main targeted tools, this restriction is not really a necessity")
/* Rule-11.3 indicates that typecast of a integer value into pointer is invalid */
#pragma diag(suppress:misra_rule_11_3 : "typecasting is necessary everytimes a predefine value is written to a return pointer during error conditions")
/* Rule-16.2 indicates that  Functions shall not call themselves,either directly or indirectly */
#pragma diag(suppress:misra_rule_16_2 : "Since the OSAL is reentrant by nature (several thread could call the API) the compiler MISRA checker mistakes sometimes the reentrancy for recurrence")
/* Rule-2.1 indicates that Assembly language shall be encapsulated and isolated */
#pragma diag(suppress:misra_rule_2_1 : "In this case we use macros to isolate an assembly function, for readability reasons, it's been applied to the whole file and not around each macro call")


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
    @brief Creates a thread and puts it in the ready state.

    @param[in] pThreadAttr - Pointer to the (pre-initialized) 
                             ADI_OSAL_THREAD_ATTR structure
    @param[out] phThread   - Pointer to a location to return the thread handle 
                             if the thread is created successfully

    @return ADI_OSAL_SUCCESS              - if thread creation is successful
    @return ADI_OSAL_FAILED               - if thread creation fails
    @return ADI_OSAL_MAX_THREADS_EXCEEDED - if the maximum number of threads 
                                            supported has been exceeded
    @return ADI_OSAL_BAD_PRIORITY         - if the specified priority is invalid
    @return ADI_OSAL_CALLER_ERROR         - if function is invoked from an 
                                            invalid location (i.e an ISR)
    @return ADI_OSAL_BAD_STACK_ADDR       - Stack base pointer is not word 
                                            aligned
*****************************************************************************/

ADI_OSAL_STATUS adi_osal_ThreadCreate(ADI_OSAL_THREAD_HANDLE *phThread, const ADI_OSAL_THREAD_ATTR *pThreadAttr)
{
    INT8U    nErr;
    INT8U    nRetValue;
#pragma diag(push)
#pragma diag(suppress:misra_rule_6_3 : "uCOS definition gives a MISRA error, in theory OS_STK should by INT32U and not 'unsigned int'")
    OS_STK   *pStackStart;
    OS_STK   *pStackTop;
#pragma diag(pop)
    uint32_t nStkSize;
    ADI_OSAL_STATUS eRetStatus;
    ADI_OSAL_THREAD_INFO_PTR hThreadNode;
    uint32_t  nAssignedPrio;
    ADI_OSAL_STATUS eHeapResult;


#ifdef OSAL_DEBUG
    if (NULL == phThread)
    {
        return(ADI_OSAL_FAILED);
    }

    /* verify that the given structure is not NULL before starting to access the fields */
    if (NULL == pThreadAttr)
    {
        *phThread = ADI_OSAL_INVALID_THREAD;
        return(ADI_OSAL_FAILED);
    }

    if ( !_adi_osal_IsMemoryAligned(pThreadAttr->pStackBase) )
    {
        *phThread = ADI_OSAL_INVALID_THREAD;
         return(ADI_OSAL_BAD_STACK_ADDR);
    }

    if (CALLED_FROM_AN_ISR)
    {
        *phThread = ADI_OSAL_INVALID_THREAD;
        return(ADI_OSAL_CALLER_ERROR);
    }

    if (NULL == pThreadAttr->pThreadFunc)
    {
        *phThread = ADI_OSAL_INVALID_THREAD;
        return(ADI_OSAL_BAD_THREAD_FUNC);
    }

    if (0u == pThreadAttr->nStackSize)
    {
        *phThread = ADI_OSAL_INVALID_THREAD;
        return (ADI_OSAL_BAD_STACK_SIZE);
    }

    if (!_adi_osal_IsMemoryAligned((void*) pThreadAttr->nStackSize))
    {
        *phThread = ADI_OSAL_INVALID_THREAD;
        return (ADI_OSAL_BAD_STACK_SIZE);
    }

    if (strlen(pThreadAttr->szThreadName) > ADI_OSAL_MAX_THREAD_NAME)
    {
        *phThread = ADI_OSAL_INVALID_THREAD;
        return ( ADI_OSAL_BAD_THREAD_NAME);
    }

#endif

/* This code assumes the following
    * The OS is setup to have its stack to grow downward (OS_STK_GROWTH==1)
    * OS_STK is fixed in a per-platform basis and not per-application. This is
      true because it is part of the Micrium port 
 */

   /* Convert the stack size in bytes to elements of OS_STK */
    nStkSize = pThreadAttr->nStackSize/sizeof(OS_STK);

#pragma diag(push)
#pragma diag(suppress:misra_rule_17_1 : "Allow pointer arithmetic on non-array types")
    pStackStart = (OS_STK*) pThreadAttr->pStackBase;
    pStackTop =   &pStackStart[nStkSize-1u];
#pragma diag(pop)

    /*  OS reserve some priority level for itself. uCOS reserves
     *  priority level 0,1,2,3.  ADI_OSAL_UCOS_BASE_PRIO is defined to offset
     *  OS reserved priorities.
     */

    nAssignedPrio= pThreadAttr->nPriority + ADI_OSAL_UCOS_BASE_PRIO;
    
    /* Because the OSAL operates on thread handles rather than priority we need
     * to maintain a mapping. This is done via the ADI_OSAL_THREAD_INFO
     * structure.  Create a object of type ADI_OSAL_THREAD_INFO which will serve
     * as the handle 
     */

     eHeapResult = _adi_osal_MemAlloc((void**) &hThreadNode, sizeof(ADI_OSAL_THREAD_INFO));

    if (ADI_OSAL_SUCCESS != eHeapResult)
    {
        *phThread = ADI_OSAL_INVALID_THREAD;
        return (eHeapResult);
    }
    
    /* save information about the thread into its structure */
    hThreadNode->nThreadPrio = (ADI_OSAL_PRIORITY) nAssignedPrio;

    /* Creating a task  by calling native OS call.*/

    /* The scheduler needs to be locked so that the TaskCreation and the Name
       assignement can be done atomically. It is important because the
       OSTaskNameSet takes the priority as an argument, and the priority could
       have been changed within the task itself which starts right after
       OSTaskCreateExt is called */

    OSSchedLock();


    /* the OSTCBExtPtr TCB entry (pext) is used for storing the pointer to the
     * ADI_OSAL_THREAD_INFO structure so that
     * it can be found when given the Thread Priority (uCOS task handle) 
     */
    nRetValue = OSTaskCreateExt (pThreadAttr->pThreadFunc,
                                 pThreadAttr->pTaskAttrParam,
                                 pStackTop,
                                 (INT8U)  (nAssignedPrio & 0xFFu),
                                 (INT16U) nAssignedPrio,
                                 pStackStart,
                                 nStkSize,
                                 (void *)hThreadNode,
                                 (OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR) ) ;

    switch (nRetValue)
    {
        case  OS_ERR_NONE:
            /* set the Name of the task. Maximum characters in the
              name of the task  allowed is "OS_TASK_NAME_SIZE".
              If it exceeds, it will be truncated to OS_TASK_NAME_SIZE*/
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting to the type expexted by uC/OS is necessary, also necessary for the return value")
#pragma diag(suppress:misra_rule_11_5 : "the API between the OSAL and uCOS force a const --> non-const conversion.")

            hThreadNode->nThreadSignature = ADI_OSAL_THREAD_SIGNATURE;

/* converting from a const to non-const variable could be a risk, but since the
   data is getting copied anyway it's not an issue here.*/
/* Supression since  typecasting "szTaskName" which is char_t to INT8U */
            OSTaskNameSet((INT8U) nAssignedPrio,
                          (INT8U*) pThreadAttr->szThreadName, 
                          &nErr);
            if(nErr == OS_ERR_NONE)
            {
                /* The ADI_OSAL_THREAD_INFO structure is used as the thread handle */
               *phThread = (ADI_OSAL_THREAD_HANDLE) hThreadNode;
                eRetStatus = ADI_OSAL_SUCCESS;
            }
            else
            {
                eRetStatus = ADI_OSAL_FAILED;
            }
            break;
#pragma diag(pop)

#ifdef OSAL_DEBUG
        /* Priority of the specified task is already exists */
        case  OS_ERR_PRIO_EXIST:
            eRetStatus= ADI_OSAL_PRIORITY_IN_USE;
            break;
        /* Specified priority is out of range */
        case  OS_ERR_PRIO_INVALID:
            eRetStatus = ADI_OSAL_BAD_PRIORITY;
            break;
        case  OS_ERR_TASK_CREATE_ISR:
            eRetStatus = ADI_OSAL_CALLER_ERROR;
            break;
        /* Not enough memory to create task */
        case  OS_ERR_TASK_NO_MORE_TCB:
            eRetStatus = ADI_OSAL_MEM_ALLOC_FAILED;
            break;
#endif
        default:
            eRetStatus = ADI_OSAL_FAILED;
            break;
    } /* end of switch */

    OSSchedUnlock();
    
    if(eRetStatus != ADI_OSAL_SUCCESS)
    {
        *phThread = ADI_OSAL_INVALID_THREAD;
        if (NULL != hThreadNode)
        {
            hThreadNode->nThreadSignature=ADI_OSAL_INVALID_THREAD_SIGNATURE;
            _adi_osal_MemFree(hThreadNode);
        }
    }

    return(eRetStatus);
}


/*!
  ****************************************************************************
    @brief Deletes a thread (hence can no longer be scheduled)

    @param[in] hThread - Handle of the thread to be deleted

    @return ADI_OSAL_SUCCESS      -  If successfully removed the thread from 
                                     the system
    @return ADI_OSAL_FAILED       -  If failed to delete the thread
    @return ADI_OSAL_BAD_HANDLE   -  If the specified thread handle is invalid
    @return ADI_OSAL_CALLER_ERROR -  If function is invoked from an invalid 
                                     location (i.e an ISR)

*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_ThreadDestroy(ADI_OSAL_THREAD_HANDLE const hThread)
{

    INT8U                   nErrCode;
    INT8U                   nPrioTask;
    ADI_OSAL_STATUS         eRetStatus = ADI_OSAL_FAILED;
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a useful type")
    ADI_OSAL_THREAD_INFO_PTR hThreadNode = (ADI_OSAL_THREAD_INFO_PTR) hThread;
#pragma diag(pop)


#ifdef OSAL_DEBUG
    /* check validity of the handle */
    if ((NULL == hThreadNode) || (ADI_OSAL_INVALID_THREAD == hThread))
    {
        return (ADI_OSAL_BAD_HANDLE);
    }

    if ( !_adi_osal_IsOSALThread(hThreadNode) )
    {
        return (ADI_OSAL_BAD_HANDLE);
    }

#endif


    /* OSAL relies on the task priority not having changed via uCOS. That 
     * behaviour is not supported. 
     */
    nPrioTask = (INT8U) (hThreadNode->nThreadPrio);

    nErrCode= OSTaskDel(nPrioTask);

    switch (nErrCode)
    {
        /* Deleted task successfully and mark the corresponding task */
        case  OS_ERR_NONE:
            hThreadNode->nThreadSignature=ADI_OSAL_INVALID_THREAD_SIGNATURE;
            _adi_osal_MemFree(hThreadNode);
            eRetStatus =  ADI_OSAL_SUCCESS;
            break;

#ifdef OSAL_DEBUG
        /* Not to be called from ISR */
        case  OS_ERR_TASK_DEL_ISR:
            eRetStatus= ADI_OSAL_CALLER_ERROR;
            break;

        /* Invalid priority of the task */
        case OS_ERR_PRIO_INVALID:
        case OS_ERR_TASK_DEL:
        case OS_ERR_TASK_NOT_EXIST:
            eRetStatus= ADI_OSAL_BAD_HANDLE;
            break;

        case OS_ERR_TASK_DEL_IDLE:
            eRetStatus= ADI_OSAL_FAILED;
            break;
#endif
        default:
            eRetStatus= ADI_OSAL_FAILED;
            break;

    } /* end of switch */

    return(eRetStatus);
}



/*!
  ****************************************************************************
    @brief Returns the native handle of the current thread.

    @param[in] phThread - pointer to a location to write the current thread
                          native handle upon successful.

    @return ADI_OSAL_SUCCESS  - if the OS was running and there is a thread

    @note "phThread" will be set to "ADI_OSAL_INVALID_THREAD" if not successful.

    @see adi_osal_ThreadGetHandle
*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_ThreadGetNativeHandle(void **phThread)
{
#ifdef OSAL_DEBUG
    if (NULL == phThread)
    {
        return(ADI_OSAL_FAILED);
    }
#endif
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4)
    if ((BOOLEAN) OS_TRUE != OSRunning)
    {
        * (ADI_OSAL_THREAD_HANDLE*) phThread = ADI_OSAL_INVALID_THREAD;
        return(ADI_OSAL_FAILED);
    }
    
    *(OS_TCB**) phThread = OSTCBCur;
#pragma diag(pop)
    return (ADI_OSAL_SUCCESS);

}

/*!
  ****************************************************************************
    @brief Returns the handle of the current thread if it is an OSAL thread.

    @param[in] phThread - pointer to a location to write the current OSAL thread
                          handle upon successful.

    @return ADI_OSAL_SUCCESS if the current thread was created with OSAL
    @return ADI_OSAL_FAILED  if the current thread was not created with OSAL

    @note "phThread" will be set to "ADI_OSAL_INVALID_THREAD" if not successful.

    @see adi_osal_ThreadGetNativeHandle
*****************************************************************************/

#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4)

/* a typecast is necessary here because the thread handle type is incomplete
 * and is just an abstract pointer, the real structure (the type of
 * _adi_osal_oStartupVirtualThread) cannot be used directly because it is
 * hidden from the public interface. */

ADI_OSAL_STATUS  adi_osal_ThreadGetHandle(ADI_OSAL_THREAD_HANDLE *phThread)
{
    ADI_OSAL_THREAD_INFO_PTR hThreadNode;

    if ((BOOLEAN) OS_FALSE == OSRunning)
    {
        *phThread  = (ADI_OSAL_THREAD_HANDLE) &_adi_osal_oStartupVirtualThread;
        return (ADI_OSAL_SUCCESS);
    }

/* In OSAL threads the uCOS thread is embedded in the OSAL structure so
 * there is a specific location for the thread's signature. Check if the
 * current thread is an OSAL thread by looking at the memory address where the
 * signature should be.
 */

#pragma diag(push)
#pragma diag(suppress:misra_rule_17_4)
#pragma diag(suppress:misra_rule_20_6)
    hThreadNode = (ADI_OSAL_THREAD_INFO_PTR) (OSTCBCur->OSTCBExtPtr);
#pragma diag(pop)

#ifdef OSAL_DEBUG

    if ( !_adi_osal_IsOSALThread(hThreadNode) )
    {
        return (ADI_OSAL_BAD_HANDLE);
    }
#endif

/* The current thread was an OSAL thread so we return the OSAL handle */
    *phThread  =(ADI_OSAL_THREAD_HANDLE)(hThreadNode);

    return(ADI_OSAL_SUCCESS);
}

#pragma diag(pop)


/*!
  ****************************************************************************
    @brief Returns the name of the currently executing thread


    @param[out] pszTaskName     - Pointer to the location to return the thread 
                                  name 
    @param[in]  nNumBytesToCopy - Number of bytes from the name to copy into 
                                  the memory supplied as output argument

    @return ADI_OSAL_FAILED     - Unable to copy the task name
    @return ADI_OSAL_SUCCESS    - Successfully copied
    @return ADI_OSAL_BAD_HANDLE - The specified string pointer is invalid.

  *****************************************************************************/

#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4)
#pragma diag(suppress:misra_rule_17_4)
#pragma diag(suppress:misra_rule_20_6)

ADI_OSAL_STATUS adi_osal_ThreadGetName(char_t *pszTaskName,
                                       uint32_t nNumBytesToCopy)
{
    char_t *pszThreadName = pszTaskName;
    char_t *pcTempStr;
    ADI_OSAL_THREAD_INFO_PTR hThreadNode;
    uint32_t valueToConvert = (uint32_t)OSTCBCur;
    int i ;

#ifdef OSAL_DEBUG
    if (NULL == pszTaskName)
    {
        return(ADI_OSAL_BAD_HANDLE);
    }
#endif

    /* if we have less than 16 characters we don't identify the thread as 
     * OSAL or not. Otherwise we do 
     */
    if (nNumBytesToCopy >= 16u)
    {
        hThreadNode = (ADI_OSAL_THREAD_INFO_PTR) (OSTCBCur->OSTCBExtPtr);

        if ( _adi_osal_IsOSALThread(hThreadNode) )
        {
          pcTempStr ="OSAL_";
        }
        else
        {
          pcTempStr ="RTOS_";
        }
        memcpy(pszThreadName,pcTempStr,5u);
        pszThreadName += 5;
    }

    if (nNumBytesToCopy>= 11u)
    {
        *pszThreadName = '0';
        pszThreadName++;
        *pszThreadName = 'x';
        pszThreadName++;
    }

    if (nNumBytesToCopy >= 9u)
    {
        pcTempStr = pszThreadName + 8u;
        *pcTempStr ='\0';
        pcTempStr--;

        for(i=0; i<8 ; i++)
        {
            *pcTempStr = "0123456789abcdef"[valueToConvert % 16u];
            valueToConvert = valueToConvert >> 4u;
            pcTempStr--;
        }
        return (ADI_OSAL_SUCCESS);
    }
    else
    {
        return (ADI_OSAL_FAILED);
    }

}
#pragma diag(pop)

/*!
  ****************************************************************************
    @brief Returns the priority of a given thread.

    @param[in]  hThread      - handle to the thread that the API will return 
                               the priority of.
    @param[out] pnThreadPrio - pointer to a location to write the thread 
                               priority.

    @return ADI_OSAL_FAILED     - If failed to return the priority of the 
                                  current thread
    @return ADI_OSAL_SUCCESS    - If successfully returns the priority of the 
                                  current thread
    @return ADI_OSAL_BAD_HANDLE - If the specified thread handle is invalid

*****************************************************************************/


ADI_OSAL_STATUS adi_osal_ThreadGetPrio(ADI_OSAL_THREAD_HANDLE const hThread, ADI_OSAL_PRIORITY *pnThreadPrio)
{
	ADI_OSAL_PRIORITY           nPrio;
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a useful type")
        ADI_OSAL_THREAD_INFO_PTR hThreadNode = (ADI_OSAL_THREAD_INFO_PTR) hThread;
#pragma diag(pop)

        /*  Not allowed from an ISR */
#ifdef OSAL_DEBUG
        if (CALLED_FROM_AN_ISR)
        {
            *pnThreadPrio = (ADI_OSAL_PRIORITY) ADI_OSAL_INVALID_PRIORITY;
            return (ADI_OSAL_CALLER_ERROR);
        }
        /* check validity of the handle */
        if ((hThreadNode==NULL) || (hThread==ADI_OSAL_INVALID_THREAD))
        {
            return (ADI_OSAL_BAD_HANDLE);
        }
        if ( !_adi_osal_IsOSALThread(hThreadNode) )
        {
            return (ADI_OSAL_BAD_HANDLE);
        }
#endif /* OSAL_DEBUG */

        nPrio  = hThreadNode->nThreadPrio;

        if (nPrio >=   ADI_OSAL_UCOS_BASE_PRIO)
        {
             nPrio = nPrio -  ADI_OSAL_UCOS_BASE_PRIO;
             *pnThreadPrio = nPrio;
             return (ADI_OSAL_SUCCESS);
        }
        *pnThreadPrio = (ADI_OSAL_PRIORITY) ADI_OSAL_INVALID_PRIORITY;
        return (ADI_OSAL_FAILED);
}


/*!
  ****************************************************************************
    @brief Changes the priority of the specified thread.

    @param[in] hThread       - Handle of the thread whose priority is to be 
                               changed.
    @param[in] nNewPriority  - New desired priority.


    @return ADI_OSAL_SUCCESS      - If successfully changed the priority of the
                                    specified thread
    @return ADI_OSAL_FAILED       - If failed to change the priority of the 
                                    specified thread to the specified priority.
    @return ADI_OSAL_BAD_PRIORITY - If the specified priority is invalid
    @return ADI_OSAL_CALLER_ERROR - If the function is invoked from an invalid 
                                    location (i.e an ISR)

*****************************************************************************/

/* MISRA RULE 16.7 is supressed to have a common API. i.e
 * Content of hTaskID may be modified in other OS */

#pragma diag(push)
#pragma diag(suppress:misra_rule_16_7)
ADI_OSAL_STATUS adi_osal_ThreadSetPrio(ADI_OSAL_THREAD_HANDLE const hThread, ADI_OSAL_PRIORITY nNewPriority)
{
    INT8U           nNewPrio;
    INT8U           nOldPrio;
    INT8U           nRetValue;
    ADI_OSAL_STATUS eRetStatus;
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a useful type")
    ADI_OSAL_THREAD_INFO_PTR hThreadNode = (ADI_OSAL_THREAD_INFO_PTR) hThread;
#pragma diag(pop)

#ifdef OSAL_DEBUG
    if (CALLED_FROM_AN_ISR)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    /* check validity of the handle */
    if ((NULL == hThreadNode) || (ADI_OSAL_INVALID_THREAD == hThread))
    {
        return (ADI_OSAL_BAD_HANDLE);
    }

    if ( !_adi_osal_IsOSALThread(hThreadNode) )
    {
        return (ADI_OSAL_BAD_HANDLE);
    }
#endif

    nOldPrio = (INT8U) hThreadNode->nThreadPrio;
    nNewPrio = (INT8U) (nNewPriority + ADI_OSAL_UCOS_BASE_PRIO);

    if (nNewPrio <  (INT8U) ADI_OSAL_UCOS_BASE_PRIO)
    {
        return(ADI_OSAL_BAD_PRIORITY);
    }

    nRetValue= OSTaskChangePrio(nOldPrio, nNewPrio);
    switch (nRetValue)
    {
        case  OS_ERR_NONE:
            /* update local structure */
            hThreadNode->nThreadPrio = nNewPrio;
            eRetStatus = ADI_OSAL_SUCCESS;
            break;

        case OS_ERR_PRIO_EXIST:
            eRetStatus = ADI_OSAL_PRIORITY_IN_USE;
            break;
#ifdef OSAL_DEBUG
        case OS_ERR_PRIO_INVALID:
            eRetStatus = ADI_OSAL_BAD_PRIORITY;
            break;
        case OS_ERR_PRIO:
            eRetStatus = ADI_OSAL_BAD_HANDLE;
            break;
#endif /* OSAL_DEBUG */
        default :
            eRetStatus = ADI_OSAL_FAILED;
            break;
    }
    return(eRetStatus);
}
#pragma diag(pop)   /* End of MISRA supression 16.7 */

/*!
  ****************************************************************************
    @brief Stops the current thread running for the specified time in system 
    ticks.

    @param[in] nTimeInTicks - Amount of time  to sleep in system ticks.

    @return ADI_OSAL_CALLER_ERROR - If function is invoked from an invalid 
                                    location (i.e an ISR)
    @return ADI_OSAL_SUCCESS      - If successfully completed the "sleep" period
*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_ThreadSleep(ADI_OSAL_TICKS nTimeInTicks)
{
        ADI_OSAL_STATUS eRetStatus = ADI_OSAL_SUCCESS;

#ifdef OSAL_DEBUG
        if ((CALLED_FROM_AN_ISR) || (CALLED_IN_SCHED_LOCK_REGION))
        {
            return (ADI_OSAL_CALLER_ERROR);
        }

        if(nTimeInTicks > ADI_OSAL_MAX_TIMEOUT)
        {
             return (ADI_OSAL_BAD_TIME);
        }

#endif /* OSAL_DEBUG */

        if (0u == nTimeInTicks)
        {
            return ADI_OSAL_SUCCESS;
        }

#pragma diag(push)
#pragma diag(suppress:misra_rule_10_1_c : "we have checked that the timeout is 16-bit or less in debug so we can cast it without losing information")
        OSTimeDly((INT16U) nTimeInTicks);
#pragma diag(pop)

        return(eRetStatus);
}

/*
**
** EOF: 
**
*/
