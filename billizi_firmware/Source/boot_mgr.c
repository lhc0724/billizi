#include "main_task.h"
#include "boot_mgr.h"
#include "log_mgr.h"

#if defined FEATURE_OAD
  #include "oad.h"
  #include "oad_target.h"
#endif

static uint8 check_first_boot();

void Billizi_BootMgr_Init(uint8 task_id)
{
    //OSAL service init의 Application중 가장 처음 불리는 초기화 함수
    setup_pin();
    adc_init();

    uart_init(NULL);
    if(check_first_boot()) {
        osal_set_event(task_id, BOOT_INIT);
    }
    osal_set_event(task_id, BOOT_PROCESS); 
}

static uint8 check_adc_calibration(Control_flag_t *apst_flags) 
{
    flash_16bit_t tmp_flash;
    uint16 calib_value;

    read_flash(FLADDR_CALIB_REF, FLOPT_UINT32, &tmp_flash.all_bits);
    if(tmp_flash.all_bits == EMPTY_FLASH) {
        //calibration value has never been set.
        //need factory initialize
        return 1;
    }

    if(tmp_flash.all_bits == 0) {
        //this battery use self-calibration.
        calib_value = search_self_calib();
    } else if (tmp_flash.high_16bit == 0x1000) {
        //adc system setup reference calibration value
        setup_calib_value(FALSE, tmp_flash.low_16bit);
        apst_flags->ref_calib = 1;
        return 0;
    }

    if (!calib_value) {
        //not found ADC Calibration value.
        //need factory initialize
        return 1;
    }else {
        //adc system setup self-calibartion value
        setup_calib_value(TRUE, calib_value);
        apst_flags->self_calib = 1;
    }

    return 0;
}

static uint8 check_billizi_conntype()
{
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
{
    Control_flag_t ctrl_flags;
    uint32 tmp_data;
    eBOOT_t boot_evt = (eBOOT_t)events;

    ctrl_flags.flag_all = 0;

    switch(boot_evt) {
        case BOOT_INIT:
            //첫부팅시 부팅에 영향을 주는 flash 저장 값 초기화
            HalFlashErase(ADDR_2_PAGE(FLADDR_MIN));
            HalFlashErase(ADDR_2_PAGE(FLADDR_LOGKEY_ST));

            erase_flash_log_area();

            //image 정보값 저장
            tmp_data = IMG_INFO;
            write_flash(FLADDR_MIN, &tmp_data);

            osal_set_event(task_id, BOOT_PROCESS);
            break;
        case BOOT_PROCESS:
            break;
    }

    if(events & BOOT_INIT) {
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
            ctrl_flags.factory_setup = 1;
            ctrl_flags.self_calib = 0;
            ctrl_flags.ref_calib = 0;
        }
        
        //현재 배터리 커넥터 정보 확인
        if(check_billizi_conntype()) {
            print_uart("ERR-CONN\r\n");
            ctrl_flags.factory_setup = 1;
        }

        //위 검사에서 통과 못한항목 존재시 공장초기화
        if(ctrl_flags.factory_setup) {
            print_uart("START FACTORY INIT\r\n");
            osal_set_event(get_main_taskID(), TASK_FACTORY_INIT);
            set_main_params(PARAM_CTRL_FLAG, sizeof(ctrl_flags), &ctrl_flags);
            return 0;
        }

#if 1
        //배터리 동작 전 상태확인
        current_battery_status_check(&ctrl_flags);
        //print_uart("FLAGS-%#04X\r\n", ctrl_flags.flag_all);
        if(ctrl_flags.abnormal & 0xFF) {
            osal_set_event(get_main_taskID(), TASK_ABNORMAL);
        }else {
            osal_set_event(get_main_taskID(), TASK_USER_SERVICE);
        }
#else
        //KC인증용 펌웨어
        ctrl_flags.abnormal = 0;
        osal_set_event(get_main_taskID(), EVT_USER_SERVICE);
#endif
        set_main_params(PARAM_CTRL_FLAG, sizeof(ctrl_flags), &ctrl_flags);

        return 0;

    }

    return 0; //task free
}

static uint8 check_first_boot() 
{
    //배터리의 부팅이 처음인지 검사하는 함수
    //사용가능 플래시의 첫번째 값을 읽어 oad image값과 맞는지 검사

    uint32 img_value;
    uint8 res = 1;      //결과값 1일경우 첫 부팅으로 취급

    read_flash(FLADDR_MIN, FLOPT_UINT32, &img_value);
    switch (get_oad_img_info()) {
        case 0x41:  //img 'A'
        case 0x42:  //img 'B'
            if (img_value == IMG_INFO) {
                res = 0;
            }
            break;
    }

    return res;
}
