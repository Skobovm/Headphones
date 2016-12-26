
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "opus.h"
#include "debug.h"
#include "opus_types.h"
#include "opus_private.h"
#include "opus_multistream.h"
#include "embeddedsound.h"
#include "codec.h"


#define MAX_PACKET 1500

static void int_to_char(opus_uint32 i, unsigned char ch[4])
{
    ch[0] = i>>24;
    ch[1] = (i>>16)&0xFF;
    ch[2] = (i>>8)&0xFF;
    ch[3] = i&0xFF;
}

static short micData[] = CUSTOM_SOUND_SIGNED;
static int readIndex = 0;
const int micDataSize = CUSTOM_SOUND_LENGTH;

static uint get_bytes_local(unsigned char* output, int size, int count, void* fin)
{
	// TODO: This function is shit. It assumes size = 16. Don't want to think hard right now
	if (micDataSize - readIndex < count)
	{
		count = micDataSize - readIndex;
	}
	if (count > 0)
	{
		memcpy(output, &micData[readIndex], count * size);
	}
	readIndex += count;
	return count;
}

#define OUTPUT_BUFFER_SIZE 4000
static int writeIndex = 0;
static char outputBuffer[OUTPUT_BUFFER_SIZE];

static uint put_bytes_local(unsigned char* input, int size, int count, void* file)
{
	// TODO: Assumes 8 bit data
	if (OUTPUT_BUFFER_SIZE - writeIndex < count)
	{
		count = OUTPUT_BUFFER_SIZE - writeIndex;
	}

	memcpy(&outputBuffer[writeIndex], input, count * size);
	writeIndex += count;
	return count;
}

int encode()
{
    int err;

    OpusEncoder *enc=NULL;
    OpusDecoder *dec=NULL;

    int len[2];
    int frame_size, channels;
    opus_int32 bitrate_bps=0;
    unsigned char *data[2];
    unsigned char *fbytes;
    opus_int32 sampling_rate;
    int use_vbr;
    int max_payload_bytes;
    int complexity;
    int use_inbandfec;
    int use_dtx;
    int forcechannels;
    int cvbr = 0;
    int packet_loss_perc;
    opus_int32 count=0, count_act=0;
    int k;
    opus_int32 skip=0;
    int stop=0;
    short *in, *out;
    int application=OPUS_APPLICATION_AUDIO;
    double bits=0.0, bits_max=0.0, bits_act=0.0, bits2=0.0, nrg;
    double tot_samples=0;
    opus_uint64 tot_in, tot_out;
    int bandwidth=OPUS_AUTO;
    const char *bandwidth_string;
    int lost = 0, lost_prev = 1;
    int toggle = 0;
    opus_uint32 enc_final_range[2];
    opus_uint32 dec_final_range;
    int encode_only=0, decode_only=0;
    int max_frame_size = 48000*2;
    int curr_read=0;
    int sweep_bps = 0;
    int random_framesize=0, newsize=0, delayed_celt=0;
    int sweep_max=0, sweep_min=0;
    int random_fec=0;
    const int (*mode_list)[4]=NULL;
    int nb_modes_in_list=0;
    int curr_mode=0;
    int curr_mode_count=0;
    int mode_switch_time = 48000;
    int nb_encoded=0;
    int remaining=0;
    int variable_duration=OPUS_FRAMESIZE_ARG;
    int delayed_decision=0;
    unsigned int time;

    tot_in=tot_out=0;

	encode_only = 1;
	application = OPUS_APPLICATION_VOIP;

    sampling_rate = 8000;
    frame_size = sampling_rate/50;

    channels = 1;
	bitrate_bps = 11000; // TODO: play with different values, like auto

    use_vbr = 0; // Default is 1
    max_payload_bytes = MAX_PACKET;
    complexity = 5; // Default is 10
    use_inbandfec = 0;
    forcechannels = OPUS_AUTO;
    use_dtx = 0;
    packet_loss_perc = 0;


    enc = opus_encoder_create(sampling_rate, channels, application, &err);
	if (err != OPUS_OK)
	{
	  printf("Cannot create encoder: %s\n", opus_strerror(err));

	  return EXIT_FAILURE;
	}
	opus_encoder_ctl(enc, OPUS_SET_BITRATE(bitrate_bps));
	opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH(bandwidth));
	opus_encoder_ctl(enc, OPUS_SET_VBR(use_vbr));
	opus_encoder_ctl(enc, OPUS_SET_VBR_CONSTRAINT(cvbr));
	opus_encoder_ctl(enc, OPUS_SET_COMPLEXITY(complexity));
	opus_encoder_ctl(enc, OPUS_SET_INBAND_FEC(use_inbandfec));
	opus_encoder_ctl(enc, OPUS_SET_FORCE_CHANNELS(forcechannels));
	opus_encoder_ctl(enc, OPUS_SET_DTX(use_dtx));
	opus_encoder_ctl(enc, OPUS_SET_PACKET_LOSS_PERC(packet_loss_perc));

	opus_encoder_ctl(enc, OPUS_GET_LOOKAHEAD(&skip));
	opus_encoder_ctl(enc, OPUS_SET_LSB_DEPTH(16));
	opus_encoder_ctl(enc, OPUS_SET_EXPERT_FRAME_DURATION(variable_duration));

    in = (short*)malloc(max_frame_size*channels*sizeof(short));
    out = (short*)malloc(max_frame_size*channels*sizeof(short));
    /* We need to allocate for 16-bit PCM data, but we store it as unsigned char. */
    fbytes = (unsigned char*)malloc(max_frame_size*channels*sizeof(short));
    data[0] = (unsigned char*)calloc(max_payload_bytes,sizeof(unsigned char));

    while (!stop)
    {
        int i;

		err = get_bytes_local(fbytes, sizeof(short)*channels, frame_size - remaining, 0);

        curr_read = err;
        tot_in += curr_read;
        for(i=0;i<curr_read*channels;i++)
        {
            opus_int32 s;
            s=fbytes[2*i+1]<<8|fbytes[2*i];
            s=((s&0xFFFF)^0x8000)-0x8000;
            in[i+remaining*channels]=s;
        }
        if (curr_read+remaining < frame_size)
        {
            for (i=(curr_read+remaining)*channels;i<frame_size*channels;i++)
                in[i] = 0;
            if (encode_only || decode_only)
                stop = 1;
        }
        len[toggle] = opus_encode(enc, in, frame_size, data[toggle], max_payload_bytes);
        nb_encoded = opus_packet_get_samples_per_frame(data[toggle], sampling_rate)*opus_packet_get_nb_frames(data[toggle], len[toggle]);
        remaining = frame_size-nb_encoded;
        for(i=0;i<remaining*channels;i++)
            in[i] = in[nb_encoded*channels+i];

        opus_encoder_ctl(enc, OPUS_GET_FINAL_RANGE(&enc_final_range[toggle]));
        if (len[toggle] < 0)
        {
            printf ("opus_encode() returned %d\n", len[toggle]);
            return EXIT_FAILURE;
        }
        curr_mode_count += frame_size;
        if (curr_mode_count > mode_switch_time && curr_mode < nb_modes_in_list-1)
        {
            curr_mode++;
            curr_mode_count = 0;
        }

        if (encode_only)
        {
            unsigned char int_field[4];
            int_to_char(len[toggle], int_field);
			if (put_bytes_local(int_field, 1, 4, NULL) != 4) {
               printf("Error writing.\n");
               return EXIT_FAILURE;
            }
            int_to_char(enc_final_range[toggle], int_field);
			if (put_bytes_local(int_field, 1, 4, NULL) != 4) {
               printf("Error writing.\n");
               return EXIT_FAILURE;
            }
			if (put_bytes_local(data[toggle], 1, len[toggle], NULL) != (unsigned)len[toggle]) {
               printf("Error writing.\n");
               return EXIT_FAILURE;
            }
            tot_samples += nb_encoded;
        }

        lost_prev = lost;
        if( count >= use_inbandfec ) {
            /* count bits */
            bits += len[toggle]*8;
            bits_max = ( len[toggle]*8 > bits_max ) ? len[toggle]*8 : bits_max;
            bits2 += len[toggle]*len[toggle]*64;
            if (!decode_only)
            {
                nrg = 0.0;
                for ( k = 0; k < frame_size * channels; k++ ) {
                    nrg += in[ k ] * (double)in[ k ];
                }
                nrg /= frame_size * channels;
                if( nrg > 1e5 ) {
                    bits_act += len[toggle]*8;
                    count_act++;
                }
            }
        }
        count++;
        toggle = (toggle + use_inbandfec) & 1;
    }

    /* Print out bitrate statistics */

	for (int i = 0; i < writeIndex; i++)
	{
		printf("0x%02x, ", (unsigned char)outputBuffer[i]);
		if (i % 30 == 0 && i != 0)
		{
			printf("\\\n");
		}
	}

    opus_encoder_destroy(enc);
    opus_decoder_destroy(dec);
    free(data[0]);
    free(in);
    free(out);
    free(fbytes);
    return EXIT_SUCCESS;
}
