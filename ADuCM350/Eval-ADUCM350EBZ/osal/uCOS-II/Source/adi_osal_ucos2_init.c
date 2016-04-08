/* $Revision: 21629 $
 * $Date: 2013-05-29 08:01:48 -0400 (Wed, 29 May 2013) $
******************************************************************************
Copyright (c), 2008-2011 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************
*/

/*!
    @file adi_osal_ucos_init.c

    Operating System Abstraction Layer - OSAL for uCOS-II 

    This file contains the APIs designed to abstract uCOS-II the operating
    system from the user. 

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
/* Rule-5.1 indicates that all identifiers shall not rely on more than 31 characters of significance */
#pragma diag(suppress:misra_rule_5_1:"prefixes added in front of identifiers quickly increases their size. In order to keep the code self explanatory, and since ADI tools are the main targeted tools, this restriction is not really a necessity")
/* Rule-11.3 indicates that typecast of a integer value into pointer is invalid */
#pragma diag(suppress:misra_rule_11_3 : "typecasting is necessary when a predefined value is written to a return pointer during error conditions")


#pragma file_attr(  "libGroup=adi_osal.h")
#pragma file_attr(  "libName=libosal")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")
#endif

/* "version.h" contain macro "ADI_BUILD_VER"  which gives the
    build version for OSAL for uCOS. This is a generated file.
*/
#include "version.h"

/*=============  D E F I N E S  =============*/


/* priority of the virtual startup "thread" */
#define STARTUP_PRIORITY    0u
#define ADI_OSAL_INVALID_HEAP_INDEX (-1)



/*=============  D A T A  =============*/
/*!
    @internal
    @var _adi_osal_gnTickPeriod
         defines the length of system ticks in microseconds
    @endinternal
 */
uint32_t _adi_osal_gnTickPeriod = UINT_MAX;


/*!
    @internal
    @var _adi_osal_oStartupVirtualThread
         This thread is not a real thread, but is active until the OS starts.
         It will allow the TLS functions to operate until the OS takes over.
    @endinternal
*/

ADI_OSAL_THREAD_INFO _adi_osal_oStartupVirtualThread;

#if (OS_DEBUG_EN >0u)
extern INT16U  const  OSTCBSize;
#endif

/*=============  C O D E  =============*/

/*!
  ****************************************************************************
    @brief Initializes OSAL.  

    This function initializes the internal OSAL data structure. It should be
    called during the system startup.

    @return ADI_OSAL_SUCCESS          - Initialization is done successfully.
    @return ADI_OSAL_FAILED           - OSAL was already initialised
    @return ADI_OSAL_OS_ERROR         - The version of OSAL is not compatible 
                                        with the uCOS version
    @return ADI_OSAL_MEM_ALLOC_FAILED - Error initializing dynamic memory heap

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_Init(void)
{
    uint32_t* pHeapMemory = NULL; /*ptr to the memory to use for the heap */
    uint32_t nHeapMemorySize = 0u;/*size of memory pointed by pHeapMemory */
    /*!
        @internal
        @var snOsalInitializationState
             Static variable to record if OSAL has already been initialized
        @endinternal
    */
#ifdef OSAL_DEBUG
    static uint32_t snOsalInitializationState = 0u ;
#endif


    /* Checks that the version of uC/OS is compatible with this version of OSAL */
    if (OSVersion() < COMPATIBLE_OS_VERSION)
    {

        return (ADI_OSAL_OS_ERROR);
    }

#ifdef OSAL_DEBUG
    /* Check if already initialized. If the parameters are the same then the
     * call succeeds. Otherwise the call fails. The priority inheritance
     * setting is not recorded so we cannot check it
     */
    if (OSAL_INITIALIZED == snOsalInitializationState)
    {
        return (ADI_OSAL_SUCCESS);
    }
#endif

   /* we should check that the ADI_OSAL_UCOS_MAX_TCB_SIZE value is big enough for the
       current TCB definition
       Since the OS_TCB definition is variable depending on the configuration
       options, we need to have a value defined for the worst case.
       Unfortunately the only check that we can do is only possible if uCOS debug is on
       so we cannot rely on it. If we could it would be the one below
    */
#if (OS_DEBUG_EN >0u)
    if (OSTCBSize > ADI_OSAL_UCOS_MAX_TCB_SIZE)
    {
        return (ADI_OSAL_OS_ERROR);
    }
#endif

    if (ADI_OSAL_SUCCESS != _adi_osal_HeapInstall(pHeapMemory,nHeapMemorySize))
    {
        return (ADI_OSAL_MEM_ALLOC_FAILED);
    }


    /* Create the thread that represents the current execution as a thread until
     * the OS actually starts 
     */
    _adi_osal_oStartupVirtualThread.nThreadSignature = ADI_OSAL_THREAD_SIGNATURE;
    _adi_osal_oStartupVirtualThread.nThreadPrio = STARTUP_PRIORITY;

#ifdef OSAL_DEBUG
    snOsalInitializationState = OSAL_INITIALIZED;
#endif
    
    return( ADI_OSAL_SUCCESS);
}


/*
**
** EOF: 
**
*/
