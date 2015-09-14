/*
 * stopwatch.c
 *
 *  Created on: Sep 13, 2015
 *      Author: sws52
 */

#include "include/stopwatch.h"





// Free RTOS task for testing stopwatch operation
void vTaskTestStopwatch(void)
{
	stopwatch_t stopwatch;
	unsigned long time;

	for(;;)
	{
		stopwatch_start(&stopwatch);
		vTaskDelay(300 / portTICK_RATE_MS);
		stopwatch_stop(&stopwatch);
//		vTaskDelay(300);
		time = stopwatch_get_time_ms(&stopwatch);

		// Test reliable tick level operation
		int i;
//		for (i = 0; i < 100; i++)
//		{
//			stopwatch_start(&stopwatch);
//			vTaskDelay(100 / portTICK_RATE_MS);
//			stopwatch_stop(&stopwatch);
//			time = stopwatch_get_time_ms(&stopwatch);
//			if (time != 100)
//				while(1){}
//		}

		// TODO: test reliable subtick operation
		for (i = 0; i < 100; i++)
		{
			int j;
			stopwatch_start(&stopwatch);
			for (j = 0; j < 100; j++)
				continue;
			stopwatch_stop(&stopwatch);
			time = stopwatch_get_subticks(&stopwatch);
		}
	}
}

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


unsigned long stopwatch_get_subticks(stopwatch_t* stopwatch)
{
	unsigned long subticks;
	if (stopwatch->stop_ticks == stopwatch->start_ticks)
		subticks = stopwatch->start_subticks - stopwatch->stop_subticks;
	else
	{
		unsigned long systick_period = SysTickPeriodGet();
		subticks = stopwatch->start_subticks + (systick_period-stopwatch->stop_subticks);
	}
	return subticks;
}

unsigned long stopwatch_get_time_ms(stopwatch_t* stopwatch)
{
	return (stopwatch->stop_ticks-stopwatch->start_ticks)*portTICK_RATE_MS;
}

//WARNING: uses a devide operation, do not call from a high frequency ISR!
//TODO: Fix for data dependancies, currently the airspeed ISR is pretty well controlled, so not immediately an issue
//TODO: possible to improve accuracy?  good to +/- 2 us
//TODO: impliment rollover case
unsigned long stopwatch_get_time_us(stopwatch_t* stopwatch)
{
//	static unsigned long subtick_period;


	// Subtract one because there is almost certainly some subticks contrubuting to the time
	unsigned long ticks_us = (stopwatch->stop_ticks-stopwatch->start_ticks-1)*portTICK_RATE_MS*1000;
	unsigned long subticks_us = stopwatch_get_subticks(stopwatch)/configCPU_CLOCK_HZ*1000000;

	return ticks_us + subticks_us;
}
