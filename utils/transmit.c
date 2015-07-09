/**
 * @file transmit.c 
 * @author 
 * @version 1.0
 *
 * @section LICENSE
 *
 * This software embodies materials and concepts that are confidential to Redpine
 * Signals and is made available solely pursuant to the terms of a written license
 * agreement with Redpine Signals
 *
 * @section DESCRIPTION
 *
 * This file handles the transmission of Per frames to the PPE in continuous/Burst mode
 * as per user's choice
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/if.h>
#include <linux/wireless.h>
#include "onebox_util.h"

int cal_rate(char* );

//main
int main(int argc, char *argv[])
{      
	struct iwreq iwr;
	int tx_pwr, tx_pktlen, tx_mode, chan_number; 
	int sockfd, i;
	char *tmp_rate;
	per_params_t per_params;
	unsigned char rate_flags = 0;
	unsigned int valid_channels_5_Ghz[]   = { 36, 40, 44, 48, 52, 56, 60, 64, 100,\
									  		  104, 108, 112, 116, 120, 124, 128, 132, 136,\
									          140, 149, 153, 157, 161, 165 
											};
	unsigned int valid_channels_5_Ghz_40Mhz[]   = { 38, 42, 46, 50, 54, 58, 62, 102,\
									  		  106, 110, 114, 118, 122, 126, 130, 134, 138,\
									          151, 155, 159, 163 
											};

	/*Creating a Socket*/
	sockfd = socket(PF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		printf("Unable to create a socket\n");
		return sockfd;
	}
	
	memset(&per_params, 0, sizeof(per_params_t));
	if (argc == 11 )
	{
		memset(&iwr, 0, sizeof(iwr));
		strncpy(iwr.ifr_name, "rpine0", 6);

		per_params.enable = 1;

		tx_pwr = atoi(argv[1]);
		printf("TX PWR is %d\n", tx_pwr);
#if 0
		if (tx_pwr > 63)
		{
			tx_pwr = 63; 
		}
		if (tx_pwr > 31 && tx_pwr < 63)
		{
			tx_pwr += 32;
		}
#endif
		per_params.power = tx_pwr;

		//Rate
		tmp_rate = argv[2];
		char *tmp_rate = argv[2];

		if(!strcmp(tmp_rate,"1"))
			per_params.rate = RSI_RATE_1;
		else if(!strcmp(tmp_rate,"2"))
			per_params.rate = RSI_RATE_2;
		else if(!strcmp(tmp_rate,"5.5"))
			per_params.rate = RSI_RATE_5_5; 
		else if(!strcmp(tmp_rate,"11"))
			per_params.rate = RSI_RATE_11;
		else if(!strcmp(tmp_rate,"6"))
			per_params.rate = RSI_RATE_6;
		else if(!strcmp(tmp_rate,"9"))
			per_params.rate = RSI_RATE_9;
		else if(!strcmp(tmp_rate,"12"))
			per_params.rate = RSI_RATE_12;
		else if(!strcmp(tmp_rate,"18"))
			per_params.rate = RSI_RATE_18;
		else if(!strcmp(tmp_rate,"24"))
			per_params.rate = RSI_RATE_24;
		else if(!strcmp(tmp_rate,"36"))
			per_params.rate = RSI_RATE_36;
		else if(!strcmp(tmp_rate,"48"))
			per_params.rate = RSI_RATE_48;
		else if(!strcmp(tmp_rate,"54"))
			per_params.rate = RSI_RATE_54;
		else if(!strcmp(tmp_rate,"mcs0"))
			per_params.rate = RSI_RATE_MCS0;
		else if(!strcmp(tmp_rate,"mcs1"))
			per_params.rate = RSI_RATE_MCS1;
		else if(!strcmp(tmp_rate,"mcs2"))
			per_params.rate = RSI_RATE_MCS2;
		else if(!strcmp(tmp_rate,"mcs3"))
			per_params.rate = RSI_RATE_MCS3;
		else if(!strcmp(tmp_rate,"mcs4"))
			per_params.rate = RSI_RATE_MCS4;
		else if(!strcmp(tmp_rate,"mcs5"))
			per_params.rate = RSI_RATE_MCS5;
		else if(!strcmp(tmp_rate,"mcs6"))
			per_params.rate = RSI_RATE_MCS6;
		else if(!strcmp(tmp_rate,"mcs7"))
			per_params.rate = RSI_RATE_MCS7;
		else
			per_params.rate = RSI_RATE_1;


		//pkt length
		tx_pktlen = atoi(argv[3]);

		per_params.pkt_length = tx_pktlen;

		//mode
		tx_mode = atoi(argv[4]);
		if(tx_mode == 1 || tx_mode == 0)
		{
			per_params.mode = tx_mode;
		}
		else
		{
			per_params.mode = 0;
		}
		
		chan_number = atoi(argv[5]);
		rate_flags  = atoi(argv[7]);
		per_params.rate_flags = rate_flags;
		per_params.per_ch_bw = rate_flags >> 2;
		per_params.aggr_enable  = atoi(argv[8]);
		per_params.aggr_count   = (per_params.pkt_length/PER_AGGR_LIMIT_PER_PKT);
                if((per_params.pkt_length - (per_params.aggr_count * PER_AGGR_LIMIT_PER_PKT)) > 0)
                {	
			per_params.aggr_count++; 
		}
		if(per_params.aggr_count == 1)
		{
			per_params.aggr_enable = 0;
			per_params.aggr_count = 0;
		}
		per_params.no_of_pkts   = atoi(argv[9]);
		per_params.delay   = atoi(argv[10]);
#if 1
		if(tx_pktlen > 1536 && per_params.aggr_enable ==0 )
		{
			printf("Invalid length,Give the length <= 1536 \n");
			exit(0);
		}
		if((tx_pktlen > 30000) && (per_params.aggr_enable))
		{
			printf("Cant aggregate,Give the length <= 30000 \n");
			exit(0);
		}
		if((per_params.aggr_enable) && !(per_params.rate >= RSI_RATE_MCS0 && per_params.rate <= RSI_RATE_MCS7))
		{
			printf("Cant aggregate,Give 11n rate \n");
			exit(0);
		}
#endif
		
        	if (chan_number == 0xFF)
        	{
			per_params.channel = chan_number;	
			/* Pass 0xFF so as to skip channel programming */
        	}
		else if(chan_number <= 14)
		{
			per_params.channel = chan_number;
		}
		else if((chan_number >= 36 && chan_number <= 165) && !per_params.per_ch_bw) /* For 20Mhz BW */
		{
			for(i = 0; i < 24; i++)
			{
				if(chan_number == valid_channels_5_Ghz[i])
				{
					per_params.channel = chan_number;
					break;
				}
			}
			if(!(per_params.channel == chan_number))
			{
				printf("Invalid Channel issued by user for 20Mhz BW\n");
				exit(0);
			}
		}
		else if((chan_number >= 36 && chan_number <= 165) && per_params.per_ch_bw) /* For 20Mhz BW */
		{
			for(i = 0; i < 21; i++)
			{
				if(chan_number == valid_channels_5_Ghz_40Mhz[i])
				{
					per_params.channel = chan_number;
					break;
				}
			}
			if(!(per_params.channel == chan_number))
			{
				printf("Invalid Channel issued by user for 40Mhz BW\n");
				exit(0);
			}
		}
		else
		{
			printf("Invalid Channel issued by user\n");
			exit(0);
		}

		printf("\n--Tx TEST CONFIGURATION--\n\n");
		printf("Tx POWER      : %d\n",atoi(argv[1]));
		printf("Tx RATE       : %s\n", argv[2]);
		printf("PACKET LENGTH : %d\n",per_params.pkt_length);
		if(tx_mode == 1)
		{
			printf("Tx MODE       : CONTINUOUS\n");
			per_params.pkt_length = 28;
		}
		else if (tx_mode == 0)
		{
			printf("Tx MODE       : BURST\n");
		}
		else
		{
			printf("Tx MODE       : CONTINUOUS\n");
		}
		printf("CHANNEL NUM   : %d\n", chan_number);
		printf("RATE_FLAGS    : %d\n", per_params.rate_flags);
		printf("CHAN_WIDTH    : %d\n", per_params.per_ch_bw);
		printf("AGGR_ENABLE   : %d\n", per_params.aggr_enable);
		printf("NO OF PACKETS : %d\n", per_params.no_of_pkts);
		printf("DELAY         : %d\n", per_params.delay);

		/* Filling the iwreq structure */ 
		memset(&iwr, 0, sizeof(iwr));
		strncpy(iwr.ifr_name, "rpine0", 6);
		
		/*Indicates that it is transmission*/
		iwr.u.data.flags = (unsigned short)PER_TRANSMIT;
		iwr.u.data.pointer = (unsigned char *)&per_params;
		iwr.u.data.length  = sizeof(per_params);
		
		if(ioctl(sockfd, ONEBOX_HOST_IOCTL, &iwr) < 0)
		{
			perror(argv[0]);
			printf("Please ensure OneBox Driver is running with valid arguments \tor stop existing transmit utility\n");
		}
		else
		{
			printf("Tx Started\n");
		}
	}
   
	else if(argc == 2)
	{
		i = atoi(argv[1]);
		if((i == 0) || (i == 1))
		{
			per_params.mode = atoi(argv[1]);
			per_params.enable = 0;
			memset(&iwr, 0, sizeof(iwr));
			strncpy(iwr.ifr_name, "rpine0", 7);
			 /*Indicates that it is transmission*/
			iwr.u.data.flags = (unsigned short)PER_TRANSMIT;    
			iwr.u.data.pointer = (unsigned char *)&per_params;
			iwr.u.data.length  = sizeof(per_params);

			if(ioctl(sockfd, ONEBOX_HOST_IOCTL, &iwr) < 0)
			{
				perror(argv[0]);
				printf("&&Please ensure Burst or Continuous Mode is running\n");
			}
			else
			{
				printf("Tx Stopped\n");
			}
		}
		else
		{
			printf("Please enter either 0 or 1 as an argument, instead of %d to stop..\n",i);
		}
	}

	else
	{
		printf("\nUSAGE to start transmit: %s tx_power rate length tx_mode channel ExtPA-Enable Rate_flags Aggr_enable no_of_packets delay \n",argv[0]);
		printf("\nUSAGE to stop transmit: %s tx_mode\n\t****** FIELDS *******",argv[0]);
		printf("\ntx_mode : 0 - Burst , 1 - Continuous mode\n");
		printf("\nTX-Power 127 to use max Power in Flash\n");
		printf("\nRate_flags Bits: \n");
		printf("Bit 0		: (Short_GI for HT mode)/(Short_preamble in 11b)\n");
		printf("Bit 1		: (GreenField for HT mode)/(preamble enable for 11b)\n");
		printf("Bit 5-2		: CH_BW flags\n");
		printf("Bit 15-6	: Reserved\n\n");
		return 0;
	}

	return 0;
}
