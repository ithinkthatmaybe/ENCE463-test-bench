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
	for(;;)
	{
		stopwatch_start(&stopwatch);
		vTaskDelay(300 / portTICK_RATE_MS);
		stopwatch_stop(&stopwatch);
//		vTaskDelay(300);
//		time = stopwatch_get_time_ms(&stopwatch);

		// Test reliable tick level operation
//		unsigned long time;
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
//			time = stopwatch_get_subticks(&stopwatch);
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
	stopwatch->state = STOPWATCH_STARTED;
}

void stopwatch_stop(stopwatch_t* stopwatch)
{
	stopwatch->stop_ticks = xTaskGetTickCount();
	stopwatch->stop_subticks = SysTickValueGet();
	stopwatch->state = STOPWATCH_STOPPED;
}

// Takes a stoped stopwatch and returns the number of subticks that
// occured during the operating interval
// Does not return the total stored time interval in subticks,
// ie if a number of ticks has occured this will not return a
// value representing the total time difference measured
// so care must be taken
unsigned long stopwatch_get_subticks(stopwatch_t* stopwatch)
{
	// Stopwatch may not have been stopped, thats ok, but we need to update it.
	if (stopwatch->state != STOPWATCH_STOPPED)
	{
		stopwatch->stop_ticks = xTaskGetTickCount();
		stopwatch->stop_subticks = SysTickValueGet();
	}

	unsigned long subticks;
	if (stopwatch->start_subticks >= stopwatch->stop_subticks) //not an overflow
		subticks = stopwatch->start_subticks - stopwatch->stop_subticks;
	else
	{
		unsigned long systick_period = SysTickPeriodGet();
		subticks = stopwatch->start_subticks + (systick_period-stopwatch->stop_subticks);
	}
	return subticks;
}

// Returns the number of milliseconds measured by a
// stopped stopwatch instance.
unsigned long stopwatch_get_time_ms(stopwatch_t* stopwatch)
{
	// Stopwatch may not have been stopped, thats ok, but we need to update it.
	if (stopwatch->state != STOPWATCH_STOPPED)
	{
		stopwatch->stop_ticks = xTaskGetTickCount();
		stopwatch->stop_subticks = SysTickValueGet();
	}
	return (stopwatch->stop_ticks-stopwatch->start_ticks)*portTICK_RATE_MS;
}

//WARNING: uses a divide operation, do not call from a high frequency ISR!
unsigned long stopwatch_get_time_us(stopwatch_t* stopwatch)
{
	// Stopwatch may not have been stopped, thats ok, but we need to update it.
	if (stopwatch->state != STOPWATCH_STOPPED)
	{
		stopwatch->stop_ticks = xTaskGetTickCount();
		stopwatch->stop_subticks = SysTickValueGet();
	}

	unsigned long ticks_us = 0;
	unsigned long subticks_us = 0;

	if (stopwatch->start_ticks != stopwatch->stop_ticks)
		ticks_us = (stopwatch->stop_ticks-stopwatch->start_ticks - 1)*portTICK_RATE_MS*1000;	//subtract one as the subticks represent that particular step
	subticks_us = stopwatch_get_subticks(stopwatch)/50;//configCPU_CLOCK_HZ*1000000;

	return ticks_us + subticks_us;
}
