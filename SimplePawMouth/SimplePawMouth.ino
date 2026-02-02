/*
 LEFT PAW / EYES!!!!!!

------------------------------------------------------
PINOUTS AND CONSTANTS
------------------------------------------------------
*/

const uint8_t M_NORM  = 0x01;  //           00000001
const uint8_t M_SAD  =  0x02; //            00000010
const uint8_t M_MAD = 0x03;   //            00000011
const uint8_t M_BLUSH  = 0x04; //           00000100

bool IL = 0xff;

const uint8_t BTN_4 = 5;
const uint8_t BTN_3 = 4;
const uint8_t BTN_2 = 0;
const uint8_t BTN_1 = 2;

const int DEBOUNCE = 180;

const bool IDLE = HIGH;

void printFlags(uint8_t flags);
// ---------------------------------------------------SETUP LOOP--------------------------------------------------
void setup() {  
  delay(1000);
  Serial1.begin(38400);

  pinMode(BTN_1, INPUT_PULLUP);
  pinMode(BTN_2, INPUT_PULLUP);
  pinMode(BTN_3, INPUT_PULLUP);
  pinMode(BTN_4, INPUT_PULLUP);

  pinMode(LED_BUILTIN, OUTPUT);
}
long int lastsend = 0;
int sel = 0;
uint8_t flags = 0x00; 
//----------------------------------------------------MAIN LOOP----------------------------------------------------
void loop() {
  sel = 0;
  long int now = millis();
  uint8_t flagvalues[4] = {M_NORM, M_SAD, M_MAD, M_BLUSH};
  int button[4] = {BTN_1, BTN_2, BTN_3, BTN_4};
  bool ON = LOW;

  for (int i = 0; i < 4; i++) {
    if (digitalRead(button[i]) == ON && (now - lastsend) > DEBOUNCE){
      digitalWrite(LED_BUILTIN, HIGH);
      lastsend = now;

      sel = flagvalues[i];
      Serial1.write(sel);
      delay(50);
      digitalWrite(LED_BUILTIN, LOW);

    }
  }
}

