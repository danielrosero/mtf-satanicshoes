
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


int number_of_tracks = 2;

char *tracks[] = {"/bass.mp3", 
              "/15seconds.mp3"
};

AudioGeneratorMP3 *mp3;
AudioFileSourceSPIFFS *file;
AudioOutputI2SNoDAC *out;
AudioFileSourceID3 *id3;

int track_counter = 0;
const int hard_reset_counter = 50;
int debouncer = 0;

int threshold = 50;
int threshold_reset = threshold - 20; // Provides hysterisis
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


void start_mp3(int tracknumber) {
  Serial.println("Beginning MP3");    
  file = new AudioFileSourceSPIFFS(tracks[tracknumber]);
  id3 = new AudioFileSourceID3(file);
  id3->RegisterMetadataCB(MDCallback, (void*)"ID3TAG");
  mp3 = new AudioGeneratorMP3();
  Serial.println("Begin");    
  mp3->begin(id3, out);
  Serial.println("Started MP3");    
}

void end_mp3() {
  Serial.println("Ending MP3");    
  //id3 = NULL;
  mp3->stop();
 
 
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

  
  out = new AudioOutputI2SNoDAC();
  
  value = analogRead(sensorPin);


    Serial.print(" Pizo value: ");    
    Serial.println(value);   

    start_mp3(0);
}


void loop()
{
  //Read in Data
  value = analogRead(sensorPin);  
    
  if (value > threshold && debouncer > 10) {
    Serial.print(" Pizo value: ");    
    Serial.println(value);    
    debouncer = 0;
    end_mp3();
    track_counter+=1;
    Serial.print(" Track counter: ");    
    Serial.println(track_counter);    
    if ( hard_reset_counter && track_counter>50) {
    Serial.println(" Too many tracks rebooting. What a hack! ");      
      ESP.restart();}
    
    
      if (track_counter % 5 == 0 ) {
        start_mp3(1);
      } else {
        start_mp3(0);
      }
    }
   else {
    if (value < threshold_reset) { debouncer+=1; }
    
    if (mp3 != NULL && mp3->isRunning()) {
      if ( !mp3->loop() ) {
        end_mp3();
      }
    } 
    
   }

  //delay(1);
}
