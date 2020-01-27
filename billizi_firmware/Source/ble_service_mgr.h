#ifndef __BLE_SERVICE_MANAGER__
#define __BLE_SERVICE_MANAGER__

#include "OSAL.h"
#include "OSAL_PwrMgr.h"
#include "bcomdef.h"

#include "OnBoard.h"
#include "gatt.h"
#include "hal_adc.h"
#include "hal_uart.h"

#include "hci.h"

#include "gapgattserver.h"
#include "gattservapp.h"
#include "simpleGATTprofile_Bridge.h"

#include "peripheral.h"
#include "gapbondmgr.h"

// How often to perform periodic event
#define SBP_SEND_EVT_PERIOD                       7
// What is the advertising interval when device is discoverable (units of 625us, 160=100ms)
#define DEFAULT_ADVERTISING_INTERVAL          160
// Whether to enable automatic parameter update request when a connection is formed
#define DEFAULT_ENABLE_UPDATE_REQUEST         FALSE

// Limited discoverable mode advertises for 30.72s, and then stops
// General discoverable mode advertises indefinitely
#define DEFAULT_DISCOVERABLE_MODE             GAP_ADTYPE_FLAGS_GENERAL

// Minimum connection interval (units of 1.25ms, 80=100ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL     8
// Maximum connection interval (units of 1.25ms, 800=1000ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL     8
// Slave latency to use if automatic parameter update request is enabled
#define DEFAULT_DESIRED_SLAVE_LATENCY         0
// Supervision timeout value (units of 10ms, 1000=10s) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_CONN_TIMEOUT          200
// Connection Pause Peripheral time value (in seconds)
#define DEFAULT_CONN_PAUSE_PERIPHERAL         6

#define INVALID_CONNHANDLE                    0xFFFF
// Length of bd addr as a string
#define B_ADDR_STR_LEN                        15

#define CMD_SETUP_BATT_ID   0x0010
#define CMD_SETUP_CALIB     0x0020
#define CMD_SETUP_CONN      0x0040
#define CMD_SYS_REBOOT      0xFFFF
#define CMD_RESET_FLASH     0xA000

#define APP_FACTORY_INIT     0x01
#define APP_USER_COMM        0x02

#define BD_NAME_LEN_ADDR    0
#define BD_NAMETYPE_ADDR    1

#define BILLIZI_RES_LEN   0x11  //17

/* local functions */
static void peripheralStateNotificationCB(gaprole_States_t newState);
static void simpleProfileChangeCB(uint8 paramID);
static void user_certification_cb(uint8 paramID);

void setup_app_register_cb(uint8 opt);
void ble_advert_control(uint8 en_opt);
void set_simpleprofile(uint8 props_addr, uint8 size, uint8 *p_charValue);

void ble_setup_rspData();

void setup_gap_gatt_service();
void setup_gap_peripheral_profile();
void setup_advert_interval();
void setup_simple_prof_service();
void GAPRole_Serv_Start();

#endif