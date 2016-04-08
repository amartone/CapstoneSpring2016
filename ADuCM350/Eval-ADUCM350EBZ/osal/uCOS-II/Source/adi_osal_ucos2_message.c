/* $Revision: 11977 $
 * $Date: 2011-11-04 07:23:10 -0400 (Fri, 04 Nov 2011) $
******************************************************************************
Copyright (c), 2008-2011 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************/
/*!
    @file adi_osal_ucos_message.c

    Operating System Abstraction Layer - OSAL for uCOS-II - Message Queue
    functions

    This file contains the Message Queue APIs for the uCOS-II implementation
    of OSAL

    @endinternal
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
#pragma diag(suppress:misra_rule_11_3 : "typecasting is necessary when a predefined value is written to a return pointer during error conditions")


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
    @brief Creates a message queue used for inter-task communication.
    
    The Message is always a pointer which points to the base of
    a buffer which contains the actual message (indirect message). Hence the
    size of the message in the queue is always 4 bytes (a pointer).


    @param[out] phMsgQ    - Pointer to a location to write the returned message
                            queue ID
    @param[in]  aMsgQ     - Buffer to be used to store the messages
    @param[in]  nMaxMsgs  - Maximum number of messages the queue can hold

    @return ADI_OSAL_SUCCESS       - If message queue is created successfully
    @return ADI_OSAL_FAILED        - If failed to create message queue
    @return ADI_OSAL_CALLER_ERROR  - If function is invoked from an invalid 
                                     location

*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_MsgQueueCreate(ADI_OSAL_QUEUE_HANDLE *phMsgQ, void* aMsgQ[], uint32_t nMaxMsgs)
{
    ADI_OSAL_STATUS eRetStatus;
    OS_EVENT *pMessageQ;

#ifdef OSAL_DEBUG
    if (NULL == phMsgQ)
    {
        return(ADI_OSAL_BAD_HANDLE);
    }

    if (CALLED_FROM_AN_ISR)
    {
        *phMsgQ = ADI_OSAL_INVALID_QUEUE;
        return (ADI_OSAL_CALLER_ERROR);
    }

#pragma diag(push)
#pragma diag(suppress:misra_rule_13_7 : "If sizeof(short) == sizeof(int) then we have a boolean operation which is always false. The check is required for all other platforms")
    /* message numbers are store in a short variable in uCOS */
    if (nMaxMsgs > USHRT_MAX)
    {
        *phMsgQ = ADI_OSAL_INVALID_QUEUE;
        return (ADI_OSAL_BAD_COUNT);
    }
#pragma diag(pop)

#endif /* OSAL_DEBUG */

    pMessageQ = OSQCreate(aMsgQ,(INT16U)nMaxMsgs );
    if(NULL != pMessageQ)
    {
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the uCOS type into an OSAL type")
        *phMsgQ = (ADI_OSAL_QUEUE_HANDLE) pMessageQ;
#pragma diag(pop)
        eRetStatus  =   ADI_OSAL_SUCCESS;
    }
    else
    {
        *phMsgQ = ADI_OSAL_INVALID_QUEUE;
        return (ADI_OSAL_FAILED);
    }

    return(eRetStatus);
}


/*!
  ****************************************************************************
    @brief Deletes the specified message queue

    @param[in] hMsgQ   -  handle of the message queue to be deleted

    @return ADI_OSAL_SUCCESS        - If message queue is deleted successfully
    @return ADI_OSAL_FAILED         - If failed to delete message queue
    @return ADI_OSAL_BAD_HANDLE     - If the specified message queue ID is 
                                      invalid
    @return ADI_OSAL_CALLER_ERROR   - If function is invoked from an invalid 
                                      location (i.e an ISR)
    @return ADI_OSAL_THREAD_PENDING - If one or more thread is pending for the 
                                      message on the specified queue.
*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_MsgQueueDestroy(ADI_OSAL_QUEUE_HANDLE const hMsgQ)
{
    INT8U    nErr;
    ADI_OSAL_STATUS eRetStatus;
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a native uCOS pointer")
    OS_EVENT *hMsgQNative = (OS_EVENT *) hMsgQ;
#pragma diag(pop)

#ifdef OSAL_DEBUG
    if (CALLED_FROM_AN_ISR)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if ((NULL == hMsgQ) || (ADI_OSAL_INVALID_QUEUE == hMsgQ))
    {
        return (ADI_OSAL_BAD_HANDLE);
    }
#endif


    OSQDel(hMsgQNative, OS_DEL_NO_PEND, &nErr);
    switch (nErr)
    {
        case OS_ERR_NONE:
            eRetStatus = ADI_OSAL_SUCCESS;
            break;

#ifdef OSAL_DEBUG
        case OS_ERR_EVENT_TYPE:
            eRetStatus = ADI_OSAL_BAD_HANDLE;
            break;

        case OS_ERR_DEL_ISR:
            eRetStatus = ADI_OSAL_CALLER_ERROR;
            break;
        case OS_ERR_INVALID_OPT:        /* FALLTHROUGH */
        case OS_ERR_PEVENT_NULL:
            eRetStatus = ADI_OSAL_FAILED;
            break;
#endif /* OSAL_DEBUG */

        case OS_ERR_TASK_WAITING:
            eRetStatus= ADI_OSAL_THREAD_PENDING;
            break;

        default :
            eRetStatus = ADI_OSAL_FAILED;
            break;
    } /* end of switch */

    return(eRetStatus);
}


/*!
  ****************************************************************************
    @brief Sends a message to the specified message queue.

    @param[in] hMsgQ     - Handle of the message queue to use.
    @param[in] pMsg      - Pointer to the message to send

    @return ADI_OSAL_SUCCESS    - If message queued successfully
    @return ADI_OSAL_FAILED     - If failed to queue the message
    @return ADI_OSAL_BAD_HANDLE - If the specified message queue handle is 
                                  invalid
    @return ADI_OSAL_QUEUE_FULL - If queue is full
*****************************************************************************/

ADI_OSAL_STATUS adi_osal_MsgQueuePost(ADI_OSAL_QUEUE_HANDLE const hMsgQ, void *pMsg)
{
    INT8U           nRetValue;
    ADI_OSAL_STATUS eRetStatus;
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a native uCOS pointer")
    OS_EVENT *hMsgQNative = (OS_EVENT *) hMsgQ;
#pragma diag(pop)

#ifdef OSAL_DEBUG
    if ((ADI_OSAL_INVALID_QUEUE == hMsgQ) || (NULL == hMsgQ))
    {
        return (ADI_OSAL_BAD_HANDLE);
    }
#endif /* OSAL_DEBUG */

    nRetValue = OSQPost(hMsgQNative, pMsg);

    switch (nRetValue)
    {
        case OS_ERR_NONE:
            eRetStatus = ADI_OSAL_SUCCESS;
            break;

#ifdef OSAL_DEBUG
        case OS_ERR_Q_FULL:
            eRetStatus = ADI_OSAL_QUEUE_FULL ;
            break;

        case OS_ERR_EVENT_TYPE:     /* FALLTHROUGH */
        case OS_ERR_PEVENT_NULL:
            eRetStatus = ADI_OSAL_BAD_HANDLE;
            break;
#endif /* OSAL_DEBUG */

        default :
            eRetStatus =  ADI_OSAL_FAILED;
            break;
    } /* end of switch */

    return(eRetStatus);
}


/*!
  ****************************************************************************
    @brief Receives a message from the specified message queue.

    @param[in]  hMsgQ             -  handle of the Message queue to retrieve 
                                     the message from
    @param[out] ppMsg             -  Pointer to a location to store the message
    @param[in]  nTimeoutInTicks   -  Timeout in system ticks for retrieving the
                                     message.

      Valid timeouts are:

         ADI_OSAL_TIMEOUT_NONE     -   No wait. Results in an immediate return
                                       from this service regardless of whether
                                       or not it was successful

         ADI_OSAL_TIMEOUT_FOREVER  -   suspends the calling thread indefinitely
                                       until a message is obtained

         1 ... 0xFFFFFFFE          -   Selecting a numeric value specifies the
                                       maximum time limit (in system ticks) for
                                       obtaining a message from the queue

    @return ADI_OSAL_SUCCESS      - If message is received and copied to ppMsg 
                                    buffer and removed from queue.
    @return ADI_OSAL_FAILED       - If failed to get a message.
    @return ADI_OSAL_TIMEOUT      - If failed get message due to timeout.
    @return ADI_OSAL_BAD_HANDLE   - If the specified message queue is invalid
    @return ADI_OSAL_CALLER_ERROR - If the function is invoked from an invalid 
                                    location (i.e an ISR)
*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_MsgQueuePend(ADI_OSAL_QUEUE_HANDLE const hMsgQ, void **ppMsg, ADI_OSAL_TICKS nTimeoutInTicks)
{
    INT8U nErr = OS_ERR_NONE;
    ADI_OSAL_STATUS eRetStatus;
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a native uCOS pointer")
    OS_EVENT *hMsgQNative = (OS_EVENT *) hMsgQ;
#pragma diag(pop)


#ifdef OSAL_DEBUG
    if((nTimeoutInTicks > ADI_OSAL_MAX_TIMEOUT) && 
       (nTimeoutInTicks != ADI_OSAL_TIMEOUT_FOREVER))
    {
         return (ADI_OSAL_BAD_TIME);
    }

    if ((CALLED_FROM_AN_ISR) || (CALLED_IN_SCHED_LOCK_REGION))
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if ((NULL == hMsgQ) || (ADI_OSAL_INVALID_QUEUE == hMsgQ))
    {
        return (ADI_OSAL_BAD_HANDLE);
    }
#endif /* OSAL_DEBUG */


    switch (nTimeoutInTicks)
    {
        case ADI_OSAL_TIMEOUT_NONE:
            *ppMsg = OSQAccept(hMsgQNative, &nErr);
            break;
        case ADI_OSAL_TIMEOUT_FOREVER:
            *ppMsg = OSQPend(hMsgQNative, 0u,  &nErr);
            break;
        default:
#pragma diag(push)
#pragma diag(suppress:misra_rule_10_1_c : "we have checked that the timeout is 16-bit or less in debug so we can cast it without losing information")
            *ppMsg = OSQPend(hMsgQNative, (INT16U)nTimeoutInTicks,  &nErr);
#pragma diag(pop)
            break;
    } /* end of switch */
    /* Only one switch-case for error code from  both OSQPend and OSQAccept. */
    switch (nErr)
    {
        case OS_ERR_NONE:
            eRetStatus = ADI_OSAL_SUCCESS;
            break;

        case OS_ERR_TIMEOUT:
            eRetStatus = ADI_OSAL_TIMEOUT;
            break;

#ifdef OSAL_DEBUG
        case OS_ERR_PEVENT_NULL: /* FALLTHROUGH */
        case OS_ERR_EVENT_TYPE:
            eRetStatus = ADI_OSAL_BAD_HANDLE;
            break;

        case OS_ERR_PEND_ISR:           /* FALLTHROUGH */
        case OS_ERR_PEND_LOCKED:
            eRetStatus = ADI_OSAL_CALLER_ERROR;
            break;
        case OS_ERR_Q_EMPTY:
            /* the call that returned OS_Q_EMPTY will have written NULL to pMsgPtr */
            eRetStatus = ADI_OSAL_QUEUE_EMPTY;
            break;

#endif /*OSAL_DEBUG */

        default:
            eRetStatus = ADI_OSAL_FAILED;
            break;
    } /* end of switch */

    return( eRetStatus );
}


/*
**
** EOF: 
**
*/
