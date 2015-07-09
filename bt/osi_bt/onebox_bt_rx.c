/**
 *
* @file   onebox_dev_ops.c
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
* The file contains the initialization part of the SDBus driver and Loading of the 
* TA firmware.
*/

/* include files */
#include "onebox_common.h"
#include "onebox_linux.h"
#include "onebox_pktpro.h"
#include "onebox_bt_lmp.h"
#include "onebox_core.h"

#define HOST_WLCSP 1 

extern short watch_bufferfull_count;
extern char hci_events[78][47];

static void print_lmp_pkt_details(uint8 *buffer, uint16 pkt_len)
{
	uint8 slave_id, i;
	uint8 lmp_opcode;
	uint8 buffer_index = 0;
	uint8 tx_or_rx;
	char  prefix_string[6] = "LMPTX";

	slave_id = buffer[buffer_index];
	buffer_index++;

	tx_or_rx = buffer[buffer_index];
	buffer_index++;
	if(tx_or_rx == RX_PKT_DEBUG_MSG)
	{
		prefix_string[3] = 'R';
	}

	lmp_opcode = buffer[buffer_index] >> 1;
	buffer_index++;

	if(lmp_opcode != LMP_ESCAPE_OPCODE)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s %s\n"), prefix_string, lmp_opcodes[lmp_opcode]));
		switch(lmp_opcode)
		{
			case LMP_NAME_REQ:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s NAME_OFFSET: %02x \n"), prefix_string, buffer[buffer_index]));
			}
			break;
			case LMP_NAME_RES:
			{
				uint8 len;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s NAME_OFFSET: %02x \n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s NAME_FRAG_LEN: %02x \n"), prefix_string, buffer[buffer_index]));
				len = buffer[buffer_index];
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s NAME_FRAGMENT:"), prefix_string));
				for(i = 0; i < len; i++)
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%c"), buffer[buffer_index]));
					buffer_index++;
				}
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("\n")));
			}
			break;
			case LMP_ACCEPTED:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s REQ_OPCODE: %s \n"), prefix_string, lmp_opcodes[buffer[buffer_index]]));
			}
			break;
			case LMP_NOT_ACCEPTED:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s REQ_OPCODE: %s \n"), prefix_string, lmp_opcodes[buffer[buffer_index]]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s REJECTION_REASON: %02x \n"), prefix_string, buffer[buffer_index]));//TODO: create an array for error codes?
			}
			break;
			case LMP_CLKOFFSET_REQ:
			{
			}
			break;
			case LMP_CLKOFFSET_RES:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s CLOCK_OFFSET: %04x \n"), prefix_string, *(uint16 *)&buffer[buffer_index]));
			}
			break;
			case LMP_DETACH:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s REASON_CODE: %02x \n"), prefix_string, buffer[buffer_index]));//TODO: create an array for error codes?
			}
			break;
			case LMP_IN_RAND:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s IN_RAND_RANDOM_NUM:"), prefix_string));
				for(i = 0; i < 16; i++) //TODO replace with onebox_dump
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%02x "), buffer[buffer_index]));
					buffer_index++;
				}
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("\n")));
			}
			break;
			case LMP_COMB_KEY:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s COMB_KEY_RANDOM_NUM:"), prefix_string));
				for(i = 0; i < 16; i++) //TODO replace with onebox_dump
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%02x "), buffer[buffer_index]));
					buffer_index++;
				}
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("\n")));
			}
			break;
			case LMP_UNIT_KEY:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s UNIT_KEY_RANDOM_NUM:"), prefix_string));
				for(i = 0; i < 16; i++)
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%02x "), buffer[buffer_index]));
					buffer_index++;
				}
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("\n")));
			}
			break;
			case LMP_AU_RAND:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s AU_RANDOM_NUM:"), prefix_string));
				for(i = 0; i < 16; i++)
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%02x "), buffer[buffer_index]));
					buffer_index++;
				}
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("\n")));
			}
			break;
			case LMP_SRES:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s SRES: %08x\n"), prefix_string, *(uint32 *)&buffer[buffer_index]));
			}
			break;
			case LMP_TEMP_RAND:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s LMP TEMP RAND: "), prefix_string));
				for(i = 0; i < 16; i++)
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%02x "), buffer[buffer_index]));
					buffer_index++;
				}
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("\n")));
			}
			break;
			case LMP_TEMP_KEY:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s LMP TEMP KEY : "), prefix_string));
				for(i = 0; i < 16; i++)
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%02x "), buffer[buffer_index]));
					buffer_index++;
				}
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("\n")));
			}
			break;
			case LMP_ENCRYPTION_MODE_REQ:
			{
				if(buffer[buffer_index])
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s ENC_ENABLE\n"), prefix_string));
				}
				else
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s ENC_DISABLE\n"), prefix_string));
				}

			}
			break;
			case LMP_ENCRYPTION_KEY_SIZE_REQ:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s ENC_KEY_SIZE: %02x\n"), prefix_string, buffer[buffer_index]));
			}
			break;
			case LMP_START_ENCRYPTION_REQ:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s ENC_RANDOM_NUM:"), prefix_string));
				for(i = 0; i < 16; i++)
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%02x "), buffer[buffer_index]));
					buffer_index++;
				}
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("\n")));
			}
			break;
			case LMP_STOP_ENCRYPTION_REQ:
			{
			}
			break;
			case LMP_SWITCH_REQ:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s SWITCH INSTANT: %08x\n"), prefix_string, *(uint32 *)&buffer[buffer_index]));
			}
			break;
			case LMP_HOLD_PKT:
			{
			}
			case LMP_HOLD_REQ:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s HOLD TIME %04x\n"), prefix_string, *(uint16 *)&buffer[buffer_index]));
				buffer_index += 2;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s HOLD TIME %08x\n"), prefix_string, *(uint32 *)&buffer[buffer_index]));
			}
			break;
			case LMP_SNIFF_REQ:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s TIMING CONTROL FLAGS %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Dsniff %04x\n"), prefix_string, *(uint16 *)&buffer[buffer_index]));
				buffer_index += 2;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Tsniff %04x\n"), prefix_string, *(uint16 *)&buffer[buffer_index]));
				buffer_index += 2;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s SNIFF ATTEMPT %04x\n"), prefix_string, *(uint16 *)&buffer[buffer_index]));
				buffer_index += 2;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s SNIFF TIMEOUT %04x\n"), prefix_string, *(uint16 *)&buffer[buffer_index]));
			}
			break;
			case LMP_UNSNIFF_REQ:
			{
			}
			break;
			case LMP_PARK_REQ:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s TIMING CONTROL FLAGS %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Db %04x\n"), prefix_string, *(uint16 *)&buffer[buffer_index]));
				buffer_index += 2;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Tb %04x\n"), prefix_string, *(uint16 *)&buffer[buffer_index]));
				buffer_index += 2;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Nb %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s DELTAb %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s PM ADDR %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s AR ADDR %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Nbsleep %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Dbsleep %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Daccess %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Taccess %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Nacc slots %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Npoll %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Maccess %01x\n"), prefix_string, (buffer[buffer_index] & 0xF)));
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s ACCESS SCHEME %01x\n"), prefix_string, (buffer[buffer_index] >> 4)));
			}
			break;
			case LMP_SET_BROADCAST_SCAN_WINDOW:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s TIMING CONTROL FLAGS %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Db %04x\n"), prefix_string, *(uint16 *)&buffer[buffer_index]));
				buffer_index += 2;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Broadcast scan window %04x\n"), prefix_string, *(uint16 *)&buffer[buffer_index]));
			}
			break;
			case LMP_MODIFY_BEACON:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s TIMING CONTROL FLAGS %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Db %04x\n"), prefix_string, *(uint16 *)&buffer[buffer_index]));
				buffer_index += 2;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Tb %04x\n"), prefix_string, *(uint16 *)&buffer[buffer_index]));
				buffer_index += 2;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Nb %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s DELTAb %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Daccess %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Taccess %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Nacc slots %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Npoll %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Maccess %01x\n"), prefix_string, (buffer[buffer_index] & 0xF)));
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s ACCESS SCHEME %01x\n"), prefix_string, (buffer[buffer_index] >> 4)));
			}
			break;
			case LMP_UNPARK_BD_ADDR_REQ:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s TIMING CONTROL FLAGS %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Db %04x\n"), prefix_string, *(uint16 *)&buffer[buffer_index]));
				buffer_index += 2;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s LT ADDR FIRST UNPARK %01x\n"), prefix_string, (buffer[buffer_index] & 0x7)));
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s LT ADDR SECOND UNPARK %01x\n"), prefix_string, ((buffer[buffer_index] & 0x70) >> 4)));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s BD ADDR FIRST UNPARK %01x\n"), prefix_string, (buffer[buffer_index] & 0x7)));
				for(i = 0; i < 6; i++)
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%02x "), buffer[buffer_index]));
					buffer_index++;
				}
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("\n")));
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s BD ADDR SECOND UNPARK %01x\n"), prefix_string, ((buffer[buffer_index] & 0x70) >> 4)));
				for(i = 0; i < 6; i++)
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%02x "), buffer[buffer_index]));
					buffer_index++;
				}
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("\n")));
			}
			break;
			case LMP_UNPARK_PM_ADDR_REQ:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s TIMING CONTROL FLAGS %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Db %04x\n"), prefix_string, *(uint16 *)&buffer[buffer_index]));
				buffer_index += 2;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s LT ADDR FIRST UNPARK %01x\n"), prefix_string, (buffer[buffer_index] & 0x7)));
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s LT ADDR SECOND UNPARK %01x\n"), prefix_string, ((buffer[buffer_index] & 0x70) >> 4)));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s PM ADDR FIRST UNPARK %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s PM ADDR SECOND UNPARK %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s LT ADDR THIRD UNPARK %01x\n"), prefix_string, (buffer[buffer_index] & 0x7)));
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s LT ADDR FOURTH UNPARK %01x\n"), prefix_string, ((buffer[buffer_index] & 0x70) >> 4)));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s PM ADDR THIRD UNPARK %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s PM ADDR FOURTH UNPARK %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s LT ADDR FIFTH UNPARK %01x\n"), prefix_string, (buffer[buffer_index] & 0x7)));
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s LT ADDR SIXTH UNPARK %01x\n"), prefix_string, ((buffer[buffer_index] & 0x70) >> 4)));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s PM ADDR FIFTH UNPARK %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s PM ADDR SIXTH UNPARK %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s LT ADDR SEVENTH UNPARK %01x\n"), prefix_string, (buffer[buffer_index] & 0x7)));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s PM ADDR SEVENTH UNPARK %02x\n"), prefix_string, buffer[buffer_index]));
			}
			break;
			case LMP_INCR_POWER_REQ:
			{
			}
			case LMP_DECR_POWER_REQ:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s FOR FUTURE USE %02x\n"), prefix_string, buffer[buffer_index]));
			}
			break;
			case LMP_MAX_POWER:
			{
			}
			case LMP_MIN_POWER:
			{
			}
			break;
			case LMP_AUTO_RATE:
			{
			}
			break;
			case LMP_PREFERRED_RATE:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s PREFERRED_RATE: %02x\n"), prefix_string, buffer[buffer_index]));
			}
			break;
			case LMP_VERSION_REQ:
			{
				/* Same as LMP_VERSION_RES */
			}
			case LMP_VERSION_RES:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s VERSION_NUM: %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s COMP_ID: %04x\n"), prefix_string, *(uint16 *)&buffer[buffer_index]));
				buffer_index += 2;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s SUB_VERSION_NUM: %04x\n"), prefix_string, *(uint16 *)&buffer[buffer_index]));
				buffer_index += 2;
			}
			break;
			case LMP_FEATURES_REQ:
			{
				/* Same as LMP_FEATURES_RES */
			}
			case LMP_FEATURES_RES:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s FEATURES_BITMAP:"), prefix_string));
				for(i = 0; i < 8; i++)
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%02x "), buffer[buffer_index]));
					buffer_index++;
				}
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("\n")));
			}
			break;
			case LMP_QUALITY_OF_SERVICE:
			{
				/* Same as LMP_QUALITY_OF_SERVICE_REQ */
			}
			case LMP_QUALITY_OF_SERVICE_REQ:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s POLL_INTERVAL: %04x\n"), prefix_string, *(uint16 *)&buffer[buffer_index]));
				buffer_index += 2;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s NBR: %02x\n"), prefix_string, buffer[buffer_index]));
			}
			break;
			case LMP_SCO_LINK_REQ:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_REMOVE_SCO_LINK_REQ:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_MAX_SLOT:
			{
				/* Same as LMP_MAX_SLOT_REQ */
			}
			case LMP_MAX_SLOT_REQ:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s MAX_NUM_SLOTS: %02x\n"), prefix_string, buffer[buffer_index]));
			}
			break;
			case LMP_TIMING_ACCURACY_REQ:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_TIMING_ACCURACY_RES:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s DRIFT: %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s JITTER: %02x\n"), prefix_string, buffer[buffer_index]));
			}
			break;
			case LMP_SETUP_COMPLETE:
			{
			}
			break;
			case LMP_USE_SEMI_PERMANENT_KEY:
			{
			}
			break;
			case LMP_HOST_CONNECTION_REQ:
			{
			}
			break;
			case LMP_SLOT_OFFSET:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s SLOT_OFFSET: 0x%04x = %d\n"), prefix_string, *(uint16 *)&buffer[buffer_index], *(uint16 *)&buffer[buffer_index]));
				buffer_index += 2;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s BD_ADDR: %02x:%02x:%02x:%02x:%02x:%02x\n"), prefix_string, buffer[buffer_index], buffer[buffer_index + 1], buffer[buffer_index + 2], buffer[buffer_index + 3], buffer[buffer_index + 4], buffer[buffer_index + 5]));
			}
			break;
			case LMP_PAGE_MODE_REQ:
			{
				/* Same as LMP_PAGE_SCAN_MODE_REQ */
			}
			case LMP_PAGE_SCAN_MODE_REQ:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s PAGING_SCHEME: %02x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s PAGING_SCHEME_SETTINGS: %02x\n"), prefix_string, buffer[buffer_index]));
			}
			break;
			case LMP_SUPERVISION_TIMEOUT:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s LINK_SUPERVISION_TOUT: %04x\n"), prefix_string, *(uint16 *)&buffer[buffer_index]));
			}
			break;
			case LMP_TEST_ACTIVATE:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_TEST_CONTROL:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_ENCRYPTION_KEYSIZE_MASK_REQ:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_ENCRYPTION_KEYSIZE_MASK_RES:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_SET_AFH:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s AFH_INSTANT: %08x\n"), prefix_string, *(uint32 *)&buffer[buffer_index]));
				buffer_index += 4;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s AFH_MODE: %x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s AFH_CHANNEL_MAP:"), prefix_string));
				for(i = 0; i < 10; i++)
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%02x "), buffer[buffer_index]));
					buffer_index++;
				}
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("\n")));
			}
			break;
			case LMP_ENCAPSULATED_HEADER:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_ENCAPSULATED_PAYLOAD:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_SIMPLE_PAIRING_CONFIRM:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_SIMPLE_PAIRING_NUMBER:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_DHKEY_CHECK:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			default:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Unknown LMP packet, opcode = %d\n"), prefix_string, lmp_opcode));
			}
			break;
		}
	}
	else
	{
		lmp_opcode = buffer[buffer_index];
		buffer_index++;
		
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s %s\n"), prefix_string, lmp_ext_opcodes[lmp_opcode]));
		switch(lmp_opcode)
		{
			case LMP_ACCEPTED_EXT:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s REQ_OPCODE: %s \n"), prefix_string, lmp_opcodes[buffer[buffer_index]]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s REQ_EXT_OPCODE: %s \n"), prefix_string, lmp_ext_opcodes[buffer[buffer_index]]));
			}
			break;
			case LMP_NOT_ACCEPTED_EXT:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s REQ_OPCODE: %s \n"), prefix_string, lmp_opcodes[buffer[buffer_index]]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s REQ_EXT_OPCODE: %s \n"), prefix_string, lmp_ext_opcodes[buffer[buffer_index]]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s REJECTION_REASON: %02x \n"), prefix_string, buffer[buffer_index]));//TODO: create an array for error codes?
			}
			break;
			case LMP_FEATURES_REQ_EXT:
			{
				/* Same as LMP_FEATURES_RES_EXT */
			}
			case LMP_FEATURES_RES_EXT:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s FEATURES_PAGE: %x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s MAX_SUPPORTED_FEATURES_PAGE: %x\n"), prefix_string, buffer[buffer_index]));
				buffer_index++;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s EXT_FEATURES_BITMAP: "), prefix_string));
				for(i = 0; i < 8; i++)
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%02x "), buffer[buffer_index]));
					buffer_index++;
				}
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("\n")));
			}
			break;
			case LMP_PACKET_TYPE_TABLE_REQ:
			{
				if(buffer[buffer_index])
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s DATA_RATE: EDR RATE\n"), prefix_string));
				}
				else
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s DATA_RATE: BR RATE\n"), prefix_string));
				}
			}
			break;
			case LMP_ESCO_LINK_REQ:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_REMOVE_eSCO_LINK_REQ:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_CHANNEL_CLASSIFICATION_REQ:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_CHANNEL_CLASSIFICATION:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_SNIFF_SUBRATING_REQ:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_SNIFF_SUBRATING_RES:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_PAUSE_ENCRYPTION_REQ:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_RESUME_ENCRYPTION_REQ:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_IO_CAPABILITIES_REQ:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_IO_CAPABILITIES_RES:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_NUMERIC_COMPARISON_FAILED:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_PASSKEY_FAILED:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_OOB_FAILED:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_KEYPRESS_NOTIFICATION:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_POWER_CONTROL_REQ:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			case LMP_POWER_CONTROL_RES:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s \n"), prefix_string));
			}
			break;
			default:
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s Unknown EXT LMP packet, opcode = %d\n"), prefix_string, lmp_opcode));
			}
			break;
		}
	}
}


static ONEBOX_STATUS print_hci_event_type(PONEBOX_ADAPTER adapter, uint8 *buffer)
{
	uint8 pkt_type = buffer[14];

	switch (pkt_type)
	{
		case HCI_ACLDATA_PKT:
		{
			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("ACL DATA PACKET\n")));
			break;
		}
		case HCI_SCODATA_PKT:
		{
			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("SCO DATA PACKET\n")));
			break;
		}
		case HCI_EVENT_PKT:
		{
			uint8 event_type = buffer[16];

			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s"), hci_events[event_type]));
			if(event_type == HCI_EV_CMD_COMPLETE)
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT(" - ")));
				print_rx_hci_cmd_type(adapter, *(uint16 *)&buffer[19], HCI_COMMAND_PKT);
			}
			else
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("\n")));
			}
			break;
		}
		case HCI_VENDOR_PKT:
		{
			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("VENDOR PACKET\n")));
			{
				uint16 pkt_buff_index = 16;
				uint16 vendor_pkt_type;
				uint16 pkt_debug_msg_len;

				vendor_pkt_type = *(uint16 *)&buffer[pkt_buff_index];
				pkt_buff_index += 2;
				switch(vendor_pkt_type)
				{
					case LMP_PKT_DEBUG_MSG:
					{
						pkt_debug_msg_len = *(uint16 *)&buffer[pkt_buff_index];
						pkt_buff_index += 2;

						print_lmp_pkt_details(&buffer[pkt_buff_index], pkt_debug_msg_len);

						return ONEBOX_DEBUG_MESG_RECVD;
					}
					break;
					case LLP_PKT_DEBUG_MSG:
					{
						pkt_debug_msg_len = *(uint16 *)&buffer[pkt_buff_index];
						pkt_buff_index += 2;

						//print_llp_pkt_details(&buffer[pkt_buff_index], pkt_debug_msg_len);

						return ONEBOX_DEBUG_MESG_RECVD;
					}
					break;
					default:
					{
						ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Unknown VENDOR PACKET, type = 0x%04x\n"), vendor_pkt_type));
					}
					break;
				}
			}
			break;
		}
		default:
		{
			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("UNKNOWN PACKET TYPE\n")));
			break;
		}
	}
	return ONEBOX_STATUS_SUCCESS;
}

/**
 * This function prepares the netbuf control block
 *
 * @param 
 * adapter pointer to the driver private structure
 * @param 
 * buffer pointer to the packet data
 * @param 
 * len length of the packet 
 * @return .This function returns ONEBOX_STATUS_SUCCESS.
 */
int prepare_netbuf_cb(PONEBOX_ADAPTER adapter, uint8 *buffer,
		                    uint32 pkt_len)
{
	netbuf_ctrl_block_t *netbuf_cb;

	adapter->stats.rx_packets++;

	{
		/* Receive packet dump */
		//ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("\nRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRX\n")));
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("RX PACKET LENGTH = %d\n"), pkt_len+16));
		if(print_hci_event_type(adapter, buffer))
		{
			adapter->osi_bt_ops->onebox_dump(ONEBOX_ZONE_ERROR, buffer, pkt_len+16);
			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("\nRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRX\n")));
			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("\n***************************************************\n")));
			/* this packet need not to be sent to upper layers and is debug message packet */
			return ONEBOX_STATUS_SUCCESS;
		}
		adapter->osi_bt_ops->onebox_dump(ONEBOX_ZONE_ERROR, buffer, pkt_len+16);
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("\nRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRX\n")));
	}

	netbuf_cb = adapter->os_intf_ops->onebox_alloc_skb(pkt_len);

	if(netbuf_cb != NULL)
	{
		/* Assigning packet type */
		netbuf_cb->bt_pkt_type = buffer[14];
	}
	else
	{
		printk("Unable to allocate skb\n");
		return ONEBOX_STATUS_FAILURE;
	}
	/* Preparing The netbuf_cb To Indicate To core */
	adapter->os_intf_ops->onebox_add_data_to_skb(netbuf_cb, pkt_len);

	/* Copy the packet to into netbuf_cb */
	/*
	 * netbuf_cb->data contains extend_desc + payload
	 */ 
	adapter->os_intf_ops->onebox_memcpy((VOID *)netbuf_cb->data, (VOID *)(buffer + FRAME_DESC_SZ), pkt_len);

	adapter->osi_bt_ops->onebox_dump(ONEBOX_ZONE_INFO, netbuf_cb->data, netbuf_cb->len);
	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("@@@ Indicating packet to core:\n")));
	adapter->osi_bt_ops->onebox_indicate_pkt_to_core(adapter, netbuf_cb);
	return ONEBOX_STATUS_SUCCESS;
}

/**
 * This function receives the management packets from the hardware and process them
 *
 * @param 
 *  adapter  Pointer to driver private structure
 * @param 
 *  msg      Received packet
 * @param 
 *  len      Length of the received packet
 *
 * @returns 
 *  ONEBOX_STATUS_SUCCESS on success, or corresponding negative
 *  error code on failure
 */
int32 onebox_bt_mgmt_pkt_recv(PONEBOX_ADAPTER adapter, netbuf_ctrl_block_t *netbuf_cb)
{
	int32 msg_type;
	int32 msg_len;
	int32 total_len;
	uint8 *msg;
	int8 ret;
	//EEPROM_READ read_buf;

	FUNCTION_ENTRY(ONEBOX_ZONE_MGMT_RCV);

	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("In %s function inside file mgmt.c\n"),__func__));

	msg =  netbuf_cb->data;
	msg_len = ONEBOX_CPU_TO_LE16(*(uint16 *)&msg[0]) & 0xFFF;
	msg_type = msg[14] & 0xFF;
	total_len = msg_len + 16;
	ONEBOX_DEBUG(ONEBOX_ZONE_MGMT_RCV,(TEXT("Rcvd Mgmt Pkt Len = %d\n"), total_len));
	adapter->osi_bt_ops->onebox_dump(ONEBOX_ZONE_MGMT_DUMP, msg, total_len);

	ONEBOX_DEBUG(ONEBOX_ZONE_MGMT_RCV,
	             (TEXT("Msg Len: %d, Msg Type: %#x\n"), msg_len, msg_type));

	switch (adapter->fsm_state)
	{
#if 0
		case FSM_CARD_NOT_READY: 
		{
      			if (msg_type == MGMT_DESC_TYP_CARD_RDY_IND)
      			{
        			ONEBOX_DEBUG(ONEBOX_ZONE_FSM,(TEXT("onebox_mgmt_fsm: CARD_READY\n")));
				adapter->fsm_state = FSM_DEVICE_READY;
				adapter->osi_bt_ops->onebox_core_init(adapter);
		  	}
		}
		break;
#endif
		case FSM_DEVICE_READY:
		{
			printk("Inside case FSM_DEVICE_READY\n");
			switch (msg_type)
			{
				case RESULT_CONFIRM: 
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_MGMT_RCV,
					             (TEXT("Rcvd Result Confirm  Msg_type: %x\n"),msg[3]));
				}
				break;
				case STATION_STATISTICS_RESP:
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_MGMT_RCV,
					             (TEXT("Rcvd Station Statics Response\n")));
				}
				break;
				default:
				{
					/* Forward the frames to mgmt sw module */
					ret = onebox_mgmt_pkt_to_core(adapter, netbuf_cb, total_len, 1);
					return ret;
				}
				break;
			} /* End switch (msg_type) */
			break; 
		}
		default:
		{
			adapter->osi_bt_ops->onebox_dump(ONEBOX_ZONE_INIT, msg, total_len);
			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
			             (TEXT("Invalid FSM State %d\n"), adapter->fsm_state));
		} 
		break;
	} /* End switch (adapter->fsm_state) */

	adapter->os_intf_ops->onebox_free_pkt(adapter, netbuf_cb, 0);
	FUNCTION_EXIT(ONEBOX_ZONE_MGMT_RCV);
	return ONEBOX_STATUS_SUCCESS;
}

/**
 * Entry point of Management module
 *
 * @param
 *  adapter    Pointer to driver private data
 * @param
 *  msg    Buffer recieved from the device
 * @param
 *  msg_len    Length of the buffer
 * @return
 *  Status
 */
int onebox_mgmt_pkt_to_core(PONEBOX_ADAPTER adapter,
                          netbuf_ctrl_block_t *netbuf_cb,
                          int32 msg_len,
                          uint8 type)
{
	uint8 *msg;

	printk("+%s",__func__);

	FUNCTION_ENTRY(ONEBOX_ZONE_MGMT_RCV);

	msg = netbuf_cb->data;
	adapter->osi_bt_ops->onebox_dump(ONEBOX_ZONE_MGMT_DUMP, msg, msg_len);
	if (type)
	{
		msg_len -= FRAME_DESC_SZ;
		if ((msg_len <= 0) || (!msg))
		{
			ONEBOX_DEBUG(ONEBOX_ZONE_MGMT_SEND, 
			             (TEXT("Invalid incoming message of message length = %d\n"), msg_len));
			adapter->os_intf_ops->onebox_free_pkt(adapter, netbuf_cb, 0);
			return ONEBOX_STATUS_FAILURE;
		}
		
#ifdef USE_BLUEZ_BT_STACK
		adapter->os_intf_ops->onebox_netbuf_adj(netbuf_cb, FRAME_DESC_SZ);
#endif
		adapter->osi_bt_ops->onebox_indicate_pkt_to_core(adapter, netbuf_cb);
	}
	else
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_MGMT_SEND,(TEXT ("Internal Packet\n")));
	}

	FUNCTION_EXIT(ONEBOX_ZONE_MGMT_RCV);
	return 0;
}

/**
 * This function read frames from the SD card.
 *
 * @param  Pointer to driver adapter structure.  
 * @param  Pointer to received packet.  
 * @param  Pointer to length of the received packet.  
 * @return 0 if success else -1. 
 */
ONEBOX_STATUS bt_read_pkt(PONEBOX_ADAPTER adapter, netbuf_ctrl_block_t *netbuf_cb)
{
	uint32 queueno;
	uint8 extended_desc;
	uint8 *frame_desc_addr = netbuf_cb->data;
	uint32 length = 0;
	uint16 offset =0;
	
  ONEBOX_DEBUG(ONEBOX_ZONE_INFO,(TEXT("adapter = %p\n"),
	             adapter));
	
	queueno = (uint32)((*(uint16 *)&frame_desc_addr[offset] & 0x7000) >> 12);
	length   = (*(uint16 *)&frame_desc_addr[offset] & 0x0fff);
	extended_desc   = (*(uint8 *)&frame_desc_addr[offset + 4] & 0x00ff);
	ONEBOX_DEBUG(ONEBOX_ZONE_DATA_RCV, (TEXT("###Received in QNumber:%d Len=%d###!!!\n"), queueno, length));
	
	switch(queueno)
	{
		case BT_DATA_Q:      
		{
			if (length > (ONEBOX_RCV_BUFFER_LEN * 4 ))
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
						(TEXT("%s: Toooo big packet %d\n"), __func__, length));	    
				adapter->osi_bt_ops->onebox_dump(ONEBOX_ZONE_DATA_RCV, frame_desc_addr, FRAME_DESC_SZ);
				length = ONEBOX_RCV_BUFFER_LEN * 4 ;
			}
			if (length < 3)
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
						(TEXT("%s: Too small packet %d\n"), __func__, length));
				adapter->osi_bt_ops->onebox_dump(ONEBOX_ZONE_DATA_RCV, frame_desc_addr, FRAME_DESC_SZ);
			}

			if (!length)
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s: Pkt size is zero\n"), __func__));
				adapter->os_intf_ops->onebox_free_pkt(adapter,netbuf_cb,0);
				return ONEBOX_STATUS_FAILURE;
			}

			adapter->osi_bt_ops->onebox_dump(ONEBOX_ZONE_DATA_RCV, frame_desc_addr, length + FRAME_DESC_SZ);
			netbuf_cb->bt_pkt_type = frame_desc_addr[14];
			adapter->stats.rx_packets++;

			{
				/* Receive packet dump */
				//ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("\nRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRX\n")));
				ONEBOX_DEBUG(ONEBOX_ZONE_DATA_RCV, (TEXT("RX PACKET LENGTH = %d\n"), length+16));
				if(print_hci_event_type(adapter, netbuf_cb->data))
				{
					adapter->osi_bt_ops->onebox_dump(ONEBOX_ZONE_DATA_RCV, netbuf_cb->data, length+16);
					ONEBOX_DEBUG(ONEBOX_ZONE_DATA_RCV, (TEXT("\nRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRX\n")));
					ONEBOX_DEBUG(ONEBOX_ZONE_DATA_RCV, (TEXT("\n***************************************************\n")));
					/* this packet need not to be sent to upper layers and is debug message packet */
					return ONEBOX_STATUS_SUCCESS;
				}
				adapter->osi_bt_ops->onebox_dump(ONEBOX_ZONE_DATA_RCV, netbuf_cb->data, length+16);
				ONEBOX_DEBUG(ONEBOX_ZONE_DATA_RCV, (TEXT("\nRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRXRX\n")));
			}
			/*
			 * frame_desc_addr contains FRAME DESC + extended DESC + payload
			 * lenght = extended descriptor size + payload size 
			 */
#ifdef USE_BLUEZ_BT_STACK
			adapter->os_intf_ops->onebox_netbuf_adj(netbuf_cb, FRAME_DESC_SZ);
#endif
			ONEBOX_DEBUG(ONEBOX_ZONE_INFO,(TEXT("@@@ Indicating packet to core:\n")));
			adapter->osi_bt_ops->onebox_dump(ONEBOX_ZONE_INFO, netbuf_cb->data, netbuf_cb->len);
			adapter->osi_bt_ops->onebox_indicate_pkt_to_core(adapter, netbuf_cb);
		}
		break;
/*
		case RCV_LMAC_MGMT_Q:
		case RCV_TA_MGMT_Q1:
		case RCV_TA_MGMT_Q:
*/
		case BT_INT_MGMT_Q:
		{
			printk("*****received qno is %d********\n",queueno);
			onebox_bt_mgmt_pkt_recv(adapter, netbuf_cb);
		} 
		break;

		default:
		{
			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s: pkt from invalid queue\n"), __func__));
			adapter->os_intf_ops->onebox_free_pkt(adapter, netbuf_cb, 0);
		}
		break;
	} /* End switch */      


	return ONEBOX_STATUS_SUCCESS;
}
EXPORT_SYMBOL(bt_read_pkt);
