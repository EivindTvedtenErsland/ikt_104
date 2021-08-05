#include <Arduino.h>
#include <Wire.h>
#include <HTS221Sensor.h>
#include <DFRobot_RGBLCD.h>

#define I2C2_SCL    PB10
#define I2C2_SDA    PB11

// I2C
TwoWire dev_i2c(I2C2_SDA, I2C2_SCL);

// Components
HTS221Sensor  HumTemp(&dev_i2c);

//16 characters and 2 lines of show
DFRobot_RGBLCD lcd(16,2);  

int buttonState, counter;
int * counterPointer;
uint8_t color[3] = {0, 0, 0};

void setColorTemp(float temp, uint8_t arr[3]);
void setColorHum(float hum, uint8_t arr[3]);
float floatMap(float x, float in_min, float in_max, float out_min, float out_max);

void setup() {
  // Led.
  pinMode(LED_BUILTIN, OUTPUT);

  // Initialize serial for output
  Serial.begin(9600);

  // initialize lcd
  lcd.init();
  lcd.print("Suck My Dick");

  // Initialize I2C bus.
  dev_i2c.begin();

  // Initlialize components
  HumTemp.begin();
  HumTemp.Enable();
}

void loop() {

  // Led blinking.
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);

  // Read humidity and temperature.
  float humidity, temperature;
  HumTemp.GetHumidity(&humidity);
  HumTemp.GetTemperature(&temperature);

  // wait a bit for the entire message to arrive
  delay(1000);
  // clear the screen and set color
  lcd.clear();
  lcd.setRGB(color[0], color[1], color[2]);

  if (Serial.available() > 0) {

    // read incoming serial data:
    char buttonState = Serial.read();
    Serial.print(byte(buttonState));
  
  //Check if "1" is pressed on keyboard, check counter state, then reset buttonState value
  //to clear it for new input
  if ( buttonState == byte(49) && counter == 0) {
       counter = 1; 
       buttonState = byte(48);
    } 
    else if ( buttonState == byte(49) && counter == 1) {
      counter = 0;
      buttonState = byte(48);
    }
  }
  
  //Check current counter state to know if to switch to temperature or humidity
  if (counter == 0){
    lcd.print("Temp[C]: ");
    lcd.print(temperature, 2);
    setColorTemp(temperature, color);
  }
  else if (counter == 1){
    lcd.print("Hum[%]: ");
    lcd.print(humidity, 2);
    setColorHum(humidity, color);
  }
}

//Float take on the standard long map in the arduino library, because why not
float floatMap(float x, float in_min, float in_max, float out_min, float out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//Set color array based on temp, which is passed to lcd.setRGB later on in the loop 
void setColorTemp(float temp, uint8_t arr[3]){
    if (temp < float(20)){
      arr[0] = 0;
      arr[1] = 0;
      arr[2] = 255;
    }
    else if (temp <= float(24)) {
      arr[0] = 255;
      arr[1] = 69;
      arr[2] = 0;
    }
    else if (temp > float(24)) {
      arr[0] = 255;
      arr[1] = 0;
      arr[2] = 0;
    }
}

//Set color array based on hum, which is passed to lcd.setRGB later on in the loop 
void setColorHum(float hum, uint8_t arr[3]){
  float colorValueGradient = floatMap(hum, 0, 100, 0, 255);
  arr[0] = 0;
  arr[1] = 0;
  arr[2] = (uint8_t) colorValueGradient;
}

