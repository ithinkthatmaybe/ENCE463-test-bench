/*
 * stopwatch.h
 *
 *  Created on: Sep 13, 2015
 *      Author: sws52
 */

/* Measures time intervals, example:
 *
 *
 * stopwatch_t stopwatch;
 * stopwatch_start(&stopwatch);
 * delay_ms(500);
 * stopwatdh_stop(&stopwacth);
 * unsigned long time_meas = stopwatch_get_time_ms(&stopwatch); //Will return 500
 */

#ifndef STOPWATCH_H_
#define STOPWATCH_H_

#include "FreeRTOS.h"
#include "task.h"
#include "driverlib/systick.h"

// TODO: stopwatch module
typedef struct _stopwatch_t{
	unsigned long start_ticks;
	unsigned long start_subticks;
	unsigned long stop_ticks;
	unsigned long stop_subticks;
} stopwatch_t;

//Test the operation of the stopwatch
void vTaskTestStopwatch(void);

// We expect the uut to produce delays in the range of
// ~10us to 300us from looking at the scope so the subtick
// value is usefull
void stopwatch_start(stopwatch_t* stopwatch);

void stopwatch_stop(stopwatch_t* stopwatch);

unsigned long stopwatch_get_ticks(stopwatch_t* stopwatch);

unsigned long stopwatch_get_subticks(stopwatch_t* stopwatch);

unsigned long stopwatch_get_time_ms(stopwatch_t* stopwatch);

unsigned long stopwatch_get_time_us(stopwatch_t* stopwatch);




#endif /* STOPWATCH_H_ */
