
/*** Modified Reflow Oven Code.
* Original Author: Rocket Scream Electronics, www.rocketscream.com
* Modified Author: James Lewis, james@cmiyc.com, www.cmiyc.com
* Modifiet Auther: Juan-Antonio Søren E. Pedersen (Solved). 
 
 - Workes with ReflowOvenProcessing.pde Skech.

* This firmware owed very much on the works of other talented individuals as
* follows:
* ==========================================
* Brett Beauregard (www.brettbeauregard.com)
* ==========================================
* Author of Arduino PID library. On top of providing industry standard PID
* implementation, he gave a lot of help in making this reflow oven controller
* possible using his awesome library.
*
* ==========================================
* Limor Fried of Adafruit (www.adafruit.com)
* ==========================================
* Author of Arduino MAX6675 library. Adafruit has been the source of tonnes of
* tutorials, examples, and libraries for everyone to learn.
*
* Disclaimer
* ==========
* Dealing with high voltage is a very dangerous act! Please make sure you know
* what you are dealing with and have proper knowledge before hand. Your use of
* any information or materials on this reflow oven controller is entirely at
* your own risk, for which we shall not be liable.
*
* Licences
* ========
* This reflow oven controller hardware and firmware are released under the
* Creative Commons Share Alike v3.0 license
* http://creativecommons.org/licenses/by-sa/3.0/
* You are free to take this piece of code, use it and modify it.
* All we ask is attribution including the supporting libraries used in this
* firmware.*/

#include "Adafruit_MAX31855.h"
#include "PID_v1.h"
 
// ***** TYPE DEFINITIONS *****
typedef enum REFLOW_STATE{
REFLOW_STATE_IDLE,
REFLOW_STATE_PREHEAT,
REFLOW_STATE_SOAK,
REFLOW_STATE_REFLOW,
REFLOW_STATE_COOL,
REFLOW_STATE_TOO_HOT,
REFLOW_STATE_COMPLETE,
REFLOW_STATE_ERROR,
REFLOW_STATE_HOVER
} reflowState_t;
 
typedef	enum SWITCH
{
	SWITCH_NONE,
	SWITCH_1,	
	SWITCH_2
}	switch_t;

typedef enum REFLOW_STATUS{
REFLOW_STATUS_OFF,
REFLOW_STATUS_ON
} reflowStatus_t;
 
typedef enum DEBOUNCE_STATE{
DEBOUNCE_STATE_IDLE,
DEBOUNCE_STATE_CHECK,
DEBOUNCE_STATE_RELEASE
} debounceState_t;
 
// ***** CONSTANTS *****
#define TEMPERATURE_ROOM 220
#define TEMPERATURE_SOAK_MIN 150
#define TEMPERATURE_SOAK_MAX 210   
#define TEMPERATURE_REFLOW_MAX 235
#define TEMPERATURE_COOL_MIN 220
#define SENSOR_SAMPLING_TIME 1000
#define SOAK_TEMPERATURE_STEP 5
#define SOAK_MICRO_PERIOD 9000
#define DEBOUNCE_PERIOD_MIN 50
#define THERMOCOUPLE_DISCONNECTED 10000
 
// ***** PID PARAMETERS *****
// ***** PRE-HEAT STAGE *****
#define PID_KP_PREHEAT 100
#define PID_KI_PREHEAT 0.025
#define PID_KD_PREHEAT 20
// ***** SOAKING STAGE *****
#define PID_KP_SOAK 200
#define PID_KI_SOAK 0.03
#define PID_KD_SOAK 100
// ***** REFLOW STAGE *****
#define PID_KP_REFLOW 200
#define PID_KI_REFLOW 0.025
#define PID_KD_REFLOW 100
#define PID_SAMPLE_TIME 1000
 
// ***** LCD MESSAGES *****
const char* lcdMessagesReflowStatus[] = {
"Ready",
"Pre-heat",
"Soak",
"Reflow",
"Cool",
"Complete",
"Wait,hot",
"Error"
};
 
// ***** DEGREE SYMBOL FOR LCD *****
unsigned char degree[8] = {140,146,146,140,128,128,128,128};
 
// ***** PIN ASSIGNMENT *****
int ssr = 6;
int thermoDO = 8;
int thermoCS = 9;
int thermoCLK = 10; 
 
int ledRed = 3;
int ledGreen = 2;

unsigned int hover_count;
/* int lcdRs = 2;
int lcdE = 3;
int lcdD4 = 4;
int lcdD5 = 5;
int lcdD6 = 11;
int lcdD7 = 12;*/
 
int buzzerPin = 5;

 
// ***** PID CONTROL VARIABLES *****
double setpoint;
double input;
double output;
double kp = PID_KP_PREHEAT;
double ki = PID_KI_PREHEAT;
double kd = PID_KD_PREHEAT;
int windowSize = 2000;
// Set window size

unsigned long windowStartTime;
unsigned long nextCheck;
unsigned long nextRead;
unsigned long timerSoak;
unsigned long buzzerPeriod;
unsigned long reflowPeriod;
// Reflow oven controller state machine state variable
reflowState_t reflowState;
// Reflow oven controller status
reflowStatus_t reflowStatus;
// Button debounce state machine state variable
debounceState_t debounceState;

switch_t switchStatus;

// Button debounce timer
long lastDebounceTime;
// Button press status
boolean buttonPressStatus;
// Seconds timer
int timerSeconds;
 
// Specify PID control interface
PID reflowOvenPID(&input, &output, &setpoint, kp, ki, kd, DIRECT);
// Specify LCD interface
//LiquidCrystal lcd(lcdRs, lcdE, lcdD4, lcdD5, lcdD6, lcdD7);
 
Adafruit_MAX31855 thermocouple(thermoCLK, thermoCS, thermoDO);
 
void setup()
{

  
  
// SSR pin initialization to ensure reflow oven is off
Serial.println(F("Init and Turn off SSR"));
digitalWrite(ssr, LOW);
pinMode(ssr, OUTPUT);
 
// Buzzer pin initialization to ensure annoying buzzer is off
/* digitalWrite(buzzer, LOW);
pinMode(buzzer, OUTPUT);
*/
// LED pins initialization and turn on upon start-up (active low)
digitalWrite(ledRed, LOW);
digitalWrite(ledGreen, LOW);
pinMode(ledRed, OUTPUT);
pinMode(ledGreen, OUTPUT);
/* // Push button pins initialization
pinMode(button1, INPUT);
pinMode(button2, INPUT);
*/
// Start-up splash
/* digitalWrite(buzzer, HIGH);
lcd.begin(8, 2);
lcd.createChar(0, degree);
lcd.clear();
lcd.print("Reflow");
lcd.setCursor(0, 1);
lcd.print("Oven 1.1");
digitalWrite(buzzer, LOW);
delay(2500);
lcd.clear();
*/
Serial.println(F("ReflowAwe 101"));
// Serial communication at 57600 bps
Serial.begin(9600);
 
// Turn off LED (active low)
digitalWrite(ledRed, HIGH);
digitalWrite(ledGreen, HIGH);
 

// Initialize time keeping variable
nextCheck = millis();
// Initialize thermocouple reading varible
nextRead = millis();
}
 
void loop()
{
// Current time
unsigned long now;


 
// Time to read thermocouple?
if (millis() > nextRead)
{
// Read thermocouple next sampling period
nextRead += SENSOR_SAMPLING_TIME;
// Read current temperature
input = thermocouple.readCelsius();
 
// If thermocouple is not connected
if (input == THERMOCOUPLE_DISCONNECTED)
{
// Illegal operation without thermocouple
reflowState = REFLOW_STATE_ERROR;
reflowStatus = REFLOW_STATUS_OFF;
}
}


 
if (millis() > nextCheck)
{
    
      
// Check input in the next seconds
nextCheck += 1000;


// Toggle red LED as system heart beat
digitalWrite(ledRed, !(digitalRead(ledRed)));
// Increase seconds timer for reflow curve analysis
timerSeconds++;
// Send temperature and time stamp to serial
//Serial.print(timerSeconds);
//Serial.print(" ");
//Serial.print(setpoint);
//Serial.print(" ");
Serial.print(input);
Serial.print("\n");
//Serial.println(output);

// Clear LCD
// lcd.clear();
// Print current system state
// lcd.print(lcdMessagesReflowStatus[reflowState]);
//Serial.println(lcdMessagesReflowStatus[reflowState]);
// Move the cursor to the 2 line
// lcd.setCursor(0, 1);
 
// If currently in error state
if (reflowState == REFLOW_STATE_ERROR)
{
// No thermocouple wire connected
Serial.println("No TC!");
}
else
{
// Print current temperature
/* lcd.print(input);
#if ARDUINO >= 100
lcd.write((uint8_t)0);
#else
// Print degree Celsius symbol
lcd.print(0, BYTE);
#endif
lcd.print("C "); */
}
}
else
{
// Turn off red LED
digitalWrite(ledRed, HIGH);
}


// PID computation and SSR control
  if (input <= 235)
  {
    now = millis();

    reflowOvenPID.Compute();

    if((now - windowStartTime) > windowSize)
    { 
      // Time to shift the Relay Window
      windowStartTime += windowSize;
    }
    if(output > (now - windowStartTime)) digitalWrite(ssr, HIGH);
    else digitalWrite(ssr, LOW);   
  }
  // Reflow oven process is off, ensure oven is off
  else 
  {
    digitalWrite(ssr, LOW);
  }
 

                if (Serial.available() > 0) {
                int inByte = Serial.read();
    
                 switch (inByte) {
                  case 'g':
                switchStatus=SWITCH_1;
                  break;
                  case 'o':
                switchStatus=SWITCH_2;
                   break;
                    default:
                switchStatus=SWITCH_NONE;
                 }
                    }
  
    
    
                if (switchStatus == SWITCH_2)
			{
                // If currently reflow process is on going
                if (reflowStatus == REFLOW_STATUS_ON)
{
                // Button press is for cancelling
                // Turn off reflow process
                  reflowStatus = REFLOW_STATUS_OFF;
                // Reinitialize state machine
                  reflowState = REFLOW_STATE_IDLE;
                    }
                  }
    
 
 switch (reflowState)
  {
    
       case REFLOW_STATE_IDLE:
		        // If oven temperature is still above room temperature
		        if (input >= TEMPERATURE_ROOM)
		{
                        //  Note: If temperature is above set-room-temperature, the Reflow process can ´t start.
			reflowState = REFLOW_STATE_TOO_HOT;
		}
		        else
		{        
                        reflowStatus = REFLOW_STATUS_OFF;

			// If switch is pressed to start reflow process
			if (switchStatus == SWITCH_1)
			{
        // Send header for CSV file
        //Serial.println("Time Setpoint Input Output");
        // Intialize seconds timer for serial debug information
        timerSeconds = 0;
        // Initialize PID control window starting time
        windowStartTime = millis();
        // Ramp up to minimum soaking temperature
        setpoint = TEMPERATURE_SOAK_MIN;
        // Tell the PID to range between 0 and the full window size
        reflowOvenPID.SetOutputLimits(0, windowSize);
        reflowOvenPID.SetSampleTime(PID_SAMPLE_TIME);
        // Turn the PID on
        reflowOvenPID.SetMode(AUTOMATIC);
        // Proceed to preheat stage
        reflowState = REFLOW_STATE_PREHEAT;
      }
    }
    break;




  
   case REFLOW_STATE_PREHEAT:
    reflowStatus = REFLOW_STATUS_ON;
    // If minimum soak temperature is achieve       
    if (input >= TEMPERATURE_SOAK_MIN)
    {
      // Chop soaking period into smaller sub-period
      timerSoak = millis() + SOAK_MICRO_PERIOD;
      // Set less agressive PID parameters for soaking ramp
      reflowOvenPID.SetTunings(PID_KP_SOAK, PID_KI_SOAK, PID_KD_SOAK);
      // Ramp up to first section of soaking temperature
      setpoint = TEMPERATURE_SOAK_MIN + SOAK_TEMPERATURE_STEP;   
      // Proceed to soaking state
      reflowState = REFLOW_STATE_SOAK; 
    }
    break;



 
case REFLOW_STATE_SOAK:
// If micro soak temperature is achieved
if (millis() > timerSoak)
{
timerSoak = millis() + SOAK_MICRO_PERIOD;
// Increment micro setpoint
setpoint += SOAK_TEMPERATURE_STEP;
if (setpoint > TEMPERATURE_SOAK_MAX)
{
// Set agressive PID parameters for reflow ramp
reflowOvenPID.SetTunings(PID_KP_REFLOW, PID_KI_REFLOW, PID_KD_REFLOW);
// Ramp up to first section of soaking temperature
setpoint = TEMPERATURE_REFLOW_MAX;
// Proceed to reflowing state
reflowState = REFLOW_STATE_REFLOW;
}
}
break;
 
case REFLOW_STATE_REFLOW:
// We need to avoid hovering at peak temperature for too long
// Crude method that works like a charm and safe for the components
if (input >= (TEMPERATURE_REFLOW_MAX - 5))

{

hover_count = millis();
// hold the temp for 3 minutes.
reflowState = REFLOW_STATE_HOVER;


}
break;

case REFLOW_STATE_HOVER:



// Crude method that works like a charm and safe for the components
if (hover_count - millis() > 180000)

{
// Set PID parameters for cooling ramp
reflowOvenPID.SetTunings(PID_KP_REFLOW, PID_KI_REFLOW, PID_KD_REFLOW);
// Ramp down to minimum cooling temperature
setpoint = TEMPERATURE_COOL_MIN;
digitalWrite(buzzerPin, HIGH);
// Proceed to cooling state
reflowState = REFLOW_STATE_COOL;
}
break;




 
case REFLOW_STATE_COOL:
// If minimum cool temperature is achieve
if (input <= (TEMPERATURE_COOL_MIN))
{
  
  buzzerPeriod = millis() + 3000;
// Turn off buzzer and green LED
// digitalWrite(buzzer, LOW);
digitalWrite(ledGreen, HIGH);
// Reflow process endedF
reflowStatus = REFLOW_STATUS_OFF;
reflowState = REFLOW_STATE_COMPLETE;
}
break;

case REFLOW_STATE_COMPLETE:
    if (millis() > buzzerPeriod)
    {
 		// Reflow process ended
      reflowState = REFLOW_STATE_IDLE; 
    }
    break;
   
             
     case REFLOW_STATE_TOO_HOT:
	// If oven temperature drops below room temperature
		if (input < TEMPERATURE_ROOM)
		{
			// Ready to reflow
			reflowState = REFLOW_STATE_IDLE;
		}
		break;

 
case REFLOW_STATE_ERROR:
// If thermocouple is still not connected
if (input == THERMOCOUPLE_DISCONNECTED)
{
// Wait until thermocouple wire is connected
reflowState = REFLOW_STATE_ERROR;
}
else
{
// Clear to perform reflow process
reflowState = REFLOW_STATE_IDLE;
}
break;


  }
    }



  







