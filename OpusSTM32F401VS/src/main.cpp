#include "mbed.h"
#include "sound.h"
#include "soundsigned.h"
#include "opus.h"
#include "opus_types.h"
#include "opus_private.h"
#include "opus_multistream.h"
#include "codec.h"

//****************************************************************************
//
// Opus Encode and Decoder structure variables are declared here.
//
//****************************************************************************
OpusEncoder *sOpusEnc;
OpusDecoder *sOpusDec;

#define OPUS_DATA_SCALER      1
#define OPUS_BITRATE_SCALER   2
#define OPUS_FRAME_SIZE_IN_MS 20
#define OPUS_MAX_PACKET       255


DigitalOut myled(LED1);
DigitalIn myButton(USER_BUTTON);

Ticker ticker;
bool recording = false;
bool recordingFinished = false;
const uint16_t data[] = CUSTOM_SOUND_SIGNED;
uint8_t compressedData[4000];
uint16_t uncompressedData[8000];

//uint8_t encoder[40838];
int dataIndex = 0;
int compressedDataIndex = 0;
int encodedDataTotalSize = 0;
int decodedDataIndex = 0;


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
    compressedDataIndex = 0;


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
//           printf("Encoding %d channel %d bits %d Hz WAV file\n",
//                   1, 16, 8000);
    }

    //
    // Set the OPUS encoder parameters
    //
    opus_encoder_ctl(sOpusEnc, OPUS_SET_BITRATE(
            (8000*OPUS_BITRATE_SCALER)));
    opus_encoder_ctl(sOpusEnc, OPUS_SET_BANDWIDTH(OPUS_AUTO));
    opus_encoder_ctl(sOpusEnc, OPUS_SET_VBR(0));
    opus_encoder_ctl(sOpusEnc, OPUS_SET_VBR_CONSTRAINT(0));
    opus_encoder_ctl(sOpusEnc, OPUS_SET_COMPLEXITY(10));
    opus_encoder_ctl(sOpusEnc, OPUS_SET_INBAND_FEC(0));
    opus_encoder_ctl(sOpusEnc, OPUS_SET_FORCE_CHANNELS(OPUS_AUTO));
    opus_encoder_ctl(sOpusEnc, OPUS_SET_DTX(0));
    opus_encoder_ctl(sOpusEnc, OPUS_SET_PACKET_LOSS_PERC(0));
    opus_encoder_ctl(sOpusEnc, OPUS_SET_LSB_DEPTH(16));
    opus_encoder_ctl(sOpusEnc, OPUS_SET_EXPERT_FRAME_DURATION(
    		OPUS_FRAMESIZE_20_MS));
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
    unsigned int time;
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
//        i32len = opus_encode(sOpusEnc,
//                popi16fmtBuffer,
//                (ui32Sizeofpopi16fmtBuffer/2),
//                pui8data,
//                OPUS_MAX_PACKET);
    	time = us_ticker_read();
        i32len = opus_encode(sOpusEnc,
                        popi16fmtBuffer,
                        (ui32Sizeofpopi16fmtBuffer),
                        pui8data,
                        OPUS_MAX_PACKET);
        time = us_ticker_read() - time;
		printf("\n");
		printf("Time to encode: %d us\n", time);
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

        // Attach size header to each packet
        compressedData[compressedDataIndex] = (uint8_t)i32len;
        compressedDataIndex++;

        memcpy(&compressedData[compressedDataIndex], pui8data, i32len);
        compressedDataIndex += i32len;


        //
        // Add the length of the wav file and the compressed data for printing
        // the statistics
        //
        ui32EncodedLen += i32len;
        ui32RawLen     += ui32BytesRead;

        printf("OPUS Encoder Completion: %03d\n", i32len);

//        printf("OPUS Encoder Completion: %03d\n",
//                ((ui32RawLen*100)/16));
    }
    while(codecDataIndex < 8000);
    printf("Compressed size: %d bytes", compressedDataIndex);
    encodedDataTotalSize = compressedDataIndex;

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

//*****************************************************************************
//
// This function implements the "dec" command. It is provided with 2 parameters
// The first parameter is an input opx file and the second parameter is the
// output wav file decompressed with OPUS.
//
//*****************************************************************************
int
Cmd_decode()
{
    uint8_t  *pcRdBuf;
    uint8_t  ui8ProgressDisplay=0;
    char     cOpxDelimiter[4];
    uint16_t ui32BitsPerSample = 16;
    uint16_t ui32Channel = 1;
    uint32_t ui32BytesRead;
    uint32_t ui32BytesWrite;
    uint32_t ui32SizeOfOutBuf;
    uint32_t ui32Loop;
    uint32_t ui32SamplingRate = 8000;
    uint32_t ui32WavFileSize;
    uint32_t ui32EncodedLen=0;
    uint32_t ui32RawLen=0;
    int  i32error;
    int32_t  i32len;
    int32_t  i32OutSamples;
    opus_int16 *pcop16OutBuf;

    // Reset while loop counter
    compressedDataIndex = 0;
    decodedDataIndex = 0;

    //
    // If the opx file format is correct then create the decoder
    //
    sOpusDec = opus_decoder_create(ui32SamplingRate, ui32Channel, &i32error);

    //
    // If there was some problem creating the OPUS decoder, then close the write
    // and read file handle and return an error
    //
    if (i32error != OPUS_OK)
    {
       printf("DEC_ERR: Cannot create decoder: %s\n", opus_strerror(i32error));
    }
    else
    {
        printf("Decoding %d Channels at %d Sampling Rate\n",ui32Channel,ui32SamplingRate);
    }

    //
    // Set the parameters for the OPUS decoder
    //
    opus_decoder_ctl(sOpusDec, OPUS_SET_LSB_DEPTH(ui32BitsPerSample));

    //
    // Dynamic allocation of memory for the sd card read buffer and the output
    // from the codec.
    //
    ui32SizeOfOutBuf = (ui32SamplingRate*
            ui32Channel*
            OPUS_FRAME_SIZE_IN_MS*
            OPUS_DATA_SCALER)/1000;
    pcop16OutBuf     = (int16_t*)calloc((
            (ui32SizeOfOutBuf/OPUS_DATA_SCALER)+1),
            sizeof(int16_t));
    pcRdBuf          = (uint8_t *)calloc(OPUS_MAX_PACKET,sizeof(uint8_t));

    //
    // Enter a loop to repeatedly read data from the file and display it, until
    // the end of the file is reached.
    //
    do
    {
        //
        // Read the delimiter and length first
        //
//        iFRdResult = f_read(&g_sFileReadObject, &cOpxDelimiter, 4,
//                          (UINT *)&ui32BytesRead);
//        iFRdResult = f_read(&g_sFileReadObject, &i32len, 4,
//                          (UINT *)&ui32BytesRead);
//
//        //
//        // Read a block of data from the file as specified by the length.
//        //
//        iFRdResult = f_read(&g_sFileReadObject, pcRdBuf, i32len,
//                          (UINT *)&ui32BytesRead);

        //
        // Now start the decompression process
        //

    	// Packet size per my encoding
    	i32len = compressedData[compressedDataIndex];
    	compressedDataIndex++;

        i32OutSamples = opus_decode(sOpusDec,
                (const unsigned char *)&compressedData[compressedDataIndex],
                i32len,
                pcop16OutBuf,
                (ui32SizeOfOutBuf/OPUS_DATA_SCALER), 0);

        // Update the index to next packet header
        compressedDataIndex += i32len;

        //
        // If there is an error in the decoder then free the buffer and
        // destroy the decoder to free the memory
        //
        if (i32OutSamples < OPUS_OK)
        {
           printf("DEC_ERR: Decode Failed %s\n", opus_strerror(i32error));

           free(pcop16OutBuf);
           free(pcRdBuf);


           opus_decoder_destroy(sOpusDec);

           return(1);
        }

        //
        // Based on the original bits per sample, perform bit operation for
        // final wav file
        //
        if(ui32BitsPerSample == 8)
        {
            //
            // If the data is 8 bit then convert from signed to unsigned format
            //
//            for(ui32Loop = 0 ; ui32Loop < (i32OutSamples) ; ui32Loop++)
//            {
//                pcRdBuf[ui32Loop] = (uint8_t)(pcop16OutBuf[ui32Loop] ^ 0x80);
//            }
//
//            //
//            // Write the data to the temporary file
//            //
////            iFWrResult = f_write(&g_sFileWriteObject, pcRdBuf, i32OutSamples,
////                                  (UINT *)&ui32BytesWrite);
//
//            //
//            // Add the number of bytes from the decoder output for statistics
//            //
//            ui32RawLen     += i32OutSamples;
        	printf("We shouldn't be hereeeee\n");
        }
        else
        {
            //
            // If the data is 16 bit then write the data as is to the output
            // temporary file
            //
//            iFWrResult = f_write(&g_sFileWriteObject, pcop16OutBuf, (i32OutSamples*OPUS_DATA_SCALER),
//                                  (UINT *)&ui32BytesWrite);

            //
            // Add the number of bytes from the decoder output for statistics
            //
            ui32RawLen     += i32OutSamples*OPUS_DATA_SCALER;

            memcpy(&uncompressedData[decodedDataIndex], pcop16OutBuf, i32OutSamples);
            decodedDataIndex += i32OutSamples;

            printf("Decoded %d samples\n", i32OutSamples);
        }

        //
        // Add the opx file byte length for each of the segments for statistic.
        //
        ui32EncodedLen += i32len;

    }
    while(compressedDataIndex < encodedDataTotalSize);

    // Total sample out
    printf("Total samples output: %d\n", decodedDataIndex);

    //
    // Free the buffers
    //
    free(pcop16OutBuf);
    free(pcRdBuf);

    //
    // destriy the decoder to free the memory allocated to it.
    //
    opus_decoder_destroy(sOpusDec);

    //
    // Return success.
    //
    return(0);
}

int main()
{
	dataIndex = 0;
	//ticker.attach(&play, .000125);
	printf("Encoding builtin buffer\n");
	unsigned int time = us_ticker_read();
	Cmd_encode();
	int stuff = encode();
	time = us_ticker_read() - time;
	myled = 1;
	printf("\n");
	printf("Time to encode: %d us\n", time);
	printf("Decoding data...\n");
	//Cmd_decode();
	printf("Decoding complete\n");

	// Print out decoded data
	dataIndex = 0;
//	while(dataIndex < 4000)
//	{
//		printf("0x%04x, ", uncompressedData[dataIndex]);
//		if(dataIndex % 20 == 0)
//		{
//			printf("\\\n");
//		}
//		dataIndex++;
//	}

	while(1)
	{

    }

}
