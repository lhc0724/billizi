#ifndef __MAIN_TASK__
#define __MAIN_TASK__

/*** ti cc254x api includes ***/

#include "bcomdef.h"
#include "OSAL.h"
#include "OSAL_PwrMgr.h"

#include "OnBoard.h"
#include "hal_adc.h"
#include "hal_uart.h"
#include "gatt.h"

#include "hci.h"

#include "gapgattserver.h"
#include "gattservapp.h"
#include "simpleGATTprofile_Bridge.h"

#include "peripheral.h"

#include "gapbondmgr.h"

/* billizi main task events */
#define EVT_FACTORY_INIT         0x0001
#define EVT_USER_SERVICE         0x0002
#define EVT_KIOSK_PROCESS        0x0004
#define EVT_ABNORMAL_TASK        0x0008

/* billizi user service events */
#define EVT_CERTIFICATION       0x0010    
#define EVT_DISCHARGE           0x0020
#define EVT_MORNITORING         0x0040

/* billizi kiosk events */
#define EVT_EXT_V_MONITORING    0x0080
#define EVT_COMM                0x0100
#define EVT_CHARGE              0x0200
#define EVT_HOLD_BATT           0x0400

#define DEBUG                    0x4000
#define DBG_EVT_A                0x1000

#define PARAM_LOGADDR       0x01
#define PARAM_LOGDATA       0x02
#define PARAM_CTRL_FLAG     0x03

typedef enum _TASK_LOCATION {
    TASK_FACTORY_INIT, TASK_USER, TASK_KIOSK, TASK_ABNORMAL
}state_task_t;

/*********************************************************************
 * MACROS
 */
#define FIRMWARE_VERSION    0x01

/* Local Function Group */

static void peripheralStateNotificationCB(gaprole_States_t newState);
static void simpleProfileChangeCB(uint8 paramID);


uint16 get_main_taskID();
void get_main_params(uint8 opt, void *pValue);
void set_main_params(uint8 opt, uint8 size, void *pValue);

/** Task Initialization for the BLE Application **/
extern void Billizi_Process_Init(uint8 task_id);
extern void StateMachine_Process_init(uint8 task_id);

/** Task Event Processor for the BLE Application **/
extern uint16 Billizi_Main_ProcessEvent(uint8 task_id, uint16 events);
extern uint16 Factory_Init_Process(uint8 task_id, uint16 events);
extern uint16 User_Service_Process(uint8 task_id, uint16 events);

#endif