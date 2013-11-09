/*

 EEPROM Structure:
 
 | Address | What   |
 |       0 | Width  |
 |       1 | 0,0 R  |
 |       2 | 0,0 G  |
 |       3 | 0,0 B  |
 |       4 | 0,1 R  |
 |       5 | 0,1 G  |
 |       6 | 0,1 B  |
 | .....   | .....  |
 |         | 0,24 B |
 |         | 1,0 R  |
 | .....   | .....  |
 
 
 */

#include <SPI.h>
#include <EEPROM.h>
#include <Adafruit_WS2801.h>

#define numPixels 25

uint8_t dataPin = 2; // Yellow
uint8_t clockPin = 3; // Green
uint8_t mode = 0;
uint32_t startTime = 0;
byte byte1, byte2, byte3;
char column[numPixels*3];
uint8_t i=0,j=0,width=0;
uint32_t color = Color(65, 0, 0);
uint32_t address = 0;

#define MODE_STARTUP 0
#define MODE_LOAD 1
#define MODE_RUN 99
#define MODE_ERROR 100
#define MAX_WAIT 10000
#define MAX_WIDTH 25
#define VERSION "1.0"

Adafruit_WS2801 strip = Adafruit_WS2801(numPixels, dataPin, clockPin);

void setup() {
  strip.begin();
  strip.show();

  Serial.begin(9600);
}

void loop() {
  switch(mode) {
  case MODE_STARTUP:
    loop_startup();
    break;

  case MODE_LOAD:
    loop_load();
    break;

  case MODE_ERROR:
    loop_error();
    break;

  default:
    load_run();
    break;
  }
}

void loop_startup() {

  if (i == 0) {
    strip.setPixelColor(numPixels-1, 0);
  } 
  else {
    strip.setPixelColor(i-1, 0);
  }
  if (millis()-startTime > MAX_WAIT) {
    mode = MODE_RUN;
    strip.show();
    i=0;
    return;
  }
  strip.setPixelColor(i, color);
  strip.show();
  i = (i+1) % numPixels;

  if (Serial.available() >= 2) {
    byte1 = Serial.read();
    if (byte1 == 'x') {
      byte2 = Serial.read();
      if (byte2 == 'o') {
        Serial.read(); // Ignore the new line
        Serial.print("OKv");
        Serial.println(VERSION);
        Serial.print("D");
        Serial.println(numPixels);
        Serial.flush();
        mode = MODE_LOAD;
        return;
      }
    }
  }
  delay(50);
}

void clearStrip() {
  for(i=0;i<numPixels;i++) {
    strip.setPixelColor(i, 0);
  }
  strip.show();
}

void readUntil(byte c) {
  byte1 = Serial.read();
  while(byte1 != c) byte1 = Serial.read();
}

void loop_load() {
  readUntil('W');
  width = Serial.parseInt();  
  Serial.println(width);
  if (width > MAX_WIDTH) {
    mode = MODE_ERROR;
    color = Color(12, 0, 36);
    return;
  }

  clearStrip();

  // EEPROM.write(0, width); 

  for(i=0;i<width;i++) {
    strip.setPixelColor(i, Color(0, 36, 0)); strip.show();
    byte1 = Serial.readBytes(column, numPixels*3);

    for(j=0;j<byte1;j++) {
      strip.setPixelColor(i, Color(0,0,36)); strip.show();
      ++address;
      EEPROM.write(address,column[j]);
      strip.setPixelColor(i, 0); strip.show();
    }
    strip.setPixelColor(i, 0); strip.show();
  }
  clearStrip();
  delay(1000);
  i=0;
  mode = MODE_RUN;
}

void loop_error() {
  if (Serial.available() >= 2) {
    byte1 = Serial.read();
    if (byte1 == 'R' || byte1 == 'x') {
      byte2 = Serial.read();
      if (byte1 == 'R' && byte2 == 'T') {
        startTime = millis();
        clearStrip();
        mode = MODE_STARTUP;
        i=0;
        Serial.println("RESET");
        return;
      }
      if (byte1 == 'x' && byte2 == 'o') {
        mode = MODE_LOAD;
        i=0;
        return;
      }
    }
  } 
  for(i=0;i<numPixels;i++) {
    if (i%2 == j) { 
      strip.setPixelColor(i, color); 
    }
    else { 
      strip.setPixelColor(i, 0); 
    }
  }
  strip.show();
  delay(100);
  if (j == 1) j = 0; 
  else j = 1;
}

void load_run() {
  // i is the current column
  if (width == 0) {
    width = EEPROM.read(0); // Width is always stored at zero address
  }
  address = numPixels*3 * i;
  for(j=0;j<numPixels*3;j+=3) {
    byte1 = EEPROM.read(address+j);
    byte2 = EEPROM.read(address+j+1);
    byte3 = EEPROM.read(address+j+2);
    strip.setPixelColor(j/3, Color(byte1, byte2, byte3));
  }
  strip.show();
  i = (i+1)%width;
  delay(100);
}

uint32_t Color(byte r, byte g, byte b)
{
  uint32_t c;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}


