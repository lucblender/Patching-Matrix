
#include "AnalogMux.h"
#include <EEPROM.h>
#include <Wire.h>

#include "grove_alphanumeric_display.h"

#include "Adafruit_NeoTrellis.h"

Adafruit_NeoTrellis trellis;

#define INT_PIN 22

#define EN 2
#define SELECT_INPUT A14

#define WRITE_BTN 32
#define READ_BTN 34


#define BAUD_RATE         115200

int programValue = 0;
int oldProgramValue = -1;

bool showProgram = false;
int showProgramCounter = 0;

Seeed_Digital_Tube tube;

// Keeps track of the current stage of tube 2's animation.
int currentSegment = 0;

// The frames of tube 2's animation.
uint16_t tubeTwoFrames[] = {
    SEGMENT_TOP,
    SEGMENT_TOP_LEFT,
    SEGMENT_TOP_LEFT_DIAGONAL,
    SEGMENT_TOP_VERTICAL,
    SEGMENT_TOP_RIGHT_DIAGONAL,
    SEGMENT_TOP_RIGHT,
    SEGMENT_MIDDLE_LEFT,
    SEGMENT_MIDDLE_RIGHT,
    SEGMENT_BOTTOM_LEFT,
    SEGMENT_BOTTOM_LEFT_DIAGONAL,
    SEGMENT_BOTTOM_VERTICAL,
    SEGMENT_BOTTOM_RIGHT_DIAGONAL,
    SEGMENT_BOTTOM_RIGHT,
    SEGMENT_BOTTOM
};

AnalogMux *mux0;
AnalogMux *mux1;
AnalogMux *mux2;
AnalogMux *mux3;

//AnalogMux * muxs[4] = {mux0, mux1, mux2, mux3};

int currentValue [4] = {255, 255, 255, 255};

int arrayValue[8][4] = {
  {0, 1, 2, 3},
  {0, 1, 2, 3},
  {0, 1, 2, 3},
  {0, 1, 2, 3},
  {0, 1, 2, 3},
  {0, 1, 2, 3},
  {0, 1, 2, 3},
};

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    return trellis.pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return trellis.pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
    WheelPos -= 170;
    return trellis.pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

//define a callback for key presses
TrellisCallback blink(keyEvent evt) {

  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
    int line = evt.bit.NUM / 4;
    AnalogMux *mux;
    if (line == 0)
      mux = mux0;
    if (line == 1)
      mux = mux1;
    if (line == 2)
      mux = mux2;
    if (line == 3)
      mux = mux3;

    if (mux->getSelectedOut() == evt.bit.NUM % 4)
      mux->selectMuxPin(255);
    else {
      mux->selectMuxPin(evt.bit.NUM % 4);
    }

    currentValue[0] = mux0->getSelectedOut();
    currentValue[1] = mux1->getSelectedOut();
    currentValue[2] = mux2->getSelectedOut();
    currentValue[3] = mux3->getSelectedOut();

    //trellis.pixels.setPixelColor(evt.bit.NUM, Wheel(map(evt.bit.NUM, 0, trellis.pixels.numPixels(), 0, 255))); //on rising
  }
  //else if(evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING)
  //  trellis.pixels.setPixelColor(evt.bit.NUM, 0); //off falling

  trellis.pixels.show();
  return 0;
}

void setup() {
  // put your setup code here, to run once:  // Set up the select pins, as outputs

  Serial.begin(BAUD_RATE);

  Wire.begin();
  // If using four digital tubes, use this configuration.
  tube.setTubeType(TYPE_2, TYPE_2_DEFAULT_I2C_ADDR);

  // If using two digital tubes, use this configuration.
  // tube.setTubeType(TYPE_2,TYPE_2_DEFAULT_I2C_ADDR);

  tube.setBrightness(15);
  tube.setBlinkRate(BLINK_OFF);

  pinMode(READ_BTN, INPUT_PULLUP);
  pinMode(WRITE_BTN, INPUT_PULLUP);

  mux0 = new AnalogMux(4, 6, 8, 2);
  mux1 = new AnalogMux(5, 7, 9, 3);
  mux2 = new AnalogMux(12, 14, 16, 10);
  mux3 = new AnalogMux(13, 15, 17, 11);

  mux0->selectMuxPin(255);
  mux1->selectMuxPin(255);
  mux2->selectMuxPin(255);
  mux3->selectMuxPin(255);

  for (int i = 0; i < 8; i++) {
    // uncomment only one time to setup
    //saveValue(i);
    loadValue(i);
  }

  pinMode(INT_PIN, INPUT);

  if (!trellis.begin()) {
    Serial.println("could not start trellis");
    while (1) delay(1);
  }
  else {
    Serial.println("trellis started");
  }
  //activate all keys and set callbacks
  for (int i = 0; i < NEO_TRELLIS_NUM_KEYS; i++) {
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_FALLING);
    trellis.registerCallback(i, blink);
  }

  //do a little animation to show we're on
  for (uint16_t i = 0; i < trellis.pixels.numPixels(); i++) {
    trellis.pixels.setPixelColor(i, Wheel(map(i, 0, trellis.pixels.numPixels(), 0, 255)));
    trellis.pixels.show();
    tube.clearBuf();
    displayTubeTwo();
    tube.display();
    delay(50);
  }
  
  tube.displayString("LX");
  for (uint16_t i = 0; i < trellis.pixels.numPixels(); i++) {
    trellis.pixels.setPixelColor(i, 0x000000);
    trellis.pixels.show();
    delay(50);
  }


}

void selectRead() {
  programValue = analogRead(SELECT_INPUT) / 128;

  if (programValue != oldProgramValue)
  {
    tube.clearBuf();
    char toDisplay [2]= {'P', programValue+48};
    tube.setTubeSingleChar(FIRST_TUBE,'P');
    tube.setTubeSingleNum(SECOND_TUBE,programValue);    
    tube.display();
    showProgram = true;
    showProgramCounter = 0;
    Serial.print("Showing program n° ");
    Serial.println(programValue);
  }
  oldProgramValue = programValue;
}

void saveValue(int program) {
  arrayValue[program][0] = mux0->getSelectedOut();
  arrayValue[program][1] = mux1->getSelectedOut();
  arrayValue[program][2] = mux2->getSelectedOut();
  arrayValue[program][3] = mux3->getSelectedOut();
  for (int i = 0; i < 4; i++) {
    EEPROM.write(program * 4 + i, arrayValue[program][i]);
  }
}

void writeCurrentProgram() {
  mux0->selectMuxPin(arrayValue[programValue][0]);
  mux1->selectMuxPin(arrayValue[programValue][1]);
  mux2->selectMuxPin(arrayValue[programValue][2]);
  mux3->selectMuxPin(arrayValue[programValue][3]);

  currentValue[0] = mux0->getSelectedOut();
  currentValue[1] = mux1->getSelectedOut();
  currentValue[2] = mux2->getSelectedOut();
  currentValue[3] = mux3->getSelectedOut();
}

void loadValue(int program) {
  for (int i = 0; i < 4; i++) {
    arrayValue[program][i] = EEPROM.read (program * 4 + i);
  }
}

void lightProgram(int program) {
  for (int i = 0; i < 4; i++) {
    if (arrayValue[program][i] != 255) {
      trellis.pixels.setPixelColor(i * 4 + arrayValue[program][i], trellis.pixels.Color(0, 128, 0)); //on rising
    }
  }
  trellis.pixels.show();
}

void light() {
  int g = 0;
  int r = 0;
  for (int line = 0; line < 4; line++) {
    for (int i = 0; i < 4; i++) {
      trellis.pixels.setPixelColor(i + line * 4, 0); //on rising}
    }
  }
  for (int line = 0; line < 4; line++) {

    for (int i = 0; i < 4; i++) {
      g = 0;
      r = 0;
      if (showProgram == true) {
        if (arrayValue[programValue][line] == i) {
          g = 128;
        }
      }
      if (currentValue[line] == i) {
        r = 128;
      }
      trellis.pixels.setPixelColor(i + line * 4, trellis.pixels.Color(r, g, 0)); //on rising
    }
  }

  trellis.pixels.show();
}

void displayTubeOne() {
    // To display specific segments add together their values.
    tube.setTubeSegments(FIRST_TUBE, SEGMENT_TOP + SEGMENT_BOTTOM);
}

// Display an animation going through all 14 segments in turn.
void displayTubeTwo() {
    // Increment tube 2's animation frame.
    currentSegment += 1;

    // Restart the animation if it has finished.
    if (currentSegment >= 14) {
        currentSegment = 0;
    }

    // Display the current frame of tube 2's animation.
    tube.setTubeSegments(FIRST_TUBE, tubeTwoFrames[currentSegment]);
    tube.setTubeSegments(SECOND_TUBE, tubeTwoFrames[currentSegment]);
}

void loop() {
  // put your main code here, to run repeatedly:
  //for(int i = 0; i<4; i++)
  //{
  //  mux0->selectMuxPin(i);
  //  Serial.print("Selecting output n° ");
  //  Serial.println(i);
  //  delay(10000);
  //}

  if (!digitalRead(READ_BTN)) {
    Serial.println("A");
    saveValue(programValue);
    Serial.println("SaveCurrentProgram");
    oldProgramValue = -1;
    tube.displayString("sv",0);
    light();
    delay(500);
  }

  if (!digitalRead(WRITE_BTN)) {
    writeCurrentProgram();
    Serial.println("WriteCurrentProgram");
    oldProgramValue = -1;
    tube.displayString("ld",0);
    light();
    delay(500);

  }

  if (!digitalRead(INT_PIN)) {
    trellis.read(false);
  }

  selectRead();

  if (showProgram == true) {
    showProgramCounter++;
    if (showProgramCounter > 100) {
      showProgram = false;
      Serial.println("showProgram = false;");
      showProgramCounter = 0;
    }
  }

  light();
  delay(2);
}
