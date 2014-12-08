#include <LiquidCrystal.h>  // Insira as chaves menor no inicio e maior no final de LiquidCrystal.h
#include <TEA5767N.h>
#include <Wire.h>
#include <stdlib.h>
#include <EEPROM.h>
#include <Serial.h>

// Init radio object
TEA5767N radio = TEA5767N();
// Seleciona os pinos utilizados no Painel
LiquidCrystal lcd(8, 9, 4, 5, 6, 7); 

// Define as variaveis de botoes
int lcd_key = 0;
int adc_key_in = 0;

#define btnRIGHT    0
#define btnUP       1
#define btnDOWN     2
#define btnLEFT     3
#define btnSELECT   4
#define btnNONE     5

#define DELAY_VOLUME_TRANSITION 50

#define MENU_DEPTH 3
#define MENU_LINES 8
#define MENU_TEXT  16

int backLightPin = 10;
int upDownPin = 11;
int incPin = 12;

/*
Station found: 91.3 MHz
Station found: 98.5 MHz
Station found: 100.1 MHz
Station found: 101.7 MHz
Station found: 102.1 MHz
Station found: 107.4 MHz
Station found: 108.0 MHz
Station found: 95.4 MHz
Station found: 87.6 MHz
Station selected: 89.1 MHz
*/

float default_stations[16] = {87.5, 89.1, 91.3, 96.1, 98.5, 100.1, 100.9, 102.1, 107.4, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
float stations[16] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
byte stationIndex = 0;

byte state = 0;       //0 - 
                      //1 - 
                      //2 - 
                      //3 - 
                      //4 - 
                      //5 - 
                      //6 - 
byte selectedMenuItem = 0;
float selectedStation;
boolean isStandByOn = false;
boolean wasReleased = true;
boolean mute = false;

char menu[MENU_DEPTH][MENU_LINES][MENU_TEXT] = {
                       {{" Mute"}, {" Search"}, {" Fine search"}, {" Register statn"}, {" Configuration"}, {" Stand by"}, {" Load deflt stn"}, {" Exit"}},
                       {{" Search level"}, {" Backlit inten."}},
                       {{" Low"}, {" Medium"}, {" High"}}
                      };

byte searchLevel;

void loadDefaultStations() {
  for (int i=0 ; i < 16 ; i++) {
     //TODO Do this using pointers
     stations[i] = default_stations[i];
  }
}

int read_LCD_buttons(){
  adc_key_in = analogRead(0);
  if (adc_key_in > 1000) return btnNONE; 
  if (adc_key_in < 50) return btnRIGHT; 
  if (adc_key_in < 195) return btnUP; 
  if (adc_key_in < 380) return btnDOWN; 
  if (adc_key_in < 555) return btnLEFT; 
  if (adc_key_in < 790) return btnSELECT; 
  return btnNONE;
}

void loadStation() {
  byte intStation, floatStation;
  float station;
  
  intStation = EEPROM.read(0);
  floatStation = EEPROM.read(1);
  
  if (intStation != 0xFF) {
    station = (intStation * 1.0) +  (floatStation * .1);
    radio.selectFrequency(station);
  } else {
    radio.selectFrequency(stations[stationIndex]);
  }
}

void loadSearchLevel() {
  searchLevel = EEPROM.read(2);
  if (searchLevel == 0xFF) {
    searchLevel = MID_STOP_LEVEL;
  }
  switch(searchLevel) {
    case LOW_STOP_LEVEL: {
      radio.setSearchLowStopLevel();
      break;
    }
    case MID_STOP_LEVEL: {
      radio.setSearchMidStopLevel();
      break; 
    }
    case HIGH_STOP_LEVEL: {
      radio.setSearchHighStopLevel();
      break; 
    }
  }
}

void loadConfiguration() {
  //loadStation();
  loadDefaultStations();
  loadSearchLevel();
}

void saveStation(float station) {
  float aux;
  byte byteValue;
  
  byteValue = (byte) floor(station);
  EEPROM.write(0, byteValue);
  aux = station - floor(station);
  aux *= 10.0;
  if (aux > 5.0) {
    byteValue = byte(aux);
    if ((((float)byteValue) - aux) > 0.1) {
      byteValue--;
    } else if ((aux - ((float)byteValue)) > 0.1) {
      byteValue++;
    }
  } else {
    byteValue = (int) ceil(aux);
  }
  EEPROM.write(1, byteValue);
}

void saveSearchLevel(byte searchLevel) {
  EEPROM.write(2, searchLevel);
}

void setup(){
  pinMode(backLightPin, OUTPUT);  
  pinMode(upDownPin, OUTPUT);
  pinMode(incPin, OUTPUT);
  
  digitalWrite(upDownPin, HIGH);
  digitalWrite(incPin, HIGH);
  analogWrite(backLightPin, 255);
  
  lcd.begin(16, 2); 
  lcd.setCursor(6,0);
  lcd.print("MHz");
  
  // Loads configuration
  loadConfiguration();
  radio.selectFrequency(stations[0]);
  //delay(2000);
  printSelectedFrequency(radio.readFrequencyInMHz());
  //radio.setSearchMidStopLevel();
  //radio.selectFrequency(102.1);
  
  //From application_note_tea5767-8.pdf, limit the amount of noise energy.
  radio.setSoftMuteOn();
  //From application_note_tea5767-8.pdf, cut high frequencies from the audio signal.
  radio.setHighCutControlOn();
}

void printSelectedFrequency(float frequency) {
  printSelectedFrequency(frequency, 0, 0);
}

void printSelectedFrequency(float frequency, byte col, byte row) {
  char str_freq[5];
  dtostrf(frequency, 1, 1, str_freq);
  
  lcd.setCursor(col, row);
  lcd.print("     ");
  lcd.setCursor(col, row);
  lcd.print(str_freq);  
}

void printMuteStatus() {
  lcd.setCursor(12, 0);
  lcd.print("M");  
}

void printNotMuteStatus() {
  lcd.setCursor(12, 0);
  lcd.print(" ");  
}

void printStereoStatus() {
  lcd.setCursor(10, 0);
  if (radio.isStereo()) {
    lcd.print("S");
  } else {
    lcd.print("M");
  }
}

void markSelectedMenuItem(char *firstLine, char *secondLine) {
  lcd.setCursor(0,0);
  lcd.print(firstLine);
  lcd.setCursor(0,1);
  lcd.print(secondLine);
}

void updateLevelIndicator() {
  byte x, y, sl;
  char barGraph[17];
  
  lcd.setCursor(0,1);
  sl = radio.getSignalLevel();
  for (x=0 ; x<sl ; x++) {
    barGraph[x] = 255;
  }
  for (y=x ; y<16 ; y++) {
    barGraph[y] = 32;
  }
  barGraph[y] = '\0';
  lcd.print(barGraph);
}

void loop(){
  lcd_key = read_LCD_buttons();
  
  //Serial.print("Selected button: ");
  //Serial.println(lcd_key);
  
  //Any button turns the radio on again
  if ((lcd_key != btnNONE) && isStandByOn && wasReleased) {
    wasReleased = false;
    state = 0;
    isStandByOn = false;
    
    //Necessary to elliminate noise while turning the radio back on
    radio.mute();
    radio.setStandByOff();
    //Necessary to elliminate noise while turning the radio back on
    delay(150);
    radio.turnTheSoundBackOn();
    analogWrite(backLightPin, 255);
    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("      MHz");
    
    // Starts station
    printSelectedFrequency(radio.readFrequencyInMHz());
    //Serial.println("Consuming button click event");
    ////Consumes button click event to force to another valid choice
    //lcd_key = btnNONE;
  }
  
  switch (lcd_key) {
    case btnRIGHT: {
      if (wasReleased) {
        wasReleased = false;
        switch (state) {
          case 0: {
            if ((stationIndex < 16) && (stations[stationIndex+1] != 0.0)) {
              stationIndex++;
            } else {
              stationIndex = 0;
            }
            radio.selectFrequency(stations[stationIndex]);
            printSelectedFrequency(radio.readFrequencyInMHz());
            saveStation(stations[stationIndex]);
            break;
          }
          case 5: {
            byte isBandLimitReached;
            if (selectedMenuItem == 0) {
              radio.setSearchUp();
              isBandLimitReached = radio.searchNextMuting();
              if (isBandLimitReached) {
                radio.setSearchDown();
                isBandLimitReached = radio.searchNextMuting();
              }
              printSelectedFrequency(radio.readFrequencyInMHz(), 2, 0);
            }
            break; 
          }
          case 6: {
            if (selectedStation < 108.0) {
              selectedStation += .1;
              radio.selectFrequency(selectedStation);
              printSelectedFrequency(selectedStation, 2, 0);
            }
          }
        }
      }
      break;
    }
    case btnLEFT: {
      if (wasReleased) {
        wasReleased = false;
        switch (state) {
          case 0: {
            if (stationIndex > 0) {
              stationIndex--;
            } else {
              stationIndex = 6;
            }
            radio.selectFrequency(stations[stationIndex]);
            printSelectedFrequency(radio.readFrequencyInMHz());
            saveStation(stations[stationIndex]);
            break;
          }
          case 5: {
            byte isBandLimitReached;
            if (selectedMenuItem == 0) {
              radio.setSearchDown();
              isBandLimitReached = radio.searchNextMuting();
              if (isBandLimitReached) {
                radio.setSearchUp();
                isBandLimitReached = radio.searchNextMuting();
              }
              printSelectedFrequency(radio.readFrequencyInMHz(), 2, 0);
            }
            break; 
          }
          case 6: {
            if (selectedStation > 88.0) {
              selectedStation -= .1;
              radio.selectFrequency(selectedStation);
              printSelectedFrequency(selectedStation, 2, 0);
            }
          }
        }
      }
      break;
    } 
    case btnUP: {
      //For up and down volume it´s ok let it execute
      //continuously
      if (wasReleased || (state == 0)) {
        wasReleased = false;
        switch (state) {
          //Volume UP
          case 0: {
            digitalWrite(upDownPin, HIGH);
            digitalWrite(incPin, LOW);
            delay(DELAY_VOLUME_TRANSITION);
            digitalWrite(incPin, HIGH);
            delay(DELAY_VOLUME_TRANSITION);
            break;
          }
          case 1: {
            if (selectedMenuItem > 0) {
              selectedMenuItem--;
              
              if (selectedMenuItem != 3) {
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print(menu[0][selectedMenuItem]);
                lcd.setCursor(0,1);
                lcd.print(menu[0][selectedMenuItem+1]);
              }
            }
            markSelectedMenuItem(">", " ");
            break; 
          }
          case 4: {
            if (selectedMenuItem > 0) {
              selectedMenuItem--;
              
              if (selectedMenuItem != 2) {
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print(menu[2][selectedMenuItem]);
                lcd.setCursor(0,1);
                lcd.print(menu[2][selectedMenuItem+1]);
              }
            }
            markSelectedMenuItem(">", " ");
            break; 
          }
        }
      }
      break;
    }
    case btnDOWN: {
      //For up and down volume it´s ok let it execute
      //continuously
      if (wasReleased || (state == 0)) {
        wasReleased = false;
        switch (state) {
          //Volume DOWN
          case 0: {
            digitalWrite(upDownPin, LOW);
            digitalWrite(incPin, LOW);
            delay(DELAY_VOLUME_TRANSITION);
            digitalWrite(incPin, HIGH);
            delay(DELAY_VOLUME_TRANSITION);
            break;
          }
          case 1: {
            if (selectedMenuItem < (MENU_LINES-1)) {
              selectedMenuItem++;
              
              if (selectedMenuItem != 1) {
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print(menu[0][selectedMenuItem-1]);
                lcd.setCursor(0,1);
                lcd.print(menu[0][selectedMenuItem]);
              }
            }
            markSelectedMenuItem(" ", ">");
            break; 
          }
          case 4: {
            if (selectedMenuItem < 2) {
              selectedMenuItem++;
              if (selectedMenuItem != 1) {
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print(menu[2][selectedMenuItem-1]);
                lcd.setCursor(0,1);
                lcd.print(menu[2][selectedMenuItem]);
              }
            }
            markSelectedMenuItem(" ", ">");
            break; 
          }
        }
      }
      break;
    }
    case btnSELECT: {
      if (wasReleased) {
        wasReleased = false;
        switch (state) {
          // Show SELECT menu items
          // >Mute
          //  Search
          case 0: {
            state = 1;
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print(menu[0][0]);
            lcd.setCursor(0,1);
            lcd.print(menu[0][1]);
            markSelectedMenuItem(">", " ");
            selectedMenuItem = 0;
            break;
          }
          //Execute first level menu selection
          case 1: {
            switch(selectedMenuItem) {
              case 0: {
                state = 0;
                if (!mute) {
                  mute = true;
                  radio.mute();
                } else {
                  mute = false;
                  radio.turnTheSoundBackOn();
                }
                
                lcd.setCursor(0,0);
                lcd.print("      MHz        ");
                lcd.setCursor(0,1);
                lcd.print("                 ");
                printMuteStatus();
                
                // Starts station
                printSelectedFrequency(radio.readFrequencyInMHz());
                break; 
              }
              case 1: {
                state = 5;
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("<");
                lcd.setCursor(15, 0);
                lcd.print(">");
                printSelectedFrequency(radio.readFrequencyInMHz(), 2, 0);
                break; 
              }
              case 2: {
                state = 6;
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("<<");
                lcd.setCursor(14, 0);
                lcd.print(">>");
                selectedStation = radio.readFrequencyInMHz();
                printSelectedFrequency(selectedStation, 2, 0);
                break; 
              }
              case 3: {
                byte isBandLimitReached = 0;
                byte progress = 0;
                state = 0;
                lcd.clear();
                radio.mute();
                radio.selectFrequency(88.0);
                radio.setSearchUp();
                while(!isBandLimitReached && (progress < 16)) {
                  isBandLimitReached = radio.searchNext();
                  stations[progress] = radio.readFrequencyInMHz();
                  lcd.setCursor(progress, 1);
                  lcd.print((char)255);
                  progress++;
                }
                radio.turnTheSoundBackOn();
                
                lcd.setCursor(0,0);
                lcd.print("      MHz        ");
                lcd.setCursor(0,1);
                lcd.print("                 ");
                
                // Starts station
                stationIndex = 0;
                saveStation(stations[stationIndex]);
                printSelectedFrequency(stations[stationIndex]);
                
                break;
              }
              case 4: {
                state = 3;
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print(menu[1][0]);
                markSelectedMenuItem(">", " ");
                selectedMenuItem = 0;
                break;
              }
              //Puts the radio in standy by mode and turns off
              //the display backlight
              case 5: {
                state = 2;
                lcd.clear();
                
                radio.setStandByOn();
                analogWrite(backLightPin, 0);
                
                isStandByOn = true;
                break; 
              }
              case 6: {
                state = 0;
                
                lcd.setCursor(0,0);
                lcd.print("      MHz        ");
                lcd.setCursor(0,1);
                lcd.print("                 ");
                
                // Starts station
                printSelectedFrequency(radio.readFrequencyInMHz());
                break;
              }
              case 7: {
                state = 0;
                
                lcd.setCursor(0,0);
                lcd.print("      MHz        ");
                lcd.setCursor(0,1);
                lcd.print("                 ");
                
                // Starts station
                printSelectedFrequency(radio.readFrequencyInMHz());
                break;
              }
            }
            break;
          }
          case 3: {
            state = 4;
            lcd.clear();
            if (searchLevel < 3) {
              lcd.setCursor(0,0);
              lcd.print(menu[2][0]);
              lcd.setCursor(0,1);
              lcd.print(menu[2][1]);
            } else {
              lcd.setCursor(0,0);
              lcd.print(menu[2][1]);
              lcd.setCursor(0,1);
              lcd.print(menu[2][2]);
            }
            if (searchLevel == 1) {
              markSelectedMenuItem(">", " ");
            } else {
              markSelectedMenuItem(" ", ">");
            }
            selectedMenuItem = searchLevel - 1;
            break; 
          }
          case 4: {
            state = 0;
            switch(selectedMenuItem+1) {
              case LOW_STOP_LEVEL: {
                radio.setSearchLowStopLevel();
                saveSearchLevel(LOW_STOP_LEVEL);
                break;
              }
              case MID_STOP_LEVEL: {
                radio.setSearchMidStopLevel();
                saveSearchLevel(MID_STOP_LEVEL);
                break; 
              }
              case HIGH_STOP_LEVEL: {
                radio.setSearchHighStopLevel();
                saveSearchLevel(HIGH_STOP_LEVEL);
                break; 
              }
            }
            
            lcd.setCursor(0,0);
            lcd.print("      MHz        ");
            lcd.setCursor(0,1);
            lcd.print("                 ");
            
            // Starts station
            printSelectedFrequency(radio.readFrequencyInMHz());
            break; 
          }
          case 5: 
          case 6: {
            state = 0;
            
            lcd.setCursor(0,0);
            lcd.print("      MHz        ");
            lcd.setCursor(0,1);
            lcd.print("                 ");
            
            // Starts station
            printSelectedFrequency(radio.readFrequencyInMHz());
            break; 
          }
        }
      }
      break;
    }
    case btnNONE: {
      //Serial.println("Btn None");
      if (!wasReleased) {
        wasReleased = true;
      }
      switch (state) {
        case 0: {
          if (!isStandByOn) {
            printStereoStatus();
          }
          updateLevelIndicator();
          break;
        }
        case 5: {
          updateLevelIndicator(); 
          break;
        }
      }
      break;
    }
  }
  delay(10);
}
