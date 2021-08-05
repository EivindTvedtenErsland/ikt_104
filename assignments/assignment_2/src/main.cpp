#include <Arduino.h>

const int potPin = A0;
const int butPin = 2;
const int ledPin_1 = 1;
const int ledPin_2 = 9;
const int ledPin_3 = 13;

void setup()
{
  pinMode(potPin, INPUT);
  pinMode(butPin, INPUT);
  pinMode(ledPin_1, OUTPUT);
  pinMode(ledPin_2, OUTPUT);
  pinMode(ledPin_3, OUTPUT);
  
  Serial.begin(9600);
}
void loop()
{
  int buttonState = 0;
  int potValue = analogRead(potPin);
  potValue = map(potValue, 0, 1023, 0, 255); 
  buttonState = digitalRead(butPin);
  
  Serial.print("potValue: ");
  Serial.print(potValue);
  Serial.print(" buttonState: ");
  Serial.print(buttonState);
  Serial.print(" ");
  delay(100); // Wait for 100 millisecond(s)
  
  if (potValue >= 0){
  	analogWrite(ledPin_1, potValue);
  	analogWrite(ledPin_2, 0);
  	analogWrite(ledPin_3, 0);
  }
  if (potValue >= 86){
  	analogWrite(ledPin_1, potValue);
  	analogWrite(ledPin_2, map(potValue, 86, 172, 0, 255));
  	analogWrite(ledPin_3, 0);
  }
  if (potValue >= 172){
     analogWrite(ledPin_1, potValue);
  	analogWrite(ledPin_2, map(potValue, 86, 172, 0, 255));
  	analogWrite(ledPin_3, map(potValue, 172, 255, 0, 255));
  }
  
  if (buttonState == HIGH){
    analogWrite(ledPin_1, 255);
    analogWrite(ledPin_2, 255);
    analogWrite(ledPin_3, 255);
  }  
}