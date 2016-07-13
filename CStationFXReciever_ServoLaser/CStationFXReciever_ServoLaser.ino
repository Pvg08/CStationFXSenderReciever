/* 
 *  This is program for control of
 *  2 Servos + Laser diode
 *  via USB
*/

#include <Crc16.h>
#include <Servo.h>

#define SERVO_X_PIN 8
#define SERVO_Y_PIN 9
#define LASER_PIN 51

#define SERVOX_MIN_ANGLE 45
#define SERVOX_MAX_ANGLE 135
#define SERVOY_MIN_ANGLE 15
#define SERVOY_MAX_ANGLE 100
#define SERVOX_FACE_DIR_ANGLE 100
#define SERVOY_FACE_DIR_ANGLE 65

#define SERVO_FULL_DELAY 50
#define SERVO_MIN_DELAY 5

#define STATE_BUFFER_SIZE 2
#define BAUD_RATE 115200
#define MAX_RECIEVE_INTERVAL 25000

#define CMD_PLAY 0
#define CMD_NONE 1
#define CMD_RESET 'R'
#define CMD_SLEEP 'S'

#define __PACKED __attribute__((packed))

struct StateStruct {
    uint32_t state_index __PACKED;
    uint32_t timeout __PACKED;
    uint16_t command __PACKED;
    uint16_t hash __PACKED;
} __PACKED;

struct ServoLaserState : StateStruct {
  uint8_t x __PACKED;
  uint8_t y __PACKED;
  uint8_t laser __PACKED;
} __PACKED;

#define STATE_SIZE sizeof(ServoLaserState)

struct ServoLaserState state_buffer[STATE_BUFFER_SIZE] = {};
ServoLaserState state_old;

unsigned buffer_playposition;
unsigned buffer_writeposition;
bool first_reading;
uint32_t last_played_pos;
uint32_t last_readed_pos;
uint32_t last_recieve_millis;
uint32_t last_state_millis;
uint32_t last_state_delay;

Crc16 crc;
Servo myservo_x;
Servo myservo_y;

void deattachServos() {
  if (myservo_x.attached()) myservo_x.detach();
  if (myservo_y.attached()) myservo_y.detach();
}

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
  state_old.x = 0;
  state_old.y = 0;
  state_old.laser = 0;
  analogWrite(SERVO_X_PIN, 0);
  analogWrite(SERVO_Y_PIN, 0);
  analogWrite(LASER_PIN, 0);
  if (!myservo_x.attached()) myservo_x.attach(SERVO_X_PIN);
  if (!myservo_y.attached()) myservo_y.attach(SERVO_Y_PIN);
  myservo_x.write(0);
  myservo_y.write(0);
}

void setup() {
  Serial.begin(BAUD_RATE);
  pinMode(13, OUTPUT);
  pinMode(SERVO_X_PIN, OUTPUT);
  pinMode(SERVO_Y_PIN, OUTPUT);
  pinMode(LASER_PIN, OUTPUT);
  resetState();
}

unsigned nextBufferPosition(unsigned cpos)
{
  cpos++;
  if (cpos>=STATE_BUFFER_SIZE) cpos = 0;
  return cpos;
}

void setState(ServoLaserState* state) {
  if (state->command != CMD_PLAY && state->command != CMD_SLEEP) return;
  bool sx_changed = false, sy_changed = false;
  float delay_k = 0;
  if (state_old.x != state->x) {
    uint8_t angle_x = state->x;
    if (angle_x<SERVOX_MIN_ANGLE) angle_x = SERVOX_MIN_ANGLE;
    if (angle_x>SERVOX_MAX_ANGLE) angle_x = SERVOX_MAX_ANGLE;
    if (state_old.x != angle_x) {
      delay_k += abs(state_old.x - angle_x);
      state_old.x = angle_x;
      if (!myservo_x.attached()) myservo_x.attach(SERVO_X_PIN);
      myservo_x.write(angle_x);
      sx_changed = true;
    }
  }
  if (state_old.y != state->y) {
    uint8_t angle_y = state->y;
    if (angle_y<SERVOY_MIN_ANGLE) angle_y = SERVOY_MIN_ANGLE;
    if (angle_y>SERVOY_MAX_ANGLE) angle_y = SERVOY_MAX_ANGLE;
    if (state_old.y != angle_y) {
      delay_k += abs(state_old.y - angle_y);
      state_old.y = angle_y;
      if (!myservo_y.attached()) myservo_y.attach(SERVO_Y_PIN);
      myservo_y.write(angle_y);
      sy_changed = true;
    }
  }
  if (state_old.laser != state->laser) {
    digitalWrite(LASER_PIN, state_old.laser = state->laser);
  }
  if (sx_changed || sy_changed) {
    delay_k = SERVO_MIN_DELAY + ((SERVO_FULL_DELAY-SERVO_MIN_DELAY) * delay_k/180);
  }
  if (state->command == CMD_SLEEP) {
    deattachServos();
  }
  state->command = CMD_NONE;
  last_played_pos = state->state_index;
  last_state_delay = state->timeout > delay_k ? state->timeout : round(delay_k);
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
  if (Serial.available()>=STATE_SIZE) return;
  unsigned avail = 0;
  while (avail < Serial.available()) {
    avail = Serial.available();
    delay(5);
  }
}

void loop() { 
  bool new_state, new_play_state;

  delay(100);
  deattachServos();

  while (true) {
    new_state = false;
    new_play_state = false;
    while (Serial.available() >= STATE_SIZE) {
      last_recieve_millis = millis();
      char* next_pos = (char*) (void*) &(state_buffer[buffer_writeposition]);
      for(byte i=0; i<STATE_SIZE; i++) {
        next_pos[i] = Serial.read();
      }
      if (state_buffer[buffer_writeposition].hash == 0 && state_buffer[buffer_writeposition].state_index == 0 && state_buffer[buffer_writeposition].timeout == 0 && state_buffer[buffer_writeposition].command == CMD_RESET) {
        resetState();
        delay(100);
        return;
      }
      uint16_t ihash = state_buffer[buffer_writeposition].hash;
      state_buffer[buffer_writeposition].hash = 0;
      if ((!first_reading || state_buffer[buffer_writeposition].state_index==(last_readed_pos+1)) && ihash==crc.XModemCrc((uint8_t*) (void*) &(state_buffer[buffer_writeposition]), 0, STATE_SIZE)) {
        first_reading = true;
        last_readed_pos = state_buffer[buffer_writeposition].state_index;
        buffer_writeposition=nextBufferPosition(buffer_writeposition);
        WaitORClear();
      } else {
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
        deattachServos();
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
