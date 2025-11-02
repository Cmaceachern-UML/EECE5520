// Colin MacEachern
// EECE 5520 SEC 201P
// Lab 2
// Arduino program for handling the joystick, gyro module, and sending data serially to the python program


// Based on the ELEGOO videos
// Wire.h is used for I2C communication with the MPU gyro/accel module, the first section of const ints 
// are for defining the GPIO pins on the arduino

#include <Wire.h>
const int joyButton = 2; 
const int joyX = A0;
const int joyY = A1;
const int MPU = 0x68; 
const int buzzer = 7;


// The int16_t variables are from the ELEGOO lessions, the gyro control axis ones are the GyX, GyY, and GyZ variables
// The AcX, AcY, and AcZ ones are for the accelerometer.

int16_t AcX, AcY, AcZ;
int16_t GyX, GyY, GyZ;

// shakeTime is used in a similar manner to how the arduino forums recommend blinking an LED
// It is used in a loop to make the shake status go to true or false based on if a shake was detected
unsigned long shakeTime = 0;
char lastDirection = ' ';


// The setup function opens with the IC2 setup for the MPU module, it is basically accessing different registers
// in order to initialize the protocol. It is worth noting that some of this code was taken from the ELEGOO lesson
// for joystick operation

void setup() 
{
  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  pinMode(joyButton, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);
  Serial.begin(9600);
  delay(500);

  Serial.println("READY"); // this READY statement is part of the serial communication handshake mentioned in the python game
}

void loop() 
{

// this is where the arduino starts accessing the MPU module, it used the Wire.read() lines to read if the gyro/accel has been pitched
// rolled, or yawed. It is worth noting that the Z axis was not needed, but it was kept in because it didn't work unless I kept it

  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 14, true);

  AcX = Wire.read() << 8 | Wire.read();
  AcY = Wire.read() << 8 | Wire.read();
  AcZ = Wire.read() << 8 | Wire.read();
  Wire.read(); 
  Wire.read();
  GyX = Wire.read() << 8 | Wire.read();
  GyY = Wire.read() << 8 | Wire.read();
  GyZ = Wire.read() << 8 | Wire.read();

// This section is also taken from the ELEGOO lessons. It handles the raw data coming from the accelerometer, if a certain magnitude was detected
// it sent a "z" to the python script to change the score and color of the apple.
  float gX = (float)AcX / 16384.0;
  float gY = (float)AcY / 16384.0;
  float gZ = (float)AcZ / 16384.0;
  float magnitude = sqrt(gX * gX + gY * gY + gZ * gZ);

// Arduino recommends a similar method of using the millis() - someSortOfTimeVariable as an effective way to change the state of an LED
// a similar method was used here however it includes the magnitude > 1.5 operator. This states that if the magnitude was measured to be
// above 1.5 (normalized from the raw data as part of the ELEGOO lesson) AND if the time was within a certain state then the "z" can be
// written serially over the COM3 port to change the python game.

  if (magnitude > 1.5 && millis() - shakeTime > 1000) 
  {
    Serial.write('z');
    shakeTime = millis();
  }

// This is the more intuitive portion of the program, the joystick is read through the analog ports to control the X and Y
// directions. It's my understanding that the joystick functions as a potentiometer and as it is moved effects a voltage
// differential measured by the arduino. The value is represented as a 0-1023 value. Based on the value certain characters
// are sent serially. The char direction is used as a blank initally to control what the python program sees

  char direction = ' ';

  if (analogRead(joyX) < 490) 
  {

  direction = 'd';

  }
  
  else if (analogRead(joyX) > 530) 
  {
  
  direction = 'a';
  
  }
  
  else if (analogRead(joyY) > 530) 
  {

  direction = 'w';
  
  }
  
  else if (analogRead(joyY) < 490) 
  {

  direction = 's';

  }

// This section is similar to the joystick however it handles the normalized data from the gyroscope to act as a controller
  if (direction == ' ') 
  {
    if (GyY > 2000) 
    {
    
    direction = 'w';
    
    }
    
    else if (GyY < -2000) 
    {
    
    direction = 's';
    
    }
    
    else if (GyX > 2000) 
    {
    
    direction = 'a';
    
    }    
    
    else if (GyX < -2000) 
    {
    
    direction = 'd';
    
    }
  }

// This section is for sending the direction character serially over to the python game. As long as the direction isnt empty(the button has been pressed 
// or MPU module moved) and the direction is not repeated send it over. This is to stop the serial port from being clogged with uncessary data
  if (direction != ' ' && direction != lastDirection) 
  {
    
    Serial.write(direction);
    lastDirection = direction;
  
  }

// This section is for activating the buzzer. Once the serial port has been initalized it waits for an "E" character, which python sends if an apple is
// eaten. The buzzer then briefly buzzes.
  if (Serial.available() > 0) 
  {
    
    char incomingByte = Serial.read();
    if (incomingByte == 'E') 
    {
      
      digitalWrite(buzzer, HIGH);
      delay(100);
      digitalWrite(buzzer, LOW);
    
    }
  }

  delay(50);
}
