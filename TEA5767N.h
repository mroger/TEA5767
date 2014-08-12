#include <Arduino.h>

#ifndef TEA5767N_h
#define TEA5767N_h

//Note that not all of these constants are being used. They are here for
//convenience, though.
#define TEA5767_I2C_ADDRESS           0x60
#define FIRST_DATA                    0
#define SECOND_DATA                   1
#define THIRD_DATA                    2
#define FOURTH_DATA                   3
#define FIFTH_DATA                    4

#define LOW_STOP_LEVEL                1
#define MID_STOP_LEVEL                2
#define HIGH_STOP_LEVEL               3

#define HIGH_SIDE_INJECTION           1
#define LOW_SIDE_INJECTION            0

#define STEREO_ON                     0
#define STEREO_OFF                    1

#define MUTE_RIGHT_ON                 1
#define MUTE_RIGHT_OFF                0
#define MUTE_LEFT_ON                  1
#define MUTE_LEFT_OFF                 0

#define SWP1_HIGH                     1
#define SWP1_LOW                      0
#define SWP2_HIGH                     1
#define SWP2_LOW                      0

#define STBY_ON                       1
#define STBY_OFF                      0

#define JAPANESE_FM_BAND              1
#define US_EUROPE_FM_BAND             0

#define SOFT_MUTE_ON                  1
#define SOFT_MUTE_OFF                 0

#define HIGH_CUT_CONTROL_ON           1
#define HIGH_CUT_CONTROL_OFF          0

#define STEREO_NOISE_CANCELLING_ON    1
#define STEREO_NOISE_CANCELLING_OFF   0

#define SEARCH_INDICATOR_ON           1
#define SEARCH_INDICATOR_OFF          0

class TEA5767N {
	private:
	  float frequency;
	  byte hiInjection;
	  byte frequencyH;
	  byte frequencyL;
	  byte transmission_data[5];
	  byte reception_data[5];
		
	  void setFrequency(float);
	  void transmitFrequency(float);
	  void transmitData();
	  void initializeTransmissionData();
	  void readStatus();
	  float getFrequencyInMHz(unsigned int);
	  void calculateOptimalHiLoInjection(float);
	  void setHighSideLOInjection();
	  void setLowSideLOInjection();
		
	public:
	  TEA5767N();
	  void selectFrequency(float);
	  void mute();
	  void turnTheSoundBackOn();
	  void muteLeft();
	  void turnTheLeftSoundBackOn();
	  void muteRight();
	  void turnTheRightSoundBackOn();
	  float readFrequencyInMHz();
	  void setSearchUp();
	  void setSearchDown();
	  void setSearchLowStopLevel();
	  void setSearchMidStopLevel();
	  void setSearchHighStopLevel();
	  void setStereoReception();
	  void setMonoReception();
	  void setSoftMuteOn();
	  void setSoftMuteOff();
	  
	  void setStandByOn();
	  void setStandByOff();
	  void setHighCutControlOn();
	  void setHighCutControlOff();
	  void setStereoNoiseCancellingOn();
	  void setStereoNoiseCancellingOff();
	  
	  void searchNext();
	  void searchFromBeginning();
	  void searchFromEnd();
	  byte getSignalLevel();
	  byte isStereo();
	  byte isReady();
	  byte isBandLimitReached();
	  
};

#endif