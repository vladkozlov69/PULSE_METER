/*
 * MAX301xxFilter.h
 *
 *  Created on: Oct 24, 2018
 *      Author: vkozloff@gmail.com
 *  Based on the code developed by Raivis Strogonovs (https://morf.lv)
 */

#ifndef MAX301XXFILTER_H_
#define MAX301XXFILTER_H_

#include <Arduino.h>
#include <math.h>


/* SaO2 parameters */
#define RESET_SPO2_EVERY_N_PULSES     4

/* Filter parameters */
#define ALPHA 0.95  //dc filter alpha value
#define MEAN_FILTER_SIZE        15

/* Pulse detection parameters */
#define PULSE_MIN_THRESHOLD         100 //300 is good for finger, but for wrist you need like 20, and there is shitloads of noise
#define PULSE_MAX_THRESHOLD         2000
#define PULSE_GO_DOWN_THRESHOLD     1

#define PULSE_BPM_SAMPLE_SIZE       10 //Moving average size


/* Enums, data structures and typdefs. DO NOT EDIT */
struct pulseoxymeter_t {
  bool pulseDetected;
  float heartBPM;

  float irCardiogram;

  float irDcValue;
  float redDcValue;

  float SaO2;

  uint32_t lastBeatThreshold;

  float dcFilteredIR;
  float dcFilteredRed;
};

typedef enum PulseStateMachine {
    PULSE_IDLE,
    PULSE_TRACE_UP,
    PULSE_TRACE_DOWN
} PulseStateMachine;

struct fifo_t {
  uint16_t rawIR;
  uint16_t rawRed;
};

struct dcFilter_t {
  float w;
  float result;
};

struct butterworthFilter_t
{
  float v[2];
  float result;
};

struct meanDiffFilter_t
{
  float values[MEAN_FILTER_SIZE];
  byte index;
  float sum;
  byte count;
};

class MAX30105Filter {
public:
	MAX30105Filter();
    pulseoxymeter_t update(long redValue, long irValue);
    dcFilter_t dcRemoval(float x, float prev_w, float alpha);
    void lowPassButterworthFilter( float x, butterworthFilter_t * filterResult );
    float meanDiff(float M, meanDiffFilter_t* filterValues);

  private:
    bool detectPulse(float sensor_value);
    void balanceIntesities( float redLedDC, float IRLedDC );

    void writeRegister(byte address, byte val);
    uint8_t readRegister(uint8_t address);
    void readFrom(byte address, int num, byte _buff[]);

  private:
    bool debug;

    uint8_t redLEDCurrent;
    float lastREDLedCurrentCheck;

    uint8_t currentPulseDetectorState;
    float currentBPM;
    float valuesBPM[PULSE_BPM_SAMPLE_SIZE];
    float valuesBPMSum;
    uint8_t valuesBPMCount;
    uint8_t bpmIndex;
    uint32_t lastBeatThreshold;

    fifo_t prevFifo;

    dcFilter_t dcFilterIR;
    dcFilter_t dcFilterRed;
    butterworthFilter_t lpbFilterIR;
    meanDiffFilter_t meanDiffIR;

    float irACValueSqSum;
    float redACValueSqSum;
    uint16_t samplesRecorded;
    uint16_t pulsesDetected;
    float currentSaO2Value;

};

#endif /* MAX301XXFILTER_H_ */
