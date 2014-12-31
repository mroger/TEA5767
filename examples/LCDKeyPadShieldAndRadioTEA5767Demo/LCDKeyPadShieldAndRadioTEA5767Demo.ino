/*
   Copyright 2014 Marcos R. Oliveira

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <LiquidCrystal.h>
#include <TEA5767N.h>
#include <Wire.h>
#include <stdlib.h>
#include <EEPROM.h>

// Init radio object
TEA5767N radio = TEA5767N();
// Init LCD display object
// LiquidCrystal lcd(RS, E, D4, D5, D6, D7);
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Buttons codes
#define btnRIGHT    0
#define btnUP       1
#define btnDOWN     2
#define btnLEFT     3
#define btnSELECT   4
#define btnNONE     5

// Delay between clock transitions
#define DELAY_VOLUME_TRANSITION 50

// Definition of the 3-dimension array
#define MENU_DEPTH 3
#define MENU_LINES 8
#define MENU_TEXT  16

// Arduino pin for backlight intensity control
#define BACKLIGHT_PIN 10

// Arduino pins connected to digital potentiometers
// for volume control
#define UPDOWN_PIN    11
#define INC_PIN       12

// Predefined stations array
float defaultStations[16] = {88.1, 89.1, 89.7, 91.3, 92.5, 93.7, 94.7, 95.3, 96.1, 98.5, 100.1, 100.9, 101.7, 102.1, 102.9, 103.3};
// Initialized station arrays
float stations[16] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
// Keep the current station index
byte stationIndex = 0;
// Keep the application state where the execution
// is trapped so a specific set of menu items is shown to the user
byte applicationState = 0;
// Index of the menu item currently selected
byte selectedMenuItem = 0;
// Used for fine search commands
float selectedStation;
// This variable helps to keep track of the buttons state:
// the command for a pressed button is only accepted if 
// previously it was in  not pressed state
boolean buttonWasReleased = true;

// LCD key that was pressed
int lcd_key = 0;

// Menu labels
char menu[MENU_DEPTH][MENU_LINES][MENU_TEXT] = {
   {{" Mute"}, {" Search"}, {" Fine search"}, {" Register statn"}, {" Configuration"}, {" Stand by"}, {" Load deflt stn"}, {" Exit"}},
   {{" Search level"}, {" Backlit inten."}, {" Exit"}},
   {{" Low"}, {" Medium"}, {" High"}, {"Exit"}}
  };

// Level required to search
byte searchLevel;
// Intensity of LCD backlight.
// Varies from 0 to 255
int backlightIntensity;

// Copy the predefined stations into the
// array used by the application
void loadDefaultStations() {
  for (int i=0 ; i < 16 ; i++) {
     stations[i] = defaultStations[i];
  }
}

// Convert the analog values read from Arduino pin A0
// into one of the five commands plus the NONE button 
int read_LCD_buttons() {
  int adc_key_in = analogRead(0);
  if (adc_key_in > 1000) return btnNONE; 
  if (adc_key_in < 50) return btnRIGHT; 
  if (adc_key_in < 195) return btnUP; 
  if (adc_key_in < 380) return btnDOWN; 
  if (adc_key_in < 555) return btnLEFT; 
  if (adc_key_in < 790) return btnSELECT; 
  return btnNONE;
}

// Given a station (e.g. 102.1), returns its index
// in the array
byte findSelectedStationIndex(float station) {
  for (int i = 0 ; i < 16 ; i++) {
    if (stations[i] == station) {
      return i;
    }
  }
  return 0;
}

// Load the station stored in the EEPROM static memory
void loadStation() {
  byte intStation, floatStation;
  float station;
  
  intStation = EEPROM.read(0);
  floatStation = EEPROM.read(1);
  
  if (intStation != 0xFF) {
    station = (intStation * 1.0) +  (floatStation * .1);
    radio.selectFrequency(station);
    printSelectedFrequency(radio.readFrequencyInMHz());
    stationIndex = findSelectedStationIndex(station);
  } else {
    radio.selectFrequency(stations[stationIndex]);
  }
}

// Load the search level stored in the EEPROM static memory
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

// Load the backlight intensity stored in the EEPROM static memory
void loadBacklightIntensity() {
  backlightIntensity = EEPROM.read(3);
  analogWrite(BACKLIGHT_PIN, (byte) backlightIntensity);
}

// Interacts directly with the digital potentiometer
// to control the volume. Firstly puts the volume to a minimum
// and then raises it to a pleasant level (both channels)
void setupVolume() {
  // Lowest volume
  digitalWrite(UPDOWN_PIN, LOW);
  for (int i = 0 ; i < 100 ; i++) {
    digitalWrite(INC_PIN, LOW);
    delay(1);
    digitalWrite(INC_PIN, HIGH);
    delay(1);
  }
  // Pleasant level
  digitalWrite(UPDOWN_PIN, HIGH);
  for (int i = 0 ; i < 15 ; i++) {
    digitalWrite(INC_PIN, LOW);
    delay(1);
    digitalWrite(INC_PIN, HIGH);
    delay(1);
  }
}

// This is a facade for all configurations that must
// be loaded on startup. Starts muting the radio to
// avoid all the annoying noise
void loadConfiguration() {
  radio.mute();
  loadDefaultStations();
  loadStation();
  loadSearchLevel();
  loadBacklightIntensity();
  setupVolume();
  radio.turnTheSoundBackOn();
}

// Saves the station passed as parameter in the EEPROM
// The station is stored in two parts
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

// Saves the search level in the EEPROM
void saveSearchLevel(byte searchLevel) {
  EEPROM.write(2, searchLevel);
}

// Does all the initialization of the application
// - Confiration of the pins
// - Initialization of the LCD
// - Load of the configuration
// - Smoothing of the radio noise
void setup(){
  pinMode(BACKLIGHT_PIN, OUTPUT);  
  pinMode(UPDOWN_PIN, OUTPUT);
  pinMode(INC_PIN, OUTPUT);
  
  digitalWrite(UPDOWN_PIN, HIGH);
  digitalWrite(INC_PIN, HIGH);
  analogWrite(BACKLIGHT_PIN, 255);
  
  lcd.begin(16, 2); 
  lcd.setCursor(6,0);
  lcd.print("MHz");
  
  // Loads configuration
  loadConfiguration();
  
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
  if (radio.isMuted()) {
    lcd.print("M");
  } else {
    lcd.print(" ");
  }
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

void loop() {
  // Keeps looking for new commands from LCD buttons
  lcd_key = read_LCD_buttons();
  
  //Any button turns the radio on again, after it's released
  //to avoid spurious commands.
  if ((lcd_key != btnNONE) && radio.isStandBy() && buttonWasReleased) {
    buttonWasReleased = false;
    applicationState = 0;
    
    //Necessary to eliminate noise while turning the radio back on
    radio.mute();
    radio.setStandByOff();
    //Necessary to elliminate noise while turning the radio back on
    delay(150);
    radio.turnTheSoundBackOn();
    analogWrite(BACKLIGHT_PIN, (byte) backlightIntensity);
    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("      MHz");
    
    // Starts station
    printSelectedFrequency(radio.readFrequencyInMHz());
  }
  
  //Interprets user commands using button code and the current
  //application state in the moment the button was pressed and released
  switch (lcd_key) {
    // Right button was pressed
    case btnRIGHT: {
      // If the button was released...
      if (buttonWasReleased) {
        // ... now it's pressed
        buttonWasReleased = false;
        // What is the current application state
        switch (applicationState) {
          // First screen, so change station
          case 0: {
            if ((stationIndex < 15) && (stations[stationIndex+1] != 0.0)) {
              stationIndex++;
            } else {
              stationIndex = 0;
            }
            // Use the library to select the station
            radio.mute();
            radio.selectFrequency(stations[stationIndex]);
            radio.turnTheSoundBackOn();
            
            printSelectedFrequency(radio.readFrequencyInMHz());
            saveStation(stations[stationIndex]);
            break;
          }
          // Search ahead
          case 5: {
            byte isBandLimitReached = false;
            if (selectedMenuItem == 1) {
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
          // Increment fine search station
          case 6: {
            if (selectedStation < 108.0) {
              selectedStation += .1;
              radio.selectFrequency(selectedStation);
              printSelectedFrequency(selectedStation, 2, 0);
              saveStation(selectedStation);
            }
            break;
          }
          // Increment backlight intensity
          case 7: {
            backlightIntensity += 10;
            if (backlightIntensity > 255) {
              backlightIntensity = 255;
            }
            // Adjusts the level of the backlight intensity
            analogWrite(BACKLIGHT_PIN, (byte) backlightIntensity);
            EEPROM.write(3, (byte) backlightIntensity);
            break;
          }
        }
      }
      break;
    }
    // For the left button is pretty much the same logic
    // as is for the right button 
    case btnLEFT: {
      if (buttonWasReleased) {
        buttonWasReleased = false;
        switch (applicationState) {
          // First screen, so change station
          case 0: {
            if (stationIndex > 0) {
              stationIndex--;
            } else {
              stationIndex = 15;
              while((stations[stationIndex] == 0.0) && (stationIndex > 0)) {
                stationIndex--;
              }
            }
            radio.mute();
            radio.selectFrequency(stations[stationIndex]);
            radio.turnTheSoundBackOn();
            printSelectedFrequency(radio.readFrequencyInMHz());
            saveStation(stations[stationIndex]);
            break;
          }
          // Search backwards
          case 5: {
            byte isBandLimitReached;
            if (selectedMenuItem == 1) {
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
          // Decrement fine search station
          case 6: {
            if (selectedStation > 88.0) {
              selectedStation -= .1;
              radio.selectFrequency(selectedStation);
              printSelectedFrequency(selectedStation, 2, 0);
              saveStation(selectedStation);
            }
            break;
          }
          // Decrement backlight intensity
          case 7: {
            backlightIntensity -= 10;
            if (backlightIntensity < 0) {
              backlightIntensity = 0;
            }
            analogWrite(BACKLIGHT_PIN, (byte) backlightIntensity);
            EEPROM.write(3, (byte) backlightIntensity);
            break;
          }
        }
      }
      break;
    }
    case btnUP: {
      // While turning up and down the volume, 
      // it's ok let it execute continuously
      if (buttonWasReleased || (applicationState == 0)) {
        buttonWasReleased = false;
        switch (applicationState) {
          //Volume UP
          case 0: {
            digitalWrite(UPDOWN_PIN, HIGH);
            digitalWrite(INC_PIN, LOW);
            delay(DELAY_VOLUME_TRANSITION);
            digitalWrite(INC_PIN, HIGH);
            delay(DELAY_VOLUME_TRANSITION);
            break;
          }
          // Menu navigation up
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
          // Selecting one of the three menu items: "Search level", "Backlit inten." or "Exit"
          case 3: {
            if (selectedMenuItem > 0) {
              selectedMenuItem--;
              
              if (selectedMenuItem != 3) {
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print(menu[1][selectedMenuItem]);
                lcd.setCursor(0,1);
                lcd.print(menu[1][selectedMenuItem+1]);
              }
            }
            markSelectedMenuItem(">", " ");
            break;
          }
          // Search level navigation up
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
    // For the down button is pretty much the same logic
    // as is for the up button 
    case btnDOWN: {
      //For up and down volume it´s ok let it execute
      //continuously
      if (buttonWasReleased || (applicationState == 0)) {
        buttonWasReleased = false;
        switch (applicationState) {
          //Volume DOWN
          case 0: {
            digitalWrite(UPDOWN_PIN, LOW);
            digitalWrite(INC_PIN, LOW);
            delay(DELAY_VOLUME_TRANSITION);
            digitalWrite(INC_PIN, HIGH);
            delay(DELAY_VOLUME_TRANSITION);
            break;
          }
          // Menu navigation down
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
          // Selecting one of the three menu items: "Search level", "Backlit inten." or "Exit"
          case 3:
            if (selectedMenuItem < 2) {
              selectedMenuItem++;
              
              if (selectedMenuItem != 1) {
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print(menu[1][selectedMenuItem-1]);
                lcd.setCursor(0,1);
                lcd.print(menu[1][selectedMenuItem]);
              }
            }
            markSelectedMenuItem(" ", ">");
            break;
          // Search level navigation down
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
    // Typically starts the menu or execute a command
    case btnSELECT: {
      if (buttonWasReleased) {
        buttonWasReleased = false;
        switch (applicationState) {
          // Show SELECT menu items
          // >Mute
          //  Search
          case 0: {
            applicationState = 1;
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
              //Mute function
              case 0: {
                applicationState = 0;
                if (!radio.isMuted()) {
                  radio.mute();
                } else {
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
              //Search
              case 1: {
                applicationState = 5;
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("<");
                lcd.setCursor(15, 0);
                lcd.print(">");
                printSelectedFrequency(radio.readFrequencyInMHz(), 2, 0);
                break; 
              }
              //Fine search
              case 2: {
                applicationState = 6;
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("<<");
                lcd.setCursor(14, 0);
                lcd.print(">>");
                selectedStation = radio.readFrequencyInMHz();
                printSelectedFrequency(selectedStation, 2, 0);
                break; 
              }
              // Register stations
              // Stores all stations found in the array
              case 3: {
                byte isBandLimitReached = 0;
                byte progress = 0;
                applicationState = 0;
                
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("Searching ...");
                
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
              //Configuration
              case 4: {
                applicationState = 3;
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print(menu[1][0]);
                lcd.setCursor(0,1);
                lcd.print(menu[1][1]);
                markSelectedMenuItem(">", " ");
                selectedMenuItem = 0;
                break;
              }
              //Puts the radio in standy by mode and turns off
              //the display backlight as a way to save energy
              case 5: {
                applicationState = 2;
                lcd.clear();
                
                radio.setStandByOn();
                analogWrite(BACKLIGHT_PIN, 0);
                
                break; 
              }
              //Load default station
              case 6: {
                applicationState = 0;
                
                lcd.setCursor(0,0);
                lcd.print("      MHz        ");
                lcd.setCursor(0,1);
                lcd.print("                 ");
                
                loadDefaultStations();
                
                radio.mute();
                radio.selectFrequency(stations[0]);
                radio.turnTheSoundBackOn();
                
                printSelectedFrequency(radio.readFrequencyInMHz());
                saveStation(stations[stationIndex]);
                break;
              }
              //Exit
              case 7: {
                applicationState = 0;
                
                lcd.setCursor(0,0);
                lcd.print("      MHz        ");
                lcd.setCursor(0,1);
                lcd.print("                 ");
                printMuteStatus();
                
                // Starts station
                printSelectedFrequency(radio.readFrequencyInMHz());
                break;
              }
            }
            break;
          }
          // One of the 3 itens selected: "Search level", "Backlit intensity" or "Exit"
          case 3: {
            switch(selectedMenuItem) {
              // Search level item selected
              case 0: {
                applicationState = 4;
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
              // Backlight intensity selected
              case 1: {
                applicationState = 7;
            
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("Backlight int.");
                break;
              }
              // Exit
              case 2: {
                applicationState = 0;
            
                lcd.setCursor(0,0);
                lcd.print("      MHz        ");
                lcd.setCursor(0,1);
                lcd.print("                 ");
                printMuteStatus();
                
                // Starts station
                printSelectedFrequency(radio.readFrequencyInMHz());
                break; 
              }
            }
            break; 
          }
          // One of the levels selected
          case 4: {
            applicationState = 0;
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
            printMuteStatus();
            
            // Starts station
            printSelectedFrequency(radio.readFrequencyInMHz());
            break; 
          }
          // Exit
          // Several states to exit from
          case 5: // Exit from Search
          case 6: // Exit from Fine search
          case 7: { // Exit from Backlight intensity
            applicationState = 0;
            
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("      MHz        ");
            printMuteStatus();
            
            // Starts station
            printSelectedFrequency(radio.readFrequencyInMHz());
            break; 
          }
        }
      }
      break;
    }
    // The buttons were scanned and none were pressed
    case btnNONE: {
      if (!buttonWasReleased) {
        buttonWasReleased = true;
      }
      // Since no button was pressed, takes to update some states
      switch (applicationState) {
        case 0: {
          // If radio is on...
          if (!radio.isStandBy()) {
            // ... prints S(Stereo) or M(Mono)
            printStereoStatus();
          }
          // Updates the bar graph with the signal level read from the radio
          updateLevelIndicator();
          break;
        }
        case 5: {
          // Updates the bar graph with the signal level read from the radio
          updateLevelIndicator(); 
          break;
        }
      }
      break;
    }
  }
  // Let's take a breath... :)
  delay(10);
}
