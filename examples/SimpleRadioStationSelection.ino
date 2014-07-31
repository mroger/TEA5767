#include <Wire.h>
#include <TEA5767N.h>

TEA5767N radio = TEA5767N();

void setup() {

  radio.selectFrequency(89.1);
  
}
 
void loop() {

}


