/******************************************************************************

 @file  scanparamservice.h

 @brief This file contains the Scan Parameters Service definitions and
        prototypes.

 Group: WCS, BTS
 Target Device: CC2540, CC2541

 ******************************************************************************
 
 Copyright (c) 2011-2019, Texas Instruments Incorporated
 All rights reserved.

 IMPORTANT: Your use of this Software is limited to those specific rights
 granted under the terms of a software license agreement between the user
 who downloaded the software, his/her employer (which must be your employer)
 and Texas Instruments Incorporated (the "License"). You may not use this
 Software unless you agree to abide by the terms of the License. The License
 limits your use, and you acknowledge, that the Software may not be modified,
 copied or distributed unless embedded on a Texas Instruments microcontroller
 or used solely and exclusively in conjunction with a Texas Instruments radio
 frequency transceiver, which is integrated into your product. Other than for
 the foregoing purpose, you may not use, reproduce, copy, prepare derivative
 works of, modify, distribute, perform, display or sell this Software and/or
 its documentation for any purpose.

 YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
 PROVIDED �AS IS� WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
 NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
 TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
 NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
 LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
 INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
 OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
 OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
 (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

 Should you have any questions regarding your right to use this Software,
 contact Texas Instruments Incorporated at www.TI.com.

 ******************************************************************************
 Release Name: ble_sdk_1.5.0.16
 Release Date: 2019-04-18 08:53:32
 *****************************************************************************/

#ifndef SCANPARAMSERVICE_H
#define SCANPARAMSERVICE_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */

// Scan Characteristic Lengths
#define SCAN_INTERVAL_WINDOW_CHAR_LEN     4
#define SCAN_PARAM_REFRESH_LEN            1

// Scan Parameter Refresh Values
#define SCAN_PARAM_REFRESH_REQ            0x00

// Callback events
#define SCAN_INTERVAL_WINDOW_SET          1

// Get/Set parameters
#define SCAN_PARAM_PARAM_INTERVAL         0
#define SCAN_PARAM_PARAM_WINDOW           1

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * Profile Callbacks
 */

// Scan Parameters Service callback function
typedef void (*scanParamServiceCB_t)(uint8 event);

/*********************************************************************
 * API FUNCTIONS 
 */

/*********************************************************************
 * @fn      ScanParam_AddService
 *
 * @brief   Initializes the Service by registering
 *          GATT attributes with the GATT server.
 *
 * @return  Success or Failure
 */
extern bStatus_t ScanParam_AddService(void);

/*********************************************************************
 * @fn      ScanParam_Register
 *
 * @brief   Register a callback function with the Scan Parameters Service.
 *
 * @param   pfnServiceCB - Callback function.
 *
 * @return  None.
 */
extern void ScanParam_Register(scanParamServiceCB_t pfnServiceCB);

/*********************************************************************
 * @fn      ScanParam_SetParameter
 *
 * @brief   Set a Scan Parameters Service parameter.
 *
 * @param   param - Profile parameter ID
 * @param   len - length of data to right
 * @param   value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
extern bStatus_t ScanParam_SetParameter(uint8 param, uint8 len, void *value);
  
/*********************************************************************
 * @fn      ScanParam_GetParameter
 *
 * @brief   Get a Scan Parameters Service parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to get.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
extern bStatus_t ScanParam_GetParameter(uint8 param, void *value);

/*********************************************************************
 * @fn      ScanParam_RefreshNotify
 *
 * @brief   Notify the peer to refresh the scan parameters.
 *
 * @param   connHandle - connection handle
 *
 * @return  None
 */
extern void ScanParam_RefreshNotify(uint16 connHandle);


/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* SCANPARAMSERVICE_H */
