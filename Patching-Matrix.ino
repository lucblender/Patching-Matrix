
#include "AnalogMux.h"

#include "Adafruit_NeoTrellis.h"

Adafruit_NeoTrellis trellis;

#define INT_PIN 22

#define EN 2


#define BAUD_RATE         115200

AnalogMux *mux0;
AnalogMux *mux1;
AnalogMux *mux2;
AnalogMux *mux3;

//AnalogMux * muxs[4] = {mux0, mux1, mux2, mux3};

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return trellis.pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return trellis.pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return trellis.pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

//define a callback for key presses
TrellisCallback blink(keyEvent evt){
  
  if(evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING){
    int line = evt.bit.NUM / 4;
    Serial.println(evt.bit.NUM);
    Serial.println(evt.bit.NUM%4);
    Serial.println(line);
    AnalogMux *mux;
    Serial.print("Clearing :");
    if(line == 0)
      mux = mux0;
    if(line == 1)
      mux = mux1;
    if(line == 2)
      mux = mux2;
    if(line == 3)
      mux = mux3;
    
    for(int i = 0; i<4; i++)
    { Serial.print(i+line*4);
      Serial.print(" ");
      trellis.pixels.setPixelColor(i+line*4, 0); //on rising
    }
    Serial.println(" ");
   
    if(mux->getSelectedOut() == evt.bit.NUM%4)
      mux->selectMuxPin(-1);
    else{
      mux->selectMuxPin(evt.bit.NUM%4);
      trellis.pixels.setPixelColor(evt.bit.NUM, trellis.pixels.Color(128,0,0)); //on rising
    }
    
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
  mux0 = new AnalogMux(4,6,8,2);
  mux1 = new AnalogMux(5,7,9,3);
  mux2 = new AnalogMux(12,14,16,10);
  mux3 = new AnalogMux(13,15,17,11);
  
  mux0->selectMuxPin(0);
  mux1->selectMuxPin(0);
  mux2->selectMuxPin(0);
  mux3->selectMuxPin(0);

  pinMode(INT_PIN, INPUT);
  
  if(!trellis.begin()){
    Serial.println("could not start trellis");
    while(1) delay(1);
  }
  else{
    Serial.println("trellis started");
  }

  //activate all keys and set callbacks
  for(int i=0; i<NEO_TRELLIS_NUM_KEYS; i++){
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_FALLING);
    trellis.registerCallback(i, blink);
  }

  //do a little animation to show we're on
  for(uint16_t i=0; i<trellis.pixels.numPixels(); i++) {
    trellis.pixels.setPixelColor(i, Wheel(map(i, 0, trellis.pixels.numPixels(), 0, 255)));
    trellis.pixels.show();
    delay(50);
  }
  for(uint16_t i=0; i<trellis.pixels.numPixels(); i++) {
    trellis.pixels.setPixelColor(i, 0x000000);
    trellis.pixels.show();
    delay(50);
  }
  

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
  
  if(!digitalRead(INT_PIN)){
    trellis.read(false);
  }
  delay(2);
}
