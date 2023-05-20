/*
  Working version of the Arduino-based precision clock with Multi-function Shield

  Features:
  > Precision clock accuracy
    (limited by the accuracy of the quartz oscillator on the Arduino board)
  > Hourly signal (currently with no option to disable)
  > Every second click to instantly check the accuracy of the clock using the "Clock Tuner" app (currently with no option to disable)
  > Displaying minutes and seconds when button 3 is pressed
  v0.2.3 08.05.2022
*/


// avr-libc library includes
#include <avr/io.h>
#include <avr/interrupt.h>

// shift register pins used for the LED-display
#define LATCH_DIO 4  // data synchronization line, latch
#define CLK_DIO 7    // clock line
#define DATA_DIO 8   // data line

// buttons
#define B_01 A1  // button 1
#define B_02 A2  // button 2
#define B_03 A3  // button 3

// LEDs
#define LED1 13  // LED 1 (reserved for indicating that the alarm clock is on)
#define LED2 12  // LED 2 (reserved to indicate the activation of the hourly signal)
#define LED3 11  // LED 3
#define LED4 10  // LED 4

// buzzer
#define BUZZER 3  // buzzer pin
#define ON LOW    // buzzer on
#define OFF HIGH  // buzzer off

// bit masks of numbers from 0 to 9 (0 - segment off)
const byte SEGMENT_MAP[] = {
  //abcdefgh
  0b00000011,  // 0
  0b10011111,  // 1
  0b00100101,  // 2
  0b00001101,  // 3
  0b10011001,  // 4
  0b01001001,  // 5
  0b01000001,  // 6
  0b00011111,  // 7
  0b00000001,  // 8
  0b00001001,  // 9
};

// bit masks for selecting a display digit from 1 to 4
const byte SEGMENT_SELECT[] = {
  0b10000000,
  0b01000000,
  0b00100000,
  0b00010000
};

// Set the time displayed when you start the clock
volatile byte hd = 0;  // tens of hours
volatile byte he = 0;  // units of hours
volatile byte md = 0;  // tens of minutes
volatile byte me = 0;  // tens of minutes
volatile byte s = 0;   // seconds

// accuracy corrector
// number of counts per second (by default 16624, 15613 for the original Arduino Uno, 15500 and 15510 for RobotDyn board)
const unsigned short CORR = 15510;

// list of display modes
enum displayModeValues {
  MODE_SET_CLOCK_TIME,
  MODE_CLOCK_TIME,
  MODE_SET_ALARM_TIME,
  MODE_ALARM_TIME
};

// current display mode
byte displayMode = MODE_SET_CLOCK_TIME;

void setup() {
  // setting the pins of the indicator registers to the output
  pinMode(LATCH_DIO, OUTPUT);
  pinMode(CLK_DIO, OUTPUT);
  pinMode(DATA_DIO, OUTPUT);

  // setting the operating mode of the button pins
  pinMode(B_01, INPUT);  // pin for button 1 to the input (pull-up resistor on the board) - button for clock increment
  pinMode(B_02, INPUT);  // pin for button 2 to the input (pull-up resistor on the board) - button for minutes increment
  pinMode(B_03, INPUT);  // pin for button 2 to the input (pull-up resistor on the board) - button to start the clock / view the seconds

  // LED pins to output
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);

  // buzzer pin to output
  pinMode(BUZZER, OUTPUT);

  // extinguishing all LEDs
  digitalWrite(LED1, OFF);
  digitalWrite(LED2, OFF);
  digitalWrite(LED3, OFF);
  digitalWrite(LED4, OFF);

  // Timer1 initialization
  cli();       // disable global interrupts
  TCCR1A = 0;  // set the registers to 0
  TCCR1B = 0;

  OCR1A = CORR;            // Set the Match Register to the corrector value (default is 15624)
  TCCR1B |= (1 << WGM12);  // Switching to CTC (Clear Timer on Compare) mode

  // Setting bits CS10 and CS12 to a division factor of 1024
  TCCR1B |= (1 << CS10);
  TCCR1B |= (1 << CS12);

  TIMSK1 |= (1 << OCIE1A);  // enabling interruptions by coincidence
  sei();                    // enable global interrupts
}

void loop() {
  switch (displayMode) {
    // current time setting mode
    case MODE_SET_CLOCK_TIME:
      while (digitalRead(B_03) != LOW) {
        if (digitalRead(B_01) == LOW) {
          if (hd == 2) {
            if (he > 2) {
              he = 0;
              hd = 0;
            } else {
              he++;
            }
          } else if (he == 9) {
            he = 0;
            hd++;
          } else {
            he++;
          }
          for (int k = 0; k < 300; k++) {
            digitalWrite(LATCH_DIO, LOW);
            shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_MAP[hd]);
            shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_SELECT[0]);
            digitalWrite(LATCH_DIO, HIGH);

            digitalWrite(LATCH_DIO, LOW);
            shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_MAP[he]);
            shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_SELECT[1]);
            digitalWrite(LATCH_DIO, HIGH);

            digitalWrite(LATCH_DIO, LOW);
            shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_MAP[md]);
            shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_SELECT[2]);
            digitalWrite(LATCH_DIO, HIGH);

            digitalWrite(LATCH_DIO, LOW);
            shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_MAP[me]);
            shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_SELECT[3]);
            digitalWrite(LATCH_DIO, HIGH);
          }
        }


        if (digitalRead(B_02) == LOW) {
          if (me == 9) {
            me = 0;
            if (md == 5) {
              md = 0;
            } else {
              md++;
            }
          } else {
            me++;
          }
          for (int k = 0; k < 300; k++) {
            digitalWrite(LATCH_DIO, LOW);
            shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_MAP[hd]);
            shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_SELECT[0]);
            digitalWrite(LATCH_DIO, HIGH);


            digitalWrite(LATCH_DIO, LOW);
            shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_MAP[he]);
            shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_SELECT[1]);
            digitalWrite(LATCH_DIO, HIGH);


            digitalWrite(LATCH_DIO, LOW);
            shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_MAP[md]);
            shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_SELECT[2]);
            digitalWrite(LATCH_DIO, HIGH);


            digitalWrite(LATCH_DIO, LOW);
            shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_MAP[me]);
            shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_SELECT[3]);
            digitalWrite(LATCH_DIO, HIGH);
          }
        }

        digitalWrite(LATCH_DIO, LOW);
        shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_MAP[hd]);
        shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_SELECT[0]);
        digitalWrite(LATCH_DIO, HIGH);


        digitalWrite(LATCH_DIO, LOW);
        shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_MAP[he]);
        shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_SELECT[1]);
        digitalWrite(LATCH_DIO, HIGH);


        digitalWrite(LATCH_DIO, LOW);
        shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_MAP[md]);
        shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_SELECT[2]);
        digitalWrite(LATCH_DIO, HIGH);


        digitalWrite(LATCH_DIO, LOW);
        shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_MAP[me]);
        shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_SELECT[3]);
        digitalWrite(LATCH_DIO, HIGH);
      }
      s = 0;
      displayMode = MODE_CLOCK_TIME;
      break;


    // current time mode
    case MODE_CLOCK_TIME:
      if (digitalRead(B_03) == LOW) {
        digitalWrite(LATCH_DIO, LOW);
        shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_MAP[md]);
        shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_SELECT[0]);
        digitalWrite(LATCH_DIO, HIGH);

        digitalWrite(LATCH_DIO, LOW);
        shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_MAP[me]);
        shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_SELECT[1]);
        digitalWrite(LATCH_DIO, HIGH);

        digitalWrite(LATCH_DIO, LOW);
        shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_MAP[s / 10]);
        shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_SELECT[2]);
        digitalWrite(LATCH_DIO, HIGH);

        digitalWrite(LATCH_DIO, LOW);
        shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_MAP[s % 10]);
        shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_SELECT[3]);
        digitalWrite(LATCH_DIO, HIGH);
      } else {
        digitalWrite(LATCH_DIO, LOW);
        shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_MAP[hd]);
        shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_SELECT[0]);
        digitalWrite(LATCH_DIO, HIGH);

        digitalWrite(LATCH_DIO, LOW);
        shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_MAP[he] - 1);  // turning on the dividing point LED in the tens of hours digit
        shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_SELECT[1]);
        digitalWrite(LATCH_DIO, HIGH);

        digitalWrite(LATCH_DIO, LOW);
        shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_MAP[md]);
        shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_SELECT[2]);
        digitalWrite(LATCH_DIO, HIGH);

        digitalWrite(LATCH_DIO, LOW);
        shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_MAP[me]);
        shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_SELECT[3]);
        digitalWrite(LATCH_DIO, HIGH);
      }
      break;
  }
  // switching to the current time setting mode by simultaneously pressing buttons 1 and 2
  if (!digitalRead(B_01) && !digitalRead(B_02)) {
    displayMode = MODE_SET_CLOCK_TIME;
    for (int k = 0; k < 300; k++) {
      digitalWrite(LATCH_DIO, LOW);
      shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_MAP[hd]);
      shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_SELECT[0]);
      digitalWrite(LATCH_DIO, HIGH);

      digitalWrite(LATCH_DIO, LOW);
      shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_MAP[he]);
      shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_SELECT[1]);
      digitalWrite(LATCH_DIO, HIGH);

      digitalWrite(LATCH_DIO, LOW);
      shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_MAP[md]);
      shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_SELECT[2]);
      digitalWrite(LATCH_DIO, HIGH);

      digitalWrite(LATCH_DIO, LOW);
      shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_MAP[me]);
      shiftOut(DATA_DIO, CLK_DIO, LSBFIRST, SEGMENT_SELECT[3]);
      digitalWrite(LATCH_DIO, HIGH);
    }
  }

  // hourly signal
  if (s == 0 && md == 0 && me == 0 && millis() % 1000 == 0)
  {
    digitalWrite(BUZZER, ON);1
    delay(10);
    digitalWrite(BUZZER, OFF);
  }
}


ISR(TIMER1_COMPA_vect)  // interrupt handler, clocking the running of the current time
{
  if (displayMode != MODE_SET_CLOCK_TIME) s++;
  digitalWrite(BUZZER, ON);  // buzzer on (for every second click)
  if (s == 60) {
    s = 0;
    if (me == 9) {
      me = 0;
      if (md == 5) {
        md = 0;
        if (hd == 2) {
          if (he == 3) {
            he = 0;
            hd = 0;
          } else
            he++;
        } else if (he == 9) {
          he = 0;
          hd++;
        } else
          he++;
      } else
        md++;
    } else
      me++;
  }
  digitalWrite(BUZZER, OFF);  // buzzer off
}
