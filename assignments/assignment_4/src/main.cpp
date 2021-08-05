#include <Arduino.h>
#include <Wire.h>
#include <DFRobot_RGBLCD.h>
#include <stm32l4xx_hal_iwdg.h>

#define BUTTON_PIN_REFRESH 1
#define BUTTON_PIN_PAUSE 2

// volatile prevents the compiler from optimizing out variables.
// volatile should be used for variables that are changed in interrupts.
volatile byte state_refresh = LOW;
volatile byte state_pause = LOW;

// Time variables
unsigned long paused_time;
unsigned long current_time;
unsigned long time;

// Structure used to configure the watchdog
IWDG_HandleTypeDef watchdog;

void refresh();
void pause();

// Lcd in 16/2 format
DFRobot_RGBLCD lcd(16,2);  

void setup() {

  // Start serial
  Serial.begin(9600);
  Serial.println("setup()");

  //lcd setup
  lcd.init();
  lcd.clear();

  delay(1);

  // Configure inputs and outputs
  Serial.println("Configuring IO");
  pinMode(BUTTON_PIN_REFRESH, INPUT_PULLUP);
  pinMode(BUTTON_PIN_PAUSE, INPUT_PULLUP);
  
  delay(1);

  // Attach an external interrupt that triggers on falling
  Serial.println("Attaching interrupts");
  attachInterrupt(BUTTON_PIN_REFRESH, refresh, FALLING);
  attachInterrupt(BUTTON_PIN_PAUSE, pause, FALLING);

    // Fill out the configuration structure
    watchdog.Instance = IWDG;                      // Set watchdog instance
    watchdog.Init.Prescaler = IWDG_PRESCALER_256;  // Set prescaler value (higher = slower timer). 32 000 / 256 = 125 ticks per second.
    watchdog.Init.Reload = 1250;                   // Set reload value on refresh (higher = more time before reset). 1250 / 125 = 10 seconds watchdog.
    watchdog.Init.Window = 0xFFF;                  // Disable window mode

    // Initialize the watchdog with the structure we filled out
    if (HAL_IWDG_Init(&watchdog) != HAL_OK)
    {
        Serial.println("An error occured while configuring the watchdog");
        while(1);
    }  
}

void loop() {

  while (!state_pause){
  
  // Add paused time to calculate real elapsed time
  paused_time += current_time;

  // Start timer
  time = (millis() - paused_time);

  // Reset current time each time you exit state_pause
  current_time = 0;

  lcd.print("Time: ");
  lcd.print((float)time/1000, 2);

  delay(200);

  lcd.clear();
  Serial.print("State refresh: ");
  Serial.print(state_refresh);
  Serial.print(" State pause: ");
  Serial.print(state_pause);
  Serial.print(" ");

  }
  while (state_pause){

    if (current_time <= 0){
      current_time = millis();
    }

    lcd.setCursor(0,0);
    lcd.print("Current time: ");
    lcd.setCursor(0,1);
    lcd.print((float)current_time/1000, 2);
    delay(200);
    lcd.clear();

    HAL_IWDG_Refresh(&watchdog);

  }

}

// Interrupt handlers.
// These functions are called when the external interrupt is triggered.

void refresh() {
  HAL_IWDG_Refresh(&watchdog);
  state_refresh = !state_refresh;
}

void pause() {
  state_pause = !state_pause;
}