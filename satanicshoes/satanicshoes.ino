
#include <Arduino.h>

#ifdef ESP32
  //#include <WiFi.h>
  #include "SPIFFS.h"
#else
  #include <ESP8266WiFi.h>
#endif
#include<Wire.h>
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


const int MPU_addr=0x68; // I2C address of the MPU-6050 
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
int acc_loop_period = 10000;
int loop_counter = 0;

bool check_I2c(byte addr){
  // We are using the return value of
  // the Write.endTransmisstion to see if
  // a device did acknowledge to the address.
  Serial.println(" ");
  Wire.beginTransmission(addr);
   
  if (Wire.endTransmission() == 0)
  {
  Serial.print(" Device Found at 0x");
  Serial.println(addr,HEX);
  return true;
  }
  else
  {
  Serial.print(" No Device Found at 0x");
  Serial.println(addr,HEX);
  return false;
  }
}

void acc_setup(){
  Wire.begin();
Serial.begin(115200);
 
check_I2c(MPU_addr); // Check that there is an MPU
 
Wire.beginTransmission(MPU_addr);
Wire.write(0x6B); // PWR_MGMT_1 register
Wire.write(0); // set to zero (wakes up the MPU-6050)
Wire.endTransmission(true);
}
void acc_loop(){
Wire.beginTransmission(MPU_addr);
Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H)
Wire.endTransmission(false);
Wire.requestFrom(MPU_addr,14,true); // request a total of 14 registers
AcX=Wire.read()<<8|Wire.read(); // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
AcY=Wire.read()<<8|Wire.read(); // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
AcZ=Wire.read()<<8|Wire.read(); // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
Tmp=Wire.read()<<8|Wire.read(); // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
GyX=Wire.read()<<8|Wire.read(); // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
GyY=Wire.read()<<8|Wire.read(); // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
GyZ=Wire.read()<<8|Wire.read(); // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
Serial.print("AcX = "); Serial.print(AcX);
Serial.print(" | AcY = "); Serial.print(AcY);
Serial.print(" | AcZ = "); Serial.print(AcZ);
Serial.print(" | Tmp = "); Serial.print(Tmp);
Serial.print(" | GyX = "); Serial.print(GyX);
Serial.print(" | GyY = "); Serial.print(GyY);
Serial.print(" | GyZ = "); Serial.println(GyZ);
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

//acc_setup();

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

  loop_counter+=1;
  if(loop_counter % acc_loop_period == 0 ) {
//    acc_loop();    
  }



  //delay(1);
}
