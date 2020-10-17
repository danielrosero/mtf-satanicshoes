/*  Example applying an ADSR envelope to an audio signal
    with Mozzi sonification library.  This shows
    internal updates as well as interpolation at CONTROL_RATE, using both
    update() and next()  in updateControl().
    This is less computationally intensive than doing interpolation with
    next() in updateAudio(), but can be less smooth, especially with fast envelopes.

    Demonstrates a simple ADSR object being controlled with
    noteOn() and noteOff() instructions.

    Tim Barrass 2013-14, CC by-nc-sa.
*/

#include <MozziGuts.h>
#include <Oscil.h>
#include <EventDelay.h>
#include <ADSR.h>
#include <tables/sin8192_int8.h>
#include <mozzi_rand.h>
#include <mozzi_midi.h>
#include "Wire.h"

#define CONTROL_RATE 128 // faster than usual to help smooth CONTROL_RATE adsr interpolation (next())

Oscil<8192, AUDIO_RATE> aOscil(SIN8192_DATA);
;

// for triggering the envelope
EventDelay noteDelay;

ADSR<CONTROL_RATE, CONTROL_RATE> envelope;

boolean note_is_on = true;
byte gain;

int threshold = 250;
const int sensorPin = A0;
int value = 0;

unsigned int duration, attack, decay, sustain, release_ms;

byte check_I2c(byte addr)
{
  // We are using the return value of
  // the Write.endTransmisstion to see if
  // a device did acknowledge to the address.
  byte error;
  Wire.beginTransmission(addr);
  error = Wire.endTransmission();

  if (error == 0)
  {
    Serial.print(" Device Found at 0x");
    Serial.println(addr, HEX);
  }
  else
  {
    Serial.print(" No Device Found at 0x");
    Serial.println(addr, HEX);
  }
  return error;
}

const int MPU_addr = 0x68; // I2C address of the MPU-6050

int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;

void setup()
{
  //Serial.begin(9600); // for Teensy 3.1, beware printout can cause glitches
  Serial.begin(115200);
  randSeed();         // fresh random
  noteDelay.set(100); // 2 second countdown
  startMozzi(CONTROL_RATE);
  Wire.begin();

  check_I2c(MPU_addr); // Check that there is an MPU

  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0);    // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
}

void loop()
{
  audioHook(); // required here
}

void updateControl()
{
  value = analogRead(sensorPin);
  if (value > threshold && noteDelay.ready())
  {
    Serial.println("BLAM");

    // choose envelope levels
    byte attack_level = 209;
    byte decay_level = 242;

    envelope.setADLevels(attack_level, decay_level);

    // generate a random new adsr time parameter value in milliseconds
    unsigned int new_value = rand(300) + 100;
    Serial.println(new_value);
    // randomly choose one of the adsr parameters and set the new value
    // switch (rand(4))
    // {
    // case 0:
    //   attack = new_value;
    //   break;

    // case 1:
    //   decay = new_value;
    //   break;

    // case 2:
    //   sustain = new_value;
    //   break;

    // case 3:
    //   release_ms = new_value;
    //   break;
    // }

     attack = 703;
     decay = 1000;
     sustain = 1000;
     release_ms = 1000;

     

    envelope.setTimes(attack, decay, sustain, release_ms);
    envelope.noteOn();

    byte midi_note = map(AcX, 2000, -10000, 0, 100);
    aOscil.setFreq((int)mtof(midi_note));

    /*
     // print to screen
     Serial.print("midi_note\t"); Serial.println(midi_note);
     Serial.print("attack_level\t"); Serial.println(attack_level);
     Serial.print("decay_level\t"); Serial.println(decay_level);
     Serial.print("attack\t\t"); Serial.println(attack);
     Serial.print("decay\t\t"); Serial.println(decay);
     Serial.print("sustain\t\t"); Serial.println(sustain);
     Serial.print("release\t\t"); Serial.println(release_ms);
     Serial.println();
*/
    noteDelay.start(attack + decay + sustain + release_ms);
  }
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 14, true); // request a total of 14 registers
  AcX = Wire.read() << 8 | Wire.read(); // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
  AcY = Wire.read() << 8 | Wire.read(); // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ = Wire.read() << 8 | Wire.read(); // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  Tmp = Wire.read() << 8 | Wire.read(); // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
  GyX = Wire.read() << 8 | Wire.read(); // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
  GyY = Wire.read() << 8 | Wire.read(); // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
  GyZ = Wire.read() << 8 | Wire.read(); // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
  Serial.print("AcX = ");
  Serial.print(AcX);
  Serial.print(" | AcY = ");
  Serial.print(AcY);
  Serial.print(" | AcZ = ");
  Serial.print(AcZ);
  Serial.print(" | Tmp = ");
  Serial.print(Tmp);
  Serial.print(" | GyX = ");
  Serial.print(GyX);
  Serial.print(" | GyY = ");
  Serial.print(GyY);
  Serial.print(" | GyZ = ");
  Serial.println(GyZ);

  delay(50); // Wait 0.5 seconds and scan again
}

int updateAudio()
{
  envelope.update();
  return (int)(envelope.next() * aOscil.next()) >> 8;
}
