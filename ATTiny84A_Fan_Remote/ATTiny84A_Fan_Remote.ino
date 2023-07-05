/*
433 Mhz Remote for Lucci Fan/Light

Fan and Remote Specifications:
The remote control system operates on 433 MHz, using protocol 11 (as determined by running an RF sniffing program), and transmissions are 12-bit.
Replaces remote control model number UC7216T (PCB version B, made by Rhine).

Remotes have five functions:
• Light on/off
• Fan off
• Fan low
• Fan medium
• Fan high

The codes for each function in each room are:
Remote No   Room            Light   Off     Low     Med     High
1           Family Room     1150    1149    1143    1135    1119
2           Bedroom 3       638     637     631     623     607
3           Bedroom 1       382     381     375     367     351
4           Bedroom 2       254     253     247     239     223

Power Source:
Two Eneloop 1.2 V rechargable batteries in series to produce 2.4 V typical operating voltage.  A low battery indicator triggers at 2 V (1.0 V per battery),
which is based on testing and online information.  The indicator will be a flashing LED when 2 V is detected and is aimed at protecting the batteries from
over-discharging and being damaged.  THE SYSTEM IS **NOT** 5 V TOLERANT - THE TRANSMITTER HAS A MAXIMUM VOLTAGE OF 3.6 V.

The system is put to sleep when not in use.  Waking up is triggered by pin change interrupts on the button pins.

LED Indication:
The LED is a Vishay TLHR4400-AS12Z.  In addition to low voltage indication, it will be lit at each button press to show that the press has been registered.
A Dickson Charge Pump is used to ensure the minimum drive voltage (2.4 - 3.0 V) is acheived as long as the batteries are above minimum charge, and it's
controlled by two PWM outputs that are inverted relative to each other.

Radio Transmitter:
Transmitter is a HopeRF RFM110W radio transmitter, factory set to 433 Mhz.  The transmitter is able to operate in a voltage range of 1.8 - 3.6 V.

Microcontroller:
Microcontroller is an Atmel AVR ATtiny84A.  The 'A' model is capable of running as low as 1.8 V @ 1 Mhz.  The registers are set to run it at 1 Mhz to
ensure that the MCU will operate to at least as low as the low battery indicator.  The MCU will be reprogramable via a 2x5 set of pins set up for USBasp.
MAKE SURE TO DISCONNECT THE BATTERIES BEFORE CONNECTING THE PROGRAMMER!

MCU Pinout:

                         VCC  1  |-\/-|  14  GND
   Radio Tx  PCINT8  10  PB0  2  |    |  13  PA0  0  PCINT0  Button
     Button  PCINT9   9  PB1  3  |    |  12  PA1  1  PCINT1  Button
 Programmer   RESET  11  PB3  4  |    |  11  PA2  2  PCINT2  Button
Charge Pump    OC0A   8  PB2  5  |    |  10  PA3  3  PCINT3  Button
Charge Pump    OC0B   7  PA7  6  |    |  9   PA4  4  USCK    Programmer
 Programmer    MOSI   6  PA6  7  |____|  8   PA5  5  MISO    Programmer

Legend:
Layer 1: Physical pin number
Layer 2: OEM pin label
Layer 3: Arduino IDE pin number
Layer 4: Pin function in this project
Layer 5: Pin purpose in this project

When not in use, pins are set input with the internal pull-ups active, as recommended by the datasheet.
*/


//-----------------------------------------------------------------------
//Libraries
#include <avr/sleep.h>                                                   //Sleep Modes.
#include <avr/power.h>                                                   //Power management.
#include <RCSwitch.h>                                                    //Library for radio transmitting and receiving.

//NicoHoods PinChangeInterrupt library provides an API that offers rising/falling edge detection on PCINTn ports (not natively available).
//NOTE: I have refactored this library and modified the settings file to prevent Port B from being deactivated by default.
#include <PinChangeInterrupt_ATTiny84_All_Pins.h>
#include <PinChangeInterrupt_ATTiny84_All_PinsBoards.h>
#include <PinChangeInterrupt_ATTiny84_All_PinsPins.h>
#include <PinChangeInterrupt_ATTiny84_All_PinsSettings.h>

//-----------------------------------------------------------------------
//Constants

//Pin assignments (Arduino pin numbers).
const byte radioTx = 10;
const byte buttonFanHigh = 9;
const byte buttonFanMed = 0;
const byte buttonFanLow = 1;
const byte buttonFanOff = 2;
const byte buttonLight = 3;
const byte chargePumpA = 8;
const byte chargePumpB = 7;
const byte mosi = 6;
const byte miso = 5;
const byte reset = 11;
const byte usck = 4;

//Button press constants.
const unsigned long debounceDelay = 50;                                  //Duration to ignore pin input on a switch to avoid spurious readings as the voltage bounces.  Long because it gets compared to a long.

const int codeLength = 12;                                               //Transmision code bit length.

const unsigned long powerDownTime = 10000;                               //Duration to leave the system running before powering down.

const unsigned long lightDuration = 500;                                 //Duration to operate the LED when a button is pressed.

const float minVoltage = 2.0;                                            //Threshold for battery charge alert.


//-----------------------------------------------------------------------
//Global variables

RCSwitch transmitter = RCSwitch();                                       //Object for the radio transmitter.

unsigned long lastDebounceTime = 0;                                      //The last time the output pin was toggled.  Unsigned long because the time (milliseconds), will quickly become too large for an int.

unsigned long powerSaveModeTime = 0;                                     //At startup, time = 0.  Updated every time a button is pressed or device is woken up.

byte lastSwitch = 5;                                                     //Identifies the previous button that was pressed.  5 = no button press/cleared.
volatile byte whichSwitch = 5;                                           //Identifies which button was pressed.  5 = no button press/cleared.
volatile bool interruptFlag = false;                                     //Flag to record whether an interrupt has been triggered or not.

unsigned long lightOnTime = 0;                                           //Time that the LED turns on.
unsigned long lightOffTime = 0;                                          //Time that the LED has been off (used when voltage is low to blink LED).
bool lightStatus = false;                                                //LED on, true; LED off, false.

unsigned long loopTime = 0;                                              //Record the start time of each loop to compare against with the timed features.

bool lowVoltageFlag = false;                                             //If low voltage is detected, set this flag.  It will stay until the MCU is reset (ie. batteries changed).


//-----------------------------------------------------------------------
//Setup
void setup(){
  //Set pin modes.
  pinMode (buttonFanHigh, INPUT_PULLUP);
  pinMode (buttonFanMed, INPUT_PULLUP);
  pinMode (buttonFanLow, INPUT_PULLUP);
  pinMode (buttonFanOff, INPUT_PULLUP);
  pinMode (buttonLight, INPUT_PULLUP);

  //Set unused pins to INPUT_PULLUP as recommended by the datasheet.
  pinMode (mosi, INPUT_PULLUP);
  pinMode (miso, INPUT_PULLUP);
  pinMode (reset, INPUT_PULLUP);                                         //We especially want this HIGH so that the MCU doesn't spontaneously reset.
  pinMode (usck, INPUT_PULLUP);

  //Set PWM pin modes.
  pinMode(chargePumpA, OUTPUT);
  pinMode(chargePumpB, OUTPUT);

  chargePumpOff();                                                       //Set initial charge pump state.

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);                                   //Set power management mode.  Power-down is the lowest power state available while using the internal oscillator.
  
  //Set up the transmitter.
  transmitter.enableTransmit(radioTx);                                   //Sets the transmitter pin to OUTPUT via the RCSwitch library.
  transmitter.setProtocol(11);                                           //Transmission protocol.  Determined using an RF Sniffer program.

  //Set up pin change interrupts, which are required to wake from sleep and used to trigger radio transmissions.
  //Note there are two PC registers (0 & 1) on the ATtiny84A that handle different sets of pins.
  attachPinChangeInterrupt(9, readButtonFanHigh, FALLING);
  attachPinChangeInterrupt(0, readButtonFanMed, FALLING);
  attachPinChangeInterrupt(1, readButtonFanLow, FALLING);
  attachPinChangeInterrupt(2, readButtonFanOff, FALLING);
  attachPinChangeInterrupt(3, readButtonLight, FALLING);
}


//-----------------------------------------------------------------------
//Main loop
void loop() {
  loopTime = millis();

  if (interruptFlag) {
    powerSaveModeTime = loopTime;                                        //Restart the power-down timer if a button is pressed.

    checkVoltage();                                                      //Set low voltage flag when low voltage detected.  Will remain on until the MCU is reset (ie. batteries changed).
    
    if (whichSwitch != lastSwitch) {                                     //Reset the debounce timer if a new button press is registered.
      lastSwitch = whichSwitch;
      lastDebounceTime = millis();
    }
/*
 * I think I need to update lastSwitch after successfully going through the switch-case function, otherwise debounce won't be reset correctly?
 */
    if ((loopTime - lastDebounceTime) > debounceDelay) {
      radioOn();                                                         //Radio must be turned on via the procedure in the data sheet. Otherwise it won't transmit the first button press after MCU wake-up.
      chargePumpOn();
      lightOnTime = millis();
      
      switch(whichSwitch) {
        case 4:
          setFanHigh();
          break;
        case 3:
          setFanMed();
          break;
        case 2:
          setFanLow();
          break;
        case 1:
          setFanOff();
          break;
        case 0:
          setLightOnOff();
          break;
        default:
          break;
      }

      radioOff();                                                        //Turn the radio off for power-saving.
      interruptFlag = false;                                             //Clear interrupt flag.  The button press was successful, so don't need to come back into this code again until next button press.
      lastSwitch = 5;                                                    //Button press has been registered.  Reset last switch to an unused value, otherwise lastDebounceTime won't be reset if the same button is pressed again.
    }
  }

  if (lowVoltageFlag) {
    if (lightStatus) {
      if ((loopTime - lightOnTime) > lightDuration) {
        chargePumpOff();
      }
    } else if ((loopTime - lightOffTime) > lightDuration) {
      chargePumpOn();
    }
  } else if ((loopTime - lightOnTime) > lightDuration) {
    chargePumpOff();
  }

  if ((loopTime - powerSaveModeTime) > powerDownTime) {
    powerDown();
  }
}


//-----------------------------------------------------------------------
//Interrupt service routines

void readButtonFanHigh() {
  interruptFlag = true;
  whichSwitch = 4;
}

void readButtonFanMed() {
  interruptFlag = true;
  whichSwitch = 3;
}

void readButtonFanLow() {
  interruptFlag = true;
  whichSwitch = 2;
}

void readButtonFanOff() {
  interruptFlag = true;
  whichSwitch = 1;
}

void readButtonLight() {
  interruptFlag = true;
  whichSwitch = 0;
}


//-----------------------------------------------------------------------
//Methods

//Turn the LED on.
void chargePumpOn() {
  digitalWrite(chargePumpA, HIGH);
  analogWrite(chargePumpB, 127);
  lightStatus = true;
  lightOnTime = millis();
}

//Turn the LED off.
void chargePumpOff() {
  digitalWrite(chargePumpA, LOW);
  analogWrite(chargePumpB, 0);
  lightStatus = false;
  lightOffTime = millis();
}

//Turn the radio on.
void radioOn() {
  noInterrupts();                                                        //delayMicros does not disable interrupts.  We do not want to do anything else until the radio is powered up, there is no point.
  digitalWrite(radioTx, LOW);                                            //Signal to turn the radio on is rising edge then 10ns hold high.
  digitalWrite(radioTx, HIGH);
  delayMicroseconds(3);                                                  //Minimum hold time tHOLD is 10ns.  Then there is about 770us of "don't care" state so holding longer isn't a problem.  3us is the smallest accurate delay possible in Arduino.
  digitalWrite(radioTx, LOW);                                            //Return output to base state.
  delay(1);                                                              //Wait through tXTAL and tTUNE (400us and 370us respectively).  1ms is more convenient and safe than exactly 770us.
  interrupts();                                                          //Re-enable interrupts.
}

//Turn the radio off.
void radioOff() {
  digitalWrite(radioTx, LOW);                                            //Radio will turn off after 90ms of low state on the input pin.  No need for delays, etc. because if the state is changed then the radio is needed to be awake.
}

//Power down the MCU
void powerDown() {
  noInterrupts();                                                        //Prevent interrupts from happening during shutdown, which may disrupt it or not work correctly.
  chargePumpOff();                                                       //Make sure the LED is off before shutting down.
  
  //Need to specifically power down ADC before shutting down, otherwise it will still consume 
  //a couple of hundred uA (source: Spence Kondes ATTiny Core Readme on Github and ATtiny datasheet).
  ADCSRA = 0;                                                            //Turn off ADC
  power_all_disable ();                                                  //Power off ADC, Timers 0 and 1, and the serial interface
  
  sleep_enable();
  interrupts();                                                          //Re-enable interrupts so that the MCU can be woken from sleep.  One further instruction is guaranteed to be executed, which is used to shut it down.
  sleep_cpu();                                                           //Last instruction to be executed.  Powers down the MCU.
  sleep_disable();                                                       //Wakes up the MCU after the interrupt is triggered.
  power_all_enable();                                                    //Power on ADC, Timers 0 and 1, and the serial interface.
  lowVoltageFlag = false;                                                //Reset the low voltage flag to prevent permanent blinky warnings off spurious low voltage detections.
  powerSaveModeTime = millis();                                          //Record the time that it turns on so that the timer can be used to power it down again when.
}

void setFanHigh() {
  transmitter.send(1119, codeLength);
}

void setFanMed() {
  transmitter.send(1135, codeLength);
}

void setFanLow() {
  transmitter.send(1143, codeLength);
}

void setFanOff() {
  transmitter.send(1149, codeLength);
}

void setLightOnOff() {
  transmitter.send(1150, codeLength);
}

void checkVoltage() {
  long voltageReading;

  ADMUX = _BV(MUX5) | _BV(MUX0);                                         //Read 1.1V reference against Vcc
  delay(2);                                                              //Wait for Vref to settle.  Datasheet only requires 1 ms, so this is fine.

  //Start ADC conversion.
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA, ADSC));
  voltageReading  = ADCL;
  voltageReading |= ADCH << 8;

  //Calculate Vcc (in mV).  1126400 = 1.1*1024*1000
  //1.1 is the reference voltage.
  //1024 is the range of results possible on the ADC pin.
  //1000 is the conversion to mV.
  voltageReading = 1126400L / voltageReading;                         

  //Internal Voltage Reference Correction
  //The 1.1V internal reference voltage has a minimum of 1.0 and maximum of 1.2 in the datasheet.
  //Measurements of 5 IC's indicated typical accuracy was around -3.8% to -1.3% at 3.3V input, and
  //-5.5% to -2.9% at 5V input.
  //An equation was found based on the median ratio of internal/multimeter measured voltages for 3.3 and 5 volt setups:
  //Uact = -0.0133Uiref + 1.0307
  float Vcc = voltageReading / 1000.0;                                   //Convert to float and to Volts to make the correction easier (function is based around Volts as the unit).
  Vcc = Vcc * (-0.0133 * Vcc + 1.0307);

  if (Vcc < minVoltage) {
    lowVoltageFlag = true;
  }
}


//-----------------------------------------------------------------------