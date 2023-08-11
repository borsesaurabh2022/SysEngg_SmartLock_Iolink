/*
  Copyright (C) 2022 unref-ptr
  This file is part of lwIOLink.
  lwIOLink is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  lwIOLink is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with lwIOLink.  If not, see <https://www.gnu.org/licenses/>.

  Contact information:
  <unref-ptr@protonmail.com>
*/
#include <arduino.h>
#include <stdint.h>
#include "lwIOLink.hpp"
#include <Keypad.h>
#include <SPI.h>
#include <ESP32Servo.h>


Servo myservo;                                    //Initialize servo motor

char initial_password[4] = {'7', '8', '9', '0'};
char password[4]; 
char key_pressed = 0;                             // Variable to store incoming keys
uint8_t i = 0;                                    // Variable used for counter

// defining how many rows and columns our keypad have
const byte rows = 4;
const byte columns = 4;

bool success;                                     //Boolean variable to Receive at Master
int Sensor = 14 ;// Declaration of the sensor input pin
int val; // Temporary variable
int pos = 0;

// Keypad pin map
char hexaKeys[rows][columns] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// Initializing pins for keypad
byte row_pins[rows] ={15, 12, 25, 27};
byte column_pins[columns] = {26, 4, 0, 2};

// Create instance for keypad
Keypad keypad_key = Keypad( makeKeymap(hexaKeys), row_pins, column_pins, rows, columns);

using namespace lwIOLink;

static  unsigned constexpr PDInSize = 1;
static  unsigned constexpr PDOutSize = 1;
static unsigned constexpr min_cycle_time = 50000; //Cycle time in microseconds for operate mode
uint8_t PDOut[PDOutSize] = {0}; //Buffer that recieves data from the Master
uint8_t PDIn[PDInSize] = {0}; //Buffer which will be sent to the Master

//Hardware configuration for the device
constexpr Device::HWConfig HWCfg =
{
  .SerialPort = Serial2,
  .Baud = lwIOLink::COM3,
  .WakeupMode = FALLING,
  .Pin =
  {
    .TxEN = 18,
    .Wakeup = 5,
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO)
    .Tx = 17,
    .Rx = 16
#endif
  }
};

Device iol_device( PDInSize, PDOutSize, min_cycle_time);

/*
 * Optional static callback to know when Master has read events
 */
void Device::OnEventsProcessed()
{
  Serial.println("Events Processed by Master!");
}

void SecurityCheck(){
  val = digitalRead (Sensor) ; // The current signal at the sensor is read out

  if (val == 1) {
    //Serial.println("Card detected, Please enter password");ū
    key_pressed = keypad_key.getKey(); // Storing keys
    if (key_pressed)
    {
      password[i++] = key_pressed; // Storing in password variable
      Serial.print("*");
    }
    if (i == 4) // If 4 keys are completed
    {
      //delay(200);
      if (!(strncmp(password, initial_password, 4))) // If password is matched
      {
        Serial.println("Correct Password");
        Serial.println("Access Granted");
        //delay(3000);
        i = 0;
        val = 0; // Make RFID mode true      
        success = true; 

      }
      else    // If password is not matched
      {

        Serial.println("Wrong Password");
        Serial.println("Access Denied");
        //delay(3000);
        i = 0;
        val = 0;  // Make RFID mode true
        success = false; 
      }
    }
  }
  else{
    success = false;
  }
  if(success == true){
    PDIn[0] = 15;   
    }
  if(success == false){
    PDIn[0] = 0;
    for (pos = 0; pos <= 180; pos += 1){
    myservo.write(pos);  
    delay(15); 
    }
    }
  //delay(50000);
}

void Device::OnNewCycle()
{
  PDStatus PDOutStatus;
  bool pdOut_ok = iol_device.GetPDOut(PDOut, &PDOutStatus);
  if (pdOut_ok && PDOutStatus == Valid)
  {
    if(PDOut[0] == 0x0F){
      myservo.write(90);
    }else{myservo.write(90);}
    //dosomething with pdOut
  }

  SecurityCheck();
  bool result = iol_device.SetPDIn(PDIn, sizeof(PDIn));
  if ( result == true)
  {
    iol_device.SetPDInStatus(lwIOLink::Valid);
  }
}

void setup()
{
  Serial.begin(115200);
  myservo.attach(32);
  iol_device.begin(HWCfg);
  /* Example API of how to Set IO-Link Events
  */
  pinMode (Sensor, INPUT) ; // Initialize sensor pin
  digitalWrite(Sensor, LOW) ; // Activate internal pull-up resistor
  uint16_t EventCode = 0x1234;
  // Note: Cannot Set events in ISR, not concurrent safe
  Event::POD MyEvent(Event::Qualifier::Application, Event::Qualifier::SingleShot, EventCode);
  Device::EventResult result = iol_device.SetEvent(MyEvent);
  (void)result; //Normally you would check that the Event could be set
}

void loop()
{
  iol_device.run();
}
