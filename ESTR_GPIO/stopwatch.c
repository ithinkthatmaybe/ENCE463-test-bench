/*
 * stopwatch.c
 *
 *  Created on: Sep 13, 2015
 *      Author: sws52
 */

#include "include/stopwatch.h"
#include "include/FreeRTOS.h"
#include "include/task.h"
#include "driverlib/systick.h"



// We expect the uut to produce delays in the range of
// ~10us to 300us from looking at the scope so the subtick
// value is usefull
void stopwatch_start(stopwatch_t* stopwatch)
{
	stopwatch->start_ticks = xTaskGetTickCount();
	stopwatch->start_subticks = SysTickValueGet();
}

void stopwatch_stop(stopwatch_t* stopwatch)
{
	stopwatch->stop_ticks = xTaskGetTickCount();
	stopwatch->stop_subticks = SysTickValueGet();
}

unsigned long stopwatch_get_time_ms(stopwatch_t* stopwatch)
{
	return (stopwatch->stop_ticks-stopwatch->start_ticks)*portTICK_RATE_MS;
}

//TODO: Fix for data dependancies, currently the airspeed ISR is pretty well controlled, so not immediately an issue
//TODO: improve accuracy? good to +/- 2 us
//TODO: impliment rollover case
unsigned long stopwatch_get_time_us(stopwatch_t* stopwatch)
{
//	static unsigned long subtick_period;
//	subtick_period = SysTickPeriodGet();

	unsigned long ticks_us = (stopwatch->stop_ticks-stopwatch->start_ticks)*portTICK_RATE_MS*1000;
	unsigned long subticks_us = 0;
	if (stopwatch->start_ticks == stopwatch->stop_ticks)
		subticks_us = (stopwatch->start_subticks - stopwatch->stop_subticks)*1E6/configCPU_CLOCK_HZ; //ticks count up, subticks count down
	else
		subticks_us = 0; //TODO: finish me

	if (ticks_us)
		while(1){}

	return ticks_us + subticks_us;
}
