#include "ble_service_mgr.h"
#include "serial_interface.h"
#include "flash_interface.h"

// GAP - ble scan response data (max size = 31 bytes)
// BLE 스캔시 보이는 beacon data
static uint8 res_data[B_MAX_ADV_LEN] = {
    BILLIZI_RES_LEN,                 // length of this data
    GAP_ADTYPE_LOCAL_NAME_COMPLETE,  // complete name
    'B', 'L', 'Z', '_',
    '0', '0', '0', '0', '0', '0',
    '0', '0', '0', '0', '0', '0',
    // connection interval range
    0x05,  // length of this data
    GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE,
    LO_UINT16(DEFAULT_DESIRED_MIN_CONN_INTERVAL),  // 100ms
    HI_UINT16(DEFAULT_DESIRED_MIN_CONN_INTERVAL),
    LO_UINT16(DEFAULT_DESIRED_MAX_CONN_INTERVAL),  // 1s
    HI_UINT16(DEFAULT_DESIRED_MAX_CONN_INTERVAL),

    // Tx power level
    0x02,  // length of this data
    GAP_ADTYPE_POWER_LEVEL,
    0  // 0dBm
};

/** ADVERT DATA INSTRUCTION
 * GAP - Advertisement data (max size = 31 bytes, though this is
 * best kept short to conserve power while advertisting) */
static uint8 advert_data[] = {
    0x02,  // length of this data
    // Flags; this sets the device to use limited discoverable
    GAP_ADTYPE_FLAGS,
    // discoverable mode; (advertises indefinitely)
    DEFAULT_DISCOVERABLE_MODE | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,
};

// GAP GATT Attributes
static uint8 attr_dev_name[GAP_DEVICE_NAME_LEN] = "Billizi";

static gapRolesCBs_t billizi_peripheral_cbs = {
    peripheralStateNotificationCB,  // Profile State Change Callbacks
    NULL                            // When a valid RSSI is read from controller (not used by application)
};
// Simple GATT Profile Callbacks
static simpleProfileCBs_t simple_profile_cb = {
    simpleProfileChangeCB  // Charactersitic value change callback
};
static simpleProfileCBs_t certifi_profile_cb = {
    user_certification_cb  // Charactersitic value change callback
};

/*********************************************************************
 * @fn      peripheralStateNotificationCB
 *
 * @brief   Notification from the profile of a state change.
 *
 * @param   newState - new state
 *
 * @return  none
 */
static void peripheralStateNotificationCB(gaprole_States_t newState) 
{
    switch (newState) {
        case GAPROLE_ADVERTISING:
            break;
        case GAPROLE_CONNECTED:
            break;
        default:
            break;
    }
}

/*********************************************************************
 * @fn      simpleProfileChangeCB
 *
 * @brief   Callback from SimpleBLEProfile indicating a value change
 *
 * @param   paramID - parameter ID of the value that was changed.
 *
 * @return  none
 */
static void simpleProfileChangeCB(uint8 paramID) {
    uint8 data_char3[20];
    uint8 data_char1;

    uint16 command;
    
    switch (paramID) {
        case SIMPLEPROFILE_CHAR1:
            SimpleProfile_GetParameter(SIMPLEPROFILE_CHAR1, &data_char1);
            if(data_char1 & 0x07) {
                if(!stored_conn_type((eConnType_t)data_char1)) {
            //        print_uart("ConnType - %d\r\n", data_char1);
                }
            }else {
             //   print_uart("%X\r\n", data_char1);
            }

            break;
        case SIMPLEPROFILE_CHAR3:
            SimpleProfile_GetParameter(SIMPLEPROFILE_CHAR3, &data_char3);
            command = BUILD_UINT16(data_char3[0], data_char3[1]);

            switch(command) {
                case CMD_SETUP_BATT_ID:
                    //write_flash(FLADDR_CALIB_REF, &command);
                    break;
                case CMD_SETUP_CALIB:
                    //stored_calib_value(hex2float(BUILD_UINT16(data_char3[2], data_char3[3])));
                    break;
                case CMD_SETUP_CONN:
                    //stored_conn_type((eConnType_t)data_char3[2]);
                    break;
                case CMD_SYS_REBOOT:
                    HAL_SYSTEM_RESET();
                    break;
                case CMD_RESET_FLASH:
                    break;
            }
            break;
        default:
            // do nothing
            break;
    }
}

static void user_certification_cb(uint8 paramID) {
    uint8 data_char3[20];
    uint8 data_char1;

    uint16 command;
    
    switch (paramID) {
        case SIMPLEPROFILE_CHAR3:
            SimpleProfile_GetParameter(SIMPLEPROFILE_CHAR3, &data_char3);
            command = BUILD_UINT16(data_char3[0], data_char3[1]);

            switch(command) {
                case 0xAAAA:
                    data_char1 = 0x30;
                    print_uart("CMD_Certifi\r\n");
                    set_simpleprofile(SIMPLEPROFILE_CHAR2, sizeof(uint8), &data_char1);
                    break;
            }
            break;
        case SIMPLEPROFILE_CHAR1:
            SimpleProfile_GetParameter(SIMPLEPROFILE_CHAR1, &data_char1);
            set_simpleprofile(SIMPLEPROFILE_CHAR2, sizeof(uint8), &data_char1);
            break;
        default:
            // do nothing
            break;
    }
}

uint8 check_certification()
{
    uint8 status;
    SimpleProfile_GetParameter(SIMPLEPROFILE_CHAR2, &status);
    print_uart("%#X\r\n", status);
    if(status == 0x30) {
        //certification success.
        return 0;
    }

    return 1;
}

// Setup the SimpleProfile Characteristic Values
void set_simpleprofile(uint8 props_addr, uint8 size, uint8 *p_charValue) 
{
    switch(props_addr) {
        case SIMPLEPROFILE_CHAR1:
        case SIMPLEPROFILE_CHAR2:
        case SIMPLEPROFILE_CHAR4:
            if(size == sizeof(uint8)) {
                SimpleProfile_SetParameter(props_addr, size, p_charValue);
            }
            break;
        case SIMPLEPROFILE_CHAR3:
            if (size <= (sizeof(uint8) * SIMPLEPROFILE_CHAR3_LEN)) {
                SimpleProfile_SetParameter(props_addr, size, p_charValue);
            }
            break;
        case SIMPLEPROFILE_CHAR5:
            if(size == SIMPLEPROFILE_CHAR5_LEN) {
                SimpleProfile_SetParameter(props_addr, size, p_charValue);
            }
            break;
    }

}

/**********************************************************************
 * @fn    ble_advert_control
 * @param en_opt - beacon advertisment on/off control value
 */
void ble_advert_control(uint8 en_opt) 
{
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8), &en_opt);
}

void ble_setup_rspData() 
{
    uint8 *pMac = NULL;
    uint8 str_Mac[12];
    uint8 i;

    LL_ReadBDADDR(pMac);

    /* Hex to String */
    for(i = 0; i < B_ADDR_LEN; i++) {
        str_Mac[i*2] = (pMac[i] >> 4) & 0x0F;
        str_Mac[(i*2) + 1] = pMac[i] & 0x0F;
    }

    for(i = 0; i < 12; i++) {
        if(str_Mac[i] < 0x0A) {
            str_Mac[i] = str_Mac[i] + 48;
        }else {
            str_Mac[i] = str_Mac[i] + 55;
        }
        res_data[i+6] = str_Mac[i];
    }

    GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, sizeof(res_data), res_data);
}

/***********************************************************************
 * setup BLE GAP/GATT Services functions group. */

//setup the GAP Peripheral Profile parameters.
void setup_gap_peripheral_profile()
{
    uint8 init_advert_en = FALSE;

    /*****************
     * @param gapRole_AdvertOffTime :
     *  settting this to zero, the device will go into the waiting state after 
     *  being discoverable for 30.72 second, and will not being advertising again
     *  until the enabler is set back to TRUE */
    uint16 gapRole_AdvertOffTime = 0;

    uint8 enable_update_request = DEFAULT_ENABLE_UPDATE_REQUEST;
    uint16 desired_min_interval = DEFAULT_DESIRED_MIN_CONN_INTERVAL;
    uint16 desired_max_interval = DEFAULT_DESIRED_MAX_CONN_INTERVAL;
    uint16 desired_slave_latency = DEFAULT_DESIRED_SLAVE_LATENCY;
    uint16 desired_conn_timeout = DEFAULT_DESIRED_CONN_TIMEOUT;

    // Set the GAP Role Parameters
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8), &init_advert_en);
    GAPRole_SetParameter(GAPROLE_ADVERT_OFF_TIME, sizeof(uint16), &gapRole_AdvertOffTime);

    GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, sizeof(res_data), res_data);
    GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(advert_data), advert_data);

    GAPRole_SetParameter(GAPROLE_PARAM_UPDATE_ENABLE, sizeof(uint8), &enable_update_request);
    GAPRole_SetParameter(GAPROLE_MIN_CONN_INTERVAL, sizeof(uint16), &desired_min_interval);
    GAPRole_SetParameter(GAPROLE_MAX_CONN_INTERVAL, sizeof(uint16), &desired_max_interval);
    GAPRole_SetParameter(GAPROLE_SLAVE_LATENCY, sizeof(uint16), &desired_slave_latency);
    GAPRole_SetParameter(GAPROLE_TIMEOUT_MULTIPLIER, sizeof(uint16), &desired_conn_timeout);
}

//setup the GAP GATT Service 
void setup_gap_gatt_service()
{
    GGS_SetParameter(GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, attr_dev_name);
    GGS_AddService(GATT_ALL_SERVICES); 
    GATTServApp_AddService(GATT_ALL_SERVICES);    // GATT attributes
}
    // Set advertising interval
void setup_advert_interval() 
{
    uint16 advInt = DEFAULT_ADVERTISING_INTERVAL;

    GAP_SetParamValue(TGAP_LIM_DISC_ADV_INT_MIN, advInt);
    GAP_SetParamValue(TGAP_LIM_DISC_ADV_INT_MAX, advInt);
    GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MIN, advInt);
    GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MAX, advInt);
}

void setup_simple_prof_service()
{
    SimpleProfile_AddService(GATT_ALL_SERVICES);  // Simple GATT Profile
}

void setup_app_register_cb(uint8 opt)
{   
    uint8 init_chars;
    switch(opt) {
        case APP_FACTORY_INIT:
            VOID SimpleProfile_RegisterAppCBs(&simple_profile_cb);
            break;
        case APP_USER_COMM:
            VOID SimpleProfile_RegisterAppCBs(&certifi_profile_cb);
            init_chars = 0;
            set_simpleprofile(SIMPLEPROFILE_CHAR2, sizeof(uint8), &init_chars);
            break;
    }
}

void GAPRole_Serv_Start()
{
    VOID GAPRole_StartDevice(&billizi_peripheral_cbs);
}