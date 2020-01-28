#ifndef __FLASH_INTERFACE__
#define __FLASH_INTERFACE__

/* includes */
#include "bcomdef.h"
#include "OSAL.h"
#include "OSAL_PwrMgr.h"

#include "OnBoard.h"

#include "hci.h"
#include "gap.h"

#include "hal_flash.h"

/* Flash memory address values 
 * can use the available memory range is 0x8000 ~ 0xffff
 * flash can be erased by one page.
 * the size of a page is 512 block and one block is 4 byte.(512 * 4 = 2048 byte)
 * 0xN000 ~ 0xN1FF is a single page range.
 */
#define PG_END_OFFSET   0x01FF
#define EMPTY_FLASH     0xFFFFFFFF

#define ADDR_2_PAGE(address)    (address >> 9)
#define PAGE_2_ADDR(page)       (uint16)(page << 9)
#define ADDR_2_RECORD(address)  (address&PG_END_OFFSET)

/* flash read buffer type transfer */
#define _32BIT_2_16BIT(__VA__) (uint16*)(&__VA__)
#define _16BIT_2_32BIT(__VA__) *(uint32*)__VA__

/**
 * @common:
 * FLADDR_MIN: 사용가능한 메모리 영역의 최소 주소값
 * FLADDR_LOGKEY: 마지막 로그 기록 위치를 저장하는 메모리영역
 * FLADDR_LOGDATA: 로그 데이터가 저장되는 메모리 영역
 *      
 * Image A:
 * The available memory area is from 0x1000(8 page) to 0x8BFF(69 Page).
 * Image B:
 * The available memory area is from 0x8E00(71 page) to 0xF5FF(122 Page).
 */

#ifdef HAL_IMAGE_A
#define FLADDR_MIN             0x1000    //8 Page
#define FLADDR_CALIB_REF       0x1002
#define FLADDR_CALIB_SELF_ST   0x1003
#define FLADDR_CALIB_SELF_ED   0x1008
#define FLADDR_CONNTYPE        0x1009

#define FLADDR_LOGKEY_ST    0x1010
#define FLADDR_LOGKEY_ED    0x11FE

#define FLADDR_LOGDATA_ST   0x1200  //9 Page
#define FLADDR_LOGDATA_ED   0x8BFE  //69 Page
#else 
#define FLADDR_MIN        0x8E00    //71 Page
#define FLADDR_CALIB      0x8E03
#define FLADDR_CONNTYPE   0x8E04

#define FLADDR_LOGKEY_ST    0x8E05
#define FLADDR_LOGKEY_ED    0x8FFE

#define FLADDR_LOGDATA_ST   0x9000  //72Page
#define FLADDR_LOGDATA_ED   0xF5FE  //122 Page
#endif

typedef enum FLASH_VARIABLE_OPT {
    FLOPT_UINT8,
    FLOPT_UINT16,
    FLOPT_UINT32,
    FLOPT_FLOAT
}eFlash_Var_t;

typedef enum _CONNECTOR_TYPE {
    CONN_MICRO_5  = 0x01,
    CONN_LIGHTING = 0x02,
    CONN_USB_C    = 0x04
} eConnType_t;

typedef union _FLASH_DATA {
    union _EVENTS {
        struct {
            uint8 state : 8;         //8 bit
            uint16 log_value : 16;   //24 bit

            //log type flag
            uint8 head_flag : 1;
            uint8 key_flag : 1;
            uint8 clc_flag : 1;     //clc: charge line communication

            //log data type
            uint8 impact : 1;
            uint8 cable_brk : 1;    //32 bit
            uint8 voltage : 1;
            uint8 temp : 1;         
            uint8 discharge : 1;
        };
        uint32 event_datas;
    } evt_info;  //total 32bit

    union _TIMES {
        struct {
            uint8 time_header : 8;
            uint32 time_stamp : 24;
        };
        uint32 time_datas;
    } time_info;
    uint32 source_data;
}log_data_t;

typedef union _FLASH_TYPE16 {
    struct {
        uint16 high_16bit : 16;
        uint16 low_16bit : 16;
    };
    uint32 all_bits;
}flash_16bit_t;

typedef union _FLASH_TYPE8 {
    struct {
        uint8 byte_1 : 8;
        uint8 byte_2 : 8;
        uint8 byte_3 : 8;
        uint8 byte_4 : 8;
    };
    uint32 all_bits;
}flash_8bit_t;

/************************************
 * KEY는 키 로그의 주소를 나타냄.
 * 키 로그는 가장 마지막에 기록되며, 서비스 로그가 아닌 컨트롤을 위한 로그.
 * KIOSK와 통신을 한 단위로 로그를 묶으며 마지막 로그는 이번 로그묶음의 첫 주소를 가지고있음.
 */
typedef struct _FLASH_ADDR {
    uint16 key_addr;

    uint16 head_addr;    //log start point address
    uint16 tail_addr;    //log end point address
    uint16 offset_addr;  //log current offset

    uint16 log_cnt;
}log_addr_t;

uint8 write_flash(uint16 ai_addr, void *p_value);
void read_flash(uint16 ai_addr, eFlash_Var_t value_type, void *p_value);

float load_flash_calib();

uint8 stored_conn_type(eConnType_t ai_connType);
uint16 stored_adc_calib(uint16 calib_ref);
uint8 load_flash_conntype();

void erase_flash_range(uint16 st_addr, uint16 end_addr);
void erase_flash_log_area();

#endif