#include "mbed.h"
#include "sound.h"
#include "soundout.h"
#include "wavsound.h"
#include "soundsigned.h"

#define REC_SAMPLES 20294


DigitalOut myled(LED1);
DigitalIn myButton(p14);
AnalogOut Aout(p18);
AnalogIn Ain(p17);
Ticker ticker;
bool recording = false;
bool recordingFinished = false;
const uint16_t data[] = TESTING12_WAV;
int dataIndex = 0;

void play()
{
	if(dataIndex < REC_SAMPLES)
	{
		if(data[dataIndex] > 0x8000)
		{

		}
		else
		{

		}
		int32_t negative = (0x0000FFFF & data[dataIndex]) | 0xFFFF0000;
		int32_t writeVal = ((data[dataIndex] ^ 0x8000 > 0) ? negative : data[dataIndex]) + 0x8000;
		Aout.write_u16((uint16_t)writeVal);
		dataIndex++;
	}
	else
	{
		myled = 1;
	}
}

int main()
{
	dataIndex = 0;
	ticker.attach(&play, .000125);
	//printf("Encoding builtin buffer\n");
	//Cmd_encode();

	while(1) {
        //Aout.write_u16(Ain.read_u16());
		if(recordingFinished)
		{
			myled = 1;
			while(dataIndex < REC_SAMPLES)
			{
				printf("0x%04x, ", (uint16_t)data[dataIndex]);
				if(dataIndex % 20 == 0)
				{
					printf("\\\n");
				}
				dataIndex++;
			}

		}
    }

}
