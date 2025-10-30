#include <Keypad.h> // Library used for handling the keypad entries
#include "avr/io.h" // Used for the assembly code with the cross traffic lights

const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = { // Initialize 2D array for storing keypad values
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {12, 11, 10, 9}; // Arduino GPIO pins for the 16 button keypad
byte colPins[COLS] = {8, 7, 6, 5};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS); // Keypad library function used to synchronize the keypad button values to the pins when pushed

const int red = 26;
const int yellow = 28;
const int green = 30;
const int buzzer = 48;
const int data  = 49;  // DS
const int latch = 51;  // STCP
const int clock = 53;  // SHCP
const int digit0 = 33;  // Right digit (ones)
const int digit1 = 31;  // Left digit (tens)


byte table[] = {0x5F, 0x44, 0x9D, 0xD5, 0xC6, 0xD3, 0xDB, 0x45, 0xDF, 0xC7}; // Byte table used to send numerical values to the display

int count = 99; // Integer used to represent the highest possible keypad entry

int redTimeFirst = 0; // TimeFirst, and TimeSecond variables are used to store the first and second digits for the red and green light starts
int redTimeSecond = 0;
int totalRedTime = 0; // Total time variables are used to combine the two start time digits together for a final time in seconds

int greenTimeFirst = 0;
int greenTimeSecond = 0;
int totalGreenTime = 0;

const int yellowTime = 3; // Constant time for the yellow light

char firstRedDigit; // Since the keypad values are characters they need to be stored as such and transferred later to integers, these variables are used to hold all of that
char secondRedDigit;
char firstGreenDigit;
char secondGreenDigit;
char endSequence;
char startSequence;

int inputStage = 0;   // Input stage variable used to help with which digit is actually being entered
String redInput = ""; // Strings used to contain character inputs
String greenInput = "";
bool redTimeSet = false; // Booleans for helping with if the light times have been set, start out initally at false
bool greenTimeSet = false;


unsigned long previousMillis = 0; // These three variables are used in the while loops that make the LED blinking to be more simplfied, as opposed to using delays
const long interval = 1000;
bool ledState = LOW;

void setup(){
  
  pinMode(red, OUTPUT); // Setting pinModes 
  pinMode(yellow, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(latch,OUTPUT);
  pinMode(clock,OUTPUT);
  pinMode(data,OUTPUT);
  pinMode(digit0, OUTPUT);
  pinMode(digit1, OUTPUT);
  
  cli(); //stop interrupt
  //set timer1 interrupt at 1Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = 15624;// = (16*10^6) / (1*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei(); //allow interrupt


  digitalWrite(digit0, LOW); // Starts the system with the tens and ones place digits as off, and the blinking red light to off
  digitalWrite(digit1, LOW);
  digitalWrite(red, LOW);


//Opp lights or opposite lights for the cross traffic LEDs
  DDRL |= (1 << PL2); //redOpp pin 47
  DDRL |= (1 << PL4); //yellowOpp pin 45
  DDRL |= (1 << PL6); //greenOpp pin 43
  
  Serial.begin(9600);

}

void shiftOutByte(byte val){ // Writes to the shift register to handle the A-G pins on the display
  
  digitalWrite(latch, LOW);
  shiftOut(data, clock, MSBFIRST, val);
  digitalWrite(latch, HIGH);

}



void loop() {

  while (!redTimeSet || !greenTimeSet){ // while loop for blinking the red LED
    
    unsigned long currentMillis = millis();

    
    if (currentMillis - previousMillis >= interval){ // could technically be written with delay functions however Arduino also recommends this method for ensuring the whole system isn't put on pause while waiting for an input
     
      previousMillis = currentMillis;
      ledState = !ledState;
      digitalWrite(red, ledState);
   
    }

    char key = keypad.getKey(); // From the keypad library for waiting, without pausing the whole system, for a keypad entry
    
    if (key != NO_KEY){ // This sequence is the same for both the red and green light, essentially the arduino continously checks for a keypad entry, depending on if it is 'A' or 'B' for red or green it will start the process for
                        // collecting the tens and ones places digits for the lights. Once two integers 0-9 have been entered into the keypad AND the '#' is pressed the system will change the values from characters to integers
      if (key == 'A'){  // and go from there. Finally, the arduino will break out of the loop once both start times have been entered AND the '*' key is pressed
        
        inputStage = 1;
        redInput = "";
        Serial.println("Enter red time:");
      
      } 
      
      else if (key == 'B'){
        
        inputStage = 2;
        greenInput = "";
        Serial.println("Enter green time:");
      
      } 
      
      else if (key == '#' && inputStage == 1 && redInput.length() == 2){
       
        totalRedTime = redInput.toInt();
        redTimeSet = true;
        Serial.print("Total red time: ");
        Serial.println(totalRedTime);
        inputStage = 0;
      
      } 
      
      else if (key == '#' && inputStage == 2 && greenInput.length() == 2){
        
        totalGreenTime = greenInput.toInt();
        greenTimeSet = true;
        Serial.print("Total green time: ");
        Serial.println(totalGreenTime);
        inputStage = 0;
      
      } 
      
      else if (inputStage == 1 && isDigit(key) && redInput.length() < 2){
        
        redInput += key;
        Serial.print("Red digit: ");
        Serial.println(key);
      
      } 
      
      else if (inputStage == 2 && isDigit(key) && greenInput.length() < 2){
        
        greenInput += key;
        Serial.print("Green digit: ");
        Serial.println(key);
     
      } 
      
      else if (key != '#' && inputStage != 0){
        
        Serial.println("Invalid input");
      
      }
    }
  }

  

  Serial.println("Press *");
  previousMillis = 0;
  ledState = LOW;


  while(startSequence != '*'){ // The system will sit on standby until the '*' key is pressed, any other key could be pressed however the system won't start the light sequence until the proper key has been pressed. The blinking
                               // loop is also used again here to ensure the red light continues to blink while waiting
    unsigned long currentMillis = millis();
 
     if (currentMillis - previousMillis >= interval){
      previousMillis = currentMillis;
      ledState = !ledState;
      digitalWrite(red, ledState);
    }
    
    startSequence = keypad.getKey();
    
  }
  
  if (startSequence == '*'){
  
  digitalWrite(red, LOW);

  while (true) { // Once the system has started the sequence will continue until reset or power off, when transistoning from red to green, and green to yellow for 3 seconds each the lights will blink on and off in 0.5 second
                 // intervals, starting with 500 ms ON and then 500 ms OFF
      digitalWrite(red, HIGH); //red on
      PORTL |= (1 << PL6); // greenOpp

      for (int t = totalRedTime; t > 0; t--){
        
        
        if((t <= 6) && (t > 3)){


          displayCountdown(t);
          PORTL |= (1 << PL6);
          delay(500);
          PORTL &= ~(1 << PL6);
          delay(500);
          
        }

        if (t <= 3){
          
          displayCountdown(t);
          digitalWrite(red, HIGH);        
          delay(500);
          digitalWrite(buzzer, HIGH); 
          PORTL &= ~(1 << PL6);
          PORTL |= (1 << PL4);
          digitalWrite(red, LOW);
          delay(500);

        } 
        
        else if (t > 6){
        
          digitalWrite(buzzer, LOW);  
          displayCountdown(t);
          delay(1000);


        }        
      }
    
      digitalWrite(buzzer, LOW); 
      PORTL &= ~(1 << PL4);
      digitalWrite(red, LOW);
      digitalWrite(green, HIGH); // green on
      PORTL |= (1 << PL2); //redOpp on

      for (int t = totalGreenTime; t > 0; t--){
        
         if((t <= 6) && (t > 3)){


          displayCountdown(t);
          digitalWrite(green, HIGH); // green on
          delay(500);
          digitalWrite(green, LOW); // green off
          delay(500);          

          
        }

        if (t <= 3){
          
          displayCountdown(t);
          digitalWrite(yellow, HIGH);        
          digitalWrite(buzzer, HIGH); 
          PORTL |= (1 << PL2);
          delay(500);
          PORTL &= ~(1 << PL2);
          delay(500);

        } 
        
        else if (t > 6){
        
          digitalWrite(buzzer, LOW);  
          displayCountdown(t);
          delay(1000);

        }    
      
      
      
      } 

      digitalWrite(yellow, LOW); // At the end of one of the sequence phases the yellow light is reset


    }
  }
}


void displayCountdown(int secondsLeft){ // This function utilizes the shift register to actually display the time value, mathematically handles the ones and tens place
  
  unsigned long start = millis();
  
  while (millis() - start < 1000){
   
    int tens = secondsLeft / 10;
    int ones = secondsLeft % 10;

    digitalWrite(digit0, LOW);      
    digitalWrite(digit1, HIGH);     
    shiftOutByte(table[tens]);
    delay(5);                       

    digitalWrite(digit1, LOW);      
    digitalWrite(digit0, HIGH);     
    shiftOutByte(table[ones]);
    delay(5);
  }

  digitalWrite(digit0, LOW);
  digitalWrite(digit1, LOW);

}
