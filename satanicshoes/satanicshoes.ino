#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

#include "AudioFileSourcePROGMEM.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"
#include "AudioOutputMixer.h"

// VIOLA sample taken from https://ccrma.stanford.edu/~jos/pasp/Sound_Examples.html
//#include "viola.h"
#include "kick.h"

AudioGeneratorWAV *wav[2];
AudioFileSourcePROGMEM *file[2];
AudioOutputI2S *out;
AudioOutputMixer *mixer;
AudioOutputMixerStub *stub[2];

int threshold = 500;
const int sensorPin = A0;
int value = 0;
int randomSample = 0;

void setup() {
  //file[0] = new AudioFileSourcePROGMEM(viola, sizeof(viola) );
  file[0] = new AudioFileSourcePROGMEM(kick, sizeof(kick) );
  out = new AudioOutputI2S();
  mixer = new AudioOutputMixer(32, out);
  stub[0] = mixer->NewInput();
  stub[0]->SetGain(0.4);
  wav[0] = new AudioGeneratorWAV();
  //Debug to ensure proper playback
  wav[0]->begin(file[0], stub[0]);

}

void loop() {
  //Read in Data
  value = analogRead(sensorPin);

  if (value > threshold) {

    randomSample=random(2);

    if(randomSample==0){
      
    }
    
    
    //Serial.println("BLAM");
    file[0]->open( kick, sizeof(kick) );
    wav[0]->begin(file[0], stub[0]);
    //delay(100);
  }
  if (wav[0]->isRunning()) {
    if ( !wav[0]->loop() ) {
      wav[0]->stop();
      stub[0]->stop();
    }
  }

}
