/* $Revision: 21629 $
 * $Date: 2013-05-29 08:01:48 -0400 (Wed, 29 May 2013) $
******************************************************************************
Copyright (c), 2008-2011 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************
*/

/*!
    @file adi_osal_ucos.c

    Operating System Abstraction Layer - OSAL for uCOS-II 

    This file contains the APIs designed to abstract uCOS-II the operating
    system from the user. 

*/

/*=============  I N C L U D E S   =============*/
#include <string.h>                                                             /* for strncpy */
#include <stdlib.h>                                                
#include "adi_osal.h"
#include "osal_ucos.h"
#include "osal_common.h"

#if defined (__ECC__)
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


/* "version.h" contain macro "ADI_BUILD_VER"  which gives the
    build version for OSAL for uCOS. This is a generated file.
*/


/*=============  D E F I N E S  =============*/



#pragma file_attr(  "libGroup=adi_osal.h")
#pragma file_attr(  "libName=libosal")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#endif
#include "version.h"


/*=============  D A T A  =============*/

/*=============  C O D E  =============*/

/*!
   ****************************************************************************
     @brief Returns the code-base version information.

     The code-base version differs for each target operating system of the OSAL
     although the major and minor revs are the same for all OS variants.

     @param[out] pVersion - the location to store the retrieved version 
                            information.

     @return ADI_OSAL_SUCCESS - if able to successfully return the version
     @return ADI_OSAL_FAILED  - in the unlikely event that the version 
                               information could not be obtained.

    @note 
    Version number is mentioned in the format major.minor.patch.build.
    For example,Version "1.0.2.2022" means

    => major  = 1.
    => minor  = 0.
    => patch  = 2.
    => build  = 2022.

    Members of structure ADI_OSAL_VERSION_PTR are also declared in above order.
*****************************************************************************/

ADI_OSAL_STATUS adi_osal_GetVersion(ADI_OSAL_VERSION *pVersion)
{
    pVersion->nMajor = ADI_OSAL_MAJOR_VER;
    pVersion->nMinor = ADI_OSAL_MINOR_VER;
    pVersion->nPatch = ADI_OSAL_PATCH_VER;
    pVersion->nBuild = ADI_BUILD_VER;
    return(ADI_OSAL_SUCCESS);
}


/*!
  ****************************************************************************
    @brief Configures OSAL.  

    This function configures the internal OSAL data structure. 

    @param[in] pConfig - pointer to a ADI_OSAL_CONFIG data structure that 
                         contains the OSAL configuration options.

    @return ADI_OSAL_SUCCESS          - Configuration is done successfully.
    @return ADI_OSAL_FAILED           - OSAL was already configured
    @return ADI_OSAL_OS_ERROR         - The version of OSAL is not compatible 
                                        with the uCOS version
    @return ADI_OSAL_BAD_SLOT_KEY     - Number of thread local storage slots 
                                        specified greater than the maximum 
                                        allowed.
    @return ADI_OSAL_BAD_PRIO_INHERIT - Priority inheritance specified when it 
                                        is not supported or vice versa.
    @return ADI_OSAL_MEM_ALLOC_FAILED - Error initializing dynamic memory heap
    @return ADI_OSAL_INVALID_ARGS     - The arguments do not describe a viable 
                                        configuration

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_Config( const ADI_OSAL_CONFIG *pConfig)
{
   uint32_t* pHeapMemory = NULL; /*ptr to the memory to use for the heap */
    uint32_t nHeapMemorySize = 0u;/*size of memory pointed by pHeapMemory */

    /*!
        @internal
        @var snOsalConfigurationState
             Static variable to record if OSAL has already been configured
        @endinternal
    */
 #ifdef OSAL_DEBUG
    static uint32_t snOsalConfigurationState = 0u ;
#endif

    /* Checks that the version of uC/OS is compatible with this version of OSAL */
    if (OSVersion() < COMPATIBLE_OS_VERSION)
    {

        return (ADI_OSAL_OS_ERROR);
    }

#ifdef OSAL_DEBUG
    /* Check if already configured. If the parameters are the same then the
     * call succeeds. Otherwise the call fails. The priority inheritance
     * setting is not recorded so we cannot check it
     */
    if (OSAL_INITIALIZED == snOsalConfigurationState)
    {
        if (NULL == pConfig)
        {
            return (ADI_OSAL_SUCCESS);
        }
        else
        {
            if ( (pConfig->nNumTLSSlots != _adi_osal_gnNumSlots) ||
                (pConfig->nSysTimerPeriodInUsec != _adi_osal_gnTickPeriod) ||
                (pConfig->pHeap != pHeapMemory) ||
                (pConfig->nHeapSizeBytes != nHeapMemorySize))
            {
                return (ADI_OSAL_FAILED);
            }
            else
            {
                return (ADI_OSAL_SUCCESS);
            }
        }
    }
#endif


    /* checks that arguments are all valid */
    if (NULL != pConfig)
    {
#ifdef OSAL_DEBUG
        if ( (ADI_OSAL_PRIO_INHERIT_ENABLED != pConfig->eEnablePrioInherit) &&
             (ADI_OSAL_PRIO_INHERIT_AUTO    != pConfig->eEnablePrioInherit))
        {
            /* incorrect value for priority inheritance */
            return(ADI_OSAL_BAD_PRIO_INHERIT);
        }

        if (pConfig->nNumTLSSlots > (uint32_t) ADI_OSAL_MAX_NUM_TLS_SLOTS)
        {
            return (ADI_OSAL_BAD_SLOT_KEY);
        }
#endif

        if (pConfig->nSysTimerPeriodInUsec != (uint32_t) 0)
        {
            _adi_osal_gnTickPeriod = pConfig->nSysTimerPeriodInUsec;
        }

        _adi_osal_gnNumSlots = pConfig->nNumTLSSlots;


        pHeapMemory = pConfig->pHeap;
        nHeapMemorySize = pConfig->nHeapSizeBytes;


    } 
    /* Create a heap with the information provided. If pHeapMemory was NULL
       then _adi_osal_HeapInstall will set its heap to the default
     */

    if ((NULL != pHeapMemory) && (0u == nHeapMemorySize))
    {
        return (ADI_OSAL_INVALID_ARGS);
    }

    if (ADI_OSAL_SUCCESS != _adi_osal_HeapInstall(pHeapMemory,nHeapMemorySize))
    {
        return (ADI_OSAL_MEM_ALLOC_FAILED);
    }


#ifdef OSAL_DEBUG
    snOsalConfigurationState = OSAL_INITIALIZED;
#endif

    return( ADI_OSAL_SUCCESS);

}

/*
**
** EOF: 
**
*/
