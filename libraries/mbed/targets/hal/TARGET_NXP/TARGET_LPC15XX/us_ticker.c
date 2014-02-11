/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stddef.h>
#include "us_ticker_api.h"
#include "PeripheralNames.h"

#define US_TICKER_TIMER_IRQn     SCT0_IRQn

int us_ticker_inited = 0;

void us_ticker_init(void) {
    if (us_ticker_inited) return;
    us_ticker_inited = 1;
    
    // Enable the SCT0 clock
    LPC_SYSCON->SYSAHBCLKCTRL1 |= (1 << 3);
    
    // Clear peripheral reset the SCT0:
    LPC_SYSCON->PRESETCTRL1 |= (1 << 2);
    
    // Unified counter (32 bits)
    LPC_SCT0->CONFIG |= 1;
    
    // halt and clear the counter
    LPC_SCT0->CTRL |= (1 << 2) | (1 << 3);
    
    // System Clock (12)MHz -> us_ticker (1)MHz
    LPC_SCT0->CTRL |= ((SystemCoreClock/1000000 - 1) << 5);
    
    // unhalt the counter:
    //    - clearing bit 2 of the CTRL register
    LPC_SCT0->CTRL &= ~(1 << 2);
    
    NVIC_SetVector(US_TICKER_TIMER_IRQn, (uint32_t)us_ticker_irq_handler);
    NVIC_EnableIRQ(US_TICKER_TIMER_IRQn);
}

uint32_t us_ticker_read() {
    if (!us_ticker_inited)
        us_ticker_init();
    
    return LPC_SCT0->COUNT;
}

void us_ticker_set_interrupt(unsigned int timestamp) {
    // halt the counter: 
    //    - setting bit 2 of the CTRL register
    LPC_SCT0->CTRL |= (1 << 2);
    
    // set timestamp in compare register
    LPC_SCT0->MATCH0 = timestamp;
    
    // unhalt the counter:
    //    - clearing bit 2 of the CTRL register
    LPC_SCT0->CTRL &= ~(1 << 2);
    
    // if events are not enabled, enable them
    if (!(LPC_SCT0->EVEN & 0x01)) {
        
        // comb mode = match only
        LPC_SCT0->EV0_CTRL = (1 << 12);
        
        // ref manual:
        //   In simple applications that do not 
        //   use states, write 0x01 to this 
        //   register to enable an event
        LPC_SCT0->EV0_STATE |= 0x1;
        
        // enable events
        LPC_SCT0->EVEN |= 0x1;
    }
}

void us_ticker_disable_interrupt(void) {
    LPC_SCT0->EVEN &= ~1;
}

void us_ticker_clear_interrupt(void) {
    LPC_SCT0->EVFLAG = 1;
}
