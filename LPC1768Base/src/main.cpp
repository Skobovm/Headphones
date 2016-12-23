#include "mbed.h"
#include "sound.h"
#include "soundout.h"
#include "wavsound.h"
#include "soundsigned.h"
#include "opus.h"
#include "opus_types.h"
#include "opus_private.h"
#include "opus_multistream.h"

//****************************************************************************
//
// Opus Encode and Decoder structure variables are declared here.
//
//****************************************************************************
OpusEncoder *sOpusEnc;
OpusDecoder *sOpusDec;

#define OPUS_DATA_SCALER      2
#define OPUS_BITRATE_SCALER   2
#define OPUS_FRAME_SIZE_IN_MS 20
#define OPUS_MAX_PACKET       1024


DigitalOut myled(LED1);
DigitalIn myButton(p14);
AnalogOut Aout(p18);
AnalogIn Ain(p17);
Ticker ticker;
bool recording = false;
bool recordingFinished = false;
uint16_t data[] = HELLO_WAV;
uint8_t compressedData[8000];
uint8_t encoder[40838];
int dataIndex = 0;


//*****************************************************************************
//
// This function implements the "enc" command. It is provided with 2 parameters
// The first parameter is an input wav file and the second parameter is the
// output opx file compressed with OPUS.
//
//*****************************************************************************
int
Cmd_encode()
{
    uint8_t  *pui8data;
    uint8_t  ui8ScaleFactor;
    char     cOpxDelimiter[4];
    char     *pcRdBuf;
    uint32_t ui32BytesRead;
    uint32_t ui32BytesWrite;
    uint32_t ui32Sizeofpopi16fmtBuffer;
    uint32_t ui32Loop;
    uint32_t ui32EncodedLen=0;
    uint32_t ui32RawLen=0;
    uint32_t ui32SizeOfRdBuf;
    int  i32error;
    int32_t  i32len;

    opus_int16 *popi16fmtBuffer;

    int codecDataIndex = 0;
    int compressedDataIndex = 0;


    //
    // Create the encoder
    //
    sOpusEnc = opus_encoder_create(8000, 1, OPUS_APPLICATION_AUDIO, &i32error);

    //
    // If there is an error creating the encoder then close the input and
    // output file handle. Else print the information on the file parameters
    //
    if (i32error != OPUS_OK)
    {
       printf("ENC_ERR: Cannot create encoder: %s\n",
               opus_strerror(i32error));
       return(0);
    }
    else
    {
           printf("Encoding %d channel %d bits %d Hz WAV file\n",
                   1, 16, 8000);
    }

    //
    // Set the OPUS encoder parameters
    //
    opus_encoder_ctl(sOpusEnc, OPUS_SET_BITRATE(
            (8000*OPUS_BITRATE_SCALER)));
    opus_encoder_ctl(sOpusEnc, OPUS_SET_BANDWIDTH(OPUS_AUTO));
    opus_encoder_ctl(sOpusEnc, OPUS_SET_VBR(1));
    opus_encoder_ctl(sOpusEnc, OPUS_SET_VBR_CONSTRAINT(0));
    opus_encoder_ctl(sOpusEnc, OPUS_SET_COMPLEXITY(0));
    opus_encoder_ctl(sOpusEnc, OPUS_SET_INBAND_FEC(0));
    opus_encoder_ctl(sOpusEnc, OPUS_SET_FORCE_CHANNELS(OPUS_AUTO));
    opus_encoder_ctl(sOpusEnc, OPUS_SET_DTX(0));
    opus_encoder_ctl(sOpusEnc, OPUS_SET_PACKET_LOSS_PERC(0));
    opus_encoder_ctl(sOpusEnc, OPUS_SET_LSB_DEPTH(16));
    opus_encoder_ctl(sOpusEnc, OPUS_SET_EXPERT_FRAME_DURATION(
            OPUS_FRAMESIZE_ARG));
    opus_encoder_ctl(sOpusEnc, OPUS_SET_FORCE_MODE(MODE_CELT_ONLY));

//    //
//    // Dynamic allocation of memory for the sd card read buffer, output from
//    // the codec and formatted input buffer for the codec
//    //
    ui8ScaleFactor = (16) >> 3;

    pui8data = (uint8_t *)calloc(OPUS_MAX_PACKET,sizeof(uint8_t));
//    pcRdBuf = (char *)calloc((((8000*
//            OPUS_FRAME_SIZE_IN_MS*
//            1*
//            ui8ScaleFactor)/1000)+1),
//            sizeof(uint8_t));
    popi16fmtBuffer = (opus_int16 *)calloc((((8000*
            OPUS_FRAME_SIZE_IN_MS*
            1)/1000)+1),
            sizeof(opus_int16));

    ui32SizeOfRdBuf = (8000*
            1*
            OPUS_FRAME_SIZE_IN_MS*
            ui8ScaleFactor)/1000;
    ui32Sizeofpopi16fmtBuffer = (8000*
            OPUS_FRAME_SIZE_IN_MS*
            1*
            sizeof(opus_int16))/1000;


    //
    // Enter a loop to repeatedly read the wav data from the file, encode it
    // and then store it in the sd card.
    //
    do
    {
        //
        // Read a block of data from the file as specified by the selected
        // frame size in the header file
        //
//        iFRdResult = f_read(&g_sFileReadObject, pcRdBuf, ui32SizeOfRdBuf,
//                          (UINT *)&ui32BytesRead);

        //
        // Process the data as per the scale factor. A scale factor of 1 is
        // applied when the data is 8 bit and a scale factor of 2 is applied
        // when the data is 16 bit.
        //

//        for(ui32Loop = 0 ; ui32Loop < ui32BytesRead ; ui32Loop++)
//        {
//            if(ui8ScaleFactor == 1)
//            {
//                   popi16fmtBuffer[ui32Loop] = (opus_int16)pcRdBuf[ui32Loop];
//            }
//            else if(ui8ScaleFactor == 2)
//            {
//                if(ui32Loop%2 == 0)
//                    popi16fmtBuffer[ui32Loop/2] = pcRdBuf[ui32Loop];
//                else
//                    popi16fmtBuffer[ui32Loop/2] |= (pcRdBuf[ui32Loop] << 8);
//
//            }
//
//        }

    	for(ui32Loop = 0; ui32Loop < 160; ui32Loop++)
    	{
    		popi16fmtBuffer[ui32Loop] = data[codecDataIndex + ui32Loop];
    	}
    	codecDataIndex += ui32Loop; // This determines when we're done

        //
        // If no error with file handling then start the compression.
        //
        i32len = opus_encode(sOpusEnc,
                popi16fmtBuffer,
                (ui32Sizeofpopi16fmtBuffer/2),
                pui8data,
                OPUS_MAX_PACKET);

        //
        // If this is not the last packet then add the 'Mid ' as delimiter
        // else add 'End ' as the delimiter which will be used during the
        // decompression process.
        //
//        if(ui32BytesRead == ui32SizeOfRdBuf)
//        {
//            strcpy(cOpxDelimiter, "Mid\0");
//        }
//        else
//        {
//            strcpy(cOpxDelimiter, "End\0");
//        }

        //
        // Store the compressed data with length of data, followed by the
        // delimiter and then the actual compressed data
        //
//        iFWrResult = f_write(&g_sFileWriteObject, &cOpxDelimiter[0], 4,
//                              (UINT *)&ui32BytesWrite);
//        iFWrResult = f_write(&g_sFileWriteObject, &i32len, 4,
//                              (UINT *)&ui32BytesWrite);
//        iFWrResult = f_write(&g_sFileWriteObject, pui8data, i32len,
//                              (UINT *)&ui32BytesWrite);
        memcpy(&compressedData[compressedDataIndex], pui8data, i32len);
        compressedDataIndex += i32len;

        //
        // Add the length of the wav file and the compressed data for printing
        // the statistics
        //
        ui32EncodedLen += i32len;
        ui32RawLen     += ui32BytesRead;

        printf("OPUS Encoder Completion: %03d\r",
                ((ui32RawLen*100)/16));
    }
    while(codecDataIndex < 4000);


    //
    // Free the buffers that have been dynamically allocated
    //
    free(pui8data);
    free(pcRdBuf);
    free(popi16fmtBuffer);

    //
    // free the memory assigned to the encode
    //
    opus_encoder_destroy(sOpusEnc);

    //
    // Return success.
    //
    return(0);
}

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
		if(dataIndex < 8000)
		{
			uint16_t readVal = Ain.read_u16();
			if(readVal > 0x8000)
			{
				data[dataIndex] = readVal - 0x8000;
			}
			else
			{
				data[dataIndex] = ((0x8000 - readVal) ^ 0xFFFF) + 1;
			}
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
	if(dataIndex < 8000)
	{
		if(data[dataIndex] > 0x8000)
		{

		}
		else
		{

		}
		uint16_t writeVal = (data[dataIndex] + 0x8000);
		Aout.write_u16(writeVal);
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
			while(dataIndex < 8000)
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
