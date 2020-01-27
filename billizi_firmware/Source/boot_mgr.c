#include "main_task.h"
#include "boot_mgr.h"
#include "log_mgr.h"

void Billizi_BootMgr_Init(uint8 task_id)
{
    //OSAL service init의 Application중 가장 처음 불리는 초기화 함수
    setup_pin();
    adc_init();

    uart_init(NULL);
    
    osal_set_event(task_id, BOOT); 
}

static uint8 check_adc_calibration() 
{
    flash_16bit_t tmp_flash;

    read_flash(FLADDR_CALIB_REF, FLOPT_UINT32, &tmp_flash.all_bits);
    if(tmp_flash.all_bits == EMPTY_FLASH) {
        //calibration value has never been set.
        //need factory initialize
        return 1;
    }

    if(tmp_flash.all_bits == 0) {
        //this battery use self-calibration.
        tmp_flash = search_self_calib();
    } else if (tmp_flash.high_16bit == 0x1000) {
        //adc system setup reference calibration value
        setup_calib_value(FALSE, tmp_flash.low_16bit);
        return 0;
    }

    if (tmp_flash.all_bits == EMPTY_FLASH) {
        //not found ADC Calibration value.
        //need factory initialize
        return 1;
    }else {
        //adc system setup self-calibartion value
        if(tmp_flash.high_16bit != 0xFFFF) {
            setup_calib_value(TRUE, tmp_flash.high_16bit);
        }else {
            setup_calib_value(TRUE, tmp_flash.low_16bit);
        }
    }

    return 0;
}

static uint8 check_billizi_conntype()
{
    flash_8bit_t tmp_flash;
    uint8 conn_type = load_flash_conntype();


    if (conn_type == 0xFF) {
        //this is not stored connector type
        return 1;
    }

    if(!(tmp_flash.byte_1 & 0x03)) {
        return 1;
    }
    
    return 0;
}

/************************************************
 * @fn      recent_batt_status_check
 * 
 * @brief   이번 부팅 이전 배터리 마지막 상태를 확인.
 *          문제 없으면 0 리턴 */
static Control_flag_t recent_batt_status_check()
{
    log_addr_t latest_logAddr;      
    log_data_t tmp_logdata;
    Control_flag_t ctrl_flag;
    
    latest_logAddr.key_addr = get_key_address();

    if(latest_logAddr.key_addr < FLADDR_LOGKEY_ST) {
        //key address all empty.
        //this battery starts service for the first time.
        ctrl_flag.flag_all = 0;
        return ctrl_flag;
    }

    //search to the key-log location.
    latest_logAddr.tail_addr = log_location_parser(latest_logAddr.key_addr);

    if (!latest_logAddr.tail_addr) {
        //couldn't find the key-log location.
        ctrl_flag.abnormal |= ERR_FLASH_MEMS;
    } else {
        //read key-log.
        read_flash(latest_logAddr.tail_addr, FLOPT_UINT32, &tmp_logdata.evt_info);

        //battery last status analysis.
        if (tmp_logdata.evt_info.clc_flag) {
            latest_logAddr.head_addr = tmp_logdata.evt_info.log_value;
            set_main_params(PARAM_LOGADDR, sizeof(log_addr_t), &latest_logAddr);
            ctrl_flag.need_comm = 1;
        }

        if (!tmp_logdata.evt_info.key_flag) { 
            //key-log, not match flag
            ctrl_flag.abnormal |= ERR_FLASH_MEMS;
        }

        if(tmp_logdata.evt_info.cable_brk) {
            ctrl_flag.abnormal |= ERR_BROKEN_CABLE;
        }
    }
    
    //check current condition.

    if (check_cable_status()) {
        ctrl_flag.abnormal |= ERR_BROKEN_CABLE;
    }

    if (read_temperature() >= 70) {
        ctrl_flag.abnormal |= ERR_TEMP_OVER;
    }

    if (read_voltage(READ_BATT_SIDE) < 2.5) {
        ctrl_flag.abnormal |= ERR_LOSS_BATTERY;
    }

    return ctrl_flag;
}

uint16 Billizi_BootMgr_ProcessEvent(uint8 task_id, uint16 events)
{
    //log_addr_t keylog_addr;
    //log_addr_t main_addr;
    Control_flag_t ctrl_flags;

    ctrl_flags.flag_all = 0;

    if(events & BOOT) {
        /**
         * TODO: 
         * - check factory init status
         * - check last power off status
         * - log, flash mems initialize
         */
        uart_enable();

        /* check  */
        if (check_adc_calibration()) {
            print_uart("ERR-CALIB\r\n");
            ctrl_flags.factory_setup = 1;
        }
        
        if(check_billizi_conntype()) {
            print_uart("ERR-CONN\r\n");
            ctrl_flags.factory_setup = 1;
        }
        
        if(ctrl_flags.factory_setup) {
            print_uart("START FACTORY INIT\r\n");
            osal_set_event(get_main_taskID(), EVT_FACTORY_INIT);
            set_main_params(PARAM_CTRL_FLAG, sizeof(ctrl_flags), &ctrl_flags);
            return 0;
        }

        ctrl_flags = recent_batt_status_check();
        if(ctrl_flags.abnormal & 0x1F) {
            osal_set_event(get_main_taskID(), EVT_ABNORMAL_TASK);
        }else {
            osal_set_event(get_main_taskID(), EVT_USER_SERVICE);
        }
        set_main_params(PARAM_CTRL_FLAG, sizeof(ctrl_flags), &ctrl_flags);

        return 0;
#if 0
        keylog_addr.key_addr = get_key_address();
        keylog_addr.tail_addr = log_location_parser(keylog_addr.key_addr);

        if(keylog_addr.tail_addr == 0 && keylog_addr.key_addr == FLADDR_LOGKEY_ST) {
            
            main_addr.key_addr = keylog_addr.key_addr;
            main_addr.head_addr = FLADDR_LOGDATA_ST;
            main_addr.offset_addr = main_addr.head_addr;
            main_addr.tail_addr = 0;

        }else {
            main_addr.head_addr = check_key_log(keylog_addr.tail_addr, &ctrl_flags);
            if(ctrl_flags.need_comm) {
                main_addr.tail_addr = keylog_addr.tail_addr;
            }
        }
#endif
    }

    return 0; //task free
}