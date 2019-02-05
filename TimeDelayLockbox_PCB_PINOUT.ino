/*  The Time Delay Lockbox V1.11 - By Lucian Blankevoort www.sirlucian.me
 *   
 *  For more instructions on building out such a box, please see the instructables for Time Delay Lockbox under my Instructables profile https://www.instructables.com/member/LucianB10/
 *  All materials needed, schematics etc are detailed there
 *  
 *  You will require the TM1637Display and the Encoder libraries
 *  
 * Author: Lucian BLankevoort
 * License: MIT License (https://opensource.org/licenses/MIT)
 * 
 * V1.11 Serial update on switching case & display indication of state, rotary encoder now goes up by 10min increments when setting clock, Pin change because of failed 
 * V1.1 - New pinout to match circuit layout on protoboard
 * V1.01 - Now using minutes and hours instead of seconds, and includes the colon
 *
 *
 * ToDo:
 * Low power mode
 * Saving clock to eeprom so power off and on doesnt reset clock
 * Push Rotary button to reset clock
 * 
 */
 
//Libraries to include
#include <Arduino.h>
#include <TM1637Display.h>
#include <Encoder.h>

//State machine
#define COUNTING_DOWN 0
#define SET_CLOCK 1
#define BOX_UNLOCKED 2
uint8_t device_state;
uint8_t old_state;

// Switch pins for your setting your box's state. Put the middle pin on the negative rail
#define SET 7
#define ARM 11

// Display module connection pins (Digital Pins)
#define CLK 8
#define DIO 9

//The word "OPEn" in something the display can understand... for that special moment when you may finally open your box
const uint8_t OPEN[] = {
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  SEG_A | SEG_B | SEG_E | SEG_F | SEG_G,           // P
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,           // E
  SEG_C | SEG_E | SEG_G                            // n
  };

const uint8_t SETCLOCK[] = {
  SEG_A | SEG_F | SEG_G | SEG_C | SEG_D,           // S
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,           // E
  SEG_F | SEG_G | SEG_E | SEG_D,                   // t
  SEG_G                                            // -
  };

//Rotary encoder Push button (not used currently in this version), A and B
#define ROT_BTN 3
#define ROTA 4
#define ROTB 2

//Lock mechanism
#define SOLENOID 5
#define UNLOCK_BTN 10

// Initialise the display
TM1637Display display(CLK, DIO);

// Initialise the rotary encoder
Encoder Rotary(ROTA, ROTB);


// Clock tracking
long seconds = 0;
uint8_t hours;
uint8_t minutes;

// Rotary encoder
long posRotary = -999;
long newPosRotary;
bool setHours = true;

// Display Variables
//uint8_t displayValue;

// This function unlocks the box when/if you can
void unlockBox(){
  Serial.println("Unlocking Box");
  display.setSegments(OPEN); 
  digitalWrite(SOLENOID, HIGH);
  delay(2000);
  digitalWrite(SOLENOID, LOW);
  display.clear();
}

// Function to be called when beginning the countdown. Not used yet. supposed to blink out the value a few times
void beginCountdown(){
    display.setBrightness(7, true);
    displayValue();
    delay(500);
    display.clear();
    delay(500);
    displayValue();
    delay(500);
    display.clear();
    delay(500);
}

void displayValue(){
  secsToHourMin(seconds);
    // This displays the minutes in the last two places
  display.showNumberDecEx(minutes, 0, true, 2, 2);
  // Display the hours in the first two places, with colon
  display.showNumberDecEx(hours, 64, true, 2, 0);
}

// Function to convert seconds into hours and minutes for the tube display
void secsToHourMin(long sec){
  hours = sec / 3600;
  sec %= 3600;
  minutes = sec / 60;
}
//and from hours and minutes to seconds
long HHMMtoSec (uint8_t h, uint8_t m) {
  m = h*60 + m;
  seconds = m*60;
  return seconds;
}

// Start up Arduino
void setup() {
  Serial.begin(9600);
  pinMode(SET, INPUT_PULLUP);
  pinMode(ARM, INPUT_PULLUP);
  pinMode(ROT_BTN, INPUT_PULLUP);
  pinMode(UNLOCK_BTN, INPUT_PULLUP);
  pinMode(SOLENOID, OUTPUT);
  digitalWrite(SOLENOID, LOW);

  display.setBrightness(0x0f);
  // Clear display of any text
  display.clear();
}

void loop() {
    // If you press the unlock button while the device isn't armed, the solenoid will activate. This means if you close the box while setting it, or if you put the switch in the neutral position, it will still unlock
  if (digitalRead(UNLOCK_BTN) == LOW && device_state != COUNTING_DOWN) {
    unlockBox();
  }

  // If the switch is in the set position, you activate the state in which you can set the clock
  if (digitalRead(SET) == LOW){
    device_state = SET_CLOCK;
  } 
  
  // If the switch is set to the ARM position, you start the countdown. Once you close the box, you will not be able to unlock it until the countdown timer reaches zero.
  else if (digitalRead(ARM) == LOW && seconds != 0){
    device_state = COUNTING_DOWN;
  } 
  
  // When in the switch is in the neutral position, the box is naturally unlockable
  else {
    device_state = BOX_UNLOCKED;
  }
  
  // If state has changed
  if (old_state != device_state) {
    if (device_state == 0) beginCountdown();
    if (device_state == 1) display.setSegments(SETCLOCK);
    old_state = device_state;
    Serial.println("State changed to: "+ device_state);
    delay(1000);
    display.clear();
  }

  //uint8_t displayValue = seconds;
  secsToHourMin(seconds);
  
  //Switch to control states
  switch (device_state) {
    // State 0 - Counting down. When device is armed the clock starts to count down
    case COUNTING_DOWN:
      display.clear();
      if (seconds>0){
        seconds--;
        delay(1000);
        Serial.println(seconds);
      }
      // If device is not yet ready to unlock, it will display the current time left on the tube display
      if (digitalRead(UNLOCK_BTN) == LOW && seconds !=0) {
        displayValue();
        delay(1000);
        seconds--;
      }
      // If the countdown timer reaches zero, it will switch to the unlockable state
      if (seconds == 0) {
        display.showNumberDecEx(0, 64, true);
        delay(1000);
        display.clear();
        Serial.println("Box unlockable now");
        device_state = BOX_UNLOCKED;
        break;
      }
      Serial.print("Seconds: ");
      Serial.println(seconds);
      break;

    // State 1 - Box Unlocked. In this state, the solenoid will be activated when you press the unlock button. 
    case BOX_UNLOCKED:
      display.clear();
      break;

    // State 2 - Setting the clock. When the switch is set to this position, you may rotate the rotary encoder to adjust the duration that the box will be in countdown mode for. 
    // Pressing the rotary encoder push button swaps between setting hours and minutes
    case SET_CLOCK:
      newPosRotary = Rotary.read();
      if (newPosRotary != posRotary) {
        posRotary = newPosRotary*150;
        seconds = posRotary;
      }
      

      //Reset clock to Zero
      //if (digitalRead(ROT_BTN) == LOW) seconds = 0;

      // The display will show the time set while you set it
      displayValue();
      Serial.print("seconds = ");
      Serial.println(seconds);
    /*case SET_CLOCK:
      long newPosRotary = Rotary.read();
      if (setHours){
        if (newPosRotary != posRotary) {
          posRotary = newPosRotary/4;
          displayHours = posRotary % 99;
          display.showNumberDecEx(displayHours, true, 2, 2);
          if (ROT_BTN == LOW) setHours = false;
          Serial.println("Hours: ");
          Serial.println(displayHours);
        } 
      } else {
        if (newPosRotary != posRotary) {
          posRotary = newPosRotary*(2.5);
          displayMinutes = posRotary % 59;
          display.showNumberDecEx(displayMinutes);
          if (ROT_BTN == LOW) setHours = true;
          Serial.println("Mins: ");
          Serial.println(displayMinutes);
        }
      }
      seconds = HHMMtoSec(displayHours, displayMinutes);
      displayValue = secsToHourMin(seconds);
      display.showNumberDecEx(displayValue, 64, true);*/
      break;
      
    default:
      break;
  }
  

}
