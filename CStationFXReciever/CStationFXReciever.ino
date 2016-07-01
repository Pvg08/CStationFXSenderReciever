#include <Crc16.h>
#include "LedControl.h"

#define MATRIX_COUNT 5
#define MATRIX_ROWS_COUNT 8
#define MATRIX_STATE_BUFFER_SIZE 4
#define BAUD_RATE 115200

#define MAX_RECIEVE_INTERVAL 15000

#define __PACKED __attribute__((packed))

typedef uint8_t LEDMatrixState[MATRIX_ROWS_COUNT] __PACKED;
struct LEDScreenState {
    uint32_t block_index __PACKED;
    uint32_t timeout __PACKED;
    LEDMatrixState blocks[MATRIX_COUNT] __PACKED;
    uint8_t played __PACKED;
    uint16_t hash __PACKED;
} __PACKED;

struct LEDScreenState state_buffer[MATRIX_STATE_BUFFER_SIZE] = {0};
unsigned buffer_playposition = 0;
unsigned buffer_writeposition = 0;
bool new_state;

Crc16 crc;
LedControl lc=LedControl(12, 11, 10, MATRIX_COUNT);
uint32_t last_played_pos;
uint32_t last_readed_pos;
unsigned long int last_recieve_millis = 0;

void setup() {
  Serial.begin(BAUD_RATE);
  pinMode(13, OUTPUT);
  for (byte page=0; page<MATRIX_COUNT; page++) {
    lc.shutdown(page,false);
    lc.setIntensity(page,1);
    lc.clearDisplay(page);
  }
  state_buffer[0].block_index = 0;
  last_played_pos = 0;
  last_readed_pos = 0;
}

void setState(LEDScreenState* state) {
  if (state->played) return;
  for(byte page=0; page<MATRIX_COUNT; page++) {
    for(byte row=0; row<MATRIX_ROWS_COUNT; row++) {
      lc.setRow(page,row,state->blocks[page][row]);
    }
  }
  state->played = true;
  last_played_pos = state->block_index;
  delay(state->timeout);
}

void sendPosConfirmation(uint32_t pos) {
  byte* last_pos = (byte*) (void*) &pos;
  Serial.write('S');
  Serial.write('C');
  Serial.write('E');
  Serial.write(last_pos[0]);
  Serial.write(last_pos[1]);
  Serial.write(last_pos[2]);
  Serial.write(last_pos[3]);
  Serial.flush();
}

void loop() { 
  new_state = false;
  while (Serial.available() >= sizeof(LEDScreenState)) {
    last_recieve_millis = millis();
    char* next_pos = (char*) (void*) &(state_buffer[buffer_writeposition]);
    for(byte i=0; i<sizeof(LEDScreenState); i++) {
      next_pos[i] = Serial.read();
    }
    uint16_t ihash = 1;//state_buffer[buffer_writeposition].hash;
    state_buffer[buffer_writeposition].hash = 0;
    uint16_t chash = 1;//crc.XModemCrc((uint8_t*) (void*) next_pos, 0, sizeof(LEDScreenState));
    if (chash == ihash) {
      last_readed_pos = state_buffer[buffer_writeposition].block_index;
      buffer_writeposition++;
      if (buffer_writeposition>=MATRIX_STATE_BUFFER_SIZE) buffer_writeposition = 0;
    } else {
      digitalWrite(13, HIGH);
      while (Serial.available()) Serial.read();
      delay(500);
      digitalWrite(13, LOW);
      delay(500);
      digitalWrite(13, HIGH);
      delay(500);
      digitalWrite(13, LOW);
    }
    new_state = true;
    if (Serial.available() && (Serial.available() < sizeof(LEDScreenState))) {
      delay(10);
    }
  }
  if (new_state) {
    sendPosConfirmation(last_readed_pos);
  } else {
    if (buffer_playposition != buffer_writeposition) {
      setState(&(state_buffer[buffer_playposition]));
      buffer_playposition++;
      if (buffer_playposition>=MATRIX_STATE_BUFFER_SIZE) buffer_playposition = 0;
    } else if ((millis()-last_recieve_millis) >= MAX_RECIEVE_INTERVAL) {
      sendPosConfirmation(last_played_pos);
      last_recieve_millis = millis();
    }
  }
}
