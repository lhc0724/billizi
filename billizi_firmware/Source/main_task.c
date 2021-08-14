#include "main_task.h"

#include "serial_interface.h"
#include "hw_mgr.h"
#include "log_mgr.h"
#include "ble_service_mgr.h"
#include "boot_mgr.h"

#if defined FEATURE_OAD
  #include "oad.h"
  #include "oad_target.h"
#endif

#include "gpio_interface.h"
#include "adc_interface.h"
#include "flash_interface.h"

#define NEXT_INTERVAL  50

static uint8 main_taskID;  // Task ID for internal task/event processing

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

static uint8 load_adc_calib_val(Control_flag_t *apst_flags) 
{
	/*

	return value 
		1: load successfuly
		0: laod fail
	*/
    flash_16bit_t tmp_flash;
    uint16 calib_value;

    read_flash(FLADDR_CALIB_REF, FLOPT_UINT32, &tmp_flash.all_bits);
    if(tmp_flash.all_bits == EMPTY_FLASH) {
        //calibration value has never been set.
        //need factory initialize
        //need ADC calibration 
        return 0;
    }

    if(tmp_flash.all_bits == 0) {
        //this battery use self-calibration.
        calib_value = search_self_calib();
    } else if (tmp_flash.high_16bit == 0x1000) {
        //adc system setup reference calibration value
        setup_calib_value(FALSE, tmp_flash.low_16bit);
        apst_flags->ref_calib = 1;
        return 1;
    }

    if (!calib_value) {
        //not found ADC Calibration value.
        //need factory initialize
        //need ADC calibration 
        return 10
    }else {
        //adc system setup self-calibartion value
        setup_calib_value(TRUE, calib_value);
        apst_flags->self_calib = 1;
    }

    return 1;
}

static uint8 load_blz_conntype()
{
	/*
	loading billizie connector type
	return 1 : 성공
	return 0 : 실패
	*/
    uint8 conn_type = load_flash_conntype();

    if (conn_type == 0xFF) {
        //this is not stored connector type
        return 0;
    }

    if((conn_type & 0x07) > 0x04) {
        //잘못된 커넥터 타입
        return 0;
    }

    return 1;
}

static uint8 current_battery_status_check(Control_flag_t *apst_flag)
{
	/*
    check current condition of this battery pack
	return 1 : 모두 정상
	return 0 : 불량

	*/

	if (!check_cable_status()) {
		apst_flag->abnormal |= ERR_BROKEN_CABLE;
	}

    if (read_temperature() >= 700) {
        apst_flag->abnormal |= ERR_TEMP_OVER;
    }

    if (read_voltage(READ_BATT_SIDE) < MIN_BATT_V) {
        apst_flag->abnormal |= ERR_LOSS_BATTERY;
    }

	if(apst_flag->abnormal & 0xFF) {
		return 0;
	}
	return 1;
}

/***** API Level Function *****/

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

//Factory initialization state machine
// User service state machine

uint16 Battery_Monitoring_Process(uint8 task_id, uint16 events)
{ // task_17

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
}// uint16 Battery_Monitoring_Process(uint8 task_id, uint16 events)

uint16 Kiosk_Process(uint8 task_id, uint16 events)
{ // task_15
    float tmp_voltage;
    uint16 next_evt;
    uint8 next_task;

    next_task = task_id;
    next_evt = events;

    debug_vars = events;

    if (events & EVT_EXT_V_MONITORING) { // task_15_EVT_EXT_V_MONITORING
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
                        stroed_key_value(&st_LogAddr);

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
                case EXT_COMM_V:// 통신 전압이 유지되면, 키오스크 안에 있다고 판단
                    if (timer_cnt > 0) { //100이전에 통신 전압이 걸릴 경우
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
                    timer_cnt++; //약 10msec 간격으로 숫자가 매겨짐
                    
                    break;
            }
            sys_timer = osal_GetSystemClock();
        }

        if (timer_cnt >= 100) { //100이후(약1초) 까지도 전압이 안걸릴 경우
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

    if (next_evt != events) {
        timer_cnt = 0;
        sys_timer = osal_GetSystemClock();
    }
    osal_start_timerEx(next_task, next_evt, 10);

    return 0;
} //uint16 Kiosk_Process(uint8 task_id, uint16 events)

uint16 Abnormal_Process(uint8 task_id, uint16 events)
{ //task_16
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
} //uint16 Abnormal_Process(uint8 task_id, uint16 events)

uint8 chk_ext_volt_chg(uint16 * apu16Volt, uint8 au8Idx, uint8 au8Size)
{
	/*
	checking voltage for charging

	return value :
		MODE_COMM_REQ : communication request
		MODE_CHGED : charging voltage
		MODE_NO_VOLT : no voltage

	*/


}


static uint8 giExtVoltChged = 0;
static uint8 giExtVoltComm = 0;
static uint8 giExtVoltZero = 0;
static uint8 giChgingCnt;

#define CHRGED_LOG_PERIOD		10
#define MAX_UINT8				254	
#define MAX_EXTVOLTZERO_CNT		10
#define MID_EXTVOLTZERO_CNT		5
#define MAX_EXTVOLTCHGED_CNT	10
#define MAX_EXTVOLTCOMM_CNT		10

uint8 log_ext_volt()
{

	switch (ext_voltage_result()) {
		case EXT_MIN_V : // 충전 전압
			giExtVoltChged  ++;
			giExtVoltComm  = 0;
			giExtVoltZero  = 0;
			if (giExtVoltChged == MAX_UINT8) {
				giExtVoltChged --;
			}
			break;
		case EXT_COMM_V : // 통신 전압
			giExtVoltChged = 0;
			giExtVoltComm   ++;
			giExtVoltZero  = 0;
			if (giExtVoltComm == MAX_UINT8) {
				giExtVoltComm --;
			}
			break;
		case EXT_ZERO_V : // 전압 없음
			giExtVoltChged = 0;
			giExtVoltComm  = 0;
			giExtVoltZero   ++;
			if (giExtVoltZero == MAX_UINT8) {
				giExtVoltZero --;
			}
			break;
	}
	return giExtVoltChged + giExtVoltComm + giExtVoltZero;
}

void reset_log_ext_volt()
{
	giExtVoltChged = 0;
	giExtVoltComm  = 0;
	giExtVoltZero  = 0;
	return;
}

uint8 chk_ext_volt_zero()
{ /*
	키오스크 안에 있으면서 외부 전압이 0으로 검출될때,
	comm req혹은 out_kiosk
  */
	if (giExtVoltZero > MID_EXTVOLTZERO_CNT) {
		return 1;
	}
	return 0;
}

uint8 chk_out_kiosk()
{ //외부전압이 지속적으로 0이어서 키오스크 밖인지 확인
	if (giExtVoltZero > MAX_EXTVOLTZERO_CNT) {
		return 1;
	}
	return 0;
}

uint8 chk_ext_volt_comm_req()
{ // 외부전압이 통신전압인지 확인
	if (giExtVoltComm > MAX_EXTVOLTCOMM_CNT) {
		return 1;
	}
	return 0;
}

uint8 chk_ext_volt_chg()
{
	// 외부전압이 충전전압인지 확인
	if (giExtVoltChged > MAX_EXTVOLTCHGED_CNT) {
		return 1;
	}
	return 0;
}

uint8 chk_blz_conn()
{
	return 0;
}

uint8 chk_chargeable()
{
	return 0;
}

uint8 chk_charging()
{
	return 0;
}

uint8 chk_discharging()
{
	return 0;
}

uint8 chk_in_kiosk()
{
	return 0;
}

uint8 chk_low_voltage()
{
	return 0;
}

void do_disable_blz_conn()
{
	return;
}

void do_disable_usb_a()
{
	return;
}

void do_enable_blz_conn()
{
	return;
}

void do_enable_usb_a()
{
	return;
}

uint8 ext_voltage_result()
{
	return;
}

void save_charging_log()
{
	return;
}

void save_discharging_log()
{
	return;
}

uint8 send_to_chg_comm(uint16 au16Events)
{
	return 0;
}

void start_charging()
{
	charge_enable();
	return;
}

void stop_charging()
{
	charge_disable();
	return;
}

void BlzBat_Init(uint8 task_id)
{
    //OSAL service init의 Application중 가장 처음 불리는 초기화 함수
    setup_pin();
    adc_init();

    uart_init(NULL);
    

    main_taskID = task_id; // task_12

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
    //ble_advert_control(FALSE);
    ble_advert_control(TRUE);
    
	osal_set_event(task_id, STATE_BOOT);
} // void BlzBat_Init(uint8 task_id)

uint16 BlzBat_ProcessEvent(uint8 task_id, uint16 events)
{
	print_uart("main process event\r\n");
    VOID task_id;  // OSAL required parameter that isn't used in this function

	uint16 next_state = events;
	uint16 next_state_dly = 0;

	switch(events) {
		case STATE_BOOT :
			next_state = STATE_IN_KIOSK;
			break;

		case STATE_IN_KIOSK :
		case STATE_IN_KIOSK_CHRGED :
			log_ext_volt();
			next_state_dly = 100; //0.1초마다 외부 전압 측정
			if (chk_ext_volt_zero()) { // 키오스크에 있으면서 외부 전압이 0으로 검출
				next_state = STATE_IN_KIOSK_EXT_VOLT_ZERO;
				//next_state_dly = 100; // 0.1초마다 
			}
			else if (chk_blz_conn()) { // 키오스크에 있으면서, 대여안된 상태에서 빌리지 코넥터가 빠짐
				// 나중에 콜백 함수로 바꿔서, 한 번이라도 코넥터가 빠지면 검출 되도록..
			}
			else if (chk_ext_volt_chg()) {
				// 충전 전압 상태
				reset_log_ext_volt();
				giChgingCnt = 0;
				next_state = STATE_IN_KIOSK_CHGING;
			}
			else {
			}
			break;

		case STATE_IN_KIOSK_EXT_VOLT_ZERO :
			if (log_ext_volt() < MID_EXTVOLTZERO_CNT) {
				// 외부전압이 충전전압 혹은 통신전압으로 변화
				switch (ext_voltage_result()) {
					case EXT_MIN_V : // 충전 전압, 허용되지 않은 상태천이.
						reset_log_ext_volt();
						giChgingCnt = 0;
						next_state = STATE_IN_KIOSK_CHGING;
						break;
					case EXT_COMM_V : // 통신 전압
						reset_log_ext_volt();
						next_state = STATE_IN_KIOSK_COMM_CHGING_STATUS;
						break;
					default :
						break;
				}
			}
			else {
				if (chk_out_kiosk()) { //외부전압이 지속적으로 0이어서,
										//키오스크에서 빠져나왔다고 판단.
					do_enable_usb_a();
					do_enable_blz_conn();
					reset_log_ext_volt();
					next_state = STATE_IN_KIOSK_EXT_VOLT_ZERO;
				}
			}
			next_state_dly = 100; // 0.1초마다 
			break;

		case STATE_IN_KIOSK_COMM :
			break;

		case STATE_IN_KIOSK_CHRGED_CMPLT : //충전완료
			next_state = STATE_IN_KIOSK_COMM_CHGING_LOG;
			break;

		case STATE_IN_KIOSK_CHGING :
			log_ext_volt();
			if (chk_ext_volt_chg()) {
				start_charging();
				next_state_dly = 1000; // 1초마다 
				if (++giChgingCnt == CHRGED_LOG_PERIOD) {
					// CHRGED_LOG_PERIOD 초마다 기록
					giChgingCnt = 0;
					save_charging_log();
				}
				if (!chk_charging()) {
					//충전 종료
					stop_charging();
					next_state = STATE_IN_KIOSK_COMM_CHGING_LOG;
				}
			}
			else if (chk_ext_volt_zero()) {
				stop_charging();
				next_state_dly = 100; // 0.1초마다 
				next_state = STATE_IN_KIOSK_EXT_VOLT_ZERO;
			}
			break;

		case STATE_IN_KIOSK_COMM_DISCHGING_LOG :
		case STATE_IN_KIOSK_COMM_CHGING_LOG :
		case STATE_IN_KIOSK_COMM_CHGING_STATUS :
			if(send_to_chg_comm(events)) {

			}
			else {
				 // 충전 상태(charging status)전송 완료
				// 충전(charging)로그 전송 완료
				//사용자 상태에서 기록된, 방전(discharging,핸드폰 충전)로그 전송 완료
				next_state = STATE_IN_KIOSK;
			}
			break;

		case STATE_OUT_KIOSK :
			do_enable_blz_conn();
			do_enable_usb_a();
			if (chk_in_kiosk()) { // 키오스크에 삽입
				do_disable_blz_conn();
				do_disable_usb_a();
				next_state = STATE_IN_KIOSK_COMM_DISCHGING_LOG;
			}
			else if (chk_discharging()) { // 방전 전류가 확인되면,
				if (chk_blz_conn()) { //빌리지 코넥터가 뽑혔으면,
					do_disable_usb_a(); // USB_A 코넥터를 비활성화
					next_state = STATE_OUT_KIOSK_DISCHGING_BLZ_CONN;
				}
				else { //빌리지 코넥터가 안 뽑혔으면,
					do_disable_blz_conn(); //빌리지 코넥터를 비활성화
					next_state = STATE_OUT_KIOSK_DISCHGING_USB_A;
				}
				//next_state_dly = 0;
			}
			else {// 키오스크에 들어가지 않고, 방전 전류가 확인 안됨, 방전 안하고 있음
				if (chk_low_voltage()) { //낮은 배터리 셀 전압이면,
					do_disable_blz_conn();
					do_disable_usb_a();
					next_state = STATE_OUT_KIOSK_SLEEP;
				}
				else {
					//사용자 손에서, 방전 대기 중
					next_state_dly = 1000; // 1초마다 
				}
			}

			break;

		case STATE_OUT_KIOSK_DISCHGING_USB_A : //USB_A로 방전 상태
			if (chk_blz_conn()) { // USB_A 코넥터 사용중에 빌리지 코넥터가 뽑히면,
				do_disable_usb_a(); // USB_A 코넥터를 비활성화
				do_enable_blz_conn(); // 빌리지 코넥터 활성화
				next_state = STATE_OUT_KIOSK_DISCHGING_BLZ_CONN;
				next_state_dly = 500; // 0.5초후
			} 
			else if (!chk_chargeable()) { // 방전 가능한 상태가 아니면,
				do_disable_usb_a(); // USB_A 코넥터를 비활성화
				next_state = STATE_OUT_KIOSK;
				next_state_dly = 1000; // 1초 후
			}
			else if (!chk_discharging()) { // 방전 상태가 아니면
				do_enable_blz_conn();
				next_state = STATE_OUT_KIOSK;
			}
			else { // 계속 방전 중
				save_discharging_log();
				next_state_dly = 1000; // 1초 후
			}
			break;

		case STATE_OUT_KIOSK_DISCHGING_BLZ_CONN : //빌리지 코넥터로 방전 상태
			if (!chk_blz_conn()) { // 빌리지 코넥터 사용중에 빌리지 코넥터가 들어가면,
				do_enable_usb_a(); // USB_A 코넥터를 활성화
				next_state = STATE_OUT_KIOSK;
				next_state_dly = 500; // 0.5초후
			}
			else if (!chk_chargeable()) { // 방전 가능한 상태가 아니면,
				do_disable_blz_conn(); // 빌리지 코넥터를 비활성화
				next_state = STATE_OUT_KIOSK;
				next_state_dly = 1000; // 1초 후
			}
			else if (!chk_discharging()) { // 방전 상태가 아니면
				do_enable_usb_a();
				next_state = STATE_OUT_KIOSK;
			}
			else { // 계속 방전 중
				save_discharging_log();
				next_state_dly = 1000; // 1초 후
			}
			break;

		case STATE_OUT_KIOSK_SLEEP :
			break;
		case STATE_OUT_KIOSK_DEEP_SLEEP :
			break;
		case STATE_OUT_KIOSK_POWEROFF :
			break;
	} //switch(events)

	if (next_state_dly) {
		osal_start_timerEx(main_taskID, next_state, next_state_dly);
	}
	else {
		osal_set_event(main_taskID, next_state);
	}
    return 0;  //return events process clear task free
}





/*
charge_disable();
charge_enable();
check_timer(sys_timer, 50);
ext_voltage_analysis(tmp_voltage)
get_head_packet(&ctrl_flags, &batt_status, st_LogAddr.log_cnt);
get_log_packet(&st_LogAddr);
osal_GetSystemClock();
osal_mem_free(tx_buff);
osal_start_timerEx(next_task, next_evt, 10);
read_voltage(READ_EXT);
read_voltage_sampling(10, READ_EXT);
setup_calib_value(TRUE, search_self_calib());
stroed_key_value(&st_LogAddr);
transmit_data_stream(size, tx_buff);
uart_disable();
uart_enable();
update_self_calibration(ctrl_flags.self_calib, read_adc_sampling(10, READ_BATT_SIDE));
read_temperature();
*/
