/*
 * MAX301xxFilter.h
 *
 *  Created on: Oct 24, 2018
 *      Author: vkozloff@gmail.com
 *  Based on the code developed by Raivis Strogonovs (https://morf.lv)
 */

#include "MAX301xxFilter.h"

MAX30105Filter::MAX30105Filter()
{
	  dcFilterIR.result = 0;

	  dcFilterRed.w = 0;
	  dcFilterRed.result = 0;


	  lpbFilterIR.v[0] = 0;
	  lpbFilterIR.v[1] = 0;
	  lpbFilterIR.result = 0;

	  meanDiffIR.index = 0;
	  meanDiffIR.sum = 0;
	  meanDiffIR.count = 0;


	  valuesBPM[0] = 0;
	  valuesBPMSum = 0;
	  valuesBPMCount = 0;
	  bpmIndex = 0;


	  irACValueSqSum = 0;
	  redACValueSqSum = 0;
	  samplesRecorded = 0;
	  pulsesDetected = 0;
	  currentSaO2Value = 0;

	  lastBeatThreshold = 0;

}

pulseoxymeter_t MAX30105Filter::update(long redValue, long irValue)
{
  pulseoxymeter_t result = {
    /*bool pulseDetected*/ false,
    /*float heartBPM*/ 0.0,
    /*float irCardiogram*/ 0.0,
    /*float irDcValue*/ 0.0,
    /*float redDcValue*/ 0.0,
    /*float SaO2*/ currentSaO2Value,
    /*uint32_t lastBeatThreshold*/ 0,
    /*float dcFilteredIR*/ 0.0,
    /*float dcFilteredRed*/ 0.0
  };


//  fifo_t rawData = readFIFO();

  dcFilterIR = dcRemoval( (float)irValue, dcFilterIR.w, ALPHA );
  dcFilterRed = dcRemoval( (float)redValue, dcFilterRed.w, ALPHA );

  float meanDiffResIR = meanDiff( dcFilterIR.result, &meanDiffIR);
  lowPassButterworthFilter( meanDiffResIR/*-dcFilterIR.result*/, &lpbFilterIR );

  irACValueSqSum += dcFilterIR.result * dcFilterIR.result;
  redACValueSqSum += dcFilterRed.result * dcFilterRed.result;
  samplesRecorded++;

  if( detectPulse( lpbFilterIR.result ) && samplesRecorded > 0 )
  {
    result.pulseDetected=true;
    pulsesDetected++;

    float ratioRMS = log( sqrt(redACValueSqSum/samplesRecorded) ) / log( sqrt(irACValueSqSum/samplesRecorded) );

    if( debug == true )
    {
      Serial.print("RMS Ratio: ");
      Serial.println(ratioRMS);
    }

    //This is my adjusted standard model, so it shows 0.89 as 94% saturation. It is probably far from correct, requires proper empircal calibration
    currentSaO2Value = 110.0 - 18.0 * ratioRMS;
    result.SaO2 = currentSaO2Value;

    if( pulsesDetected % RESET_SPO2_EVERY_N_PULSES == 0)
    {
      irACValueSqSum = 0;
      redACValueSqSum = 0;
      samplesRecorded = 0;
    }
  }

  //balanceIntesities( dcFilterRed.w, dcFilterIR.w );


  result.heartBPM = currentBPM;
  result.irCardiogram = lpbFilterIR.result;
  result.irDcValue = dcFilterIR.w;
  result.redDcValue = dcFilterRed.w;
  result.lastBeatThreshold = lastBeatThreshold;
  result.dcFilteredIR = dcFilterIR.result;
  result.dcFilteredRed = dcFilterRed.result;


  return result;
}

bool MAX30105Filter::detectPulse(float sensor_value)
{
  static float prev_sensor_value = 0;
  static uint8_t values_went_down = 0;
  static uint32_t currentBeat = 0;
  static uint32_t lastBeat = 0;

  if(sensor_value > PULSE_MAX_THRESHOLD)
  {
    currentPulseDetectorState = PULSE_IDLE;
    prev_sensor_value = 0;
    lastBeat = 0;
    currentBeat = 0;
    values_went_down = 0;
    lastBeatThreshold = 0;
    return false;
  }

  switch(currentPulseDetectorState)
  {
    case PULSE_IDLE:
      if(sensor_value >= PULSE_MIN_THRESHOLD) {
        currentPulseDetectorState = PULSE_TRACE_UP;
        values_went_down = 0;
      }
      break;

    case PULSE_TRACE_UP:
      if(sensor_value > prev_sensor_value)
      {
        currentBeat = millis();
        lastBeatThreshold = sensor_value;
      }
      else
      {

        if(debug == true)
        {
          Serial.print("Peak reached: ");
          Serial.print(sensor_value);
          Serial.print(" ");
          Serial.println(prev_sensor_value);
        }

        uint32_t beatDuration = currentBeat - lastBeat;
        lastBeat = currentBeat;

        float rawBPM = 0;
        if(beatDuration > 0)
          rawBPM = 60000.0 / (float)beatDuration;
        if(debug == true)
          Serial.println(rawBPM);

        //This method sometimes glitches, it's better to go through whole moving average everytime
        //IT's a neat idea to optimize the amount of work for moving avg. but while placing, removing finger it can screw up
        //valuesBPMSum -= valuesBPM[bpmIndex];
        //valuesBPM[bpmIndex] = rawBPM;
        //valuesBPMSum += valuesBPM[bpmIndex];

        valuesBPM[bpmIndex] = rawBPM;
        valuesBPMSum = 0;
        for(int i=0; i<PULSE_BPM_SAMPLE_SIZE; i++)
        {
          valuesBPMSum += valuesBPM[i];
        }

        if(debug == true)
        {
          Serial.print("CurrentMoving Avg: ");
          for(int i=0; i<PULSE_BPM_SAMPLE_SIZE; i++)
          {
            Serial.print(valuesBPM[i]);
            Serial.print(" ");
          }

          Serial.println(" ");
        }

        bpmIndex++;
        bpmIndex = bpmIndex % PULSE_BPM_SAMPLE_SIZE;

        if(valuesBPMCount < PULSE_BPM_SAMPLE_SIZE)
          valuesBPMCount++;

        currentBPM = valuesBPMSum / valuesBPMCount;
        if(debug == true)
        {
          Serial.print("AVg. BPM: ");
          Serial.println(currentBPM);
        }


        currentPulseDetectorState = PULSE_TRACE_DOWN;

        return true;
      }
      break;

    case PULSE_TRACE_DOWN:
      if(sensor_value < prev_sensor_value)
      {
        values_went_down++;
      }


      if(sensor_value < PULSE_MIN_THRESHOLD)
      {
        currentPulseDetectorState = PULSE_IDLE;
      }
      break;
  }

  prev_sensor_value = sensor_value;
  return false;
}




dcFilter_t MAX30105Filter::dcRemoval(float x, float prev_w, float alpha)
{
  dcFilter_t filtered;
  filtered.w = x + alpha * prev_w;
  filtered.result = filtered.w - prev_w;

  return filtered;
}

void MAX30105Filter::lowPassButterworthFilter( float x, butterworthFilter_t * filterResult )
{
  filterResult->v[0] = filterResult->v[1];

  //Fs = 100Hz and Fc = 10Hz
  filterResult->v[1] = (2.452372752527856026e-1 * x) + (0.50952544949442879485 * filterResult->v[0]);

  //Fs = 100Hz and Fc = 4Hz
  //filterResult->v[1] = (1.367287359973195227e-1 * x) + (0.72654252800536101020 * filterResult->v[0]); //Very precise butterworth filter

  filterResult->result = filterResult->v[0] + filterResult->v[1];
}

float MAX30105Filter::meanDiff(float M, meanDiffFilter_t* filterValues)
{
  float avg = 0;

  filterValues->sum -= filterValues->values[filterValues->index];
  filterValues->values[filterValues->index] = M;
  filterValues->sum += filterValues->values[filterValues->index];

  filterValues->index++;
  filterValues->index = filterValues->index % MEAN_FILTER_SIZE;

  if(filterValues->count < MEAN_FILTER_SIZE)
    filterValues->count++;

  avg = filterValues->sum / filterValues->count;
  return avg - M;
}



