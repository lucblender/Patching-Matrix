
#include "AnalogMux.h"
#include <EEPROM.h>
#include <Wire.h>

#include "grove_alphanumeric_display.h"

#include "Adafruit_NeoTrellis.h"

#define Y_DIM 8 //number of rows of key
#define X_DIM 8 //number of columns of keys

//create a matrix of trellis panels
Adafruit_NeoTrellis t_array[Y_DIM/4][X_DIM/4] = {
  
  { Adafruit_NeoTrellis(0x2E), Adafruit_NeoTrellis(0x2F) },

  { Adafruit_NeoTrellis(0x30), Adafruit_NeoTrellis(0x31) }
  
};

Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis *)t_array, Y_DIM/4, X_DIM/4);

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

#define MUX_NUMBER 8
#define PROGRAM_NUMBER 8

AnalogMux *mux0;
AnalogMux *mux1;
AnalogMux *mux2;
AnalogMux *mux3;
AnalogMux *mux4;
AnalogMux *mux5;
AnalogMux *mux6;
AnalogMux *mux7;

AnalogMux * muxs[MUX_NUMBER];

int currentValue [MUX_NUMBER] = {255, 255, 255, 255, 255, 255, 255, 255};

int arrayValue[PROGRAM_NUMBER][MUX_NUMBER] = {
  {0, 1, 2, 3, 4, 5, 6, 7},
  {0, 1, 2, 3, 4, 5, 6, 7},
  {0, 1, 2, 3, 4, 5, 6, 7},
  {0, 1, 2, 3, 4, 5, 6, 7},
  {0, 1, 2, 3, 4, 5, 6, 7},
  {0, 1, 2, 3, 4, 5, 6, 7},
  {0, 1, 2, 3, 4, 5, 6, 7},
};

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    return seesaw_NeoPixel::Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return seesaw_NeoPixel::Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
    WheelPos -= 170;
    return seesaw_NeoPixel::Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

//define a callback for key presses
TrellisCallback blink(keyEvent evt) {

  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
    int line = evt.bit.NUM / MUX_NUMBER;

    if (muxs[line]->getSelectedOut() == evt.bit.NUM % MUX_NUMBER)
      muxs[line]->selectMuxPin(255);
    else {
      muxs[line]->selectMuxPin(evt.bit.NUM % MUX_NUMBER);
    }

    for (int i = 0; i < MUX_NUMBER; i++)
    {
      currentValue[i] = muxs[i]->getSelectedOut();
    }
  }

  trellis.show();
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
  mux4 = new AnalogMux(13, 15, 17, 11);
  mux5 = new AnalogMux(13, 15, 17, 11);
  mux6 = new AnalogMux(13, 15, 17, 11);
  mux7 = new AnalogMux(13, 15, 17, 11);

  muxs[0] = mux0;
  muxs[1] = mux1;
  muxs[2] = mux2;
  muxs[3] = mux3;
  muxs[4] = mux4;
  muxs[5] = mux5;
  muxs[6] = mux6;
  muxs[7] = mux7;

  for (int i = 0; i < PROGRAM_NUMBER; i++) {
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
  for (int i = 0; i < Y_DIM*X_DIM; i++) {
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_FALLING);
    trellis.registerCallback(i, blink);
  }

  //do a little animation to show we're on
  for (uint16_t i = 0; i < Y_DIM*X_DIM; i++) {
    trellis.setPixelColor(i, Wheel(map(i, 0, Y_DIM*X_DIM, 0, 255)));
    trellis.show();
    tube.clearBuf();
    displayTubeTwo();
    tube.display();
  }

  tube.displayString("LX");
  for (uint16_t i = 0; i < Y_DIM*X_DIM; i++) {
    trellis.setPixelColor(i, 0x000000);
    trellis.show();
    delay(25);
  }


}

void selectRead() {
  programValue = analogRead(SELECT_INPUT) / 128;

  if (programValue != oldProgramValue)
  {
    tube.clearBuf();
    char toDisplay [2] = {'P', programValue + 48};
    tube.setTubeSingleChar(FIRST_TUBE, 'P');
    tube.setTubeSingleNum(SECOND_TUBE, programValue);
    tube.display();
    showProgram = true;
    showProgramCounter = 0;
    Serial.print("Showing program n° ");
    Serial.println(programValue);
  }
  oldProgramValue = programValue;
}

void saveValue(int program) {
  for (int i = 0; i < MUX_NUMBER; i++)
  {
    arrayValue[program][i] = muxs[i]->getSelectedOut();
  }
  for (int i = 0; i < MUX_NUMBER; i++) {
    EEPROM.write(program * MUX_NUMBER + i, arrayValue[program][i]);
  }
}

void writeCurrentProgram() {

  for (int i = 0; i < MUX_NUMBER; i++) {
    muxs[i]->selectMuxPin(arrayValue[programValue][i]);
  }
  for (int i = 0; i < MUX_NUMBER; i++) {
    currentValue[i] = muxs[i]->getSelectedOut();
  }
}

void loadValue(int program) {
  for (int i = 0; i < MUX_NUMBER; i++) {
    arrayValue[program][i] = EEPROM.read (program * MUX_NUMBER + i);
  }
}

void lightProgram(int program) {
  for (int i = 0; i < MUX_NUMBER; i++) {
    if (arrayValue[program][i] != 255) {
      trellis.setPixelColor(i * MUX_NUMBER + arrayValue[program][i], seesaw_NeoPixel::Color(0, 128, 0)); //on rising
    }
  }
  trellis.show();
}

void light() {
  int g = 0;
  int r = 0;
  for (int line = 0; line < MUX_NUMBER; line++) {
    for (int i = 0; i < MUX_NUMBER; i++) {
      trellis.setPixelColor(i + line * MUX_NUMBER, 0); //on rising}
    }
  }
  for (int line = 0; line < MUX_NUMBER; line++) {

    for (int i = 0; i < MUX_NUMBER; i++) {
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
      trellis.setPixelColor(i + line * MUX_NUMBER, seesaw_NeoPixel::Color(r, g, 0)); //on rising
    }
  }

  trellis.show();
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

  if (!digitalRead(READ_BTN)) {
    Serial.println("A");
    saveValue(programValue);
    Serial.println("SaveCurrentProgram");
    oldProgramValue = -1;
    tube.displayString("SV", 0);
    light();
    delay(500);
  }

  if (!digitalRead(WRITE_BTN)) {
    writeCurrentProgram();
    Serial.println("WriteCurrentProgram");
    oldProgramValue = -1;
    tube.displayString("ld", 0);
    light();
    delay(500);

  }

  if (!digitalRead(INT_PIN)) {
    trellis.read();
  }

  selectRead();

  if (showProgram == true) {
    showProgramCounter++;
    if (showProgramCounter > 50) {
      showProgram = false;
      Serial.println("showProgram = false;");
      showProgramCounter = 0;
    }
  }

  light();
  delay(2);
}
