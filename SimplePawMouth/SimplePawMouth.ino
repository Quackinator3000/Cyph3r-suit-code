#include <Arduino.h>

// PINOUTS  ============================================================

static int RX = 17;
static int TX = 16;

static int BTN_1 = 18;
static int BTN_2 = 19;
static int BTN_3 = 20;
static int BTN_4 = 21;

// TIMING AND DELAYS  ==========================================================

static int DEBO = 175;                    // debounce
static int DPDELAY = 333;                 // Double press delay
long int now = 0;                         // what time is it?

// USEFULL CONSTANTS AND VARIABLES  ============================================
uint ON = LOW;
int lastbutton = 0;
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

 
int lastbtn = 1;
long int lastpress = 0;

void loop() {
  long int now = millis();
  // cehck for which button pressed, save it as lastbtn and time as lasttime
  
  for (int i = 0; i < 4; i++) {
    if (digitalRead(options[i]) == ON && now - lastpress > DEBO) {            // if pressed & valid (not blocked by DEBO)
      if (lastbtn == i && now - lastpress < DPDELAY ) {                        // if DP time still alive, then DB
        Serial1.write(i + 4);                                                  // i + 4 to access past the first 4 values
      }
      else if (digitalRead(options[i]) == ON) {
        Serial1.write(i);
      }
      lastbtn = i;
      lastpress = now;
    }
  }


}