/* $Revision: 21629 $
 * $Date: 2013-05-29 08:01:48 -0400 (Wed, 29 May 2013) $
******************************************************************************
Copyright (c), 2008-2011 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
*****************************************************************************/

/*!
    @file adi_osal_ucos_tls.c

    Operating System Abstraction Layer - OSAL for uCOS-II - TLS
    functions

    This file contains the Thread Local Storage APIs for the uCOS-II
    implementation of OSAL

    It requires uCOS-II Version 2.92.08 or later

*/

/*=============  I N C L U D E S   =============*/


#include "adi_osal.h"
#include "osal_ucos.h"
#include "osal_common.h"

#ifdef _MISRA_RULES
/*Rule 14.7 indicates that a function shall have a single exit point */
#pragma diag(suppress:misra_rule_14_7:"Allowing several point of exit (mostly for handling parameter error checking) increases the code readability and therefore maintainability")
/* Rule-11.3 indicates that typecast of a integer value into pointer is invalid */
#pragma diag(suppress:misra_rule_11_3 : "typecasting is necessary to convert the OSAL Slot Value void* type to the underlying OS_TLS integral type")
#endif

/*=============  D E F I N E S  =============*/

#if defined (__ECC__)
#pragma file_attr(  "libGroup=adi_osal.h")
#pragma file_attr(  "libName=libosal")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")
#endif


/* Thread local storage numbers/keys consist of two values:
 *  - The signature, indicating that the key refers to a TLS slot 0xNNNNNNXX
 *  - The index of the slot, from 0 to Max Slots 0xXXXXXXNN
 */
#define TLS_SIGNATURE ((uint32_t)(0x544C5300))

/* Masks used to extract either the signature component, or the slot index component
 * of a TLS Key.
 */
#define TLS_MASK_SIG ((uint32_t)(0xFFFFFF00))
#define TLS_MASK_NUM ((uint32_t)(0x000000FF))

/*=============  D A T A  =============*/

/*!
    @internal
    @var _adi_osal_gnNumSlots
         stores the number of TLS slots requested by the user during
         initialization.
    @endinternal
*/
uint32_t _adi_osal_gnNumSlots = ADI_OSAL_MAX_NUM_TLS_SLOTS;

/*!
    @internal
    @var  _adi_osal_gaTLSCallbacks
          Hold the callback structure for each Thread Local Storage Slot. The
          Callback are per slot, all threads that are using that slot are using
          the same callback.  Make the array the maximum supported size
          (ADI_OSAL_MAX_THREAD_SLOTS)
    @endinternal
*/
static ADI_OSAL_TLS_CALLBACK_PTR _adi_osal_gaTLSCallbacks[ADI_OSAL_MAX_NUM_TLS_SLOTS];

/*=============  C O D E  =============*/

/* Static Prototypes */
static void _adi_osal_TLSCallback(OS_TCB *ptcb, OS_TLS_ID id, OS_TLS value);

#pragma inline
#if defined (__ECC__)
#pragma always_inline
#elif defined (__ICCARM__)
#pragma inline=forced
#endif
bool IsValidTLSKey(ADI_OSAL_TLS_SLOT_KEY key)
{
    return (TLS_SIGNATURE == (key & TLS_MASK_SIG));
}

/*!
  ****************************************************************************
    @brief Internal callback function, called when a task is destroyed.

    If TLS is being used, this function is hooked into the Task deletion
    callback chain on the first call to adi_osal_ThreadSlotAcquire.  If all
    TLS slot are then released, this function is unhooked from the chain.  The
    function simply checks what TLS slots are in use, and if they have a
    callback function then that function is called with the slot contents as
    a parameter.

    @param[in] ptcb   - Pointer to the being-deleted task.
    @param[in] id     - Index to the TLS slot which is being cleared.
    @param[in] value  - Value which is stored in the TLS slot.
*****************************************************************************/
static void _adi_osal_TLSCallback(OS_TCB *ptcb, OS_TLS_ID  id, OS_TLS value)
{
    /* The scheduler must be active to set a TLS value
     * (see adi_osal_ThreadSlotSetValue), so if the OS has not started
     * then there is no TLS data to clean-up.  This check is here for the
     * unlikely case where a thread is created and then destroyed before the
     * scheduler starts.
     */
    if(adi_osal_IsSchedulerActive())
    {
        /* If a callback is installed for the TLS slot, call it */
    	if(NULL != _adi_osal_gaTLSCallbacks[id])
    	{
    		(*_adi_osal_gaTLSCallbacks[id])(value);
    	}
    }
}


/*!
  ****************************************************************************
    @brief Allocates a thread slot and returns the slot number

    This function allocates the slot (i.e memory location of a word size ) 
    from the thread local storage buffer and returns the slot number.

    @param[out] pnThreadSlotKey       - Pointer to return the slot number if a
                                        free slot is found.  Must be populated
                                        with ADI_OSAL_TLS_UNALLOCATED.  If a
                                        valid slot is already present in the
                                        supplied address this API returns
                                        success.

    @param[in] pTerminateCallbackFunc - Pointer to a function that gets called 
                                        when the slot is freed.  Can be NULL
                                        if the callback function is not
                                        required.

    @return ADI_OSAL_SUCCESS       - If the function successfully allocated the 
                                     slot.
    @return ADI_OSAL_FAILED        - If the function failed to allocate the 
                                     slot.
    @return ADI_OSAL_CALLER_ERROR  - If the function is invoked from an invalid 
                                     location (i.e an ISR)

    Note:
     "pnThreadSlotKey"  will be set to  ADI_OSAL_INVALID_THREAD_SLOT   if
      there are no free slots.
  @ingroup thread local storage
*****************************************************************************/
ADI_OSAL_STATUS
adi_osal_ThreadSlotAcquire(ADI_OSAL_TLS_SLOT_KEY     *pnThreadSlotKey,
                           ADI_OSAL_TLS_CALLBACK_PTR pTerminateCallbackFunc)
{
    OS_TLS_ID nSlotIndex;
    uint32_t nSlotBit;

    INT8U nErr;
    ADI_OSAL_STATUS  eRetStatus = ADI_OSAL_FAILED;

    /*!
        @internal
        @var _adi_osal_gTLSUsedSlots
             Word used in the management of allocated TLS slots.
             Bits are used to represent the status of the slot. Bit 0 corresponds
             to slot number 0 and slot number 30 corresponds bit number 30.
             A slot is free if the corresponding bit is clear and a
             slot is acquired if the corresponding bit is set. Initially all
             the slot bits are clear.
        @endinternal
    */
    static uint32_t _adi_osal_gTLSUsedSlots = 0u;

#ifdef OSAL_DEBUG
    if (CALLED_FROM_AN_ISR)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }
#endif /* OSAL_DEBUG */

    /* Lock the scheduler - as a task may be deleted while a callback is being
     * installed.  */
    OSSchedLock();

    /* If the passed-in slot number has already been allocated, then we return
     * successfully.  We check that -
     *  - It has the correct TLS signature
     *  - The slot has been (and is still) allocated.
     */
    if(IsValidTLSKey(*pnThreadSlotKey))
    {
        /* Extract the slot number from the TLS key, and convert to a bit
         * position */
        nSlotBit = 1ul << (*pnThreadSlotKey & TLS_MASK_NUM);

        if(0u != (_adi_osal_gTLSUsedSlots & nSlotBit))
        {
            /* The slot has previously been allocated using the supplied
             * address */
            OSSchedUnlock();
            return(ADI_OSAL_SUCCESS);
        }
    }
    /* Before we allocate a slot, the address to be written-to must have
     * an "unallocated" key.
     */
    if(*pnThreadSlotKey != ADI_OSAL_TLS_UNALLOCATED)
    {
        OSSchedUnlock();
        *pnThreadSlotKey = ADI_OSAL_INVALID_THREAD_SLOT;
        return(eRetStatus);
    }

    nSlotIndex = OS_TLS_GetID(&nErr);

    /* If the callback is NULL we don't put anything in the slot to save us
     * from calling the OSAL function
     */
    if ((nErr == OS_ERR_NONE) && (NULL != pTerminateCallbackFunc))
    {
        OS_TLS_SetDestruct(nSlotIndex,
                           _adi_osal_TLSCallback,
                           &nErr);
    }

    /* there was an error either allocating the slot or installing the 
     * callback 
     */
    if (nErr != OS_ERR_NONE)
    {
       *pnThreadSlotKey = ADI_OSAL_INVALID_THREAD_SLOT;
        OSSchedUnlock();
       return(eRetStatus);
    }

    /* everything went well so we record the used slot and we finish */

    _adi_osal_gaTLSCallbacks[nSlotIndex] = pTerminateCallbackFunc;
        nSlotBit = ((uint32_t) 1 << (nSlotIndex));
    /*
     * Set the context switch-sensitive globals
     * before breaking and releasing the scheduler lock :
     * - set the bit in the TLS available slot word.
     * - set the callback function, NULL pointers will not be called.
     */
    _adi_osal_gTLSUsedSlots      |= nSlotBit;

    OSSchedUnlock();

    *pnThreadSlotKey = (nSlotIndex | TLS_SIGNATURE);
    return (ADI_OSAL_SUCCESS);
}



/*!
  ****************************************************************************
    @brief Frees the specified slot in the local storage buffer.

    @param[in] nThreadSlotKey     - slot which needs to be freed.

    @return ADI_OSAL_SUCCESS      - If the function successfully freed the
                                    slot.
    @return ADI_OSAL_FAILED       - If the function tried to free a slot
                                    which was not allocated.
    @return ADI_OSAL_BAD_SLOT_KEY - If the specified slot key is invalid.
    @return ADI_OSAL_CALLER_ERROR - If the function is invoked from an
                                    invalid location (i.e an ISR)

    @note This API does not do anything in the uCOS-II port because TLS
          deallocation is not supported by the RTOS
*****************************************************************************/
ADI_OSAL_STATUS
adi_osal_ThreadSlotRelease(ADI_OSAL_TLS_SLOT_KEY nThreadSlotKey)
{

    ADI_OSAL_STATUS eRetStatus = ADI_OSAL_SUCCESS;
    uint32_t nSlotIndex;

#ifdef OSAL_DEBUG
    /* Get the TLS index from the slot key */
    nSlotIndex = (nThreadSlotKey & TLS_MASK_NUM);

    if (CALLED_FROM_AN_ISR)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if((!IsValidTLSKey(nThreadSlotKey)) || (nSlotIndex >= _adi_osal_gnNumSlots))
    {
        /* The slot number does not contain the TLS signature */
        return (ADI_OSAL_BAD_SLOT_KEY);
    }

#endif /* OSAL_DEBUG */

    return(eRetStatus);
}


/*!
  ****************************************************************************
    @brief Stores the given value in the specified TLS slot.

    @param[out] nThreadSlotKey     - Slot key for the Thread Local Buffer in
                                     which "SlotValue" to be stored.
    @param[in] slotValue           - Value to be stored.

    @return ADI_OSAL_SUCCESS       - If the function successfully stored the 
                                     value in the specified slot.
    @return ADI_OSAL_FAILED        - If there was an error trying to store
                                     the value in the slot which is not
                                     acquired
    @return ADI_OSAL_BAD_SLOT_KEY  - If the specified slot key is invalid.
    @return ADI_OSAL_CALLER_ERROR  - If the function is invoked from an
                                     invalid location (i.e an ISR)
*****************************************************************************/
ADI_OSAL_STATUS
adi_osal_ThreadSlotSetValue(ADI_OSAL_TLS_SLOT_KEY nThreadSlotKey,
                            ADI_OSAL_SLOT_VALUE   slotValue)
{
    uint32_t nSlotIndex;

    INT8U nErr;

    ADI_OSAL_STATUS eRetStatus = ADI_OSAL_FAILED;

    /* Get the TLS index from the slot key */
    nSlotIndex = (nThreadSlotKey & TLS_MASK_NUM);

#ifdef OSAL_DEBUG
    if( CALLED_FROM_AN_ISR )
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if((!IsValidTLSKey(nThreadSlotKey)) || (nSlotIndex >= _adi_osal_gnNumSlots))
    {
        /* The slot number does not contain the TLS signature, or
         * there is a problem with the slot index. */
        return ADI_OSAL_BAD_SLOT_KEY;
    }
#endif /* OSAL_DEBUG */

    OS_TLS_SetValue (OSTCBCur, (OS_TLS_ID)nSlotIndex, (OS_TLS)slotValue, &nErr);

    switch (nErr)
    {
        case  OS_ERR_NONE:
			eRetStatus = ADI_OSAL_SUCCESS;
        break;

#ifdef OSAL_DEBUG
        case OS_ERR_TLS_ID_INVALID:
            eRetStatus = ADI_OSAL_BAD_SLOT_KEY;
        break;
        case OS_ERR_OS_NOT_RUNNING: 
        case OS_ERR_TLS_NOT_EN: 
            eRetStatus = ADI_OSAL_CALLER_ERROR;
        break;
#endif
        default:
            eRetStatus = ADI_OSAL_FAILED;
        break;
    }
        
    return(eRetStatus);
}



/*!
  ****************************************************************************

    @brief Gets a value for the specified TLS slot from the current thread.

    @param[in] nThreadSlotKey     - Slot key, from which the data needs
                                    to be retrieved.
    @param[out] pSlotValue        - Pointer to store the retrieved value from
                                    TLS.

    @return ADI_OSAL_SUCCESS      - If the function successfully retrieved 
                                    data from the specified slot.
    @return ADI_OSAL_FAILED       - If there was an error trying to retrieve
                                    data a slot which is not acquired, or the
                                    supplied slot value pointer is NULL.
    @return ADI_OSAL_BAD_SLOT_KEY - If the specified slot key is invalid.
    @return ADI_OSAL_CALLER_ERROR - If the function is invoked from an invalid 
                                    location (i.e an ISR)
    @note
         "*pSlotValue"  will be set to ADI_OSAL_INVALID_THREAD_SLOT if
          failed to retrieve the data from the specified slot.

*****************************************************************************/

ADI_OSAL_STATUS
adi_osal_ThreadSlotGetValue(ADI_OSAL_TLS_SLOT_KEY nThreadSlotKey,
                            ADI_OSAL_SLOT_VALUE   *pSlotValue)
{
    uint32_t nSlotIndex;
    OS_TLS value;
    INT8U nErr;

    ADI_OSAL_STATUS eRetStatus = ADI_OSAL_FAILED;


    /* Remove the TLS signature from the slot number */
    nSlotIndex = (nThreadSlotKey & TLS_MASK_NUM);
    *pSlotValue = (ADI_OSAL_SLOT_VALUE *) ADI_OSAL_INVALID_THREAD_SLOT;

#ifdef OSAL_DEBUG
    if( CALLED_FROM_AN_ISR )
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if((!IsValidTLSKey(nThreadSlotKey)) || (nSlotIndex >= _adi_osal_gnNumSlots))
    {
        /* The slot number does not contain the TLS signature, or
         * there is a problem with the slot index. */
        return ADI_OSAL_BAD_SLOT_KEY;
    }

    if(NULL == pSlotValue)
    {
        /* A null pointer has been passed */
        return (ADI_OSAL_CALLER_ERROR);
    }
#endif /* OSAL_DEBUG */

    value = OS_TLS_GetValue (OSTCBCur, (OS_TLS_ID)nSlotIndex, &nErr);

    switch (nErr)
    {
        case  OS_ERR_NONE:
			*pSlotValue = (ADI_OSAL_SLOT_VALUE)value;
            eRetStatus = ADI_OSAL_SUCCESS;
        break;

#ifdef OSAL_DEBUG
        case OS_ERR_TLS_ID_INVALID:
            eRetStatus = ADI_OSAL_BAD_SLOT_KEY;
        break;
        case OS_ERR_OS_NOT_RUNNING:
        case OS_ERR_TLS_NOT_EN:
            eRetStatus = ADI_OSAL_CALLER_ERROR;
        break;
#endif
        default:
            eRetStatus = ADI_OSAL_FAILED;
        break;
    }

    return(eRetStatus);
}

/*
**
** EOF: 
**
*/
