#ifndef __MAIN_TASK__
#define __MAIN_TASK__

/*** ti cc254x api includes ***/

#include "bcomdef.h"
#include "OSAL.h"
#include "OSAL_PwrMgr.h"

#include "OnBoard.h"
#include "hal_adc.h"
#include "hal_uart.h"
#include "gatt.h"

#include "hci.h"

#include "gapgattserver.h"
#include "gattservapp.h"
#include "simpleGATTprofile_Bridge.h"

#include "peripheral.h"

#include "gapbondmgr.h"

/* user, kiosk common event */
#define EVT_EXT_V_MONITORING    0x0080

/* billizi kiosk events */
#define EVT_COMM                0x0100
#define EVT_BATT_INFO_REQ       0x0200
#define EVT_CHARGE              0x0400
#define EVT_HOLD_BATT           0x0800 //키오스크 안에서 배터리가 충전이 아닌 상태

#define DBG_EVT_A                0x1000

#define PARAM_LOGADDR       0x01
#define PARAM_LOGDATA       0x02
#define PARAM_CTRL_FLAG     0x03
#define PARAM_EVT_VALS      0x04

typedef enum _TASK_LOCATION {
	/*
	이벤트
	스테이트 머신 입력 이벤트
0.전원 켜짐

1.외부 전원 전압
1.1 0V ~ 8V
1.2 8V ~ 20V
1.3 20V ~ 24V 

2.배터리 셀 전압
2.1 3V이하
2.2 3V ~ 4.2V
2.3 4.2V 이상

3.충전 전류
4.방전 전류
5.빌리지 코넥터 착탈 위치
6.온도 센서
7.충격 센서
8.빌리지 케이블 단선(방전출력전압 상태 확인 가능)
9. BLE 통신에 의한 코맨드


출력
빌리지 코넥터로 출력 제어
USB-A 코넥터로 출력 제어
셀충전 제어




	*/
    STATE_BOOT,//
	/*

	BlzBat_Init()에서 처리 가능
	//power on reset인지 확인 전원이 켜지고 
	//펌웨어 다운로드 직후인지
	//마지막 스테이트가 POWEROFF인지
	//위 두 조건이 아니면 오류에 의한 재부팅

	//이벤트1.1이면 , STATE_OUT_KIOSK
	//이벤트1.2,1.3이면 , STATE_IN_KIOSK
	*/

	STATE_IN_KIOSK, // 배터리가 키오스크 안에 있는 상태
	STATE_IN_KIOSK_EXT_VOLT_ZERO, // 배터리가 키오스크 안에서 외부전압이 0인 상태
	STATE_IN_KIOSK_COMM, // 충전선 통신
	STATE_IN_KIOSK_CHRGED, //(charged) 셀이 충전된 상태
	STATE_IN_KIOSK_CHGING, //(charging) 셀 충전 상태
	STATE_IN_KIOSK_COMM_CHGING_STATUS, // 셀 충전 로그 통신
	STATE_IN_KIOSK_COMM_CHGING_LOG, // 셀 충전 로그 통신
	STATE_IN_KIOSK_COMM_DISCHGING_LOG, // 방전 로그(스마트폰 충전 로그) 데이타 통신

	STATE_OUT_KIOSK, // 배터리가 키오스크 밖에 있는 상태
	STATE_OUT_KIOSK_DISCHGING_USB_A, //(discharging) 스마트폰을 방전 상태, USB-A코넥터로 방전 상태
	STATE_OUT_KIOSK_DISCHGING_BLZ_CONN, //(discharging) 스마트폰을 방전 상태, 빌리지 코넥터로 방전 상태
	STATE_OUT_KIOSK_SLEEP, // 
	STATE_OUT_KIOSK_DEEP_SLEEP,  // 
	STATE_OUT_KIOSK_POWEROFF,  // 배터리 셀 보호를 위해서 전원끔


    TASK_FACTORY_INIT,
    TASK_USER_SERVICE,
	TASK_US_CERTIFICATION,
	TASK_US_PRPRING_CHGING, // preparing discharging, 스마트폰 충전 준비 스테이트
	TASK_US_CHGING, // discharging, 방전(스마트폰을 출전) 중.
	TASK_US_END,
    TASK_KIOSK,
    TASK_KIOSK_EXT_V_CHK, // 외부 전압 확인 스테이트
    TASK_KIOSK_COMM, // 충전선 통신 상태
    TASK_KIOSK_CHG,  // 충전 되는 상태
    TASK_KIOSK_HOLD, // 충전선 통신과 충전 상태가 아닌
	 				 //  그냥 키오스크에 장착된 상태
    TASK_ABNORMAL,
    TASK_MORNITORING,
	TASK_US_, // User Service
    DEBUG
} state_task_t;

typedef enum _USER_SERVICE_EVT {
    CERTIFICATION = 1,
    BATT_PRE_DISCHG,
    BATT_DISCHG,
    SERVICE_END
} Evt_Serv_t;

/*********************************************************************
 * MACROS
 */
#define FIRMWARE_VERSION    0x01

/* Local Function Group */

static void peripheralStateNotificationCB(gaprole_States_t newState);
static void simpleProfileChangeCB(uint8 paramID);


uint16 get_main_taskID();
void get_main_params(uint8 opt, void *pValue);
void set_main_params(uint8 opt, uint8 size, void *pValue);

/** Task Initialization for the BLE Application **/
extern void BlzBat_Init(uint8 task_id);
extern void Billizi_BootMgr_Init(uint8 task_id);
extern void Billizi_Process_Init(uint8 task_id);
extern void StateMachine_Process_init(uint8 task_id);

/** Task Event Processor for the BLE Application **/

extern uint16 BlzBat_ProcessEvent(uint8 task_id, uint16 events);
extern uint16 Billizi_BootMgr_ProcessEvent(uint8 task_id, uint16 events);
extern uint16 Billizi_Main_ProcessEvent(uint8 task_id, uint16 events);
extern uint16 Factory_Init_Process(uint8 task_id, uint16 events);
extern uint16 User_Service_Process(uint8 task_id, uint16 events);
extern uint16 Kiosk_Process(uint8 task_id, uint16 events);
extern uint16 Abnormal_Process(uint8 task_id, uint16 events);
extern uint16 Battery_Monitoring_Process(uint8 task_id, uint16 events);

#endif
