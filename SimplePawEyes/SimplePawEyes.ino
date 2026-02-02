#include <Arduino.h>
/*
 LEFT PAW / EYES!!!!!!

------------------------------------------------------
PINOUTS AND CONSTANTS
------------------------------------------------------
*/

const uint8_t E_NORM = 1;
const uint8_t E_SAD = 2; 
const uint8_t E_MAD = 3; 
const uint8_t E_FISH = 4;

const uint8_t IL = 0xff; // Are you alive?
const uint8_t LI = 0xfe; // I'm alive.

const uint8_t BTN_4 = 21;
const uint8_t BTN_3 = 20;
const uint8_t BTN_2 = 18;
const uint8_t BTN_1 = 19;

const int DEBOUNCE = 180;
const long int PING = 200; // when I force a keep alive ping
const bool IDLE = HIGH;

void printFlags(uint8_t flags);

/*
--------------------------------------------------------------------------------------------------------------------------
So here's the deal, I want to make it so I CAN use multiple taps to attach a different face / change modes.
The problem is I don't actually know how to do that. I imagine it's like,
- while now - lastpress > debounce; SKIP BUTTONS
- when button release; start double click timer (seperate from debounce)
- when DCLK timer hits ~200ms; no longer double click candidate.
- else; button 2nd hit? something like oh IDK... 0x01 -> 0x02??
- on second hit i + 4
- same button twice?

idea...  each button is a class that contains
- is pressed
- last press (= to now)
so like.. if button i == LOW && last press < 200ms ago, it's a double click!
so then Serial.write i + 4? (this would send 4, not 5)
*/

class Btn {

  bool pressed;
  long int lastpress;

  Btn(
    bool btn,
    long int clk,)
    : pressed(btn),
      lastpress(clk)
  {}
}

Btn button1(
  false,
  0,
);

Btn button2(
  false,
  0,
);

Btn button3(
  false,
  0,
);

Btn button4(
  false,
  0,
);

// ---------------------------------------------------SETUP LOOP--------------------------------------------------
void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println("setting pins...");
  Serial1.setRX(17);
  Serial1.setTX(16);
  Serial.println("pins set, starting UART...");
  Serial1.begin(115200);
  Serial.println("Good to go!");

  pinMode(BTN_1, INPUT_PULLUP);
  pinMode(BTN_2, INPUT_PULLUP);
  pinMode(BTN_3, INPUT_PULLUP);
  pinMode(BTN_4, INPUT_PULLUP);

  pinMode(LED_BUILTIN, OUTPUT);
}

long int lastsend = 0;
int sel = 0;
uint8_t flags = 0;
bool islive = true; 
long int lastping = 0;
const char* const* buttons[&button1, &button2, &button3, &button4];
uint8_t flagvalues[4] = {E_NORM, E_SAD, E_MAD, E_FISH};
int button[4] = {BTN_1, BTN_2, BTN_3, BTN_4};
bool ON = LOW;
int curpress = 0;

//----------------------------------------------------MAIN LOOP----------------------------------------------------
void loop() {
  sel = 0;
  long int now = millis();

  if (Serial1.available()) { // check for heartbeat request
    uint8_t b = Serial1.read();
    if (b == 0xff) {
      Serial1.write(0xFE);
      islive = true;
    }
  }

  if (islive) {  // IF we know connection good, send input
    for (int i; i < 4; i++) {
      if (button[i] == ON && (now - lastsend) > DEBOUNCE) {
        Btn b = buttons[i]; // SELECTS THE BUTTON OBJECT I WANT TO PULL DATA FROM
        lstpr = b->lastpress; // load this data
        ispr = b->pressed; // load this data
        if ()
      }
    }
  }
  else { // we know the connection is NOT good so we send a ping
    Serial.write(0xfe);
    lastsend = now;
    delay(5);
    uint8_t b = Serial.read();
    if(b == 0xff){
      islive = true;
    }
  }
  
  
  if ((now - lastsend) >= PING) { // if we are live but havn't pinged in a while
    Serial.write(0xfe);
    delay(5);
    uint8_t b = Serial.read();
    if(b == 0xff){
      islive = true;
    }
  }
  digitalWrite(LED_BUILTIN, LOW);
}

