
#include <Arduino.h>
#ifdef ESP32
  #include <WiFi.h>
  #include "SPIFFS.h"
#else
  #include <ESP8266WiFi.h>
#endif
#include "AudioFileSourceSPIFFS.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"

// To run, set your ESP8266 build to 160MHz, and include a SPIFFS of 512KB or greater.
// Use the "Tools->ESP8266/ESP32 Sketch Data Upload" menu to write the MP3 to SPIFFS
// Then upload the sketch normally.  

// pno_cs from https://ccrma.stanford.edu/~jos/pasp/Sound_Examples.html

AudioGeneratorMP3 *mp3;
AudioFileSourceSPIFFS *file;
AudioOutputI2SNoDAC *out;
AudioFileSourceID3 *id3;

int threshold = 50;
const int sensorPin = A0;
int value = 0;
int randomSample = 0;

// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  (void)cbData;
  Serial.printf("ID3 callback for: %s = '", type);

  if (isUnicode) {
    string += 2;
  }
  
  while (*string) {
    char a = *(string++);
    if (isUnicode) {
      string++;
    }
    Serial.printf("%c", a);
  }
  Serial.printf("'\n");
  Serial.flush();
}


void start_mp3() {
  Serial.println("Beginning MP3");    
  file = new AudioFileSourceSPIFFS("/bass.mp3");
  id3 = new AudioFileSourceID3(file);
  id3->RegisterMetadataCB(MDCallback, (void*)"ID3TAG");
  mp3 = new AudioGeneratorMP3();
  Serial.println("Begin");    
  mp3->begin(id3, out);
  Serial.println("Started MP3");    
}

void end_mp3() {
  Serial.println("Ending MP3");    
  mp3->stop();
  //delete mp3;
  //delete id3;
  //delete out;
  //delete file;
  Serial.println("MP3 ended");    
}

void setup()
{
  WiFi.mode(WIFI_OFF); 
  Serial.begin(115200);
  delay(1000);
  SPIFFS.begin();
  Serial.printf("Sample MP3 playback begins...\n");

  audioLogger = &Serial;
  //file = new AudioFileSourceSPIFFS("/pno-cs.mp3");


  
  out = new AudioOutputI2SNoDAC();
  
  value = analogRead(sensorPin);


    Serial.print(" Pizo value: ");    
    Serial.println(value);   

    start_mp3();
}


void loop()
{
  //Read in Data
  value = analogRead(sensorPin);
  
  if (mp3 != NULL && mp3->isRunning()) {
    if ( !mp3->loop() ) {
      //mp3->stop();
      end_mp3();
    }
  } else {
    
    
    if (value > threshold) {
      Serial.print(" Pizo value: ");    
    Serial.println(value);    
      start_mp3();
    }
   
  }
  

}
