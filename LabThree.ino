//Colin MacEachern
//EECE 5520
//Lab Three Fall 25

// Including all of the libraries necessary for the lab to function, wire for IC2, libraries 2-4 are from the elegoo lessons, the last two are for the timer interrupt

#include <Wire.h>
#include <DS3231.h>
#include <LiquidCrystal.h>
#include <IRremote.h>
#include <avr/io.h>
#include <avr/interrupt.h>


#define ENABLE 8     //L293D Enable pin required to activate the motor
#define DIRA 3          // Input 1 and 2 for the chip per the elegoo lesson
#define DIRB 4          

#define SOUND_ANALOG A0  // microphone analog input per the elegoo lesson
#define button 7     // pushbutton to toggle direction

#define IR 11 // IR reciever pin


LiquidCrystal lcd(53, 52, 51, 47, 45, 43); // LCD pins per the elegoo lesson
DS3231 clockDS; // real time clock per the lessons

IRrecv irrecv(IR);
uint32_t lastIR = 0;


int motorSpeed = 0; // The motor speed has four settings, 0-3 that correlate to the speed
volatile bool paused = false; // True/false for the play/pause button

int direction = 0; // this changes from 0-1 to change the motor direction, it starts as clockwise and can be changed

int hours = 0;
int minutes = 0;
int seconds = 0;

volatile bool secondTick = false; //ISR Flag

bool setTimeMode = false; // true/false to enter time set mode using the remote
String timeBuffer = ""; // buffer for holding the time as a string to be displayed

int lastButtonRead = HIGH;
int buttonState = HIGH;
unsigned long lastTime = 0;
const unsigned long timeDelay = 50; 

ISR(TIMER1_COMPA_vect) // timer interrupt for clock
{
  seconds++;
  if (seconds >= 60) { seconds = 0; minutes++; }
  if (minutes >= 60) { minutes = 0; hours++; }
  if (hours >= 24)   { hours = 0; }

  secondTick = true; 
}

void setup() 
{

  // basic pin definitions 

  pinMode(ENABLE, OUTPUT);
  pinMode(DIRA, OUTPUT);
  pinMode(DIRB, OUTPUT);
  pinMode(button, INPUT_PULLUP); 
  Serial.begin(9600);

// the next few lines are from the elegoo lessons for the real time clock, the clock is started, the current time is imported and the hours, minutes, and seconds are put into variables to be used later

  clockDS.begin();
  RTCDateTime t = clockDS.getDateTime();
  hours = t.hour;
  minutes = t.minute;
  seconds = t.second;

// LCD code, the first line maintains the line below as a header

  lcd.begin(16, 2);
  lcd.print("Motor/Time/Speed");

// IR enable from the library and elegoo lesson

  irrecv.enableIRIn();

  setupTimer1(); // this is where the timer interrupt setup is 

  updateMotor(); // this function handles updating the motor throughout the program

  printToScreen(); // this function handles updating the lcd screen
}




void loop() {

  handleIR();

  int reading = digitalRead(button);

// this if statement handles the button changes for the motor directions, this is similar to what is on the arduino website for how to blink an LED. Basically it uses timing to change the button state and keep it changed
// it also updates the motor and keeps a variable to display if the motor is CW or CCW

  if (reading != lastButtonRead) 
  {
    lastTime = millis();
  }
  if ((millis() - lastTime) > timeDelay) 
  {
    if (reading != buttonState) 
    {
      buttonState = reading;
      if (buttonState == LOW) 
      {
        direction = !direction; 
        updateMotor();
      }
    }
  }
 
  lastButtonRead = reading;

// this section handles the sound, the range was found by experimenting with the sensor, it reads the analog values from the sensor and changes the motor speed accordingly
// a switch statement used later takes each number as a case and changes the motor speed

  if (!paused && !setTimeMode) 
  {
    int soundVal = analogRead(SOUND_ANALOG);
   
    if (soundVal <= 660)
    { 
    
      motorSpeed = 0;
    
    }
    else if (soundVal <= 665) 
    {
    
      motorSpeed = 1;
    
    }
    else if (soundVal <= 670) 
    {
    
      motorSpeed = 2;
    
    }
    
    else 
    {
    
      motorSpeed = 3;
    
    }
    
    updateMotor();
  }

  updateMotor();

// here's where the timer interrupt is used for the clock

  if (secondTick) 
  {

    noInterrupts();
    int hh = hours;
    int mm = minutes;
    int ss = seconds;
    secondTick = false;
    interrupts();
    printToScreen();
    
  }

  delay(5);

}

// this function is the bread and butter of the program, it first enables the chip then uses the direction to spin the motor in its respective direction

void updateMotor() 
{
  if (paused) 
  {
  
    analogWrite(ENABLE, 0);
  
  } 
  
  else 
  {
  
    int pwm = speedToPWM(motorSpeed);
    analogWrite(ENABLE, pwm);
  
  }

  if (direction == 0) 
  {
  
    digitalWrite(DIRA, HIGH);
    digitalWrite(DIRB, LOW);
  
  } 
  
  else 
  {
  
    digitalWrite(ENABLE, HIGH);
    digitalWrite(DIRA, LOW);
    digitalWrite(DIRB, HIGH);
  
  }
}

// this function handles the screen, it combines the character arrays together to display the time, direction, and speed all on the second line of the lcd

void printToScreen() 
{

  int hh, mm, ss;
  noInterrupts();
  hh = hours; mm = minutes; ss = seconds;
  interrupts();

  char timeStr[12];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", hh, mm, ss);

  const char* dirStr = (direction == 0 ? "C" : "CC");
  const char* speedStr;
  switch (motorSpeed) 
  {
  
    case 0: 
      speedStr = "0";   
      break;
    
    case 1: 
      speedStr = "1/2"; 
      break;
    
    case 2: 
      speedStr = "3/4"; 
      break;
    
    case 3: 
      speedStr = "Full";
      break;
    
    default: 
      speedStr = "0";  
      break;
  
  }

  lcd.setCursor(0, 0);
  lcd.print("Motor Time Status");
  lcd.setCursor(0, 1);
  lcd.print("                "); 
  lcd.setCursor(0, 1);
  char buf[17];
  snprintf(buf, sizeof(buf), "%s %s %s", timeStr, dirStr, speedStr);
  lcd.print(buf);
}

// this function sets the variables to actually display the time

void finalizeTimeSetting() {
  if (timeBuffer.length() != 6) 
  {
  
    timeBuffer = "";
    setTimeMode = false;
    return;
  
  }
  
  int HH = (timeBuffer[0] - '0') * 10 + (timeBuffer[1] - '0');
  int MM = (timeBuffer[2] - '0') * 10 + (timeBuffer[3] - '0');
  int SS = (timeBuffer[4] - '0') * 10 + (timeBuffer[5] - '0');

  HH = (HH % 24 + 24) % 24;
  MM = (MM % 60 + 60) % 60;
  SS = (SS % 60 + 60) % 60;

  noInterrupts();
  hours = HH;
  minutes = MM;
  seconds = SS;
  interrupts();

  clockDS.setDateTime(2025, 1, 1, HH, MM, SS);

  setTimeMode = false;
  timeBuffer = "";

}

// this function takes the IR data and uses it to pause the motor, update the speed, or update the time. Most of the code is from the elegoo lesson

void handleIR() 
{
  if (!irrecv.decode()) return;

  uint32_t code;
  if (irrecv.decodedIRData.flags) 
  {

    code = lastIR;

  } 
  else 
  {
     code = irrecv.decodedIRData.decodedRawData;
    lastIR = code;
  
  }

  if (!setTimeMode) 
  {
    if (code == 0xBF40FF00) 
    {          
      
      paused = !paused;
      updateMotor();
    
    }
    
    else if (code == 0xBC43FF00) 
    {       
    
      if (motorSpeed < 3) motorSpeed++;
      updateMotor();
    
    }
    else if (code == 0xBB44FF00) 
    {       
    
      if (motorSpeed > 0) motorSpeed--;
      updateMotor();
    
    }
    else if (code == 0xB847FF00) 
    {       
    
      setTimeMode = true;
      timeBuffer = "";
      lcd.setCursor(0, 1);
      lcd.print("Set Time: ------");
    
    }
  }
  else 
  {
    if (code == 0xE916FF00) timeBuffer += "0";
    else if (code == 0xF30CFF00) timeBuffer += "1";
    else if (code == 0xE718FF00) timeBuffer += "2";
    else if (code == 0xA15EFF00) timeBuffer += "3";
    else if (code == 0xF708FF00) timeBuffer += "4";
    else if (code == 0xE31CFF00) timeBuffer += "5";
    else if (code == 0xA55AFF00) timeBuffer += "6";
    else if (code == 0xBD42FF00) timeBuffer += "7";
    else if (code == 0xAD52FF00) timeBuffer += "8";
    else if (code == 0xB54AFF00) timeBuffer += "9";

    if (setTimeMode) 
    {
    
      lcd.setCursor(0, 1);
      lcd.print("Set Time: ");
      // print current buffer padded with '-' to length 6
      String display = timeBuffer;
      while (display.length() < 6) display += "-";
      lcd.print(display);
    
    }

    if (timeBuffer.length() == 6) 
    {
      finalizeTimeSetting();
      printToScreen();
    }
  }

  irrecv.resume(); 
}

// this function sets the speed of the motor

int speedToPWM(int sp) 
{ 
  switch (sp) {
    case 0: 
      return 0;    // stopped
    case 1: 
      return 128;  // 1/2
    case 2: 
      return 180;  // 3/4
    case 3: 
      return 255;  // Full
    default: 
      return 0;
  }
}

// interrupt setup, I liked it better as a function instead of pasting it into the setup(). Functionally the same and comes from the course github

void setupTimer1() 
{
  cli();                 
  TCCR1A = 0;
  TCCR1B = 0;
  OCR1A = 15624;         
  TCCR1B |= (1 << WGM12);           
  TCCR1B |= (1 << CS12) | (1 << CS10); 
  TIMSK1 |= (1 << OCIE1A);          
  sei();                 
}
