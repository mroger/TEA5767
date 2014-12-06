#include <Serial.h>
#include <Wire.h>
#include <TEA5767N.h>

TEA5767N radio = TEA5767N();

/*
  This code shows some of the uses of the TEA5767N API.
*/

void setup() {
  byte isBandLimitReached = 0;

  //Sets the radio station
  radio.selectFrequency(89.1);
  delay(5000);
  
  radio.mute();
  delay(2000);
  radio.turnTheSoundBackOn();
  delay(2000);
  
  //Observation: At this point your Serial Monitor window should be open.
  
  radio.muteRight();
  delay(2000);
  Serial.println(radio.isStereo() ? "Stereo" : "Mono");
  radio.turnTheRightSoundBackOn();
  delay(2000);
  radio.muteLeft();
  Serial.println(radio.isStereo() ? "Stereo" : "Mono");
  delay(2000);
  radio.turnTheLeftSoundBackOn();
  delay(2000);

  //Turns the radio off
  radio.setStandByOn();
  delay(2000);
  //Turns the radio on
  radio.setStandByOff();
  delay(2000);
  
  Serial.println("Search Up in progress...");
  radio.setSearchMidStopLevel();
  isBandLimitReached = radio.startsSearchMutingFromBeginning();
  
  float freq = radio.readFrequencyInMHz();
  Serial.println(freq, 1);
  delay(2000);
  
  while (!isBandLimitReached) {
    
    Serial.println("Search Up in progress...");
    //If you want listen to station search, use radio.searchNext() instead
    isBandLimitReached = radio.searchNextMuting();
    
    freq = radio.readFrequencyInMHz();
    Serial.println(freq, 1);
    delay(2000);
  }
  
  Serial.println("Band Limit Reached, searching down...");
  
  radio.setSearchDown();
  radio.setSearchMidStopLevel();
  
  Serial.println("Search Down in progress...");
  isBandLimitReached = radio.searchNextMuting();
  
  freq = radio.readFrequencyInMHz();
  Serial.println(freq, 1);
  delay(2000);
  
  while (!isBandLimitReached) {
    
    Serial.println("Search Down in progress...");
    isBandLimitReached = radio.searchNextMuting();
        
    freq = radio.readFrequencyInMHz();
    Serial.println(freq, 1);
    delay(2000);
  }
  
  radio.selectFrequency(89.1);

  //Read frequenci in MHz
  Serial.println(radio.readFrequencyInMHz(), 1);
  //Shows reception signal level
  Serial.println(radio.getSignalLevel());
  //Reception stereo or mono?
  Serial.println(radio.isStereo() ? "Stereo" : "Mono");
}
 
void loop() {
  
}
