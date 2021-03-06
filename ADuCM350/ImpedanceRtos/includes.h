/*******************************************************************************
*
* MASTER INCLUDES
*
*
* Filename      : includes.h
* Version       : V1.00
* Programmer(s) : FT
*******************************************************************************/

#ifndef INCLUDES_MODULES_PRESENT
#define INCLUDES_MODULES_PRESENT

/*******************************************************************************
*    STANDARD LIBRARIES
*******************************************************************************/

#include <cpu.h>
#include <lib_def.h>
#include <lib_mem.h>

/*******************************************************************************
* OS
*******************************************************************************/

#include <ucos_ii.h>

/*******************************************************************************
* APP / BSP
*******************************************************************************/

#include <app_cfg.h>
/*#include  <bsp.h>*/

/*******************************************************************************
* SERIAL
*******************************************************************************/

//#include  <app_serial.h>

/*******************************************************************************
* USB DEVICE
*******************************************************************************/

#if (APP_CFG_USBD_EN == DEF_ENABLED)
/*#include    <app_usbd.h>*/
#endif

/*******************************************************************************
*    MODULE END
*******************************************************************************/

#endif