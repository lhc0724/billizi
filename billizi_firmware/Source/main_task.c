#include "main_task.h"

#include "serial_interface.h"
#include "hw_mgr.h"
#include "log_mgr.h"
#include "ble_service_mgr.h"

#if defined FEATURE_OAD
  #include "oad.h"
  #include "oad_target.h"
#endif

static uint8 main_taskID;  // Task ID for internal task/event processing
static uint8 state_taskIDs[4];

uint8 *tx_buff;
Control_flag_t ctrl_flags;

static log_data_t st_BattLog;
static log_data_t st_Times;
static log_addr_t st_LogAddr;

static uint32 sys_timer;
static uint32 excute_timer;

batt_info_t batt_status;
sensor_info_t sensor_vals;


/***** API Level Function *****/
uint16 get_main_taskID() 
{
    return main_taskID;
}

void set_main_params(uint8 opt, uint8 size, void *pValue)
{
    switch(opt) {
        case PARAM_LOGADDR:
            if (size == sizeof(log_addr_t)) {
                st_LogAddr = *((log_addr_t *)pValue);
            }
            break;
        case PARAM_LOGDATA:
            if (size == sizeof(log_data_t)) {
                st_BattLog = *((log_data_t *)pValue);
            }
            break;
        case PARAM_CTRL_FLAG:
            if (size == sizeof(Control_flag_t)) {
                ctrl_flags = *((Control_flag_t *)pValue);
            }
            break;
    }
}

void get_main_params(uint8 opt, void *pValue)
{
    switch(opt) {
        case PARAM_LOGADDR:
            *((log_addr_t*)pValue) = st_LogAddr;
            break;
        case PARAM_LOGDATA:
            *((log_data_t*)pValue) = st_BattLog;
            break;
        case PARAM_CTRL_FLAG:
            *((Control_flag_t*)pValue) = ctrl_flags;
            break;
    }
}

/***** OSAL SYSTEM FUNCTIONS *****/

/* battery main task process initialize */
void Billizi_Process_Init(uint8 task_id) 
{
    main_taskID = task_id;

    setup_gap_peripheral_profile();
    setup_gap_gatt_service();
    setup_simple_prof_service();

// #if defined FEATURE_OAD
//     //Initialize OAD management attributes
//     VOID OADTarget_AddService();
// #endif

    setup_advert_interval();

    //disable halt during RF (needed for UART / SPI)
    HCI_EXT_HaltDuringRfCmd(HCI_EXT_HALT_DURING_RF_DISABLE);

    log_system_init(&st_BattLog, &st_LogAddr);
    st_Times.time_info.time_datas = 0;
    sensor_status_init(&sensor_vals);
    batt_status.left_cap = BATT_CAPACITY;
    ctrl_flags.flag_all = 0;

    GAPRole_Serv_Start();
}

void StateMachine_Process_init(uint8 task_id)
{
    state_taskIDs[TASK_FACTORY_INIT] = task_id++;
    state_taskIDs[TASK_USER]         = task_id++;
    state_taskIDs[TASK_KIOSK]        = task_id++;
    state_taskIDs[TASK_ABNORMAL]     = task_id;
}

uint16 Billizi_Main_ProcessEvent(uint8 task_id, uint16 events) 
{
    VOID task_id;  // OSAL required parameter that isn't used in this function
    //uint16 next_interval = 50;

    if (events & EVT_FACTORY_INIT) {
        VOID OADTarget_AddService();
        setup_app_register_cb(APP_FACTORY_INIT);
        ble_advert_control(TRUE);
        
        erase_flash_log_area();
        osal_set_event(state_taskIDs[TASK_FACTORY_INIT], events);
    }

    if (events & EVT_USER_SERVICE) {
        setup_app_register_cb(APP_USER_COMM);
        
        ble_setup_rspData();
        //uart_disable();

        if(ctrl_flags.need_comm && st_LogAddr.head_addr >= FLADDR_LOGDATA_ST) {
            osal_set_event(main_taskID, EVT_KIOSK_PROCESS);
        }else {
            //todo: get new log address
            OADTarget_DelService();
            ble_advert_control(TRUE);
            ctrl_flags.certification = 1;

            init_batt_capacity(&batt_status);
            generate_new_log_address(&st_LogAddr);

            sys_timer = osal_GetSystemClock();
            excute_timer = osal_GetSystemClock();

            osal_set_event(state_taskIDs[TASK_USER], EVT_CERTIFICATION | EVT_MORNITORING);
        }
    }

    if (events & EVT_KIOSK_PROCESS) {
        ble_advert_control(FALSE);

        // TXD_PIO = 1;
        uart_enable();
        excute_timer = 0;
        st_LogAddr.offset_addr = 0;
        sys_timer = osal_GetSystemClock();

        osal_set_event(state_taskIDs[TASK_KIOSK], EVT_EXT_V_MONITORING);
    }

    if (events & EVT_ABNORMAL_TASK) {
        if(ctrl_flags.abnormal & ERR_BROKEN_CABLE) {
        }
        osal_set_event(main_taskID, EVT_ABNORMAL_TASK);
    }

    if (events & DEBUG) {
        
    }

    return 0; //return events process clear task free
}

//Factory initialization state machine
uint16 Factory_Init_Process(uint8 task_id, uint16 events) 
{
    uint16 next_interval = 50;
    uint16 calib_value;

    flash_8bit_t conn_type;

    float f_batt_v;

    if (ctrl_flags.factory_setup) {
        //not setup battery calibration.
        if (!ctrl_flags.ref_calib && !ctrl_flags.self_calib) {
            f_batt_v = read_voltage_sampling(10, READ_BATT_SIDE);
            if (f_batt_v <= 2) {
                //배터리 감지 안됨, self-calibration 모드
                calib_value = stored_adc_calib(0);
                setup_calib_value(TRUE, calib_value);
                ctrl_flags.self_calib = 1;
            } else {
                //배터리 전압감지, 기준전압(3.7V)으로 판단하며 reference-calibration실행
                calib_value = stored_adc_calib(read_adc_sampling(10,READ_BATT_SIDE));
                setup_calib_value(FALSE, calib_value);

                ctrl_flags.ref_calib = 1;
            }
        } else {
            read_flash(FLADDR_CONNTYPE, FLOPT_UINT32, &conn_type.all_bits);
            //debug: auto setup
            if (conn_type.byte_1 == 0xFF) {
                stored_conn_type(CONN_USB_C);
            } else if (conn_type.byte_1 != 0xFF && conn_type.byte_1 <= 0x04) {
                //connector type setup ok.
                ctrl_flags.factory_setup = 0;
            }
        }
    }

    if(ctrl_flags.factory_setup) {
        osal_start_timerEx(state_taskIDs[TASK_FACTORY_INIT], events, next_interval);
    }else {
        osal_set_event(main_taskID, EVT_USER_SERVICE);
    }

    return 0;
}

// User service state machine
uint16 User_Service_Process(uint8 task_id, uint16 events)
{
    /********************
     * @TODO:   
     * - user certification / user app communication.
     * - battery status management.
     * - discharge state triggering and mornitoring.
     * - sensor logging.
     * - full discharge control.
     * - detect abnormal use.
     */

    uint8 next_task;
    uint16 next_interval = 10;
    uint16 next_evt;

    uint8 debug_val;

    next_evt = events;
    next_task = task_id;

    if (events & EVT_CERTIFICATION) {
        ctrl_flags.certification = check_certification();
        if (!ctrl_flags.certification) {
            //certification success, enable dischg state
            next_evt &= ~(EVT_CERTIFICATION);
            next_evt |= EVT_DISCHARGE;
            ctrl_flags.serv_en = 1;
        }
    }

    if (events & EVT_DISCHARGE) {
        if(!ctrl_flags.serv_en) {
            discharge_disable();
            next_evt &= ~(EVT_DISCHARGE);
        }else {
            //print_uart("DISCHG-%d\r\n", ctrl_flags.use_conn);
            ctrl_flags.use_conn = detect_use_cable();
            if(ctrl_flags.use_conn) {
                discharge_enable(DISCHG_BLZ_CONN);
            }else {
                discharge_enable(DISCHG_USB_A);
            }
        }
    }

    if (events & EVT_MORNITORING) {
        if (check_timer(excute_timer, 1000)) {
            //get the battery status information(voltage, current, capacity)
            get_batt_status(&batt_status);
            // print_uart("%.2f[V], ", batt_status.batt_v);
            // print_uart("%d[mA], ",batt_status.current);
            print_uart("%.2f[mWh]\r\n",batt_status.left_cap);

           // //battery service excute RTC
            st_Times.time_info.time_stamp++;

            //check battery voltage
            if (batt_status.batt_v <= SERVICE_BATT_V) {
                ctrl_flags.serv_en = 0;
            }

            /* DEBUG Codes */
            if(!(st_Times.time_info.time_stamp % 10)) {
                st_BattLog.evt_info.state = (EVT_USER_SERVICE & 0xFF);
                st_BattLog.evt_info.log_value = (uint16)(batt_status.batt_v * 1000);
                st_BattLog.evt_info.voltage = 1;
                write_flash(st_LogAddr.offset_addr, &st_BattLog.source_data);
                st_LogAddr.offset_addr++;
                if (st_LogAddr.offset_addr > FLADDR_LOGDATA_ED) {
                    st_LogAddr.offset_addr = FLADDR_LOGDATA_ST;
                }
            }

            excute_timer = osal_GetSystemClock();
        }
        // //read temperature sensor
        sensor_vals.temperature = read_temperature();
        if(sensor_vals.temperature >= 70) {
            
        }

        if (!VIB_SENSOR) {
            sensor_vals.impact_cnt++;
            if(sensor_vals.impact_cnt >= 30) {
                sensor_vals.impact_cnt = 0;
            }
        }

        if (read_voltage(READ_EXT) >= EXT_MIN_V) {
            //returned battery to kiosk.
            ctrl_flags.need_comm = 1;
            next_task = main_taskID;
            next_evt = EVT_KIOSK_PROCESS;
        }
    }

    if(ctrl_flags.logging) {
        //todo: logging current status
    }

    SimpleProfile_GetParameter(SIMPLEPROFILE_CHAR2, &debug_val);
    if(debug_val & 0x80) {
        debug_val &= ~(0x80);
        // GAPRole_TerminateConnection();
        // ble_advert_control(FALSE);
        // VOID OADTarget_AddService();
        // ble_advert_control(TRUE);
    }

    osal_start_timerEx(next_task, next_evt, next_interval);

    return 0;
}

uint16 Kiosk_Process(uint8 task_id, uint16 events)
{
    float tmp_voltage;
    uint16 next_evt;
    uint8 next_task;

    next_task = task_id;
    next_evt = events;

    if (events & EVT_EXT_V_MONITORING) {
        tmp_voltage = read_voltage(READ_EXT);

        if (check_timer(sys_timer, 100)) {
            //check the external voltage every interval 100ms
            if (tmp_voltage < EXT_MIN_V && tmp_voltage >= EXT_COMM_V) {
                //kiosk communication mode check ok.
                next_evt = EVT_COMM;
            } else if (tmp_voltage <= 1) {
                //cannot be detected external voltage.
                ctrl_flags.abnormal |= ERR_COMMUNICATION;
                next_task = main_taskID;
                next_evt = EVT_ABNORMAL_TASK;
            }else {
                //not communication mode, voltage is high.
                excute_timer++;
            }
            sys_timer = osal_GetSystemClock();
        }

        if(excute_timer >= 100) {
            //external voltage is unresponsive 10 seconds over.
            ctrl_flags.abnormal |= ERR_COMMUNICATION;
            excute_timer = 0;
            next_task = main_taskID;
            next_evt = EVT_ABNORMAL_TASK;
        }
    }

    if (events & EVT_COMM) {
        if (tx_buff == NULL) {
            if (st_LogAddr.offset_addr == 0) {
                tx_buff = get_head_packet(&ctrl_flags, &batt_status, calc_number_of_LogDatas(&st_LogAddr));
            }else {
                tx_buff = get_log_packet(&st_LogAddr);
            }
        }
        
        transmit_data_stream(osal_strlen(tx_buff), tx_buff);

        if (read_voltage(READ_EXT) >= EXT_MIN_V) {
            if (st_LogAddr.offset_addr == st_LogAddr.tail_addr) {
                //communication end. init log variables and transition charge state.
                uart_disable();
                charge_enable();
                next_evt = EVT_CHARGE;
            } else {
                excute_timer = 0;

                st_LogAddr.offset_addr++;
                if (st_LogAddr.offset_addr > FLADDR_LOGDATA_ED) {
                    st_LogAddr.offset_addr = FLADDR_LOGDATA_ST;
                }

                sys_timer = osal_GetSystemClock();
                next_evt = EVT_EXT_V_MONITORING;
            }
            osal_mem_free(tx_buff);
            tx_buff = NULL;
        }
    }

    if (events & EVT_CHARGE) {
        if (read_voltage(READ_EXT) < EXT_MIN_V) {
            sys_timer = osal_GetSystemClock();
            next_evt = EVT_HOLD_BATT;
            charge_disable();
        }
    }

    if (events & EVT_HOLD_BATT) {
        if (check_timer(sys_timer, 200)) {
            if (TXD_PIO) {
                tmp_voltage = read_voltage(READ_EXT);

                if (tmp_voltage >= 2 && tmp_voltage < EXT_COMM_V) {
                    //request communication from kiosk
                    next_evt = EVT_COMM;
                } else if (tmp_voltage < 2) {
                    //rented battery, service start.
                    next_evt = EVT_USER_SERVICE;
                    next_task = main_taskID;
                }
                TXD_PIO = 0;
            } else {
                TXD_PIO = 1;
            }
        }
    }

    osal_start_timerEx(next_task, next_evt, 10);

    return 0;
}

uint16 Abnormaly_Process(uint8 task_id, uint16 events)
{

    return 0;
}