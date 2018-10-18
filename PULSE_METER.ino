#include "Arduino.h"

#include <Arduino.h>
#include <math.h>
#include <Wire.h>
#include <U8g2lib.h>

#include "MAX30100.h"
#include <DigisparkJQ6500.h>
#include <SoftwareSerial.h>
#include <QueueArray.h>
#include "ESP8266WiFi.h"

MAX30100 * pulseOxymeter;

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ 16, /* clock=*/ 5, /* data=*/ 4); // @suppress("Abstract class cannot be instantiated")

#define BusyState 15 // пин BUSY плеера

SoftwareSerial swSer(14, 12); // @suppress("Abstract class cannot be instantiated")

JQ6500_Serial mp3(&swSer);

static char string_4dig[10];

QueueArray <int> queue(10);

#define CIRCULAR_BUFFER_SIZE 5

int counts[CIRCULAR_BUFFER_SIZE];
int beatCount = 0;

int lastTalk;

void ozv(int myfile);

void setup()
{
	//WiFi.mode( WIFI_OFF );
	//WiFi.forceSleepBegin();

//	WiFi.disconnect();
//	WiFi.mode(WIFI_OFF);
//	WiFi.forceSleepBegin();
//	delay(1);


	pinMode(BusyState,INPUT);

	Wire.begin();
	Serial.begin(9600);
	swSer.begin(9600);
	Serial.println("Pulse oxymeter test!");

	pulseOxymeter = new MAX30100();

	u8g2.begin();
	u8g2.setFont(u8g2_font_logisoso32_tf); // set the target font to calculate the pixel width
	u8g2.setFontMode(0);    // enable transparent mode, which is faster

	mp3.setVolume(10);

		WiFi.disconnect();
		WiFi.mode(WIFI_OFF);
		WiFi.forceSleepBegin();
}


void loop()
{
	// You have to call update with frequency at least 37Hz.
	// But the closer you call it to 100Hz the better, the filter will work.

//	int t_Start = millis();

	pulseoxymeter_t result = pulseOxymeter->update();

	if(result.pulseDetected)
	{
		Serial.println(result.heartBPM);

		u8g2.firstPage();
		do
		{
//			u8g2.setFont(u8g2_font_logisoso32_tf);   // set the target font
			u8g2.setCursor(0, 32);
			u8g2.print(result.heartBPM);
		} while ( u8g2.nextPage() );

		beatCount = (++beatCount) % CIRCULAR_BUFFER_SIZE;
		//beatCount = beatCount % CIRCULAR_BUFFER_SIZE;
		counts[beatCount] = round(result.heartBPM);

		Serial.print(beatCount); Serial.print(" - ");

		for (int i = 0; i < CIRCULAR_BUFFER_SIZE; i++)
		{
			Serial.print(counts[i]); Serial.print(" ");
		}

		Serial.println();

		int minCount = 10000, maxCount = -10000;

		for (int i = 0; i < CIRCULAR_BUFFER_SIZE; i++)
		{
			minCount = min(minCount, counts[i]);
			maxCount = max(maxCount, counts[i]);
		}

		if (maxCount - minCount < 4)
		{
			if (queue.isEmpty())
			{
				if (millis() - lastTalk > 5000)
				{
					voicedig(itoa(round(result.heartBPM), string_4dig, 10));
					lastTalk = millis();
				}
			}
		}

		if (!digitalRead(BusyState) && queue.count() > 0)
		{
			mp3.playFileByIndexNumber(queue.pop());
		}
	}

//	int t_End = millis();
//
//	if (t_End - t_Start > 0)
//	{
//		Serial.print(t_Start); Serial.print("-");Serial.print(t_End);Serial.print(" - ");Serial.println(t_End - t_Start);
//	}

//	if (t_End >= t_Start && (t_End - t_Start < 10))
//	{
//		delay(10 - (t_End - t_Start));
//	}
//	else
//	{
		delay(15);
//	}
}

void ozv(int myfile)
{
	queue.push(myfile);
}

bool fl;
char ccc[3];
byte troyka [3];

#define c19 19
#define c100 29
#define c1000 38
#define odna 41
#define dve 42

void voicedig(char cc[])
{
  int a,b,c,d,jj,sme,dp;
  a=strlen(cc);
  for (byte i=0;i<3;i++) ccc[i]=0;
  b=a%3;c=a/3;jj=0;
  for (byte i=0;i<c+1;i++)
    {strncpy(ccc,cc+jj,b);
     d=atoi(ccc); a=d;
     for (byte i=0;i<3;i++)
       { troyka[2-i]=a%10;
         a=a/10;
       }
    if (d>0)
     { dp=troyka[2];
       if (c-i==1)
        if (troyka[2]==1) dp=odna;
          else if(troyka[2]==2) dp=dve;
       if (troyka[0]>0) ozv(c100+troyka[0]-1);
       if (troyka[1]>1) ozv(c19+troyka[1]);
          else if (troyka[1]==1)
                  {ozv(troyka[1]*10+troyka[2]+1);
                   goto m1;
                  }
       if (troyka[2]>0) ozv(dp+1);
       m1: a=d%100;
        if (a>19) a=d%10;
        if (a==1) sme=0; else
        if (a>1 && a<5) sme=1; else sme=2;
       if (c-i>0) ozv(c1000+(c-i-1)*3+sme);
      }
      jj=jj+b;b=3;
      delay(100);
    }
}




