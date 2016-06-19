#include <Crc16.h>
#include "LedControl.h"

#define MATRIX_COUNT 5
#define MATRIX_ROWS_COUNT 8
#define MATRIX_STATE_BUFFER_SIZE 100
#define BAUD_RATE 38400

typedef byte LEDMatrixState[MATRIX_ROWS_COUNT];
struct LEDScreenState {
    unsigned block_index;
    unsigned timeout;
    LEDMatrixState blocks[MATRIX_COUNT];
    unsigned hash;
};

struct LEDScreenState state_buffer[MATRIX_STATE_BUFFER_SIZE];

Crc16 crc;
LedControl lc=LedControl(12, 11, 10, MATRIX_COUNT);
byte page = 0;

unsigned long delaytime=25;

void setup() {
  Serial.begin(BAUD_RATE);
  for (byte i=0; i<MATRIX_COUNT; i++) {
    lc.shutdown(i,false);
    lc.setIntensity(i,1);
    lc.clearDisplay(i);
  }
  state_buffer[0].block_index = 0;
  state_buffer[0].timeout = 0;
  state_buffer[0].blocks[0][0] = 0;
  state_buffer[0].hash = crc.XModemCrc((uint8_t*) (void*) &state_buffer, 0, sizeof(LEDScreenState)-sizeof(unsigned));
}

void writeArduinoOnMatrix() {
  byte a[5]={B01111110,B10001000,B10001000,B10001000,B01111110};
  byte r[5]={B00111110,B00010000,B00100000,B00100000,B00010000};
  byte d[5]={B00011100,B00100010,B00100010,B00010010,B11111110};
  byte u[5]={B00111100,B00000010,B00000010,B00000100,B00111110};
  byte i[5]={B00000000,B00100010,B10111110,B00000010,B00000000};
  byte n[5]={B00111110,B00010000,B00100000,B00100000,B00011110};
  byte o[5]={B00011100,B00100010,B00100010,B00100010,B00011100};

  lc.setRow(page,0,a[0]);
  lc.setRow(page,1,a[1]);
  lc.setRow(page,2,a[2]);
  lc.setRow(page,3,a[3]);
  lc.setRow(page,4,a[4]);
  delay(delaytime);
  lc.setRow(page,0,r[0]);
  lc.setRow(page,1,r[1]);
  lc.setRow(page,2,r[2]);
  lc.setRow(page,3,r[3]);
  lc.setRow(page,4,r[4]);
  delay(delaytime);
  lc.setRow(page,0,d[0]);
  lc.setRow(page,1,d[1]);
  lc.setRow(page,2,d[2]);
  lc.setRow(page,3,d[3]);
  lc.setRow(page,4,d[4]);
  delay(delaytime);
  lc.setRow(page,0,u[0]);
  lc.setRow(page,1,u[1]);
  lc.setRow(page,2,u[2]);
  lc.setRow(page,3,u[3]);
  lc.setRow(page,4,u[4]);
  delay(delaytime);
  lc.setRow(page,0,i[0]);
  lc.setRow(page,1,i[1]);
  lc.setRow(page,2,i[2]);
  lc.setRow(page,3,i[3]);
  lc.setRow(page,4,i[4]);
  delay(delaytime);
  lc.setRow(page,0,n[0]);
  lc.setRow(page,1,n[1]);
  lc.setRow(page,2,n[2]);
  lc.setRow(page,3,n[3]);
  lc.setRow(page,4,n[4]);
  delay(delaytime);
  lc.setRow(page,0,o[0]);
  lc.setRow(page,1,o[1]);
  lc.setRow(page,2,o[2]);
  lc.setRow(page,3,o[3]);
  lc.setRow(page,4,o[4]);
  delay(delaytime);
  lc.setRow(page,0,0);
  lc.setRow(page,1,0);
  lc.setRow(page,2,0);
  lc.setRow(page,3,0);
  lc.setRow(page,4,0);
  delay(delaytime);
}

void rows() {
  for(int row=0;row<8;row++) {
    delay(delaytime);
    lc.setRow(page,row,B10100000);
    delay(delaytime);
    lc.setRow(page,row,(byte)0);
    for(int i=0;i<row;i++) {
      delay(delaytime);
      lc.setRow(page,row,B10100000);
      delay(delaytime);
      lc.setRow(page,row,(byte)0);
    }
  }
}

void columns() {
  for(int col=0;col<8;col++) {
    delay(delaytime);
    lc.setColumn(page,col,B10100000);
    delay(delaytime);
    lc.setColumn(page,col,(byte)0);
    for(int i=0;i<col;i++) {
      delay(delaytime);
      lc.setColumn(page,col,B10100000);
      delay(delaytime);
      lc.setColumn(page,col,(byte)0);
    }
  }
}

void single() {
  for(int row=0;row<8;row++) {
    for(int col=0;col<8;col++) {
      delay(delaytime);
      lc.setLed(page,row,col,true);
      delay(delaytime);
      for(int i=0;i<col;i++) {
        lc.setLed(page,row,col,false);
        delay(delaytime);
        lc.setLed(page,row,col,true);
        delay(delaytime);
      }
    }
  }
}

void loop() { 
  if (page>=MATRIX_COUNT) page=0;
  writeArduinoOnMatrix();
  rows();
  columns();
  single();
  page++;
}
