/*
 * Copyright (c) 2015-2020, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== gpiointerrupt.c ========
 */
#include <stdint.h>
#include <stddef.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>

/* Driver configuration */
#include "ti_drivers_config.h"
#include <ti/drivers/Timer.h>

uint8_t SOS = 1;
uint8_t counterChar = 0;

enum LED_States {INIT, FIRST_S, SOS_O, SECOND_S, OK_O, OK_K, MESSAGE_BREAK} LED_STATE;

void timerCallback(Timer_Handle myHandle, int_fast16_t status)
{
    switch(LED_STATE) {
        case FIRST_S:
            counterChar++;

            if (counterChar == 8){
                // 'S' is done
                counterChar = 0;
                LED_STATE = SOS_O;
            } else {
                // Still sending 'S'
                LED_STATE = FIRST_S;
            }
            break;

        case SOS_O:
            counterChar++;

            if (counterChar == 14){
                // 'O' is done
                counterChar = 0;
                LED_STATE = SECOND_S;
            } else {
                // Still sending 'O'
                LED_STATE = SOS_O;
            }
            break;

        case SECOND_S:
            counterChar++;

            // Second 'S' has no buffer after the character
            if (counterChar == 5){
                // 'S' is done and message is done
                counterChar = 0;
                LED_STATE = MESSAGE_BREAK;
            } else {
                // Still sending 'S'
                LED_STATE = SECOND_S;
            }
            break;

        case OK_O:
            counterChar++;

            if (counterChar == 14){
                // 'O' is done
                counterChar = 0;
                LED_STATE = OK_K;
            } else {
                // Still sending 'O'
                LED_STATE = OK_O;
            }
            break;

        case OK_K:
            counterChar++;

            // 'K' has no buffer after the character
            if (counterChar == 9){
                // 'K' is done and message is done
                counterChar = 0;
                LED_STATE = MESSAGE_BREAK;
            } else {
                // Still sending 'K'
                LED_STATE = OK_K;
            }
            break;

        case MESSAGE_BREAK:
            // 7-count break in between messages
            counterChar++;

            if (counterChar == 7){
                // Break finished; check for change in message (from button press)
                counterChar = 0;
                if (SOS){
                    // Cycle 'SOS' message
                    LED_STATE = FIRST_S;
                } else {
                    // Cycle 'OK' message
                    LED_STATE = OK_O;
                }
            } else {
                // Break not finished
                LED_STATE = MESSAGE_BREAK;
            }
            break;

        default:
            LED_STATE = FIRST_S;
            break;
    }

    switch(LED_STATE) {
        case FIRST_S:
            // DOT - DOT - DOT

            // Turn red LED on for dots and off otherwise
            if (counterChar == 1 || counterChar == 3 || counterChar == 5){
                GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
            } else {
                GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
            }
            break;

        case SOS_O:
            // DASH - DASH - DASH

            // Turn green LED on for dashes and off for counts of 4, 8, 12, 13, and 14
            if (counterChar < 12 && counterChar % 4 != 0){
                GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_ON);
            } else {
                GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
            }
            break;

        case SECOND_S:
            // DOT - DOT - DOT

            // Turn red LED on for dots and off otherwise
            if (counterChar % 2 == 0){
                GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
            } else {
                GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
            }
            break;

        case OK_O:
            // DASH - DASH - DASH

            // Turn green LED on for dashes and off for counts of 4, 8, 12, 13, and 14
            if (counterChar < 12 && counterChar % 4 != 0){
                GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_ON);
            } else {
                GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
            }
            break;

        case OK_K:
            // DASH - DOT - DASH

            // Turn green LED on for dashes and red LED on for dot
            if (counterChar == 4 || counterChar == 6){
                // Rests between signals
                GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
                GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
            } else if (counterChar == 5){
                // Green LED already off, turn on red
                GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
            } else {
                // Red LED already off, turn on green
                GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_ON);
            }
            break;

        case MESSAGE_BREAK:
            // All LEDs off
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
            GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
            break;

    }
}

void initTimer(void)
{
    Timer_Handle timer0;
    Timer_Params params;
    Timer_init();
    Timer_Params_init(&params);
    params.period = 500000;
    params.periodUnits = Timer_PERIOD_US;
    params.timerMode = Timer_CONTINUOUS_CALLBACK;
    params.timerCallback = timerCallback;
    timer0 = Timer_open(CONFIG_TIMER_0, &params);

    if (timer0 == NULL) {
        /* Failed to initialized timer */
        while (1) {}
    }

    if (Timer_start(timer0) == Timer_STATUS_ERROR) {
        /* Failed to start timer */
        while (1) {}
    }
}


/*
 *  ======== gpioButtonFxn0 ========
 *  Callback function for the GPIO interrupt on CONFIG_GPIO_BUTTON_0.
 *
 *  Note: GPIO interrupts are cleared prior to invoking callbacks.
 */
void gpioButtonFxn0(uint_least8_t index)
{
    // Change message type on button press
    SOS = !SOS;
}

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    /* Call driver init functions */
    GPIO_init();

    /* Configure the LED and button pins */
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_LED_1, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_BUTTON_0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

    /* Install Button callback */
    GPIO_setCallback(CONFIG_GPIO_BUTTON_0, gpioButtonFxn0);

    /* Enable interrupts */
    GPIO_enableInt(CONFIG_GPIO_BUTTON_0);

//    /*
//     *  If more than one input pin is available for your device, interrupts
//     *  will be enabled on CONFIG_GPIO_BUTTON1.
//     */
//    if (CONFIG_GPIO_BUTTON_0 != CONFIG_GPIO_BUTTON_1)
//    {
//        /* Configure BUTTON1 pin */
//        GPIO_setConfig(CONFIG_GPIO_BUTTON_1, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);
//
//        /* Install Button callback */
//        GPIO_setCallback(CONFIG_GPIO_BUTTON_1, gpioButtonFxn1);
//        GPIO_enableInt(CONFIG_GPIO_BUTTON_1);
//    }

    initTimer();

    return (NULL);
}
