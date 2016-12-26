#include "mbed.h"

#define REC_SAMPLES 12000

DigitalOut myled(LED1);
DigitalIn myButton(USER_BUTTON);
AnalogIn Ain(A0);
Ticker ticker;
bool recording = false;
bool recordingFinished = false;
int16_t data[REC_SAMPLES];

int dataIndex = 0;

void record()
{
	if(!recording)
	{
		if(myButton == 0)
		{
			myled = 0;
			dataIndex = 0;
			recording = true;
		}
	}
	else
	{
		if(dataIndex < REC_SAMPLES)
		{
			int32_t readVal = Ain.read_u16();
			readVal = readVal - 0x8000;
			data[dataIndex] = (int16_t)readVal;
//			if(readVal > 0x8000)
//			{
//				data[dataIndex] = readVal - 0x8000;
//			}
//			else
//			{
//				data[dataIndex] = ((0x8000 - readVal) ^ 0xFFFF) + 1;
//			}
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

int main()
{
	dataIndex = 0;
	ticker.attach(&record, .000125);
	//printf("Encoding builtin buffer\n");
	//Cmd_encode();

	while(1) {
        //Aout.write_u16(Ain.read_u16());
		if(recordingFinished)
		{
			myled = 1;
			while(dataIndex < REC_SAMPLES)
			{
				printf("0x%04hx, ", (int16_t)data[dataIndex]);
				if(dataIndex % 20 == 0 && dataIndex != 0)
				{
					printf("\\\n");
				}
				dataIndex++;
			}

		}
    }

}
