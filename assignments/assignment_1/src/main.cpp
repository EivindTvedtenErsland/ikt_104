#include <Arduino.h>

const int ledPin = 13;   
// the setup function runs once when you press reset or power the board
void setup()
{
  Serial.begin(9600);  
  pinMode(ledPin, OUTPUT);
}

// the loop function runs over and over again forever
void loop()
{
  if (Serial.available() > 0){
 
  char inChar = Serial.read();
  Serial.print(byte(inChar));
  
  if (inChar==byte(49)){
      digitalWrite(ledPin, HIGH);                     
    }
  else if(inChar==byte(48)){
      digitalWrite(ledPin, LOW);      
    }
  }

  delay(1000);                  
}