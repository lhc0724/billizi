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
static uint8 state_taskIDs[5];

uint8 *tx_buff;
Control_flag_t ctrl_flags;

static time_data_t st_Times;
static log_addr_t st_LogAddr;
static log_data_t st_BattLog;

static uint32 sys_timer;
static uint32 main_timer;
static uint16 timer_cnt;

batt_info_t batt_status;
sensor_info_t sensor_vals;

uint16 debug_vars = 0;

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
        case PARAM_EVT_VALS:
            *((uint16*)pValue) = debug_vars;
            break;
    }
}

uint8 set_log_data(uint16 status, uint16 log_value)
{
    if(ctrl_flags.logging) {
        /* 아직 로그가 기록이 완료 되지 않은상태.
         * 로그가 기록 될때까지 로그데이터가 덮어씌워짐을 방지 */
        return 1;
    }

    //구조 수정중..
    
    //success
    return 0;
}

/***** OSAL SYSTEM FUNCTIONS *****/

/* battery main task process initialize */
void Billizi_Process_Init(uint8 task_id) 
{
    main_taskID = task_id;

    setup_gap_peripheral_profile();

#if defined FEATURE_OAD
    //Initialize OAD management attributes
    VOID OADTarget_AddService();
#endif

    setup_gap_gatt_service();
    setup_simple_prof_service();

    setup_advert_interval();

    //disable halt during RF (needed for UART / SPI)
    HCI_EXT_HaltDuringRfCmd(HCI_EXT_HALT_DURING_RF_DISABLE);

    log_system_init(&st_BattLog, &st_LogAddr);
    st_Times.log_evt = LOG_HEAD_TIME;
    st_Times.time_value = 0;

    sensor_status_init(&sensor_vals);
    ctrl_flags.flag_all = 0;

    tx_buff = NULL;

    GAPRole_Serv_Start();
    ble_advert_control(FALSE);
}

void StateMachine_Process_init(uint8 task_id)
{
    state_taskIDs[TASK_FACTORY_INIT] = task_id++;
    state_taskIDs[TASK_USER_SERVICE] = task_id++;
    state_taskIDs[TASK_KIOSK]        = task_id++;
    state_taskIDs[TASK_ABNORMAL]     = task_id++;
    state_taskIDs[TASK_MORNITORING]  = task_id;
}

uint16 Billizi_Main_ProcessEvent(uint8 task_id, uint16 events) 
{
    VOID task_id;  // OSAL required parameter that isn't used in this function
    //uint16 next_interval = 50;
    state_task_t eTask;

    eTask = (state_task_t)events;
    debug_vars = events;

    switch (eTask) {
        case TASK_FACTORY_INIT:
            setup_app_register_cb(APP_FACTORY_INIT);
            //ble_advert_control(FALSE);

            erase_flash_log_area();
            osal_set_event(state_taskIDs[TASK_FACTORY_INIT], events);
            break;
        case TASK_USER_SERVICE:
            //ble callback 등록
            setup_app_register_cb(APP_USER_COMM);

            //ble response data 등록
            ble_setup_rspData();

            if (ctrl_flags.need_comm && st_LogAddr.head_addr >= FLADDR_LOGDATA_ST) {
                //kisok와 통신 필요, 서비스 불가
                osal_set_event(main_taskID, TASK_KIOSK);
            } else {
                //ble 활성화
                ble_advert_control(TRUE);

                //배터리 상태값 초기화
                init_batt_status_info(&batt_status);

                //사용할 로그 주소 새로 발급
                generate_new_log_address(&st_LogAddr);

                //타미어, 시간 데이터 초기화
                st_Times.time_value = 0;
                timer_cnt = 0;

                main_timer = osal_GetSystemClock();
                sys_timer = osal_GetSystemClock();

                // print_uart("%X|%X|%X\r\n", st_LogAddr.head_addr, st_LogAddr.offset_addr, st_LogAddr.key_addr);
                // print_uart("%.2f[V]\r\n", batt_status.batt_v);

                osal_set_event(state_taskIDs[TASK_USER_SERVICE], CERTIFICATION);
            }
            break;
        case TASK_KIOSK:
            //ble 종료
            ble_advert_control(FALSE);

            //uart활성화
            uart_enable();
            timer_cnt = 0;

            //통신할 로그 개수 계산
            st_LogAddr.offset_addr = 0;
            st_LogAddr.log_cnt = calc_number_of_LogDatas(st_LogAddr);

            sys_timer = osal_GetSystemClock();

            osal_set_event(state_taskIDs[TASK_KIOSK], EVT_EXT_V_MONITORING);
            break;
        case TASK_ABNORMAL:
            discharge_disable();
            charge_disable();

            //set_log_data(events, sensor_vals.temperature);
            uart_enable();

            sys_timer = osal_GetSystemClock();
            osal_set_event(state_taskIDs[TASK_ABNORMAL], 0);
            break;
        case DEBUG:
            osal_set_event(main_taskID, DEBUG);
            break;
        default:
            //do not work
            break;
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
#if 1
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
            if (conn_type.byte_1 == 0xFF) {
                stored_conn_type(CONN_USB_C);
                //stored_conn_type(CONN_MICRO_5);
                //stored_conn_type(CONN_LIGHTNING);
            } else if (conn_type.byte_1 != 0xFF && conn_type.byte_1 <= 0x04) {
                //connector type setup ok.
                ctrl_flags.factory_setup = 0;
            }
        }
#else
        //KC인증용
        if (!ctrl_flags.self_calib) {
            calib_value = stored_adc_calib(0);
            setup_calib_value(TRUE, calib_value);
            ctrl_flags.self_calib = 1;
        } else {
            read_flash(FLADDR_CONNTYPE, FLOPT_UINT32, &conn_type.all_bits);

            //auto connector type setup
            if (conn_type.byte_1 == 0xFF) {
                stored_conn_type(CONN_USB_C);
                //stored_conn_type(CONN_MICRO_5);
                //stored_conn_type(CONN_LIGHTNING);
            } else if (conn_type.byte_1 != 0xFF && conn_type.byte_1 <= 0x04) {
                //connector type setup ok.
                ctrl_flags.factory_setup = 0;
            }
        }
#endif
    }

    if(ctrl_flags.factory_setup) {
        osal_start_timerEx(state_taskIDs[TASK_FACTORY_INIT], events, next_interval);
    }else {
        osal_set_event(main_taskID, TASK_USER_SERVICE);
        //osal_set_event(get_main_taskID(), DEBUG);
    }

    return 0;
}

// User service state machine
uint16 User_Service_Process(uint8 task_id, uint16 events)
{
    /********************
     * @TODO:   
     * - detect abnormal use.
     */

    uint8 next_task;
    uint16 next_state;
    uint16 mornit_evt = 0;
    Evt_Serv_t user_evt;

    uint16 next_interval = 10;
    uint16 mornitoring_interval = 100;

    debug_vars = events;

    next_task = task_id;
    user_evt = (Evt_Serv_t)events;
    next_state = events;

    switch(user_evt) {
        case CERTIFICATION:
            if(!check_certification()) {
                next_state = BATT_PRE_DISCHG;
            }
            break;
        case BATT_PRE_DISCHG:
            batt_status.batt_v = read_voltage(READ_BATT_SIDE);
            next_interval = 100;

            if (ctrl_flags.serv_en) {
                ctrl_flags.use_conn = detect_use_cable();
                if (ctrl_flags.use_conn) {
                    discharge_enable(DISCHG_BLZ_CONN);
                } else {
                    discharge_enable(DISCHG_USB_A);
                }

                //전류가 100[mA]이상 흐르고, 100[mSec] 유지시 방전 스테이트로 이동
                if (read_current(READ_CURR_DISCHG) >= 100) {
                    next_interval = 10;
                    if (check_timer(sys_timer, 100)) {
                        next_state = BATT_DISCHG;
                    }
                } else {
                    //타이머 초기화
                    sys_timer = osal_GetSystemClock();
                }

            } else {
                discharge_disable();
            }
            
            if(batt_status.batt_v < SERVICE_BATT_V) {
                next_state = SERVICE_END;
            }

            break;
        case BATT_DISCHG:
            batt_status.current = read_current(READ_CURR_DISCHG);
            batt_status.batt_v = read_voltage(READ_BATT_SIDE);
            batt_status.batt_v += ((SHUNT_R_VAL * batt_status.current)/1000);

            //전류가 100[mA]이하로 100[mSec]이상 유지시 방전 종료
            if (batt_status.current <= 100) {
                if (check_timer(sys_timer, 100)) {
                    next_state = BATT_PRE_DISCHG;
                }
            } else {
                sys_timer = osal_GetSystemClock();
            }
            
            //배터리 서비스전압 미달시 무조건 종료
            if(batt_status.batt_v < SERVICE_BATT_V) {
                next_state = SERVICE_END;
            }
            break;
        case SERVICE_END:
            batt_status.batt_v = read_voltage(READ_BATT_SIDE);
            if(batt_status.batt_v < MIN_BATT_V) {
                //로깅 및 파워오프
            }
            if (read_voltage(READ_EXT) >= EXT_COMM_V) {
                //외부전원이 감지되고 200[mSec]이 지나면 조건 충족.
                if (check_timer(sys_timer, 200)) {
                    //서비스 스테이트머신 종료, 키오스크 스테이트머신으로 이동
                    next_task = main_taskID;
                    next_state = TASK_KIOSK;
                }
            } else {
                sys_timer = osal_GetSystemClock();
                next_interval = 200;
            }
            break;
        default:
            //do not work
            break;
    }

    if (next_state != events) {
        //state transition
        switch (next_state) {
            case BATT_PRE_DISCHG:
                discharge_disable();
                ctrl_flags.batt_dischg = 0;
                break;
            case BATT_DISCHG:
                ctrl_flags.batt_dischg = 1;
                break;
            case SERVICE_END:
                discharge_disable();
                ctrl_flags.serv_en = 0;
                ctrl_flags.batt_dischg = 0;
                break;
        }
    }

    if(check_timer(main_timer, 1000)) {
        //매초간격 서비스 시간 계산
        st_Times.time_value++;

        if(st_Times.time_value % 30) {
            //30초 간격으로 배터리 케이블 끊어짐 확인
            mornit_evt = 1;
        }
        main_timer = osal_GetSystemClock();
    }

    //서비스 스테이트 머신일때만 모니터링 프로세서 콜
    if (next_task == task_id) {
        if (sensor_vals.impact_cnt > 0) {
            mornitoring_interval = 1;
        }
        osal_start_timerEx(state_taskIDs[TASK_MORNITORING], mornit_evt, mornitoring_interval);
    }

    osal_start_timerEx(next_task, next_state, next_interval);

    return 0;
}

uint16 Battery_Monitoring_Process(uint8 task_id, uint16 events)
{

    if (events) {
        //배터리 케이블 끊어짐 확인
        if(!ctrl_flags.batt_dischg) {
            //방전중이 아닐때만 검사
        }
    }

    if(!VIB_SENSOR) {
        sensor_vals.impact_cnt++;
    }else {
        if(sensor_vals.impact_cnt > 0) {
            /* 충격센서 감도 테스트해야함, 충격량에따른 로그기록은 미정 */
            print_uart("imp-%d\r\n", sensor_vals.impact_cnt);
            sensor_vals.impact_cnt = 0;
        }
    }

    sensor_vals.temperature = read_temperature();
    if (sensor_vals.temperature >= 700) {
        //70.0도 이상일때 로깅
        print_uart("temp-%d\r\n", sensor_vals.temperature);
    }

    return 0;
}

uint16 Kiosk_Process(uint8 task_id, uint16 events)
{
    float tmp_voltage;
    uint16 next_evt;
    uint8 next_task;

    next_task = task_id;
    next_evt = events;

    debug_vars = events;

#if 0
    //KC인증용
    uint8 size = BATT_INFO_LEN;

    if (events & EVT_EXT_V_MONITORING) {
        tmp_voltage = read_voltage(READ_EXT);
        if (check_timer(sys_timer, 50)) {
            //check the external voltage every interval 50ms
            switch (ext_voltage_analysis(tmp_voltage)) {
                case EXT_COMM_V:
                    timer_cnt = 0;
                    batt_status.hysteresis_cnt = 0;

                    if (!(next_evt & EVT_COMM)) {
                        next_evt |= EVT_COMM;

                        // if (tx_buff != NULL) {
                        //     osal_mem_free(tx_buff);
                        //     tx_buff = NULL;
                        // }
                        uart_enable();
                    }
                    break;
                case EXT_MIN_V:
                    if(tx_buff != NULL) {
                        timer_cnt++;
                    }
                    break;
                default:
                    batt_status.hysteresis_cnt++;
                    break;
            }
            sys_timer = osal_GetSystemClock();
        }

        if(timer_cnt >= 20) {
            next_evt = EVT_CHARGE;
            uart_disable();
            charge_enable();
        }

        if(batt_status.hysteresis_cnt >= 10) {
            next_task = main_taskID;
            next_evt = TASK_USER_SERVICE;
        }

        if(next_evt == TASK_USER_SERVICE || next_evt == EVT_CHARGE) {
            osal_mem_free(tx_buff);
            tx_buff = NULL;
        }
    }

    if (events & EVT_COMM) {
        if (tx_buff == NULL) {
            tx_buff = get_head_packet(&ctrl_flags, &batt_status, st_LogAddr.log_cnt);
        }else{
            transmit_data_stream(size, tx_buff);
        }

    }

    if (events & EVT_CHARGE) {
        tmp_voltage = read_voltage(READ_EXT);
        batt_status.batt_v = read_voltage(READ_BATT_SIDE);

        if (check_timer(sys_timer, 10)) {
            switch (ext_voltage_analysis(tmp_voltage)) {
                case EXT_MIN_V:
                    batt_status.current = read_current(READ_CURR_CHG);

                    if (batt_status.current <= 300) {
                        //print_uart("%.2f\r\n", batt_status.current);
                        batt_status.hysteresis_cnt++;
                    } else {
                        batt_status.hysteresis_cnt = 0;
                    }
                    timer_cnt = 0;
                    break;
                default:
                    timer_cnt++;
                    break;
            }

            if (batt_status.hysteresis_cnt >= 100) {
                //full charge
                batt_status.hysteresis_cnt = 0;
                next_evt = EVT_HOLD_BATT;
            }

            if (timer_cnt >= 10) {
                //kiosk is not charge mode
                next_evt = EVT_HOLD_BATT;
            }
 
            sys_timer = osal_GetSystemClock();
        }

        if (next_evt == EVT_HOLD_BATT) {
            charge_disable();
            TXD_PIO = 1;
        }
    }

    if (events & EVT_HOLD_BATT) {
        tmp_voltage = read_voltage(READ_EXT);
        batt_status.batt_v = read_voltage(READ_BATT_SIDE);

        if (tmp_voltage >= EXT_MIN_V && batt_status.batt_v <= MAX_BATT_V * 0.95) {
            next_evt = EVT_CHARGE;
            charge_enable();
        } else if (tmp_voltage < EXT_COMM_V) {
            next_evt = EVT_EXT_V_MONITORING;
        }
    }

#else 
    if (events & EVT_EXT_V_MONITORING) {
        tmp_voltage = read_voltage(READ_EXT);

        if (check_timer(sys_timer, 50)) {
            //check the external voltage every interval 100ms
            switch (ext_voltage_analysis(tmp_voltage)) {
                case EXT_COMM_V:
                    next_evt = EVT_COMM;
                    uart_enable();
                    //print_uart("%.2f\r\n", batt_status.left_cap);
                    break;
                default:
                    timer_cnt++;
                    break;      
            }
            sys_timer = osal_GetSystemClock();
        }
    }

    if (events & EVT_COMM) {
        

        tmp_voltage = read_voltage(READ_EXT);
        switch(ext_voltage_analysis(tmp_voltage)) {
            case EXT_MIN_V:
                if (check_timer(sys_timer, 100)) {
                    osal_mem_free(tx_buff);
                    tx_buff = NULL;

                    uart_disable();
                    if (st_LogAddr.offset_addr == st_LogAddr.tail_addr) {
                        next_evt = EVT_CHARGE;

                        batt_status.hysteresis_cnt = 0;
                        stored_key_address(&st_LogAddr);

                        charge_enable();
                        ctrl_flags.need_comm = 0;
                        timer_cnt = 0;
                    } else {
                        next_evt = EVT_EXT_V_MONITORING;
                        TXD_PIO = 1;
                    }
                    sys_timer = osal_GetSystemClock();
                }
                break;
            case EXT_COMM_V:
                sys_timer = osal_GetSystemClock();
                if (tx_buff == NULL) {
                    if (st_LogAddr.offset_addr == 0) {
                        tx_buff = get_head_packet(&ctrl_flags, &batt_status, st_LogAddr.log_cnt);
                        st_LogAddr.log_cnt = 0;
                        st_LogAddr.offset_addr = st_LogAddr.head_addr;
                    } else {
                        tx_buff = get_log_packet(&st_LogAddr);
                    }
                } else {
                    uint8 size;
                    if (tx_buff[0] == HEADER_INFO) {
                        size = BATT_INFO_LEN;
                    } else {
                        size = BATT_LOG_LEN;
                    }

                    transmit_data_stream(size, tx_buff);
                    //print_hex(tx_buff, size);
                }
                break;
            default:
                next_evt = TASK_USER_SERVICE;
                next_task = main_taskID;
            //    if(check_timer(sys_timer, 1000)) {
            //        ctrl_flags.abnormal = ERR_COMMUNICATION;
            //        next_evt = EVT_ABNORMAL_PROCESS;
            //        next_task = main_taskID;
            //    }
               break;
        }
    }

    if (events & EVT_CHARGE) {
        //uart_enable();
        tmp_voltage = read_voltage(READ_EXT);

        if (check_timer(sys_timer, 10)) {
            switch(ext_voltage_analysis(tmp_voltage)) {
                case EXT_MIN_V:
                    batt_status.current = read_current(READ_CURR_CHG);

                    // batt_status.batt_v = read_voltage(READ_BATT_SIDE);
                    // batt_status.batt_v -= ((SHUNT_R_VAL * batt_status.current) / 1000);

                    if (CHG_EN) {
                        if (batt_status.current <= 300) {
                            //print_uart("%.2f\r\n", batt_status.current);
                            batt_status.hysteresis_cnt++;
                        } else {
                            batt_status.hysteresis_cnt = 0;
                        }
                    }
                    break;
                default:
                  
                    timer_cnt++;
                    break;
            }

            if(batt_status.hysteresis_cnt >= 100) {
                //full charge
                charge_disable();
              
                update_self_calibration(ctrl_flags.self_calib, read_adc_sampling(10, READ_BATT_SIDE));

                setup_calib_value(TRUE, search_self_calib());
                ctrl_flags.self_calib = 1;
                batt_status.hysteresis_cnt = 0;
                next_evt = EVT_HOLD_BATT;
            }

            if (timer_cnt >= 10) {
                //kiosk is not charge mode
                next_evt = EVT_HOLD_BATT;
                charge_disable();
            }

            sys_timer = osal_GetSystemClock();
        }

        if(next_evt == EVT_HOLD_BATT) {
            sys_timer = osal_GetSystemClock();
            timer_cnt = 0;

            //uart_enable();
            TXD_PIO = 1;
         }
    }

    if (events & EVT_HOLD_BATT) {
        if (check_timer(sys_timer, 10)) {
            tmp_voltage = read_voltage_sampling(10, READ_EXT);

            switch (ext_voltage_analysis(tmp_voltage)) {
                case EXT_COMM_V:
                    if (timer_cnt > 0) {
                        timer_cnt = 0;
                        next_evt = EVT_BATT_INFO_REQ;
                        uart_enable();
                        sys_timer = osal_GetSystemClock();

                        print_uart("batt-%.2f\r\n", batt_status.batt_v);
                    }
                    break;
                case EXT_MIN_V:
                    // batt_status.batt_v = read_voltage(READ_BATT_SIDE);
                    // if (batt_status.batt_v < MAX_BATT_V) {
                        TXD_PIO = 0;
                        timer_cnt = 0;
                        next_evt = EVT_CHARGE;
                        charge_enable();
                    // }
                    break;
                default:
                    timer_cnt++;
                    
                    break;
            }
            sys_timer = osal_GetSystemClock();
        }

        if (timer_cnt >= 100) {
            uart_enable();
            next_evt = TASK_USER_SERVICE;
            next_task = main_taskID;
            timer_cnt = 0;
        }
    }

    if (events & EVT_BATT_INFO_REQ) {
        if (tx_buff == NULL) {
            batt_status.batt_v = read_voltage(READ_BATT_SIDE);
            tx_buff = get_head_packet(&ctrl_flags, &batt_status, st_LogAddr.log_cnt);
        }else {
            transmit_data_stream(BATT_INFO_LEN, tx_buff);
        }

        if (check_timer(sys_timer, 10)) {
            tmp_voltage = read_voltage(READ_EXT);
            switch (ext_voltage_analysis(tmp_voltage)) {
                case EXT_MIN_V:
                    timer_cnt++;
                    break;
                default:
                    timer_cnt = 0;
                    //next_evt = EVT_ABNORMAL_PROCESS;
                    break;
            }
        }

        if (timer_cnt >= 10) {
            timer_cnt = 0;
            next_evt = EVT_HOLD_BATT;

            uart_disable();
            charge_disable();
            TXD_PIO = 1;
        }

        if (next_evt != EVT_BATT_INFO_REQ) {
            osal_mem_free(tx_buff);
            tx_buff = NULL;
        }
    }
#endif

    if (next_evt != events) {
        timer_cnt = 0;
        sys_timer = osal_GetSystemClock();
    }
    osal_start_timerEx(next_task, next_evt, 10);

    return 0;
}

uint16 Abnormaly_Process(uint8 task_id, uint16 events)
{
    uint16 next_evt = events;
    uint8 next_task = task_id;

    float ext_voltage;
    debug_vars = events;
    //log_data_t tmp_logdata;

    //print_uart("%d\r\n", ctrl_flags.abnormal);

    if (ctrl_flags.abnormal & ERR_COMMUNICATION) {
        if (st_LogAddr.head_addr != st_LogAddr.offset_addr) {
            st_LogAddr.offset_addr = st_LogAddr.head_addr;
        }

        if (check_timer(sys_timer, 500)) {
            ext_voltage = read_voltage(READ_EXT);
            if (ext_voltage >= EXT_MIN_V) {
                next_evt = TASK_KIOSK;
                ctrl_flags.abnormal &= ~(ERR_COMMUNICATION);
                next_task = main_taskID;
            }
            sys_timer = osal_GetSystemClock();
        }
    }


    if (ctrl_flags.abnormal & ERR_LOSS_BATTERY) {
        //print_uart("%.2f\r\n", read_voltage(READ_BATT_SIDE));
        if(read_voltage(READ_BATT_SIDE) > MIN_BATT_V) {
            next_evt = TASK_USER_SERVICE;
            next_task = main_taskID;
        }
    }

    if (ctrl_flags.abnormal & ERR_FLASH_MEMS) {
        st_LogAddr.key_addr = get_key_address();
        st_LogAddr.tail_addr = analysis_keylog(st_LogAddr.key_addr);

        st_LogAddr.head_addr = st_LogAddr.tail_addr;

        if (!st_LogAddr.tail_addr) {
            st_LogAddr.head_addr = 0;
            HalFlashErase(ADDR_2_PAGE(FLADDR_LOGDATA_ST));
        }

        HalFlashErase(ADDR_2_PAGE(st_LogAddr.tail_addr));
        ctrl_flags.abnormal &= ~(ERR_FLASH_MEMS);

        next_evt = TASK_USER_SERVICE;
        next_task = main_taskID;
    }

    osal_set_event(next_task, next_evt);
    return 0;
}