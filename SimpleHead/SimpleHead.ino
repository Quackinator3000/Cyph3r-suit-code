#include <Arduino.h>
#include <LedControl.h>
#include "FastIMU.h"
#include <Wire.h>

/*
void blankFaceTemplate() {
  static const char* leftMouth[FACE_HEIGHT] = {
    "................................",
    "................................",
    "................................",
    "................................",
    "................................",
    "................................",
    "................................",
    "................................"
  };

  static const char* leftEye[FACE_HEIGHT] = {
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................"
  };

  clearFace();
  renderLeftMouthSeries();
renderRightMouthSeries();(leftMouth);
  renderLeftEyeSeries();
renderRightEyeSeries();(leftEye);
}


  TO DO LIST!!!!
    - add a microphone (easy peasy)
    - add audio waveform (HOW THE FUCK?)
    - light sensors for idle animation and boop detector (cute)
    - mouth animation arrays?
    - consider RFID antenna in nose for blush effect for the right people.
    
    
    - GET A BASE KIT! (Be open to refactoring for 1x3 mouth sections if needed! shorter nose = more cute)
    - consider single nose panel for heart shaped nose. (OG-plan, sticker [???])
    - consider wifi adaptations? (wired/wireless hotswap would be badass!) 2027 plans!
*/





// Some hex codes for controlling the face.

// MOUTH LEFT PINOUT
const int LDIN_PIN = 2;  
const int LCLK_PIN = 3;  
const int LCS_PIN  = 4;   

// MOUTH RIGHT PINOUT
const int RDIN_PIN = 10;
const int RCS_PIN = 11;
const int RCLK_PIN = 14;

// DIMENSIONS
const int FACE_HEIGHT = 8;
const int FACE_COUNT = 5; // the number of face arrays I have made.
const int MOUTH_WIDTH  = 64; 
const int EYE_WIDTH    = 16;  
const int EYES_WIDTH   = EYE_WIDTH * 2;  
const int FULL_WIDTH   = MOUTH_WIDTH + EYES_WIDTH;

const int LMOUTH_DEVS = 4;
const int LEYE_DEVS   = 2;
const int LCHAIN_DEVS = LMOUTH_DEVS + LEYE_DEVS; // 6

const int RMOUTH_DEVS = 4;
const int REYE_DEVS   = 2;
const int RCHAIN_DEVS = RMOUTH_DEVS + REYE_DEVS; // 6

// Here is where I put constants for the animations and UART comms as I need them.
const long int BLINK = 5000;
const int DC = 250;

// LEFT: mouth+eye in series on the LEFT mouth pins
LedControl Lchain(LDIN_PIN, LCLK_PIN, LCS_PIN, LCHAIN_DEVS);

// RIGHT: mouth+eye in series on the RIGHT mouth pins
LedControl Rchain(RDIN_PIN, RCLK_PIN, RCS_PIN, RCHAIN_DEVS);

// face[y][x] => y = 0..7, x = 0..95
byte face[FACE_HEIGHT][FULL_WIDTH];

/*
--------------------------------------------------------------------------------------------
ERROR FACE IF I DON'T PUT THIS HERE THEN THE COMPILER COMPLAINS ABOUT SCOPE OR SUM SHIZ
(I KNOW WHY)
--------------------------------------------------------------------------------------------
*/
static const char* ER_MOUTH[FACE_HEIGHT] = {
  "###############.#######..######.",
  "##......##.....###.....####..###",
  "##......##.....###.....###....##",
  "##......##.....###.....###....##",
  "#######.#######.#######.##....##",
  "##......####....####....##....##",
  "##......##..##..##..##..###..###",
  "#########.....####....##.######."
};

static const char* ER_EYES[FACE_HEIGHT] = {
  "...##......##...",
  "...##......##...",
  "...##......##...",
  "...##......##...",
  "...##......##...",
  "................",
  "...##......##...",
  "...##......##..."
};

/*
TIME TO GET REAL!!! I made this class when I realized I needed more scalability. Each "face" 
or whatever I'm gonna call it will hold the arrays associated with that expression or face.
*/

// AI WRITTEN CLASS BECAUSE I'M TOO DUMB FOR CLASSES WITH ARRAYS AND LOTS OF FLASH STORAGE AND POINTER WEIRDNESS!!!!

class Face {
public:
  // ---- Configuration ---- AI
  bool canBlink;
  bool canTalk;
  const char* const* mouth;
  const char* const* eye;
  const char* const* blinkA;
  const char* const* blinkB;
  const char* const* talkA;
  const char* const* talkB;
  const char* const* talkC;

  // ---- Constructor ----  AI
  Face(bool blink,
       bool talk,
       const char* const* m,
       const char* const* e,
       const char* const* bA,
       const char* const* bB,
       const char* const* tA, 
       const char* const* tB,
       const char* const* tC
      )
    : canBlink(blink),
      canTalk(talk),
      mouth(m),
      eye(e),
      blinkA(bA),
      blinkB(bB),
      talkA(tA),
      talkB(tB),
      talkC(tC)
  {
    // Defensive safety (optional but good) AI
    if (!mouth)  mouth  = ER_MOUTH;
    if (!eye)    eye    = ER_EYES;
  
    if (!blinkA || !blinkB) {
      canBlink = false;
    }
    if (!talkA || !talkB || !talkC) {
      canTalk = false;
    }
  }
};

// Looking at it now, it's exactly what I wanted with the fallback values and everything. 
// I guess the issue was with how I was using "static" in the definitions and it would be
// making things not work right? Because pointers and stuff...
// forgot to tell teh AI I wanted to hold the speaking animations here (future features)






/*
---------------------------------------------------------------------
BUFFER LOADER CODE (WTF GPT)
---------------------------------------------------------------------
*/

// leftHalfMouth is 32 chars wide. We mirror it into the 64-wide mouth region.
void loadMirroredMouthFromLeft32(const char* const* leftHalfMouth) {
  for (int y = 0; y < FACE_HEIGHT; y++) {
    for (int x = 0; x < 32; x++) {
      if (leftHalfMouth[y][x] == '#') {
        int leftX  = x;       // 0..31
        int rightX = 63 - x;  // 63..32
        face[y][leftX]  = 1;
        face[y][rightX] = 1;
      }
    }
  }
}

// leftEye is 16 chars wide. We place it in x=64..79 and mirror into x=80..95.
void loadMirroredEyesFromLeft16(const char* const* leftEye) {
  for (int y = 0; y < FACE_HEIGHT; y++) {
    for (int x = 0; x < EYE_WIDTH; x++) { // 0..15
      if (leftEye[y][x] == '#') {
        int leftGlobal  = MOUTH_WIDTH + x;                      // 64..79
        int rightGlobal = (MOUTH_WIDTH + EYES_WIDTH - 1) - x;   // 95..80
        face[y][leftGlobal]  = 1;
        face[y][rightGlobal] = 1;
      }
    }
  }
}


//  -----------------------------------------------------------------
// Buffer helpers (STARTED AI BUT DID A REWRITE OR 2 SO YEAH)
//  -----------------------------------------------------------------

void clearMouth() {
  for (int y = 0; y < FACE_HEIGHT; y++) {
    for (int x = 0; x < MOUTH_WIDTH; x++) {
      face[y][x] = 0;
    }
  }
}

void clearEyes() {
  for (int y = 0; y < FACE_HEIGHT; y++) {
    for (int x = MOUTH_WIDTH; x < FULL_WIDTH; x++) {
      face[y][x] = 0;
    }
  }
}

void clearFace() {
  clearMouth();
  clearEyes();
}

void fillFace() {
  int fillin = 8; // fill my arrays :D
  for (int x = 0; x < FULL_WIDTH; x++) { // left / right
    if (x == 32) { 
      x = MOUTH_WIDTH - 1;
      // forgot why I put this x=width-1 here but everytime I put something stupid in this code it's been
      // for a good reason and stuff breaks when I change it so this just lives here now
    }
    else { 
      for (int y = 0; y < FACE_HEIGHT; y += fillin) { // top / bottom
        for (int xx = y; xx < (y + fillin); xx++) { // fill in y 1-8 all at once.
          if (x < MOUTH_WIDTH) {
            face[xx][63 - x] = 1;
            face[xx][x] = 1;
          }
          else if ((x + 16) > FULL_WIDTH) {
            return;
          }
          else {
            int i = 192; // double the total length to do integer bs and make the eyes mirror properly
            int n = x + 32; // offset to mirror the eyes
            face[xx][x] = 1;
            face[xx][i - n] = 1;
          }
        }
        renderFaceSeries();
      }
    }
  }
}




//  -----------------------------------------------------------------
// Rendering (MOSTLY AI. EVERY OTHER PANEL WAS MIRRORED SO HELL NO!)
//  -----------------------------------------------------------------

// ---------- render functions ----------

void renderLeftMouthSeries() {
  // mouth left = x 0..31 => Lchain dev 0..3
  for (int y = 0; y < FACE_HEIGHT; y++) {
    for (int x = 0; x < 32; x++) {
      bool on = (face[y][x] == 1);
      setLedLeft(/*devBase=*/0, /*xWithinGroup=*/x, y, on);
    }
  }
}

void renderLeftEyeSeries() {
  // left eye source: global x 64..79 (16 wide)
  // rotate 180° within 16x8: (x,y)->(15-x,7-y)
  for (int y = 0; y < FACE_HEIGHT; y++) {
    for (int x = 0; x < 16; x++) {
      int gx = MOUTH_WIDTH + x;          // 64..79
      bool on = (face[y][gx] == 1);

      int outX = 15 - x;
      int outY = 7 - y;

      setLedLeft(/*devBase=*/LMOUTH_DEVS, /*xWithinGroup=*/outX, outY, on); // devBase=4
    }
  }
}

void renderRightMouthSeries() {
  // mouth right = x 32..63 => Rchain dev 0..3
  for (int y = 0; y < FACE_HEIGHT; y++) {
    for (int x = 32; x < 64; x++) {
      int localX = x - 32;                 // 0..31
      bool on = (face[y][x] == 1);
      setLedRight(/*devBase=*/0, /*groupDevs=*/RMOUTH_DEVS, /*xWithinGroup=*/localX, y, on);
    }
  }
}

void renderRightEyeSeries() {
  // right eye source: global x 80..95 (16 wide)
  // rotate 180° within 16x8: (x,y)->(15-x,7-y)
  for (int y = 0; y < FACE_HEIGHT; y++) {
    for (int x = 0; x < 16; x++) {
      int gx = MOUTH_WIDTH + 16 + x;    // 80..95
      bool on = (face[y][gx] == 1);

      int outX = 15 - x;
      int outY = 7 - y;

      // IMPORTANT: keep your right-side helper as-is (it already handles device order/orientation).
      // We just feed it the rotated coords within the 16-wide group.
      setLedRight(/*devBase=*/RMOUTH_DEVS, /*groupDevs=*/REYE_DEVS, /*xWithinGroup=*/outX, outY, on); // devBase=4, group=2
    }
  }
}

void renderFaceSeries() {
  renderLeftMouthSeries();
  renderLeftEyeSeries();
  renderRightMouthSeries();
  renderRightEyeSeries();
}
void renderEyes() {
  renderRightEyeSeries();
  renderLeftEyeSeries();
}

void renderMouth() {
  renderRightMouthSeries();
  renderLeftMouthSeries();
}

//  -----------------------------------------------------------------------
// Pattern loaders (THIS SECTION WAS AI GENERATED CAUSE F*CK INTEGER ARITH-
// Segmentation fault, core dumped.
//  -----------------------------------------------------------------------

// ---------- low-level helpers ----------

// Left-side mapping: straightforward (matches your old Lmouthlc + eyelc)
static inline void setLedLeft(int devBase, int xWithinGroup, int y, bool on) {
  int dev = devBase + (xWithinGroup / 8);
  int col = 7 - (xWithinGroup % 8);
  Lchain.setLed(dev, y, col, on);
}

// Right-side mapping: flip module order + 180° rotate inside each 8x8
static inline void setLedRight(int devBase, int groupDevs, int xWithinGroup, int y, bool on) {
  int devInGroup = (xWithinGroup / 8);      // 0..groupDevs-1
  int colInDev   = (xWithinGroup % 8);      // 0..7

  int outDev = devBase + ((groupDevs - 1) - devInGroup); // reverse device order
  int outRow = 7 - y;                                    // rotate 180°
  int outCol = colInDev;                            

  Rchain.setLed(outDev, outRow, outCol, on);
}

/*
-------------------------------------------------------------------------------------------------
SPECIAL MOUTH ONLY ARRAYS (DEPRICATED!!!!)
-------------------------------------------------------------------------------------------------
*/

void sadMouth() {
  clearMouth();
    static const char* leftMouth[FACE_HEIGHT] = {
    "..............########......####",
    "............############..#####.",
    "...........####.....##########..",
    "..........###.........######....",
    ".........###....................",
    "........###.....................",
    "........##......................",
    "........##......................"
  };
  renderLeftMouthSeries();
  renderRightMouthSeries();(leftMouth);
}


void angryMouth() {
  clearMouth();
    static const char* leftMouth[FACE_HEIGHT] = {
    "................................",
    "....######....##......#.........",
    "...#####.#...####....###....##..",
    "..###....##.##..##..#####..####.",
    "..##......###...######..####..##",
    ".##.......##.....##.....###....#",
    ".#..............................",
    "................................"
  };
  renderLeftMouthSeries();
  renderRightMouthSeries();(leftMouth);
}

/*
---------------------------------------------------------------------------------------------
NEUTRAL FACE (DEFAULT) (LEGACY) (ORIGINAL) (GO-TO) (CLASSIC) (MORE BS PADDING TO FILL THIS O-
^ Segmentation fault, core dumped :3
---------------------------------------------------------------------------------------------
*/
static const char* const N_MOUTH[FACE_HEIGHT] = {
  "##..............................",
  ".######.........................",
  "...###.............####.......##",
  "....####.........######......###",
  "......###......#####.###...####.",
  ".......####.######....##.####...",
  "........#########.....######....",
  "..........#####........####....."
};

static const char* const N_EYES[FACE_HEIGHT] = {
  ".....#####......",
  "....#######.....",
  "..###########...",
  ".#############..",
  ".##############.",
  "#####......#####",
  "###...........##",
  "##.............#"
};

static const char* const N_BA[FACE_HEIGHT] = {
  "................",
  "................",
  "................",
  "......####......",
  "....########....",
  ".####......####.",
  "###...........##",
  "##.............#"
};

static const char* const N_BB[FACE_HEIGHT] = {
  "................",
  "................",
  "................",
  "................",
  ".....######.....",
  "..###......###..",
  ".##...........##",
  "##.............#"
};
/*
---------------------------------------------------------------------------------------------
FISH EYES (LMAOOOOO)
If we never really fix the memory leaks then they will keep needing us to "fix" the code.
#JobSecurty #HustleMindset
---------------------------------------------------------------------------------------------
*/

static const char* const FE_MOUTH[FACE_HEIGHT] = { // FE = Fish Eyes
  "................................",
  "................................",
  ".............................###",
  "...........................#####",
  "..........................######",
  "........................######..",
  ".......................#####....",
  ".......................####....."
};

static const char* const FE_EYES[FACE_HEIGHT] = {
  "......####......",
  ".....#....#.....",
  "....#......#....",
  "....#......#....",
  "....#......#....",
  "....#......#....",
  ".....#....#.....",
  "......####......"
};

static const char* const FE_BA[FACE_HEIGHT] = {
  "................",
  "................",
  ".....######.....",
  "...##......##...",
  "...##......##...",
  ".....######.....",
  "................",
  "................"
};

static const char* const FE_BB[FACE_HEIGHT] = {
  "................",
  "................",
  "................",
  ".....######.....",
  "...##########...",
  "................",
  "................",
  "................"
};
/*
------------------------------------------------------------------------------------
OWO FACE (JUST FE_EYES + SPECIAL W MOUTH) 
------------------------------------------------------------------------------------
*/
static const char* const OWO_MOUTH[FACE_HEIGHT] = {
  "..........######.............###",
  "..........##..###..........####.",
  "...............###........###...",
  "................###......###....",
  ".................###....###.....",
  "..................###..###......",
  "...................######.......",
  "....................####........"
};

static const char* const OWO_EYES[FACE_HEIGHT] = {
  "....########....",
  "..##........##..",
  ".#............#.",
  "#..............#",
  "#..............#",
  ".#............#.",
  "..##........##..",
  "....########...."
};

static const char* const OWO_BA[FACE_HEIGHT] = {
  "................",
  "................",
  "...##########...",
  "###..........###",
  "###..........###",
  "...##########...",
  "................",
  "................."
};

static const char* const OWO_BB[FACE_HEIGHT] = {
  "................",
  "................",
  "................",
  "................",
  "###..........###",
  "...##########...",
  "................",
  "................",
};
/*
-----------------------------------------------------------------------------------------
BLUSH FACE (AWWW, HE'S IN LUUUUVVVV)
-----------------------------------------------------------------------------------------
*/
static const char* const BL_MOUTH[FACE_HEIGHT] = {
  "#.#.#.#.#.......####............",
  "#.#.#.#.#.........##.....#...#..",
  ".#.#.#.#.#.........##....#..###.",
  ".#.#.#.#.#..........##..###.#.##",
  "..#.#.#.#.#..........##.#####..#",
  "..#.#.#.#.#...........###.##....",
  "...#.#.#.#.#....................",
  "...#.#.#.#.#...................."
};

  static const char* const BL_EYES[FACE_HEIGHT] = {
  "..............##",
  ".............###",
  "..........######",
  "......##########",
  "....###########.",
  "...###########..",
  "...##########...",
  "....#######....."
};
 
  static const char* const BL_BA[FACE_HEIGHT] = {
  "................",
  "................",
  "..............##",
  "............###.",
  "........######..",
  ".....#######....",
  "...#######......",
  "....####........"
};
  
  static const char* const BL_BB[FACE_HEIGHT] = {
  "................",
  "................",
  "................",
  "................",
  "...........####.",
  "........#####...",
  "...######........",
  "................."
};
 


void angryEyes() {
  clearEyes();
  static const char* leftEye[FACE_HEIGHT] = {
    "##..............",
    "####............",
    "#######.........",
    "..########......",
    "....########....",
    "......########..",
    "........########",
    "..........######"
  };
  renderLeftEyeSeries();
  renderRightEyeSeries();
}



/*
----------------------------------------------------------------------
DECLARATIONS FOR FACE ARRAYS AS CLASSES!!!! (SCALABLE)
----------------------------------------------------------------------
*/

Face neutralFace(
  true,
  false,          // for the nullptr's
  N_MOUTH,
  N_EYES,
  N_BA,
  N_BB,        
  nullptr,       // placeholder null pointers (NO TALKING ANIMATIONS AVAILABLE YET)
  nullptr,
  nullptr
);

Face fishEyes( // surprise
  true,
  false,
  FE_MOUTH,
  FE_EYES,
  FE_BA,
  FE_BB,
  nullptr,
  nullptr,
  nullptr
);

Face blushFace( // Repurposes the original sad eyes and the original OWO mouth.
  true,
  false,
  BL_MOUTH,
  BL_EYES,
  BL_BA,
  BL_BB,
  nullptr,
  nullptr,
  nullptr
);

Face owoFace(
  true,
  false,
  OWO_MOUTH,
  OWO_EYES,
  OWO_BA,
  OWO_BB,
  nullptr,
  nullptr,
  nullptr
);

Face ERROR(
  false,
  false,
  ER_MOUTH,
  ER_EYES,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr
);
/*-
 --------------------------------------------
CODE FOR BUTTON CONTROLS!
 --------------------------------------------
*/
Face* classes[] = {
  &ERROR, &neutralFace, &fishEyes, &blushFace, &owoFace
};
// thinking out loud memo, seeing as I already have the 2 different inputs on their own UART lines, I can just send the same 0-3 and DIRRETLY 
// index the array. Total hack, but better to just get this concept up and running asap than spend TOO much time optimizing a MVP.

Face* selectFace(int i) {
  if (i + 1 > FACE_COUNT) {i = i % FACE_COUNT;}
  else if (i < 0) {i = 0;}
  return classes[i];
}
void mouthToBuffer(int i) {
  Face* f = selectFace(i);
  clearMouth();
  loadMirroredMouthFromLeft32(f->mouth);
}

void eyeToBuffer(int i) {
  Face* f = selectFace(i);
  clearEyes();
  loadMirroredEyesFromLeft16(f->eye);
}

void faceToBuffer(int i) {
  Face* f = selectFace(i);
  clearFace();
  loadMirroredMouthFromLeft32(f->mouth);
  loadMirroredEyesFromLeft16(f->eye);
}
void blinkToBuffer(int i) {
  Face* f = selectFace(i);
  clearEyes();  
  loadMirroredEyesFromLeft16(f->blinkA);
  renderLeftEyeSeries();
  renderRightEyeSeries();
  renderEyes();
  clearEyes();
  loadMirroredEyesFromLeft16(f->blinkB); 
  renderLeftEyeSeries();
  renderRightEyeSeries();
  renderEyes();
  clearEyes();
  loadMirroredEyesFromLeft16(f->blinkA);
  renderLeftEyeSeries();
  renderRightEyeSeries();
  renderEyes(); 
}
bool checkTrue(int i) {
  Face* f = selectFace(i);
  return f->canBlink;
}

/*
/////////////////////////////////////////////////////////////////////
AUTOMATIC BRIGHTNESS CONTROLLER
/////////////////////////////////////////////////////////////////////
*/

int setBrightness(int b);




/*
void startAnimation() {
  fillFace();
  renderFace();
  for (int y = 0; y < FACE_HEIGHT; y++){
    for (int x = 0; x < FULL_WIDTH; x++) {
      face[y][x] = 0;
      if (x < MOUTH_WIDTH) {
        renderMouth();
      }
      else {
        renderEyes();
      }
    }
  }
}
*/




// important variables I need gloablly cause I suck at this
static int selected_eyes = 5;
static int selected_mouth = 5;



bool isliveA = false;
bool isliveB = false;
int lvl = 15;
const int BRIGHTNESS = 26;
//----------------------------SETUP LOOP\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\///
void setup() {
  Wire.setSDA(16);
  Wire.setSCL(17);
  Wire.begin();
  Wire.setClock(400000); //400khz clock

  delay(1000);
  Serial.begin(115200); //USB interface!
  pinMode(LED_BUILTIN, OUTPUT);
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(50);
  }

  // start mouth UART
  Serial1.setTX(12);
  Serial1.setRX(13);
  Serial1.begin(115200); 
  delay(50);
 
  // start eye UART
  Serial2.setTX(8);
  Serial2.setRX(9);
  Serial2.begin(115200);
  delay(50);

 
  for (int i = 0; i < LCHAIN_DEVS; i++) {
    Lchain.shutdown(i, false);
    Lchain.setIntensity(i, lvl);
    Lchain.clearDisplay(i);
  }

  for (int i = 0; i < RCHAIN_DEVS; i++) {
    Rchain.shutdown(i, false);
    Rchain.setIntensity(i, lvl);
    Rchain.clearDisplay(i);
  }




/*
  for (int y = 0; y < FACE_HEIGHT; y++) { // clear the eye arrays
    for (int x = 0; x < EYES_WIDTH; x++) {
      eyebuffer[y][x] = 0;
  
    }
  }
*/
  pinMode(BRIGHTNESS, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  fillFace();
  clearFace();
  

  mouthToBuffer(1); // 0 ATM is the error fallback (Just in case I need it)
  eyeToBuffer(1);
  renderFaceSeries();
  uint8_t b = 0xff;
  Serial1.write(b);
  long int lastblink = millis();



  analogReadResolution(12);   // should give 0..4095
  analogWriteResolution(12);  // optional, just keeps things consistent



}




void setBrightness(int b, int old_b) {
  const int BAC = 90; // brightness actual constant
  const float BR = 5.4; // brightness ratio (divide by this)
  // /////////// ----  LOGIC STARTS ------------////////////////
  if (b > 80) {b = 80;}
  else if (b < 10) {b = 10;}
  float v = ((BAC - b) / BR);
  if (v > 15) {v = 15;}
  for (int i = 0; i < LCHAIN_DEVS; i++) { // left and right have to be done seperatly because they get their data from different pins
    Lchain.setIntensity(i, int(v));
  }
  for (int i = 0; i < RCHAIN_DEVS; i++) {
    Rchain.setIntensity(i, int(v));
  }
  /*
  // set min LED lvl to 1, max lvl is 14 still
  // 10 = 14 : FIXED
  // 80 = 1 : FIXED

  Serial.print(" v= "); // debug
  Serial.println(v);
  */
}


//////////////////////////////////
//-------TIMING & CLOCKING------// -- Variables for coordinating when functions activate. saves CPU cycles!
//////////////////////////////////
long int lastgyro = 0;
long int lastbrightness = 0;
long int lastblink = 0;
long int lastpingA = 0; // for left hand
long int lastpingB = 0; // for right hand
long int now = 0; // milliseconds alive from right now (DUH)
const int BUF = 100; // How big is the buffer? (For average brightness)
int buffer[BUF]; // the buffer ^^ mentioned here
int BRcount = 0; // BR (Brughtness) count. (tracking how many indexes because I don't do it every loop)

/*
The timings have to be tracked like this because the CPU runs fast enough for the loop to execute at sub millisecond speeds.
I have to count the number of milliseconds to know when it's time to check a sensor or push some data somewhere.
The advantage of this is I can set different things to activate at different times, synchronizing a lot of faster processes in the
same "IF x ms have passed" type of statement. What goes together? Fast ones. Slower things get their own whole ass checks
but at lower frequencies. When I update the faces I interupt everything and handle it immediatly. This MIGHT change but I 
think it's a good practice... when a new face is requested, DO IT NOW!!!!
*/

///////////////////////////////////////////
//-------Global values worth saving------// -- Variables that I need to persist between loops!
///////////////////////////////////////////

int old_raw = 0;







//-----------------------MAIN LOOP-----------------------
void loop() {
  now = millis();
  if(now - lastbrightness > 2){
    int b = analogRead(BRIGHTNESS);
    buffer[BRcount] = b;
    BRcount += 1;
    if (BRcount == BUF - 1) {BRcount = 0;}
  }
  if (now - lastbrightness > 100) {
    //Serial.print("updating brightness...");
    lastbrightness = now;
    int raw = analogRead(BRIGHTNESS);
    int avg = 0;
    for (int i = 0; i < BUF; i++) {
      avg += buffer[i];
    }

    avg = avg / BUF;
    setBrightness(avg, old_raw);
    /*
    Serial.print(": raw= ");
    Serial.print(raw);
    Serial.print(": old_raw= ");
    Serial.println(old_raw);
    */
    old_raw = avg;
  }

/* BRIGHTNESS VALUES (NOTES) delete ts later
lowest raw = 80; lvl =  1
Highest raw = 10; lvl = 12

data range, 80 -> 10;
lvl range 1 -> 12
*/




/*
  if (Serial1.available()) {
    uint8_t b = Serial1.read();
    if (b == '\n') {return;} // ignore newline if you kept it
    else {
      oldmouth = selected_mouth;
      oldeyes = selected_eyes;
      bmouth = (b & 0b00000111);        // bits 0-2
      beyes  = (b & 0b00111000) >> 3;   // bits 3-5, shifted to 0..3
      mode =  (b & 0b11000000) >> 6;   // FUTURE "mode" selection

      Serial.println(mode);

      // then call your face selection logic:
      mouthSelect(bmouth);
      eyeSelect(beyes);
      renderFace();
    }
  }
*/

/*
start by checking if this byte is 0xff.
if not then we can continue to read the byte
if the byte doesn't match the data, throw it our.
*/
//--------------------------------------------mouth logic---------------------------------
  if (isliveA == true) {  // if we know we are live, then just read it.
    if (Serial1.available()) { 
      uint8_t b = Serial1.read();
      if (b == 0xFE) { // if it's a heartbeat, record new ping
        lastpingA = now;
      }
      else if (b < FACE_COUNT + 1) {
        digitalWrite(LED_BUILTIN, HIGH);
        selected_mouth = b;
        mouthToBuffer(selected_mouth);
        lastpingA = now;
        renderMouth();
      }
      else {
        b = 0;
      }
    }
  }
  else if ((now - lastpingA) > DC) { // if it's been awhile since we got a heartbeat...
    Serial1.write(0xFF); // send a request
    lastpingA = now;
    delay(10); // give it time to get back to us
    if (!Serial1.available()) {
      isliveA = false; // if serial1 !available, assume dead (default)
    }
    else { 
      int h = Serial1.read(); // get the thing
      if (h == -1) {
        isliveA = false; // no response, assume dead. (static bit safety?)
      }
      isliveA = true;
    }
  }
//-------------------------------------------------eye logic--------------------------
  if (isliveB == true) {  // if we know we are live, then just read it.
    if (Serial2.available()) { 
      uint8_t b = Serial2.read();
      if (b == 0xFE) { // if it's a heartbeat, record new ping
        lastpingB = now;
      }
      else if (b < FACE_COUNT + 1) {
        selected_eyes = b;
        eyeToBuffer(selected_eyes);
        lastpingB = now;
        renderFaceSeries();
      }
      else {
        b = 0;
      }
    }
  }
  else if ((now - lastpingB) > DC) { // if it's been awhile since we got a heartbeat...
    Serial2.write(0xFF); // send a request
    lastpingB = now;
    delay(10); // give it time to get back to us
    if (!Serial2.available()) {
      isliveB = false; // if serial1 !available, assume dead (default)
    }
    else { 
      int h = Serial2.read(); // get the thing
      if (h == -1) {
        isliveB = false; // no response, assume dead. (static bit safety?)
      }
      isliveB = true;
    }
  }
//----------------------------------------------BLINKING--------------------------------
  if ((now - lastblink) >= BLINK && checkTrue(selected_eyes) == true) {
    digitalWrite(LED_BUILTIN, HIGH);
    blinkToBuffer(selected_eyes);    
    eyeToBuffer(selected_eyes);
    renderEyes();
    lastblink = now;
    digitalWrite(LED_BUILTIN, LOW);
  }
}
