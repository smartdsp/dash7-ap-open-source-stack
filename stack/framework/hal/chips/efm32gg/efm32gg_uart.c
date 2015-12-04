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

/*! \file efm32gg_uart.c
 *
 *  \author jeremie@wizzilab.com
 *  \author maarten.weyn@uantwerpen.be
 *  \author contact@christophe.vg
 */

#include <em_usart.h>
#include <em_cmu.h>
#include <em_gpio.h>
#include <em_usbd.h>
#include "hwgpio.h"
#include "hwuart.h"
#include <assert.h>
#include "em_gpio.h"
#include "hwsystem.h"

#include "efm32gg_pins.h"

#include "platform.h"

typedef struct uart_defition {
  USART_TypeDef*       channel;
  CMU_Clock_TypeDef    clock;
  IRQn_Type            tx_irqn;
  IRQn_Type            rx_irqn;
  uart_rx_inthandler_t rx_handler;
  pin_id_t             tx_pin;
  pin_id_t             rx_pin;
  uint32_t             baud;
  uint32_t             location;
} uart_definition_t;

#if defined(UART0_IN_USE) && defined(UART1_IN_USE) && defined(UART2_IN_USE) && defined(UART3_IN_USE) 
#define UARTS 4
#elif defined(UART0_IN_USE) && defined(UART1_IN_USE) && defined(UART2_IN_USE)
#define UARTS 3
#elif defined(UART0_IN_USE) && defined(UART1_IN_USE)
#define UARTS 2
#elif defined(UART0_IN_USE)
#define UARTS 1
#else
#define UARTS 0
#endif

// array of available configurations for U(S)ART used
static uart_definition_t uart[UARTS];

static void _setup_uarts(void) {
#ifdef UART0_IN_USE
  uart[0] = (uart_definition_t){
    .channel    = UART0_CHANNEL,
    .clock      = UART0_CLOCK,
    .tx_irqn    = UART0_IRQ_TX,
    .rx_irqn    = UART0_IRQ_RX,
    .rx_handler = NULL,
    .location   = UART0_LOCATION,
    .tx_pin     = UART0_TX_PIN,
    .rx_pin     = UART0_RX_PIN,
    .baud       = UART0_BAUDRATE,
  };
#endif
#ifdef UART1_IN_USE
  uart[1] = (uart_definition_t){
    .channel    = UART1_CHANNEL,
    .clock      = UART1_CLOCK,
    .tx_irqn    = UART1_IRQ_TX,
    .rx_irqn    = UART1_IRQ_RX,
    .rx_handler = NULL,
    .location   = UART1_LOCATION,
    .tx_pin     = UART1_TX_PIN,
    .rx_pin     = UART1_RX_PIN,
    .baud       = UART1_BAUDRATE,
  };
#endif  
#ifdef UART2_IN_USE
#pragma message "CONSOLE"
  uart[2] = (uart_definition_t){
    .channel    = UART2_CHANNEL,
    .clock      = UART2_CLOCK,
    .tx_irqn    = UART2_IRQ_TX,
    .rx_irqn    = UART2_IRQ_RX,
    .rx_handler = NULL,
    .location   = UART2_LOCATION,
    .tx_pin     = UART2_TX_PIN,
    .rx_pin     = UART2_RX_PIN,
    .baud       = UART2_BAUDRATE,
  };
#endif
#ifdef UART3_IN_USE
  uart[3] = (uart_definition_t){
    .channel    = UART3_CHANNEL,
    .clock      = UART3_CLOCK,
    .tx_irqn    = UART3_IRQ_TX,
    .rx_irqn    = UART3_IRQ_RX,
    .rx_handler = NULL,
    .location   = UART3_LOCATION,
    .tx_pin     = UART3_TX_PIN,
    .rx_pin     = UART3_RX_PIN,
    .baud       = UART3_BAUDRATE,
  }
#endif
};

void __uart_init() {
  CMU_ClockEnable(cmuClock_GPIO, true);

  _setup_uarts();
  for(uint8_t i=0; i<UARTS; i++) { __uart_init_port(i); }
}

void __uart_init_port(uint8_t idx) {
  CMU_ClockEnable(uart[idx].clock, true);

  error_t err;
  // configure UART TX pin as digital output, initialize high since UART TX
  // idles high (otherwise glitches can occur)
  err = hw_gpio_configure_pin(uart[idx].tx_pin, false, gpioModePushPullDrive, 1);
  assert(err == SUCCESS);
  // configure UART RX pin as input (no filter)
  err = hw_gpio_configure_pin(uart[idx].rx_pin, false, gpioModeInput, 0);
  assert(err == SUCCESS);

  USART_InitAsync_TypeDef uartInit = {
    .enable       = usartDisable,   // wait to enable the transceiver
    .refFreq      = 0,              // setting refFreq to 0 will invoke the
                                    // CMU_ClockFreqGet() function and measure
                                    // the HFPER clock
    .baudrate     = uart[idx].baud, // desired baud rate
    .oversampling = usartOVS16,     // set oversampling value to x16
    .databits     = usartDatabits8, // 8 data bits
    .parity       = usartNoParity,  // no parity bits
    .stopbits     = usartStopbits1, // 1 stop bit
    .mvdis        = false,          // use majority voting
    .prsRxEnable  = false,          // not using PRS input
    .prsRxCh      = usartPrsRxCh0,  // doesn't matter which channel we select
  };

  USART_InitAsync(uart[idx].channel, &uartInit);
  // clear RX/TX buffers and shift regs, enable transmitter and receiver pins
  uart[idx].channel->ROUTE = UART_ROUTE_RXPEN | UART_ROUTE_TXPEN | uart[idx].location;
  USART_IntClear(uart[idx].channel, _UART_IF_MASK);
  NVIC_ClearPendingIRQ(uart[idx].rx_irqn);
  NVIC_ClearPendingIRQ(uart[idx].tx_irqn);

  USART_Enable(uart[idx].channel, usartEnable);
}

void uart_set_rx_interrupt_callback(uint8_t idx, uart_rx_inthandler_t rx_handler) {
  uart[idx].rx_handler = rx_handler;
}

void uart_send_byte(uint8_t idx, uint8_t data) {
#ifdef PLATFORM_USE_USB_CDC
		uint16_t timeout = 0;
		while(USBD_EpIsBusy(0x81) && timeout < 100){
			timeout++;
			hw_busy_wait(1000);
		};
		uint32_t tempData = data;
		int ret = USBD_Write( 0x81, (void*) &tempData, 1, NULL);
#else
		while(!(uart[idx].channel->STATUS & (1 << 6))); // wait for TX buffer to empty
		uart[idx].channel->TXDATA = data;
#endif
}

void uart_send_bytes(uint8_t idx, void const *data, size_t length) {
#ifdef PLATFORM_USE_USB_CDC
    // print misaliged bytes first as individual bytes.
		int8_t* tempData = (int8_t*) data;
		while(((uint32_t)tempData & 3) && (length > 0)) {
			uart_send_byte(uart, tempData[0]);
			tempData++;
			length--;
		}

		if (length > 0)
		{
			uint16_t timeout = 0;
			while(USBD_EpIsBusy(0x81) && timeout < 100){
				timeout++;
				hw_busy_wait(1000);
			};
			int ret = USBD_Write( 0x81, (void*) tempData, length, NULL);
		}
#else
		for(uint8_t i=0; i<length; i++)	{
			uart_send_byte(idx, ((uint8_t const*)data)[i]);
		}
#endif
}

void uart_send_string(uint8_t idx, const char *string) {
  uart_send_bytes(idx, string, strnlen(string, 100));
}

error_t uart_rx_interrupt_enable(uint8_t idx) {
  if(uart[idx].rx_handler == NULL) { return EOFF; }
  USART_IntClear(uart[idx].channel, _UART_IF_MASK);
  USART_IntEnable(uart[idx].channel, UART_IF_RXDATAV);
  NVIC_ClearPendingIRQ(uart[idx].tx_irqn);
  NVIC_ClearPendingIRQ(uart[idx].rx_irqn);
  NVIC_EnableIRQ(uart[idx].rx_irqn);
  return SUCCESS;
}

void uart_rx_interrupt_disable(uint8_t idx) {
  USART_IntClear(uart[idx].channel, _UART_IF_MASK);
  USART_IntDisable(uart[idx].channel, UART_IF_RXDATAV);
  NVIC_ClearPendingIRQ(uart[idx].rx_irqn);
  NVIC_ClearPendingIRQ(uart[idx].tx_irqn);
  NVIC_DisableIRQ(uart[idx].rx_irqn);
}

void UART0_RX_IRQHandler(void) {
  if(uart[0].channel->STATUS & UART_STATUS_RXDATAV) {
    uart[0].rx_handler(USART_Rx(uart[0].channel));
    USART_IntClear(uart[0].channel, UART_IF_RXDATAV);
  }
}

void UART1_RX_IRQHandler(void) {
  if(uart[1].channel->STATUS & UART_STATUS_RXDATAV) {
    uart[1].rx_handler(USART_Rx(uart[1].channel));
    USART_IntClear(uart[1].channel, UART_IF_RXDATAV);
  }
}
