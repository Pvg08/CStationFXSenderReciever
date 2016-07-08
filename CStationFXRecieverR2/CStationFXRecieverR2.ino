#include <Crc16.h>
#include <Adafruit_NeoPixel.h>

#define LEDRING_PIN 6

#define LEDRING_PIXELS 24
#define STATE_BUFFER_SIZE 6
#define BAUD_RATE 115200

#define MAX_RECIEVE_INTERVAL 5000

#define __PACKED __attribute__((packed))

struct StateStruct {
    uint32_t state_index __PACKED;
    uint32_t timeout __PACKED;
    uint16_t played __PACKED;
    uint16_t hash __PACKED;
} __PACKED;

struct RGBPixel {
  uint8_t r __PACKED;
  uint8_t g __PACKED;
  uint8_t b __PACKED;
} __PACKED;
struct LEDRingState : StateStruct {
    RGBPixel state[LEDRING_PIXELS] __PACKED;
} __PACKED;

struct LEDRingState state_buffer[STATE_BUFFER_SIZE] = {};
LEDRingState state_old;

unsigned buffer_playposition;
unsigned buffer_writeposition;
bool first_reading;
uint32_t last_played_pos;
uint32_t last_readed_pos;
uint32_t last_recieve_millis;
uint32_t last_state_millis;
uint32_t last_state_delay;

Crc16 crc;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(LEDRING_PIXELS, LEDRING_PIN, NEO_GRB + NEO_KHZ800);

void resetState() {
  state_buffer[0].state_index = 0;
  last_played_pos = 0;
  last_readed_pos = 0;
  buffer_playposition = 0;
  buffer_writeposition = 0;
  last_recieve_millis = 0;
  last_state_millis = 0;
  last_state_delay = 0;
  first_reading = false;
  pixels.clear();
  pixels.show();
  for(byte i=0; i<LEDRING_PIXELS; i++) {
    state_old.state[i].r = 0;
    state_old.state[i].g = 0;
    state_old.state[i].b = 0;
  }
}

void setup() {
  Serial.begin(BAUD_RATE);
  pinMode(13, OUTPUT);
  pixels.begin();
  resetState();
}

unsigned nextBufferPosition(unsigned cpos)
{
  cpos++;
  if (cpos>=STATE_BUFFER_SIZE) cpos = 0;
  return cpos;
}

void setState(LEDRingState* state) {
  if (state->played) return;
  bool state_changed = false;
  for(byte i=0; i<LEDRING_PIXELS; i++) {
    if (state_old.state[i].r != state->state[i].r || state_old.state[i].g != state->state[i].g || state_old.state[i].b != state->state[i].b) {
      pixels.setPixelColor(i, pixels.Color(state->state[i].r,state->state[i].g,state->state[i].b));
      state_old.state[i] = state->state[i];
      state_changed = true;
    }
  }
  if (state_changed) {
    pixels.show();
  }
  state->played = true;
  last_played_pos = state->state_index;
  last_state_delay = state->timeout;
  last_state_millis = millis();
}

void sendPosConfirmation(uint32_t pos, bool is_play, bool do_flush = true) {
  byte* last_pos = (byte*) (void*) &pos;
  Serial.write('C');
  Serial.write('S');
  Serial.write(is_play ? 'P' : 'W');
  Serial.write(last_pos[0]);
  Serial.write(last_pos[1]);
  Serial.write(last_pos[2]);
  Serial.write(last_pos[3]);
  if (do_flush) Serial.flush();
}

void ClearSerial() {
  while (Serial.available()) {
    Serial.read();
    delay(2);
  }
}

void WaitORClear() {
  if (Serial.available()>=sizeof(LEDRingState)) return;
  unsigned avail = 0;
  while (avail < Serial.available()) {
    avail = Serial.available();
    delay(5);
  }
}

void loop() { 
  byte j;
  bool new_state, new_play_state;

  /*for(j=0; j<10; j++) {
    digitalWrite(13, HIGH);delay(200);digitalWrite(13, LOW);delay(200);
  }*/

  while (true) {
    new_state = false;
    new_play_state = false;
    //WaitORClear();
    while (Serial.available() >= sizeof(LEDRingState)) {
      last_recieve_millis = millis();
      char* next_pos = (char*) (void*) &(state_buffer[buffer_writeposition]);
      for(byte i=0; i<sizeof(LEDRingState); i++) {
        next_pos[i] = Serial.read();
      }
      if (state_buffer[buffer_writeposition].hash == 0 && state_buffer[buffer_writeposition].state_index == 0 && state_buffer[buffer_writeposition].timeout == 0 && state_buffer[buffer_writeposition].played == 'R') {
        resetState();
        delay(100);
        return;
      }
      uint16_t ihash = state_buffer[buffer_writeposition].hash;
      state_buffer[buffer_writeposition].hash = 0;
      if ((!first_reading || state_buffer[buffer_writeposition].state_index==(last_readed_pos+1)) && ihash==crc.XModemCrc((uint8_t*) (void*) &(state_buffer[buffer_writeposition]), 0, sizeof(LEDRingState))) {
        first_reading = true;
        last_readed_pos = state_buffer[buffer_writeposition].state_index;
        buffer_writeposition=nextBufferPosition(buffer_writeposition);
        WaitORClear();
      } else {
        /*for(j=0; j<3; j++) {
          digitalWrite(13, HIGH);delay(50);digitalWrite(13, LOW);delay(50);
        }*/
        ClearSerial();
      }
      new_state = true;
    }
    if (first_reading) {
      if (buffer_playposition != buffer_writeposition) {
        if (!last_state_millis || (millis()-last_state_millis)>=last_state_delay) {
          setState(&(state_buffer[buffer_playposition]));
          buffer_playposition=nextBufferPosition(buffer_playposition);
          new_play_state = true;
        }
      } else if ((millis()-last_recieve_millis) >= MAX_RECIEVE_INTERVAL) {
        new_play_state = true;
        last_recieve_millis = millis();
      }
    }
    if (new_state || new_play_state) {
      sendPosConfirmation(last_readed_pos, false, false);
      sendPosConfirmation(last_played_pos, true, true);
    }

    delay(1);
  }
}
