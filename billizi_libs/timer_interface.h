#ifndef __TIMER_INTERFACE__
#define __TIMER_INTERFACE__

#include "bcomdef.h"
#include "OSAL.h"
#include "OSAL_PwrMgr.h"

#include "OnBoard.h"
#include "hal_timer.h"

#include "hci.h"
#include "gap.h"

#include "gpio_interface.h"

void delay_us(uint16 microSecs);
uint8 check_timer(uint32 std_time, uint16 flow_time);
#endif