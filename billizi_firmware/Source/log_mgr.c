#include "log_mgr.h"

/******************************************************************
 * @command - log manager 내의 변수들 기본 개념
 * key - 해당 소스의 key개념은 각 로그의 마지막 주소를 가리키는 변수를 말함.
 * key_address - key값을 저장한 플래시메모리의 주소.
 * 
 * head_log - 각 log의 시작이 되는 로그
 * tail_log - 각 log의 마지막이 되는 로그, log_value로 head_log의 위치를 가짐.
 * 
 * 메모리에 저장시 key값은 16bit로 나타내는 로그의 주소. 즉, 16bit로 저장됨.
 * 32bit 영역을 16bit로 나누어 상위/하위에 저장하여 메모리 활용도를 증가시킴
 * 쓰기는 하위16bit부터 쓰여짐.
 */

/**
 * @fn log_system_init
 * @brief 로그 변수와 주소를 포인터로 받아 각각 초기화 하는 함수
 * 
 * @param apst_log: 로그데이터에 해당하는 스트럭쳐 변수
 * @param apst_addr: log_data를 저장할 위치를 가지고있을 주소 스트럭쳐
 */
uint8 log_system_init(log_data_t *apst_log, log_addr_t *apst_addr) 
{
    //log data structure initialize
    apst_log->data_all = 0;

    //log address information structure initialize
    apst_addr->key_addr = 0;
    apst_addr->head_addr = 0;
    apst_addr->offset_addr = 0;
    apst_addr->tail_addr = 0;
    apst_addr->log_cnt = 0;

    return 0;
}

/**
 * @fn LogAddress_valid_check
 * @brief 변수로 받은 주소가 로그로 사용할 주소에 해당하는지 확인
 *        주소 값이 잘못되었다면 FALSE 반환
 * 
 * @param addr: 플래시의 주소 값
 */
uint8 LogAddress_valid_check(uint16 addr)
{
    if(addr < FLADDR_LOGDATA_ST || addr > FLADDR_LOGDATA_ED) {                                                                      
        return FALSE;
    }
    return TRUE;
}

/**
 * @fn      get_key_address
 * @brief   저장된 key로그들 중에서 가장 마지막에 저장된 key로그위치를 반환
 * 
 * @return  key_addr: 가장 최근의 키로그가 기록되어있는 플래시 주소
 *                   만약, 키로그가 없는 상태라면 키로그의 시작주소 반환
 */
uint16 get_key_address()
{
    uint16 key_addr = FLADDR_LOGKEY_ED;
    flash_16bit_t key_value;

    /* search last valid key location 
     * 마지막 key log 주소 -> 첫번째 key log 주소로 역순 탐색
     */
    while (key_addr >= FLADDR_LOGKEY_ST) {
        read_flash(key_addr, FLOPT_UINT32, &key_value.all_bits);

        //32bit의 플래시 값중 쓰여진 값이 있다면 해당위치에서 while 탈출
        if (key_value.all_bits != EMPTY_FLASH) {
            break;
        }
        key_addr--;
    }

    //key_addr 변수가 계속 줄어들어 while 탈출시 시작주소로 초기화
    if(key_addr < FLADDR_LOGKEY_ST) {
        key_addr = FLADDR_LOGKEY_ST;
    }

    return key_addr;
}

/**
 * @fn addr_available_check_16bit
 * @brief 해당 주소가 16bit를 쓸수 있는 유효주소인지 판별
 * 
 * @param addr: 사용할 플래시 주소
 * 
 * @see 사용법 참조 -> function: stored_key_addres
 * 
 * @todo return 값을 enum으로 변경하여 활용하기 편하게 변경하는게 좋아보임.
 */
uint8 addr_available_check_16bit(uint16 addr)
{
    flash_16bit_t flash_value;  //flash의 값을 저장할 변수

    read_flash(addr, FLOPT_UINT32, &flash_value.all_bits);

    /** 
     * 읽어들인 플래시값 32bit를 16bit로 나누어
     * 상위/하위 16bit 중 한곳이라도 비어있으면(쓰기가능한상태 = 1) 해당 주소는 사용가능
     */
    if (flash_value.low_16bit == 0xFFFF) {
        return 1;
    } else if (flash_value.high_16bit == 0xFFFF) {
        return 2;
    }

    //no available address.
    return 0;
}

/**
 * @fn stroed_key_value
 * 
 * @brief key_value를 유효한 key address에 저장하는 함수
 * 
 * @param apst_addr: 로그에 대한 주소값들을 가지고있는 스트럭쳐 변수
 */
void stroed_key_value(log_addr_t *apst_addr)
{
    uint8 address_use = 0;  //address가 유효한 주소인지 판단하는 값
    flash_16bit_t key_value;

    //apst_addr이 가지고있는 key_address 값이 0일경우 값 초기화
    if(apst_addr->key_addr == 0) {
        apst_addr->key_addr = FLADDR_LOGKEY_ST;
    }

    //현재 key_addr값이 유효한지 검사 및 유효값 탐색
    while(!address_use) {
        address_use = addr_available_check_16bit(apst_addr->key_addr);

        //address_use의 값이 0이라면 해당 주소는 사용할 수 없는 주소
        if(!address_use) {
            //key_address값을 바꾸어 유효한 주소 탐색
            apst_addr->key_addr = KEYADDR_VALIDATION(apst_addr->key_addr + 1);

            //마지막 key 주소를 초과하여 시작주소로 돌아온경우 해당 플레시의 페이지를 지워 초기화
            if(apst_addr->key_addr == FLADDR_LOGKEY_ST) {
                HalFlashErase(ADDR_2_PAGE(FLADDR_LOGKEY_ST));
            }
        }
    }

    //key_value 변수 초기화
    key_value.all_bits = EMPTY_FLASH;
    switch(address_use) {
        //addr_available_check_16bit의 함수를 통과하여 유효한 영역에 key에 해당하는 값 대입
        case 1:
            key_value.low_16bit = apst_addr->tail_addr;
            break;
        case 2:
            key_value.high_16bit = apst_addr->tail_addr;
            break;
    }

    write_flash(apst_addr->key_addr, &key_value.all_bits);
}

/**
 * @fn analysis_keylog
 * @brief 받은 key주소의 key값을 분석하여 반환
 *        key값은 tail_log의 주소
 * 
 * @return tail_log_address || error=0
 */
uint16 analysis_keylog(uint16 key_addr)
{
    flash_16bit_t key_value;

    read_flash(key_addr, FLOPT_UINT32, &key_value.all_bits);
    
    //해당 주소의 값이 쓰여있지 않은 상태면 0반환
    if(key_value.all_bits == EMPTY_FLASH) {
        return 0;
    }
    
    /**
     * 하위영역부터 쓰여지므로 상위16bit에 값이 있다면
     * 상위 16bit값이 최근 기록된 값으로 상위 16bit 리턴
     */
    if(key_value.high_16bit != 0xFFFF) {
        return key_value.high_16bit;
    }else {
        return key_value.low_16bit;
    }
}

/**
 * @fn analysis_tail_log 
 * @brief tail_log 의 주소를 받아 해당 로그가 tail로그인지
 *        확인 및 분석하여 head_log의 주소 반환
 * 
 * @return head_addr||error=0
 */
uint16 analysis_tail_log(uint16 tail_addr)
{
    log_data_t tmp_log;
    uint16 head_addr;

    read_flash(tail_addr, FLOPT_UINT32, &tmp_log);

    if(tmp_log.log_type != TYPE_TAIL_LOG) {
        //tail log 플레그가 아닐경우 error값으로 0 반환
        return 0;
    } else {
        //tail 로그는 log_value로 head_log의 주소를 가지고 있고 이를 반환
        head_addr = tmp_log.log_value;
    }

    return head_addr;
}

/**
 * @fn get_newly_addresses
 * @brief 가장 최근 기록된 로그의 주소들을 가져옴
 * 
 * @param apst_addr: 로그의 주소들을 받을 포인터 변수
 */
void get_newly_addresses(log_addr_t *apst_addr)
{
    apst_addr->key_addr = get_key_address();
    apst_addr->tail_addr = analysis_keylog(apst_addr->key_addr);
    apst_addr->head_addr = analysis_tail_log(apst_addr->tail_addr);
}

/**
 * @fn search_head_log
 * @brief tail로그가 없거나 손실되었을때
 *        일정 offset에서 부터 head로그를 찾아내는 함수
 * 
 * @return head_address||error=0
 */
uint16 search_head_log(uint16 offset)
{
    log_data_t tmp_log;
    uint16 tmp_addr;

    if(offset > FLADDR_LOGDATA_ST) {
        tmp_addr = offset - 1;
    }else {
        tmp_addr = FLADDR_LOGDATA_ED;
    }

    while (tmp_addr != offset) {
        read_flash(tmp_addr, FLOPT_UINT32, &tmp_log);

        //시간데이터로 저장된 로그는 필터링
        if (tmp_log.log_evt & 0x1F) {
            //head log인지 판별
            if(tmp_log.log_type == TYPE_HEAD_LOG) {
                break;
            }
        }else {
            tmp_addr--;
            if(tmp_addr < FLADDR_LOGDATA_ST) {
                tmp_addr = FLADDR_LOGDATA_ED;
            }
        }
    }
    
    if(tmp_addr == offset) {
        //not found head log address
        return 0;
    }

    return tmp_addr;
}

/**
 * @fn clac_number_of_LogDatas
 * @brief 현재 로그의 head주소와 tail주소를 이용하여 로그개수를 계산
 * 
 * @todo 버그가 있는 함수이므로 테스트 및 수정을 요구함
 */
uint16 calc_number_of_LogDatas(log_addr_t ast_addr)
{
    uint16 log_cnt = 0;

    if (ast_addr.head_addr > ast_addr.tail_addr) {
        //Log address after ring buffer rotation
        log_cnt = FLADDR_LOGDATA_ED - ast_addr.head_addr;
        log_cnt += ast_addr.tail_addr - FLADDR_LOGDATA_ST;
    } else if (ast_addr.tail_addr > ast_addr.head_addr) {
        log_cnt = ast_addr.tail_addr - ast_addr.head_addr;
    }

    if(log_cnt > 1) {
        return log_cnt/2;
    }

    return 0;
}

/**
 * @fn generate_new_log_address
 * @brief 새로 기록할 로그주소를 생성
 * 
 * @param apst_addr: log address struct 변수로, 기존 로그주소를 받아서 새 주소로 반환
 */
void generate_new_log_address(log_addr_t *apst_addr)
{
    uint32 flash_vals;

    //log counter value init
    apst_addr->log_cnt = 0;

    if (apst_addr->head_addr == 0) {
        //head log가 저장된 곳이 없을 때(첫 로그)
        apst_addr->head_addr = FLADDR_LOGDATA_ST;
    }else {
        //이전 로그에 의해 head log가 존재할 때.
        apst_addr->head_addr = LOGADDR_VALIDATION(apst_addr->tail_addr + 1);
    }

    //tail log address init.
    apst_addr->tail_addr = 0;
    //offset address init.
    apst_addr->offset_addr = apst_addr->head_addr;

    read_flash(apst_addr->head_addr, FLOPT_UINT32, &flash_vals);
    if(flash_vals != EMPTY_FLASH) {
        //새로 발급 받은 로그주소가 빈 공간이 아닐경우 해당 페이지 초기화
        init_flash_mems(apst_addr->head_addr);
    }

}

/**
 * @fn write_tail_log
 * @brief 로그 마지막에 기록될 tail로그를 쓰는 함수
 * 
 * @param ast_flag: tail로그에 기록될 현재 기기 상태정보가 담긴 플레그 집합
 * @param tail_log: 기록할 tail로그 변수
 * 
 * @return error=1||success=0
 */
uint8 wrtie_tail_log(log_addr_t *apst_addr, Control_flag_t ast_flag)
{
    log_data_t tail_log;

    tail_log.log_value = apst_addr->head_addr;
    tail_log.log_type = TYPE_TAIL_LOG;
    tail_log.clc_flag = 1;

    tail_log.log_evt = 0;

    if(ast_flag.abnormal) {
        tail_log.log_evt |= LOG_HEAD_ABNORMAL;
    }

    if(ast_flag.serv_en) {
        tail_log.log_evt |= LOG_HEAD_EN_SERV;
    }else { 
        tail_log.log_evt |= LOG_HEAD_NO_SERV;
    }

    if(write_flash(apst_addr->offset_addr, &tail_log.data_all)) {
        //write error
        return 1;
    }

    apst_addr->offset_addr++;
    
    return 0;
}

/**
 * @fn stored_log_data
 * @brief 현재 로그와 로그발생 시각을 플래시에 저장.
 * 
 * @return error=1||success=0
 */
uint8 stored_log_data(log_addr_t *apst_addr, log_data_t *apst_data, time_data_t *apst_times)
{
    uint32 tmp_flash;
    uint16 offset_addr;

    offset_addr = LOGADDR_VALIDATION(apst_addr->offset_addr + 2);

    //사용할 플레시의 페이지 넘김을 체크 
    if ((offset_addr & PG_END_OFFSET) <= 1) {
        //새 페이지 플레시 초기화
        HalFlashErase(ADDR_2_PAGE(offset_addr));
    }

    write_flash(apst_addr->offset_addr, &(apst_data->data_all));
    read_flash(apst_addr->offset_addr, FLOPT_UINT32, &tmp_flash);
    if (apst_data->data_all != tmp_flash) {
        //로그 기록 실패
        return 1;
    }

    apst_addr->offset_addr = LOGADDR_VALIDATION(apst_addr->offset_addr + 1);

    write_flash(apst_addr->offset_addr, &(apst_times->data_all));
    read_flash(apst_addr->offset_addr, FLOPT_UINT32, &tmp_flash);
    if(apst_times->data_all != tmp_flash) {
        //시간 기록 실패
        return 1;
    }

    apst_addr->offset_addr = LOGADDR_VALIDATION(apst_addr->offset_addr + 1);

    //log write success, log data init
    apst_data->data_all = 0;

    return 0;
}
