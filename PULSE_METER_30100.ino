#include "Arduino.h"

#include <Arduino.h>
#include <math.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <QueueArray.h>
#include "MAX30100.h"
#include <DigisparkJQ6500.h>
#include <SoftwareSerial.h>
#include "ESP8266WiFi.h"

MAX30100 * pulseOxymeter;

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ 16, /* clock=*/ 5, /* data=*/ 4); // @suppress("Abstract class cannot be instantiated")

#define BusyState 15 // пин BUSY плеера

SoftwareSerial swSer(14, 12); // @suppress("Abstract class cannot be instantiated")

JQ6500_Serial mp3(&swSer);

QueueArray<int> queue(10);

#define CIRCULAR_BUFFER_SIZE 5

#define LOOP_DELAY_MS 12

int counts[CIRCULAR_BUFFER_SIZE];
int beatCount = 0;

int lastTalk;
int lastMeasurement;


void setup()
{
	pinMode(BusyState,INPUT);
	pinMode(2, OUTPUT);

	digitalWrite(2, HIGH);

	Wire.begin();
	Serial.begin(9600);
	swSer.begin(9600);
	Serial.println("Pulse oxymeter test!");

	pulseOxymeter = new MAX30100();

//	pulseOxymeter = new MAX30100(DEFAULT_OPERATING_MODE, DEFAULT_SAMPLING_RATE,
//			DEFAULT_LED_PULSE_WIDTH, MAX30100_LED_CURRENT_37MA);

	u8g2.begin();
	u8g2.setFont(u8g2_font_logisoso32_tf); // set the target font to calculate the pixel width
	u8g2.setFontMode(0);    // enable transparent mode, which is faster

	mp3.setVolume(15);

	WiFi.disconnect();
	WiFi.mode(WIFI_OFF);
	WiFi.forceSleepBegin();

	lastMeasurement = millis();
}


void loop()
{
	// You have to call update with frequency at least 37Hz.
	// But the closer you call it to 100Hz the better, the filter will work.

	int t_Start = millis();

	pulseoxymeter_t result = pulseOxymeter->update();

	if(result.pulseDetected)
	{
		Serial.println(result.heartBPM);

		u8g2.firstPage();
		do
		{
			u8g2.setCursor(0, 32);
			u8g2.print(result.heartBPM, 1);
		} while ( u8g2.nextPage() );

		beatCount = (++beatCount) % CIRCULAR_BUFFER_SIZE;
		counts[beatCount] = round(result.heartBPM);

		int minCount = 10000, maxCount = -10000;

		for (int i = 0; i < CIRCULAR_BUFFER_SIZE; i++)
		{
			minCount = min(minCount, counts[i]);
			maxCount = max(maxCount, counts[i]);
		}

		if (maxCount - minCount < 4)
		{
			if (!digitalRead(BusyState))
			{
				if (millis() - lastTalk > 5000)
				{
					if (queue.isEmpty())
					{
						// as we have files in JQ6500 starting with 0040
						queue.push(round(result.heartBPM) - 39);
					}

					lastTalk = millis();
				}
			}

			lastMeasurement = millis();
		}

		//lastMeasurement = millis();
	}

	if (millis() - lastMeasurement > 20 * 1000)
	{
		digitalWrite(2, LOW);
	}

	if (queue.count() > 0 && !digitalRead(BusyState))
	{
		mp3.playFileByIndexNumber(queue.pop());
	}

	int t_End = millis();

	if (t_End >= t_Start && (t_End - t_Start < LOOP_DELAY_MS))
	{
		delay(LOOP_DELAY_MS - (t_End - t_Start));
	}
	else
	{
		delay(LOOP_DELAY_MS);
	}
}






