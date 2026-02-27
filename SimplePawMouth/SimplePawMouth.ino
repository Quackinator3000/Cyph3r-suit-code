#include <Arduino.h>
//#include <arduinoFFT.h>
// PINOUTS  ============================================================

static int RX = 1;
static int TX = 0;

static int BTN_1 = 2;
static int BTN_2 = 3;
static int BTN_3 = 4;
static int BTN_4 = 5;

// TIMING AND DELAYS  ==========================================================

static int DEBO = 175;                    // debounce
static int DPDELAY = 500;              // Double press delay
static int HOLD = 1000;                    // Hold for rave mode
// "now" declared in main

// USEFULL CONSTANTS AND VARIABLES  ============================================
uint ON = LOW;
uint OFF = HIGH;
const int MAXFACES = 12;
// BUTTON OBJ  =================================================================



int options[4] = {BTN_1, BTN_2, BTN_3, BTN_4};

void setup() {
  delay(275);
  Serial.begin(115200);
  Serial1.setTX(TX);
  Serial1.setRX(RX);
  Serial1.begin(115200); // comms with main board 

  pinMode(BTN_1, INPUT_PULLUP);
  pinMode(BTN_2, INPUT_PULLUP);
  pinMode(BTN_3, INPUT_PULLUP);
  pinMode(BTN_4, INPUT_PULLUP);
  // buttons ready and UART started

  Serial1.write(0xF1);
  Serial.println("trying check.");
  delay(100);
  if (Serial1.available()) { // get number of valid faces from head WIP
    static const int num_faces = Serial1.read();
    int FACES[MAXFACES];
    for (int i = 0; i < num_faces; i++) {
      FACES[i] = i + 1; // +1 so I can send that to the head
    }
  }
  Serial.println("setup over");
}

 
int lastbtn = 0;
long int lastpress = 0; // misnomer, treat as last NEW button press
long int holdtime = 0;

void loop() {
  long int now = millis();                     // check for which button pressed, save it as lastbtn and time as lasttime
  for (int i = 0; i < 4; i++) {
    int b = digitalRead(options[i]);
    if (b == ON) {
      if (i != lastbtn) {
        if (now - lastpress > DEBO && now - lastpress < DPDELAY) {               // DOUBLE PRESS??
          Serial1.write(i + 4);
          lastbtn = i;
          while (digitalRead(options[i]) == ON) {
            delay(10);
          }
        }
        else {
          Serial1.write(i);
          lastbtn = i;                                       
        }
        lastpress = now;
      }
      else {
        holdtime = now - lastpress;
      }
      if (holdtime >= HOLD){
        Serial1.write(0xFA);
        while (digitalRead(options[i]) == ON) {
          delay(10);
        }
      }
    }    
  }
}