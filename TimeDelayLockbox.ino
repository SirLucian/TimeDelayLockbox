/*  The Time Delay Lockbox V1.01 - By Lucian Blankevoort www.sirlucian.me
 *   
 *  For more instructions on building out such a box, please see the instructables for Time Delay Lockbox under my Instructables profile https://www.instructables.com/member/LucianB10/
 *  All materials needed, schematics etc are detailed there
 *  
 *  You will require the TM1637Display and the Encoder libraries
 *  
 * Author: Lucian BLankevoort
 * License: MIT License (https://opensource.org/licenses/MIT)
 *
 * V1.01 - Now using minutes and hours instead of seconds, and includes the colon
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

// Switch pins for your setting your box's state. Put the middle pin on the negative rail
#define SET 8
#define ARM 9

// Display module connection pins (Digital Pins)
#define CLK 5
#define DIO 6

//The word "OPEn" in something the display can understand... for that special moment when you may finally open your box
const uint8_t OPEN[] = {
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  SEG_A | SEG_B | SEG_E | SEG_F | SEG_G,           // P
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,           // E
  SEG_C | SEG_E | SEG_G                            // n
  };

//Rotary encoder Push button (not used currently in this version), A and B
//#define ROT_BTN 2
#define ROTA 3
#define ROTB 4

//Lock mechanism
#define SOLENOID 10
#define UNLOCK_BTN 12

// Initialise the display
TM1637Display display(CLK, DIO);

// Initialise the rotary encoder
Encoder Rotary(ROTA, ROTB);


// Clock tracking
long seconds = 0;

// Rotary encoder
long posRotary = -999;
long newPosRotary;

// Display Variables
uint8_t displayValue = 0;

// This function unlocks the box when/if you can
void unlockBox(){
  display.setSegments(OPEN); 
  digitalWrite(SOLENOID, HIGH);
  delay(2000);
  digitalWrite(SOLENOID, LOW);
  display.clear();
}

// Function to be called when beginning the countdown. Not used yet. supposed to blink out the value a few times
void beginCountdown(){
    display.setBrightness(0x0f);
    display.clear(); 
    delay(500);
    display.showNumberDecEx(displayValue, 64, true);
    delay(500);
    display.clear();
}


// Function to convert seconds into hours and minutes for the tube display
uint8_t secsToHourMin(long sec){
  int hours = sec / 3600;
  sec %= 3600;
  int mins = sec / 60;
  return displayValue = hours*100 + mins;
}

// Start up Arduino
void setup() {
  Serial.begin(9600);
  pinMode(SET, INPUT_PULLUP);
  pinMode(ARM, INPUT_PULLUP);
  pinMode(UNLOCK_BTN, INPUT_PULLUP);
  pinMode(SOLENOID, OUTPUT);
  digitalWrite(SOLENOID, LOW);

  display.setBrightness(0x0f);
  // Clear display of any text
  display.clear();
}

void loop() {
  // put your main code here, to run repeatedly:
  device_state;

  //uint8_t displayValue = seconds;
  uint8_t displayValue = secsToHourMin(seconds);

  //Switch to control states
  switch (device_state) {
    // State 0 - Counting down. When device is armed the clock starts to count down
    case COUNTING_DOWN:
      Serial.println("COUNTING DOWN");
      display.clear();
      if (seconds>0){
        seconds--;
        delay(1000);
      }
      // If device is not yet ready to unlock, it will display the current time left on the tube display
      while (digitalRead(UNLOCK_BTN) == LOW && seconds !=0) {
        display.showNumberDecEx(displayValue, 64, true);
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
    case SET_CLOCK:
      newPosRotary = Rotary.read();
      if (newPosRotary != posRotary) {
        posRotary = newPosRotary*15;
      }
      seconds = posRotary;

      // The display will show the time set while you set it
      display.showNumberDecEx(displayValue, 64, true);
      Serial.print("Number = ");
      Serial.println(newPosRotary);
      break;
      
    default:
      break;
  }

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
}
