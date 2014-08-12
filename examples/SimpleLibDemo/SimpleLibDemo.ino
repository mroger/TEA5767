#include <Serial.h>
#include <Wire.h>
#include <TEA5767N.h>

TEA5767N radio = TEA5767N();

void setup() {

  //Sets the radio station
  radio.selectFrequency(89.1);
  delay(2000);
  
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
  radio.setStandByOff();
  
  radio.selectFrequency(107.9);
  radio.setSearchUp();
  radio.setSearchMidStopLevel();
  radio.searchNext();
  while(!radio.isReady()) {
    Serial.println("Search in progress...");
  }
  if (radio.isBandLimitReached()) {
     Serial.println("Band Limit Reached, searching down...");
     radio.setSearchDown();
     radio.searchNext();
  }
  delay(2000);
  
  //Reading the status
  //Read frequenci in MHz
  Serial.println(radio.readFrequencyInMHz(), 1);
  //Shows reception signal level
  Serial.println(radio.getSignalLevel());
  //Reception stereo or mono?
  Serial.println(radio.isStereo() ? "Stereo" : "Mono");
  //Ready for new instructions? Done with frequency selection and search?
  Serial.println(radio.isReady() ? "Ready" : "Not ready");
  //Has reached superior or inferior band limit?
  Serial.println(radio.isBandLimitReached() ? "Limit reached" : "Limit not reached");
}
 
void loop() {
  
}

