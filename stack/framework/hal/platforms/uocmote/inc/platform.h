/* * OSS-7 - An opensource implementation of the DASH7 Alliance Protocol for ultra
 * lowpower wireless sensor communication
 *
 * Copyright 2015 University of Antwerp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __PLATFORM_H_
#define __PLATFORM_H_

#include "platform_defs.h"
#include "cc430_chip.h"

#ifndef PLATFORM_UOCMOTE
    #error Mismatch between the configured platform and the actual platform. Expected PLATFORM_WIZZIMOTE to be defined
#endif

/*TODO:
 * - SHT21 Humidity & Temp Sensor: I2C
 * I2C: - Data P1.3
 * 		- CLOCK P1.2
 * 	- LIS3DH: Accelerometer: I2C
 */


/********************
 * LED DEFINITIONS *
 *******************/

#define HW_NUM_LEDS 4

//INT_HANDLER


/********************
 * UART DEFINITIONS *
 *******************/

#define UART_BAUDRATE PLATFORM_CC430_UART_BAUDRATE

// TODO

// UART0 location #1: P16 and P15
//#define UART_PIN_TX         16            // P16
//#define UART_PIN_RX         15         	// P15

/*************************
 * DEBUG PIN DEFINITIONS *
 ************************/

//#define DEBUG_PIN_NUM 4
//#define DEBUG0	D4
//#define DEBUG1	D5
//#define DEBUG2	D6
//#define DEBUG3	D7

/**************************
 * USERBUTTON DEFINITIONS *
 *************************/

//#define NUM_USERBUTTONS 	2
//#define BUTTON0				B9
//#define BUTTON1				B10


#endif
