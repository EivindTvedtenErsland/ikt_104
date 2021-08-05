/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"
#include "DFRobot_RGBLCD.h"
#include <chrono>
#include <cstdio>

#define WAIT_TIME_MS 150 

PwmOut piz(LED1); 
DigitalIn btn1(PA_1); DigitalIn btn2(PA_0); DigitalIn btn3(PD_14); DigitalIn btn4(PB_0);

DFRobot_RGBLCD lcd(16,2,D14,D15); 

Timer t;
Timeout reset_flip;

long time_interval = 60;
long time_interval_modifiers = 0;
long cur_time;
bool pause_state = false;

//frequency array
float frequency[]={659,554,659,554,550,494,554,587,494,659,554,440};

//beat array
float beat[]={1,1,1,1,1,0.5,0.5,1,1,1,1,2};

void pause(bool pause_state);
void reset();
void add5();
void subtract5();
void finished();

using namespace std;
using namespace chrono;

int main()
{
    printf("~~~~INIT section~~~~\n");

    lcd.init(); 

    btn1.mode(PullUp);
    btn2.mode(PullUp);
    btn3.mode(PullUp);
    btn4.mode(PullUp);

    //reset_flip.attach(&reset, 60s);
    printf("Setup complete, running on Mbed OS %d.%d.%d.\n", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);

    while (true)
    {  

        lcd.clear();
        lcd.printf("btnread: ");
        lcd.setCursor(9,0);
        lcd.printf("%i", btn4.read());
        lcd.setCursor(11,0);
        lcd.printf("%i", btn3.read());
        lcd.setCursor(13,0);
        lcd.printf("%i", btn2.read());
        lcd.setCursor(15,0);
        lcd.printf("%i", btn1.read());
        
        lcd.setCursor(0, 1);
        lcd.printf("Timer: ");
        lcd.setCursor(9,1);

        cur_time = ((time_interval + time_interval_modifiers) - (duration_cast<milliseconds>(t.elapsed_time()).count() / 1000));
        lcd.printf("%llu", cur_time);
        lcd.printf(" sec");

        if (cur_time <= 0){
            finished();
        }

        thread_sleep_for(WAIT_TIME_MS);

        if (btn1.read() == 0) {
            pause(pause_state);
            pause_state = !pause_state;
        }
        if (btn2.read() == 0) {
            reset();
        }
        if (btn3.read() == 0) {
            add5();
        }
        if (btn4.read() == 0) {
           subtract5();
        }

    }
}

void pause(bool pause_state) {
    if (pause_state == false){

    t.start();
    }
    else{

    t.stop();
    }
}

void reset() {
   
   t.reset();
    time_interval_modifiers = 0;
    piz.suspend();
}

void add5() {
    //long * p = &time_interval_modifiers;
    time_interval_modifiers += 5;
    
}

void subtract5() {
    //long * p = &time_interval_modifiers;
    time_interval_modifiers -= 5;
}

void finished() {
    t.stop();
    piz.resume();
    for (int i=0; i<=11; i++) {
        piz.period(1/(frequency[i])); // set PWM period
        piz=0.5; // set duty cycle
        thread_sleep_for(0.5*beat[i]); // hold for beat period
    }
    //piz.period(3.0f);  // 3 second period
    //piz.write(0.75f);  // 75% duty cycle
}
