#include "timer_interface.h"

void delay_us(uint16 microSecs)
{
	for(int i=0;i<microSecs;i++){
		/* 32 NOPs == 1 usecs */
		asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop");
	}
}

uint8 check_timer(uint32 std_time, uint16 flow_time)
{
	if(osal_GetSystemClock() < std_time){
		if((osal_GetSystemClock()+65536)-std_time>flow_time){
			return 1;
		}
	}else if((osal_GetSystemClock()-std_time)>flow_time){
		return 1;
	}

	return 0;
}
