#include "main_task.h"
#include "boot_mgr.h"
#include "log_mgr.h"

#if defined FEATURE_OAD
  #include "oad.h"
  #include "oad_target.h"
#endif

void Billizi_BootMgr_Init(uint8 task_id)
{
    //OSAL service init의 Application중 가장 처음 불리는 초기화 함수
    setup_pin();
    adc_init();

    uart_init(NULL);
    
    osal_set_event(task_id, BOOT);  // task_11 task_11_BOOT
}

static uint8 check_billizi_conntype()
{
	/*
	return value
		0 : already set the connector type
		1 : need to set the connector type
	*/
    uint8 conn_type = load_flash_conntype();

    if (conn_type == 0xFF) {
        //this is not stored connector type
        return 1;
    }

    if((conn_type & 0x07) > 0x04) {
        //잘못된 커넥터 타입
        return 1;
    }

    return 0;
}

static void current_battery_status_check(Control_flag_t *apst_flag)
{
    //check current condition.

    // if (!check_cable_status()) {
    //     print_uart("ERR-Broken\r\n");
    //     apst_flag->abnormal |= ERR_BROKEN_CABLE;
    // }

    if (read_temperature() >= 700) {
        apst_flag->abnormal |= ERR_TEMP_OVER;
    }

    if (read_voltage(READ_BATT_SIDE) < MIN_BATT_V) {
        apst_flag->abnormal |= ERR_LOSS_BATTERY;
    }
}

uint16 Billizi_BootMgr_ProcessEvent(uint8 task_id, uint16 events)
{ // task_11
    //log_addr_t keylog_addr;
    //log_addr_t main_addr;
    Control_flag_t ctrl_flags;

    ctrl_flags.flag_all = 0;

    if(get_OAD_ImgInfo()) {
    } else {
    }

    if(events & BOOT) { //task_11_BOOT
        /**
         * TODO: 
         * - check factory init status
         * - check last power off status
         * - log, flash mems initialize
         */
        uart_enable();

        /* check  */
        //adc 오차 보정값 확인
        if (check_adc_calibration(&ctrl_flags)) {
            print_uart("ERR-CALIB\r\n");
            ctrl_flags.just_after_fw_download = 1;
            ctrl_flags.self_calib = 0;
            ctrl_flags.ref_calib = 0;
        }
        
        //현재 배터리 커넥터 정보 확인
        if(check_billizi_conntype()) {
            print_uart("ERR-CONN\r\n");
            ctrl_flags.just_after_fw_download = 1;
        }

        //위 검사에서 통과 못한항목 존재시 공장초기화
        if(ctrl_flags.just_after_fw_download) {
            print_uart("START FACTORY INIT %d\r\n",get_main_taskID());
            osal_set_event(get_main_taskID(), TASK_FACTORY_INIT); // task_12 task_12_TASK_FACKTORY_INIT
            set_main_params(PARAM_CTRL_FLAG, sizeof(ctrl_flags), &ctrl_flags);
            return 0;
        }

#if 1
        //배터리 동작 전 상태확인
        current_battery_status_check(&ctrl_flags);
        //print_uart("FLAGS-%#04X\r\n", ctrl_flags.flag_all);
        if(ctrl_flags.abnormal & 0xFF) {
            osal_set_event(get_main_taskID(), TASK_ABNORMAL); // task_12 task_12_TASK_ABNORMAL
        }else {
            osal_set_event(get_main_taskID(), TASK_USER_SERVICE); // task_12 task_12_TASK_USER_SERVICE
        }
#else
        //KC인증용 펌웨어
        ctrl_flags.abnormal = 0;
        osal_set_event(get_main_taskID(), EVT_USER_SERVICE); // task_12
#endif
        set_main_params(PARAM_CTRL_FLAG, sizeof(ctrl_flags), &ctrl_flags);

        return 0;

    }

    return 0; //task free
}
