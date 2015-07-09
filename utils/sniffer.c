/**
 * @file receive.c 
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
 * This file  prints the Per frames received from the PPE 
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/if.h>
#include <linux/wireless.h>
#include "onebox_util.h"

#define PER_MODE

void sniffer_usage(char *argv)
{
		printf("Onebox dump stats application\n");   
		ONEBOX_PRINT
				("Usage: %s base_interface sniffer_mode channel_number channel_BW \n",argv);
		ONEBOX_PRINT
				("Usage: %s base_interface ch_utilization RSSI_threshold start/stop_bit(1 - start,0 - stop)\n",argv);
		printf("\tChannel BW  		- 0: 20MHz, 2: Upper 40MHz, 4: Lower 40MHz & 6: Full 40MHz \n");

		return ;
}

int sniffer_getcmdnumber (char *command)
{
		if (!strcmp (command, "sniffer_mode"))
		{
				return SNIFFER_MODE;
		}
		if (!strcmp (command, "ch_utilization"))
		{
				return CH_UTILIZATION;
		}
}

int channel_width_validation(unsigned short ch_width)
{

		if((ch_width != 0) && (ch_width != 2) && (ch_width != 4) && (ch_width != 6))
		{
				return 1;
		}
		return 0;

}

int freq_validation(int freq,unsigned short ch_width)
{
		unsigned int valid_channels_5_Ghz_40Mhz[]   = { 38, 42, 46, 50, 54, 58, 62, 102,\
						106, 110, 114, 118, 122, 126, 130, 134, 138,\
						151, 155, 159, 163 };
		unsigned int valid_channels_5_Ghz[]   = { 36, 40, 44, 48, 52, 56, 60, 64, 100,\
		104, 108, 112, 116, 120, 124, 128, 132, 136,\
				140, 149, 153, 157, 161, 165 };
		int i;
		if (freq == 0xFF)
		{
				/* Pass 0xFF so as to skip channel programming */
		}
		else if((freq >= 36 && freq <= 165 && !ch_width))
		{
				for(i = 0; i < 24; i++)
				{
						if(freq == valid_channels_5_Ghz[i])
						{
								break;
						}
				}
				if(i == 24)
				{
						return 1;
				}
		}
		else if((freq >= 36 && freq <= 165 && ch_width))
		{
				for(i = 0; i < 21; i++)
				{
						if(freq == valid_channels_5_Ghz_40Mhz[i])
						{
								break;
						}
				}
				if(i == 21)
				{
						return 1;
				}
		}
		else if(!(freq <= 14))
		{
				return 1;
		}
		return 0;

}
int main(int argc, char **argv)
{
		char ifName[32];
		int cmdNo = -1;
		int sockfd;
		int freq;
		int exit_var = 0;
		int rssi_threshold;
		int first_time = 0;
		int count = 0, start_bit = 0;
		int i; 
		unsigned short ch_width = 0;
		double tot_time = 0;
		double on_air_occupancy = 0;
		double tot_on_air_occupancy = 0;
		struct iwreq iwr;
		if (argc < 3)
		{
				sniffer_usage(argv[0]);
		}
		else if (argc <= 50)
		{
				/* Get interface name */
				if (strlen(argv[1]) < sizeof(ifName)) {
						strcpy (ifName, argv[1]);
				} else{
						ONEBOX_PRINT("length of given interface name is more than the buffer size\n");	
						return -1;
				}

				cmdNo = sniffer_getcmdnumber (argv[2]);
				//printf("cmd is %d \n",cmdNo);
		}


		/* Creating a Socket */
		sockfd = socket(PF_INET, SOCK_DGRAM, 0);
		if(sockfd < 0)
		{
				printf("Unable to create a socket\n");
				return sockfd;
		}

		switch (cmdNo)
		{
				case SNIFFER_MODE:     //to use in sniffer mode      
						{
								if(argc != 5)
								{
										printf("Invalid number of arguments \n");
										sniffer_usage(argv[0]);
										return;
								}
								freq = atoi(argv[3]);
								ch_width = atoi(argv[4]);
								if(channel_width_validation(ch_width))
								{
										printf("Invalid Channel BW values \n");
										return;
								}
								if(freq_validation(freq,ch_width))
								{
										printf("Invalid Channel issued by user\n");
										return;
								}
								per_stats *sta_info = malloc(sizeof(per_stats));
								memset(&iwr, 0, sizeof(iwr));
								iwr.u.data.flags = (unsigned short)PER_RECEIVE;                                
								/*Indicates that it is receive*/
								strncpy (iwr.ifr_name, ifName, IFNAMSIZ);
								iwr.u.data.pointer = sta_info;
								iwr.u.data.flags |= (unsigned short)freq << 8;
								*(unsigned short *)iwr.u.data.pointer = (unsigned short)ch_width;


								if(ioctl(sockfd, ONEBOX_HOST_IOCTL, &iwr) < 0)
								{
										printf("Unable to issue ioctl\n");
										return -1;
								}

								break;
						}
				case CH_UTILIZATION:     //to use for channel utilization      
						{
								if(argc != 5)
								{
										printf("Invalid number of arguments \n");
										sniffer_usage(argv[0]);
										return;
								}

								rssi_threshold = atoi(argv[3]);
								if(rssi_threshold < 20 || rssi_threshold > 90)
								{
										printf("Invalid Rssi Threshold should be 20 to 90 \n");
										return;
								}
								start_bit = atoi(argv[4]);
								if(start_bit != 0 && start_bit != 1)
								{
										printf("Invalid (1 - start)/(0 - stop bit)  \n");
										return;
								}

								struct channel_util ch_ut;
								while(1)
								{
										struct channel_util *ch_ut_ptr;
										ch_ut_ptr=malloc(sizeof(struct channel_util));
										sleep(1);
										memset(&iwr, 0, sizeof(iwr));
										/*Indicates that it is channel utilization start of stop*/
										if(start_bit == 1)
										{
												if(first_time == 0)
												{
														ch_ut.rssi_threshold = rssi_threshold;
														ch_ut.start_flag = start_bit;
														ch_ut.start_time = 0;
														ch_ut.stop_time = 0;
														ch_ut.on_air_occupancy = 0;
														ch_ut.num_of_pkt = 0;
														ch_ut.tot_len_of_pkt = 0;
														ch_ut.tot_time = 0;
														iwr.u.data.pointer = (unsigned char *)&ch_ut;
														first_time++;
                            iwr.u.data.flags = (unsigned short)CH_UTIL_START;
                            strncpy (iwr.ifr_name, ifName, IFNAMSIZ);
                            iwr.u.data.length = sizeof(struct channel_util);
                            //printf("Initail start\n");
                            if(ioctl(sockfd, ONEBOX_HOST_IOCTL, &iwr) < 0)
                            {
                                printf("Unable to issue ioctl\n");
                                free(ch_ut_ptr);
                                perror("ioctl:");
                                return -1;
                            }
                            sleep(1);
												}
												else
												{
														iwr.u.data.pointer = ch_ut_ptr;
												}
												iwr.u.data.flags = (unsigned short)CH_UTIL_START;
										}
										else                                
										{
												iwr.u.data.flags = (unsigned short)CH_UTIL_STOP;
												iwr.u.data.pointer = ch_ut_ptr;
										}
										strncpy (iwr.ifr_name, ifName, IFNAMSIZ);
										iwr.u.data.length = sizeof(struct channel_util);

										if(ioctl(sockfd, ONEBOX_HOST_IOCTL, &iwr) < 0)
										{
												printf("Unable to issue ioctl\n");
												free(ch_ut_ptr);
												perror("ioctl:");
												return -1;
										}

										if(start_bit == 1)
										{
												if((count % 20) == 0)
												{
														printf(" PKT_RCVD\t");
														printf(" CRC_PKT_RCVD\t");
														printf(" OCCUPANCY(usecs/sec)\t");
														printf(" TOTAL_BYTES(B/S)\t");
														printf(" UTILIZATION_RSSI(%/sec)\t");
														printf(" TOT_UTILIZATION(%/sec)\n");
												}
												if(count != 0 && count != 1)
												{
														tot_time = (double)(ch_ut_ptr->tot_time);
														on_air_occupancy = (double)((ch_ut_ptr->tot_len_of_pkt*8)/3000);
														tot_on_air_occupancy = (double)((ch_ut_ptr->tot_length_of_pkt*8)/3000);
														printf(" %8lu \t",ch_ut_ptr->num_of_pkt);
														printf(" %8lu \t",ch_ut_ptr->crc_pkt);
														printf(" %12lu \t",ch_ut_ptr->on_air_occupancy);
														printf(" %17lu \t",ch_ut_ptr->tot_len_of_pkt);
														printf(" %20.3f %%\t",(on_air_occupancy / tot_time ));
														printf(" %25.3f %%\n",(tot_on_air_occupancy / tot_time ));
												}
										}
										else
										{
												free(ch_ut_ptr);
												break;
										}
										free(ch_ut_ptr);
										count++;
								}

								break;
						}
				default:
						printf("Invalid Command\n");
						break;
		}


		close(sockfd);
		return 0;
}
