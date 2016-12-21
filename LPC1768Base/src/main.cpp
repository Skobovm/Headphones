#include "mbed.h"
#include "sound.h"

DigitalOut myled(LED1);
DigitalIn myButton(p14);
AnalogOut Aout(p18);
AnalogIn Ain(p17);
Ticker ticker;
bool recording = false;
bool recordingFinished = false;
uint16_t data[] = CUSTOM_SOUND;
int dataIndex = 0;

void record()
{
	if(!recording)
	{
		if(myButton == 1)
		{
			myled = 0;
			dataIndex = 0;
			recording = true;
		}
	}
	else
	{
		if(dataIndex < 4000)
		{
			data[dataIndex] = Ain.read_u16();
			dataIndex++;
		}
		else
		{
			recordingFinished = true;
			recording = false;
			dataIndex = 0;
		}
	}
}

void play()
{
	if(dataIndex < 4000)
	{
		Aout.write_u16(data[dataIndex]);
		dataIndex++;
	}
}

int main()
{
	dataIndex = 0;
	ticker.attach(&play, .000125);
	myled = 1;
	while(1) {
        //Aout.write_u16(Ain.read_u16());
		if(recordingFinished)
		{
			myled = 1;
			while(dataIndex < 4000)
			{
				printf("0x%x, ", data[dataIndex]);
				if(dataIndex % 20 == 0)
				{
					printf("\\\n");
				}
				dataIndex++;
			}

		}
    }

}
