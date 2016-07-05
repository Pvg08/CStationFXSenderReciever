#include <Crc16.h>
#include "LedControl.h"

#define MATRIX_COUNT 5
#define MATRIX_ROWS_COUNT 8
#define MATRIX_STATE_BUFFER_SIZE 8
#define BAUD_RATE 115200

#define MAX_RECIEVE_INTERVAL 15000

#define __PACKED __attribute__((packed))

typedef uint8_t LEDMatrixState[MATRIX_ROWS_COUNT] __PACKED;
struct LEDScreenState {
    uint32_t block_index __PACKED;
    uint32_t timeout __PACKED;
    LEDMatrixState blocks[MATRIX_COUNT] __PACKED;
    uint16_t played __PACKED;
    uint16_t hash __PACKED;
} __PACKED;

struct LEDScreenState state_buffer[MATRIX_STATE_BUFFER_SIZE] = {0};
LEDScreenState state_old = {0};

unsigned buffer_playposition;
unsigned buffer_writeposition;
bool first_reading;
uint32_t last_played_pos;
uint32_t last_readed_pos;
uint32_t last_recieve_millis;
uint32_t last_state_millis;
uint32_t last_state_delay;

Crc16 crc;
LedControl lc=LedControl(12, 11, 10, MATRIX_COUNT);

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
  buffer_playposition = 0;
  buffer_writeposition = 0;
  last_recieve_millis = 0;
  last_state_millis = 0;
  last_state_delay = 0;
  first_reading = false;
}

unsigned nextBufferPosition(unsigned cpos)
{
  cpos++;
  if (cpos>=MATRIX_STATE_BUFFER_SIZE) cpos = 0;
  return cpos;
}

void setState(LEDScreenState* state) {
  if (state->played) return;
  for(byte page=0; page<MATRIX_COUNT; page++) {
    for(byte row=0; row<MATRIX_ROWS_COUNT; row++) {
      if (state_old.blocks[page][row] != state->blocks[page][row]) {
        lc.setRow(page, row, state->blocks[page][row]);
        state_old.blocks[page][row] = state->blocks[page][row];
      }
    }
  }
  state->played = true;
  last_played_pos = state->block_index;
  last_state_delay = state->timeout;
  last_state_millis = millis();
}

void sendPosConfirmation(uint32_t pos, bool is_play) {
  byte* last_pos = (byte*) (void*) &pos;
  Serial.write('C');
  Serial.write('S');
  Serial.write(is_play ? 'P' : 'W');
  Serial.write(last_pos[0]);
  Serial.write(last_pos[1]);
  Serial.write(last_pos[2]);
  Serial.write(last_pos[3]);
  Serial.flush();
}

void loop() { 
  byte j;
  bool new_state;

  /*for(j=0; j<10; j++) {
    digitalWrite(13, HIGH);delay(200);digitalWrite(13, LOW);delay(200);
  }*/

  while (true) {
    new_state = false;
    while (Serial.available() >= sizeof(LEDScreenState)) {
      last_recieve_millis = millis();
      char* next_pos = (char*) (void*) &(state_buffer[buffer_writeposition]);
      for(byte i=0; i<sizeof(LEDScreenState); i++) {
        next_pos[i] = Serial.read();
      }
      uint16_t ihash = state_buffer[buffer_writeposition].hash;
      state_buffer[buffer_writeposition].hash = 0;
      if ((!first_reading || state_buffer[buffer_writeposition].block_index==(last_readed_pos+1)) && ihash==crc.XModemCrc((uint8_t*) (void*) &(state_buffer[buffer_writeposition]), 0, sizeof(LEDScreenState))) {
        first_reading = true;
        last_readed_pos = state_buffer[buffer_writeposition].block_index;
        buffer_writeposition=nextBufferPosition(buffer_writeposition);
      } else {
        delay(10);
        /*for(j=0; j<3; j++) {
          digitalWrite(13, HIGH);delay(250);digitalWrite(13, LOW);delay(250);
        }*/
        while (Serial.available()) Serial.read();
      }
      new_state = true;
      if (Serial.available() && (Serial.available() < sizeof(LEDScreenState))) {
        delay(20);
      }
    }
    if (new_state) {
      sendPosConfirmation(last_readed_pos, false);
    } else if (first_reading) {
      if (buffer_playposition != buffer_writeposition) {
        if (!last_state_millis || (millis()-last_state_millis)>=last_state_delay) {
          setState(&(state_buffer[buffer_playposition]));
          sendPosConfirmation(last_played_pos, true);
          buffer_playposition=nextBufferPosition(buffer_playposition);
        }
      } else if ((millis()-last_recieve_millis) >= MAX_RECIEVE_INTERVAL) {
        sendPosConfirmation(last_played_pos, true);
        last_recieve_millis = millis();
      }
    }

    delay(1);
  }

}
