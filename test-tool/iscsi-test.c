/*
   iscsi-test tool

   Copyright (C) 2010 by Ronnie Sahlberg <ronniesahlberg@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.
*/

#define _GNU_SOURCE
#include <sys/syscall.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <poll.h>
#include <popt.h>
#include <fnmatch.h>
#include "slist.h"
#include "iscsi.h"
#include "scsi-lowlevel.h"
#include "iscsi-private.h"
#include "iscsi-test.h"

#ifndef discard_const
#define discard_const(ptr) ((void *)((intptr_t)(ptr)))
#endif

const char *initiatorname1 = "iqn.2007-10.com.github:sahlberg:libiscsi:iscsi-test";
const char *initiatorname2 = "iqn.2007-10.com.github:sahlberg:libiscsi:iscsi-test-2";

uint32_t block_size;
uint64_t num_blocks;
int lbpme;
int lbppb;
int lbpme;
int removable;
enum scsi_inquiry_peripheral_device_type device_type;
int sccs;
int encserv;

int data_loss;
int show_info;

struct scsi_test {
       const char *name;
       int (*test)(const char *initiator, const char *url);
};

struct scsi_test tests[] = {
/* SCSI protocol tests */

/* testunitready*/
{ "T0000_testunitready_simple",		T0000_testunitready_simple },

/* read10*/
{ "T0100_read10_simple",		T0100_read10_simple },
{ "T0101_read10_beyond_eol",		T0101_read10_beyond_eol },
{ "T0102_read10_0blocks",		T0102_read10_0blocks },
{ "T0103_read10_rdprotect",		T0103_read10_rdprotect },
{ "T0104_read10_flags",			T0104_read10_flags },
{ "T0105_read10_invalid",      		T0105_read10_invalid },

/* readcapacity10*/
{ "T0110_readcapacity10_simple",	T0110_readcapacity10_simple },

/* read6*/
{ "T0120_read6_simple",			T0120_read6_simple },
{ "T0121_read6_beyond_eol",		T0121_read6_beyond_eol },
{ "T0122_read6_invalid",      		T0122_read6_invalid },

/* verify10*/
{ "T0130_verify10_simple",		T0130_verify10_simple },
{ "T0131_verify10_mismatch",		T0131_verify10_mismatch },
{ "T0132_verify10_mismatch_no_cmp",	T0132_verify10_mismatch_no_cmp },
{ "T0133_verify10_beyondeol",		T0133_verify10_beyondeol },

/* readcapacity16*/
{ "T0160_readcapacity16_simple",	T0160_readcapacity16_simple },
{ "T0161_readcapacity16_alloclen",	T0161_readcapacity16_alloclen },

/* unmap*/
{ "T0170_unmap_simple",			T0170_unmap_simple },
{ "T0171_unmap_zero",			T0171_unmap_zero },

/* writesame10*/
{ "T0180_writesame10_unmap",		T0180_writesame10_unmap },
{ "T0181_writesame10_unmap_unaligned",	T0181_writesame10_unmap_unaligned },
{ "T0182_writesame10_beyondeol",	T0182_writesame10_beyondeol },
{ "T0183_writesame10_wrprotect",	T0183_writesame10_wrprotect },
{ "T0184_writesame10_0blocks",		T0184_writesame10_0blocks },

/* writesame16*/
{ "T0190_writesame16_unmap",		T0190_writesame16_unmap },
{ "T0191_writesame16_unmap_unaligned",	T0191_writesame16_unmap_unaligned },
{ "T0192_writesame16_beyondeol",	T0192_writesame16_beyondeol },
{ "T0193_writesame16_wrprotect",	T0193_writesame16_wrprotect },
{ "T0194_writesame16_0blocks",		T0194_writesame16_0blocks },

/* read16*/
{ "T0200_read16_simple",		T0200_read16_simple },
{ "T0201_read16_rdprotect",		T0201_read16_rdprotect },
{ "T0202_read16_flags",			T0202_read16_flags },
{ "T0203_read16_0blocks",		T0203_read16_0blocks },
{ "T0204_read16_beyondeol",		T0204_read16_beyondeol },

/* read12*/
{ "T0210_read12_simple",		T0210_read12_simple },
{ "T0211_read12_rdprotect",		T0211_read12_rdprotect },
{ "T0212_read12_flags",			T0212_read12_flags },
{ "T0213_read12_0blocks",		T0213_read12_0blocks },
{ "T0214_read12_beyondeol",		T0214_read12_beyondeol },

/* write16*/
{ "T0220_write16_simple",		T0220_write16_simple },
{ "T0221_write16_wrprotect",		T0221_write16_wrprotect },
{ "T0222_write16_flags",	       	T0222_write16_flags },
{ "T0223_write16_0blocks",		T0223_write16_0blocks },
{ "T0224_write16_beyondeol",		T0224_write16_beyondeol },

/* write12*/
{ "T0230_write12_simple",		T0230_write12_simple },
{ "T0231_write12_wrprotect",		T0231_write12_wrprotect },
{ "T0232_write12_flags",	       	T0232_write12_flags },
{ "T0233_write12_0blocks",		T0233_write12_0blocks },
{ "T0234_write12_beyondeol",		T0234_write12_beyondeol },

/* prefetch10*/
{ "T0240_prefetch10_simple",		T0240_prefetch10_simple },
{ "T0241_prefetch10_flags",		T0241_prefetch10_flags },
{ "T0242_prefetch10_beyondeol",		T0242_prefetch10_beyondeol },
{ "T0243_prefetch10_0blocks",		T0243_prefetch10_0blocks },

/* prefetch16*/
{ "T0250_prefetch16_simple",		T0250_prefetch16_simple },
{ "T0251_prefetch16_flags",		T0251_prefetch16_flags },
{ "T0252_prefetch16_beyondeol",		T0252_prefetch16_beyondeol },
{ "T0253_prefetch16_0blocks",		T0253_prefetch16_0blocks },

/* get_lba_status */
{ "T0260_get_lba_status_simple",	T0260_get_lba_status_simple },
{ "T0264_get_lba_status_beyondeol",	T0264_get_lba_status_beyondeol },

/* verify16*/
{ "T0270_verify16_simple",		T0270_verify16_simple },
{ "T0271_verify16_mismatch",		T0271_verify16_mismatch },
{ "T0272_verify16_mismatch_no_cmp",	T0272_verify16_mismatch_no_cmp },
{ "T0273_verify16_beyondeol",		T0273_verify16_beyondeol },

/* verify12*/
{ "T0280_verify12_simple",		T0280_verify12_simple },
{ "T0281_verify12_mismatch",		T0281_verify12_mismatch },
{ "T0282_verify12_mismatch_no_cmp",	T0282_verify12_mismatch_no_cmp },
{ "T0283_verify12_beyondeol",		T0283_verify12_beyondeol },

/* write10*/
{ "T0290_write10_simple",		T0290_write10_simple },
{ "T0291_write10_wrprotect",		T0291_write10_wrprotect },
{ "T0292_write10_flags",	       	T0292_write10_flags },
{ "T0293_write10_0blocks",		T0293_write10_0blocks },
{ "T0294_write10_beyondeol",		T0294_write10_beyondeol },

/* Readonly */
{ "T0300_readonly",			T0300_readonly },

/* writeverify10*/
{ "T0310_writeverify10_simple",		T0310_writeverify10_simple },
{ "T0311_writeverify10_wrprotect",	T0311_writeverify10_wrprotect },
{ "T0314_writeverify10_beyondeol",	T0314_writeverify10_beyondeol },

/* writeverify12*/
{ "T0320_writeverify12_simple",		T0320_writeverify12_simple },
{ "T0321_writeverify12_wrprotect",	T0321_writeverify12_wrprotect },
{ "T0324_writeverify12_beyondeol",	T0324_writeverify12_beyondeol },

/* writeverify16*/
{ "T0330_writeverify16_simple",		T0330_writeverify16_simple },
{ "T0331_writeverify16_wrprotect",	T0331_writeverify16_wrprotect },
{ "T0334_writeverify16_beyondeol",	T0334_writeverify16_beyondeol },

/* compareandwrite*/
{ "T0340_compareandwrite_simple",	T0340_compareandwrite_simple },
{ "T0341_compareandwrite_mismatch",	T0341_compareandwrite_mismatch },
{ "T0343_compareandwrite_beyondeol",	T0343_compareandwrite_beyondeol },

/* orwrite*/
{ "T0350_orwrite_simple",		T0350_orwrite_simple },
{ "T0351_orwrite_wrprotect",		T0351_orwrite_wrprotect },
{ "T0354_orwrite_beyindeol",		T0354_orwrite_beyondeol },

/* startstopunit*/
{ "T0360_startstopunit_simple",		T0360_startstopunit_simple },
{ "T0361_startstopunit_pwrcnd",		T0361_startstopunit_pwrcnd },
{ "T0362_startstopunit_noloej",		T0362_startstopunit_noloej },

/* nomedia*/
{ "T0370_nomedia",			T0370_nomedia },

/* preventallowmediumremoval*/
{ "T0380_preventallow_simple",		T0380_preventallow_simple },
{ "T0381_preventallow_eject",		T0381_preventallow_eject },
{ "T0382_preventallow_itnexus_loss",	T0382_preventallow_itnexus_loss },
{ "T0383_preventallow_target_warm_reset",	T0383_preventallow_target_warm_reset },
{ "T0384_preventallow_target_cold_reset",	T0384_preventallow_target_cold_reset },
{ "T0385_preventallow_lun_reset",	T0385_preventallow_lun_reset },
{ "T0386_preventallow_2_itl_nexuses",	T0386_preventallow_2_itl_nexuses },

/* support for mandatory opcodes*/
{ "T0390_mandatory_opcodes_sbc",	T0390_mandatory_opcodes_sbc },

/* inquiry*/
{ "T0400_inquiry_basic",		T0400_inquiry_basic },
{ "T0401_inquiry_alloclen",		T0401_inquiry_alloclen },
{ "T0402_inquiry_evpd",			T0402_inquiry_evpd },
{ "T0403_inquiry_supported_vpd",	T0403_inquiry_supported_vpd },
{ "T0404_inquiry_all_reported_vpd",	T0404_inquiry_all_reported_vpd },

/* read TOC/PMA/ATIP */
{ "T0410_readtoc_basic",                T0410_readtoc_basic },

/* reserve6/release6 */
{ "T0420_reserve6_simple",              T0420_reserve6_simple },
{ "T0421_reserve6_lun_reset",           T0421_reserve6_lun_reset },
{ "T0422_reserve6_logout",              T0422_reserve6_logout },
{ "T0423_reserve6_sessionloss",         T0423_reserve6_sessionloss },
{ "T0424_reserve6_target_reset",           T0424_reserve6_target_reset },

/* Maintenance In - Report Supported Operations */
{ "T0430_report_all_supported_ops",     T0430_report_all_supported_ops },

/* iSCSI protocol tests */

/* invalid cmdsn from initiator */
{ "T1000_cmdsn_invalid",		T1000_cmdsn_invalid },

/* invalid datasn from initiator */
{ "T1010_datasn_invalid",		T1010_datasn_invalid },

/* invalid bufferoffset from initiator */
{ "T1020_bufferoffset_invalid",		T1020_bufferoffset_invalid },

/* sending too much unsolicited data */
{ "T1030_unsolicited_data_overflow",	T1030_unsolicited_data_overflow },
{ "T1031_unsolicited_data_out",		T1031_unsolicited_data_out },

/* Test that if we start blocking new I/O due to saturating maxcmdsn
   that we eventualld do recover and finish
*/
{ "T1040_saturate_maxcmdsn",		T1040_saturate_maxcmdsn },

{ "T1041_unsolicited_immediate_data",	T1041_unsolicited_immediate_data },
{ "T1042_unsolicited_nonimmediate_data",T1042_unsolicited_nonimmediate_data },

/* PERSISTENT_RESERVE_IN */
{ "T1100_persistent_reserve_in_read_keys_simple", T1100_persistent_reserve_in_read_keys_simple },
{ "T1110_persistent_reserve_in_serviceaction_range", T1110_persistent_reserve_in_serviceaction_range },
{ "T1120_persistent_register_simple", T1120_persistent_register_simple },
{ "T1130_persistent_reserve_simple", T1130_persistent_reserve_simple },

{ NULL, NULL }
};

	
void print_usage(void)
{
	fprintf(stderr, "Usage: iscsi-test [-?] [-?|--help] [--usage] [-t|--test=<test>] [-s|--skip=<test>]\n"
			"\t\t[-l|--list] [--info] [-i|--initiator-name=<iqn-name>]\n"
			"\t\t<iscsi-url>\n");
}

void print_help(void)
{
	fprintf(stderr, "Usage: iscsi-test [OPTION...] <iscsi-url>\n");
	fprintf(stderr, "  -i, --initiator-name=iqn-name     Initiatorname to use\n");
	fprintf(stderr, "  -I, --initiator-name-2=iqn-name   Second initiatorname to use\n");
	fprintf(stderr, "  -t, --test=test-name              Which test to run. Default is to run all tests.\n");
	fprintf(stderr, "  -s, --skip=test-name              Which test to skip. Default is to run all tests.\n");
	fprintf(stderr, "  -l, --list                        List all tests.\n");
	fprintf(stderr, "  --info,                           Print extra info about a test.\n");
	fprintf(stderr, "  --dataloss                        Allow destructive tests.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Help options:\n");
	fprintf(stderr, "  -?, --help                        Show this help message\n");
	fprintf(stderr, "      --usage                       Display brief usage message\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "iSCSI URL format : %s\n", ISCSI_URL_SYNTAX);
	fprintf(stderr, "\n");
	fprintf(stderr, "<host> is either of:\n");
	fprintf(stderr, "  \"hostname\"       iscsi.example\n");
	fprintf(stderr, "  \"ipv4-address\"   10.1.1.27\n");
	fprintf(stderr, "  \"ipv6-address\"   [fce0::1]\n");
}


struct iscsi_context *iscsi_context_login(const char *initiatorname, const char *url, int *lun)
{
	struct iscsi_context *iscsi;
	struct iscsi_url *iscsi_url;

	iscsi = iscsi_create_context(initiatorname);
	if (iscsi == NULL) {
		fprintf(stderr, "Failed to create context\n");
		return NULL;
	}

	iscsi_url = iscsi_parse_full_url(iscsi, url);
	if (iscsi_url == NULL) {
		fprintf(stderr, "Failed to parse URL: %s\n",
			iscsi_get_error(iscsi));
		iscsi_destroy_context(iscsi);
		return NULL;
	}

	iscsi_set_targetname(iscsi, iscsi_url->target);
	iscsi_set_session_type(iscsi, ISCSI_SESSION_NORMAL);
	iscsi_set_header_digest(iscsi, ISCSI_HEADER_DIGEST_NONE_CRC32C);

	if (iscsi_url->user != NULL) {
		if (iscsi_set_initiator_username_pwd(iscsi, iscsi_url->user, iscsi_url->passwd) != 0) {
			fprintf(stderr, "Failed to set initiator username and password\n");
			iscsi_destroy_url(iscsi_url);
			iscsi_destroy_context(iscsi);
			return NULL;
		}
	}

	if (iscsi_full_connect_sync(iscsi, iscsi_url->portal, iscsi_url->lun) != 0) {
		fprintf(stderr, "Login Failed. %s\n", iscsi_get_error(iscsi));
		iscsi_destroy_url(iscsi_url);
		iscsi_destroy_context(iscsi);
		return NULL;
	}
	if (lun != NULL) {
		*lun = iscsi_url->lun;
	}

	iscsi_destroy_url(iscsi_url);
	return iscsi;
}

void wait_until_test_finished(struct iscsi_context *iscsi, struct iscsi_async_state *state)
{
	struct pollfd pfd;
	int count = 0;
	int ret;

	while (state->finished == 0) {
		pfd.fd = iscsi_get_fd(iscsi);
		pfd.events = iscsi_which_events(iscsi);

		ret = poll(&pfd, 1, 1000);
		if (ret < 0) {
			printf("Poll failed");
			exit(10);
		}
		if (ret == 0) {
			if (count++ > 5) {
				struct iscsi_pdu *pdu;

				state->finished     = 1;
				state->status       = SCSI_STATUS_CANCELLED;
				state->task->status = SCSI_STATUS_CANCELLED;
				/* this may leak memory since we dont free the pdu */
				while ((pdu = iscsi->outqueue)) {
					SLIST_REMOVE(&iscsi->outqueue, pdu);
				}
				while ((pdu = iscsi->waitpdu)) {
					SLIST_REMOVE(&iscsi->waitpdu, pdu);
				}
				return;
			}
			continue;
		}
		if (iscsi_service(iscsi, pfd.revents) < 0) {
			printf("iscsi_service failed with : %s\n", iscsi_get_error(iscsi));
			break;
		}
	}
}

static int (*real_iscsi_queue_pdu)(struct iscsi_context *iscsi, struct iscsi_pdu *pdu);

int iscsi_queue_pdu(struct iscsi_context *iscsi, struct iscsi_pdu *pdu)
{
	if (local_iscsi_queue_pdu != NULL) {
		local_iscsi_queue_pdu(iscsi, pdu);
	}
	return real_iscsi_queue_pdu(iscsi, pdu);
}

int register_and_ignore(struct iscsi_context *iscsi, int lun,
    unsigned long long sark)
{
	struct scsi_persistent_reserve_out_basic poc;
	struct scsi_task *task;


	/* register our reservation key with the target */
	printf("Send PROUT/REGISTER_AND_IGNORE to register ... ");
	memset(&poc, 0, sizeof (poc));
	poc.service_action_reservation_key = sark;
	task = iscsi_persistent_reserve_out_sync(iscsi, lun,
	    SCSI_PERSISTENT_RESERVE_REGISTER_AND_IGNORE_EXISTING_KEY,
	    SCSI_PERSISTENT_RESERVE_SCOPE_LU, 0, &poc);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send PROUT command: %s\n",
		    iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status == SCSI_STATUS_CHECK_CONDITION &&
	    task->sense.key == SCSI_SENSE_ILLEGAL_REQUEST &&
	    task->sense.ascq == SCSI_SENSE_ASCQ_INVALID_OPERATION_CODE) {
		printf("[SKIPPED]\n");
		printf("PROUT Not Supported\n");
		scsi_free_scsi_task(task);
		return -2;
	}
	if (task->status != SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("PROUT command: failed with sense. %s\n",
		    iscsi_get_error(iscsi));
		scsi_free_scsi_task(task);
		return -1;
	}

	scsi_free_scsi_task(task);
	printf("[OK]\n");

	return 0;
}


int register_key(struct iscsi_context *iscsi, int lun,
    unsigned long long sark, unsigned long long rk)
{
	struct scsi_persistent_reserve_out_basic poc;
	struct scsi_task *task;


	/* register our reservation key with the target */
	printf("Send PROUT/REGISTER to %s ... ",
	    sark != 0 ? "register" : "unregister");
	memset(&poc, 0, sizeof (poc));
	poc.service_action_reservation_key = sark;
	poc.reservation_key = rk;
	task = iscsi_persistent_reserve_out_sync(iscsi, lun,
	    SCSI_PERSISTENT_RESERVE_REGISTER,
	    SCSI_PERSISTENT_RESERVE_SCOPE_LU, 0, &poc);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send PROUT command: %s\n",
		    iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status != SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("PROUT command: failed with sense. %s\n",
		    iscsi_get_error(iscsi));
		scsi_free_scsi_task(task);
		return -1;
	}

	scsi_free_scsi_task(task);
	printf("[OK]\n");

	return 0;
}


int verify_key_presence(struct iscsi_context *iscsi, int lun,
    unsigned long long key, int present)
{
	struct scsi_task *task;
	const int buf_sz = 16384;
	int i;
	int key_found;
	struct scsi_persistent_reserve_in_read_keys *rk = NULL;


	printf("Send PRIN/READ_KEYS to verify key %s ... ",
	    present ? "present" : "absent");
	task = iscsi_persistent_reserve_in_sync(iscsi, lun,
	    SCSI_PERSISTENT_RESERVE_READ_KEYS, buf_sz);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send PRIN command: %s\n",
		    iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status != SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("PRIN command: failed with sense. %s\n",
		    iscsi_get_error(iscsi));
		scsi_free_scsi_task(task);
		return -1;
	}
	rk = scsi_datain_unmarshall(task);
	if (rk == NULL) {
		printf("failed to unmarshall PRIN/READ_KEYS data. %s\n",
		    iscsi_get_error(iscsi));
		scsi_free_scsi_task(task);
		return -1;
	}

	scsi_free_scsi_task(task);

	key_found = 0;
	for (i = 0; i < rk->num_keys; i++) {
		if (rk->keys[i] == key)
			key_found = 1;
	}

	if ((present && key_found) ||
	    (!present && !key_found)) {
		printf("[OK]\n");
		return 0;
	} else {
	        printf("[FAILED]\n");
		if (present)
			printf("Key found when none expected\n");
		else
			printf("Key not found when expected\n");
		return -1;
	}
}


int reregister_key_fails(struct iscsi_context *iscsi, int lun,
    unsigned long long sark)
{
	struct scsi_persistent_reserve_out_basic poc;
	struct scsi_task *task;


	printf("Send PROUT/REGISTER to ensure reregister fails ... ");
	memset(&poc, 0, sizeof (poc));
	poc.service_action_reservation_key = sark;
	task = iscsi_persistent_reserve_out_sync(iscsi, lun,
	    SCSI_PERSISTENT_RESERVE_REGISTER,
	    SCSI_PERSISTENT_RESERVE_SCOPE_LU,
	    SCSI_PERSISTENT_RESERVE_TYPE_WRITE_EXCLUSIVE,
	    &poc);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send PROUT command: %s\n",
		    iscsi_get_error(iscsi));
		return -1;
	}
	
	if (task->status != SCSI_STATUS_CHECK_CONDITION ||
	    task->sense.key != SCSI_SENSE_ILLEGAL_REQUEST ||
	    task->sense.ascq != SCSI_SENSE_ASCQ_INVALID_OPERATION_CODE) {
		printf("[FAILED]\n");
		printf("PROUT/REGISTER when already registered should fail\n");
		scsi_free_scsi_task(task);
		return -1;
	}
	if (task->status == SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("PROUT/REGISTER command: succeeded when it should not have!\n");
		scsi_free_scsi_task(task);
		return -1;
	}

	scsi_free_scsi_task(task);
	printf("[OK]\n");

	return 0;
}


int reserve(struct iscsi_context *iscsi, int lun,
    unsigned long long key, enum scsi_persistent_out_type pr_type)
{
	struct scsi_persistent_reserve_out_basic poc;
	struct scsi_task *task;


	/* reserve the target using specified reservation type */
	printf("Send PROUT/RESERVE to reserve, type=%d (%s) ... ",
	    pr_type, scsi_pr_type_str(pr_type));

	memset(&poc, 0, sizeof (poc));
	poc.reservation_key = key;
	task = iscsi_persistent_reserve_out_sync(iscsi, lun,
	    SCSI_PERSISTENT_RESERVE_RESERVE,
	    SCSI_PERSISTENT_RESERVE_SCOPE_LU,
	    pr_type, &poc);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send PROUT command: %s\n",
		    iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status != SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("PROUT command: failed with sense. %s\n",
		    iscsi_get_error(iscsi));
		scsi_free_scsi_task(task);
		return -1;
	}

	scsi_free_scsi_task(task);
	printf("[OK]\n");

	return 0;
}


int release(struct iscsi_context *iscsi, int lun,
    unsigned long long key, enum scsi_persistent_out_type pr_type)
{
	struct scsi_persistent_reserve_out_basic poc;
	struct scsi_task *task;


	/* release the target using specified reservation type */
	printf("Send PROUT/RELEASE to release reservation, type=%d ... ",
	    pr_type);

	memset(&poc, 0, sizeof (poc));
	poc.reservation_key = key;
	task = iscsi_persistent_reserve_out_sync(iscsi, lun,
	    SCSI_PERSISTENT_RESERVE_RELEASE,
	    SCSI_PERSISTENT_RESERVE_SCOPE_LU,
	    pr_type, &poc);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send PROUT command: %s\n",
		    iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status != SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("PROUT command: failed with sense. %s\n",
		    iscsi_get_error(iscsi));
		scsi_free_scsi_task(task);
		return -1;
	}

	scsi_free_scsi_task(task);
	printf("[OK]\n");

	return 0;
}

int verify_reserved_as(struct iscsi_context *iscsi, int lun,
    unsigned long long key, enum scsi_persistent_out_type pr_type)
{
	struct scsi_task *task;
	const int buf_sz = 16384;
	struct scsi_persistent_reserve_in_read_reservation *rr = NULL;


	printf("Send PRIN/READ_RESERVATION to verify type=%d ... ",
	    pr_type);
	task = iscsi_persistent_reserve_in_sync(iscsi, lun,
	    SCSI_PERSISTENT_RESERVE_READ_RESERVATION, buf_sz);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send PRIN command: %s\n",
		    iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status != SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("PRIN command: failed with sense. %s\n",
		    iscsi_get_error(iscsi));
		scsi_free_scsi_task(task);
		return -1;
	}
	rr = scsi_datain_unmarshall(task);
	if (rr == NULL) {
		printf("failed to unmarshall PRIN/READ_RESERVATION data. %s\n",
		    iscsi_get_error(iscsi));
		scsi_free_scsi_task(task);
		return -1;
	}

	scsi_free_scsi_task(task);


	if (!rr->reserved) {
		printf("Failed to find Target reserved as expected.\n");
		return -1;
	}
	if (rr->reservation_key != key) {
		printf("Failed to find reservation key 0x%llx: found 0x%lx.\n",
		    key, rr->reservation_key);
		return -1;
	}
	if (rr->pr_type != pr_type) {
		printf("Failed to find reservation type %d: found %d.\n",
		    pr_type, rr->pr_type);
		return -1;
	}

	printf("[OK]\n");
	return 0;
}

int testunitready(struct iscsi_context *iscsi, int lun)
{
	struct scsi_task *task;

	printf("Send TESTUNITREADY ... ");
	task = iscsi_testunitready_sync(iscsi, lun);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send TESTUNITREADY command: %s\n",
		    iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status != SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("TESTUNITREADY command: failed with sense. %s\n",
		    iscsi_get_error(iscsi));
		scsi_free_scsi_task(task);
		return -1;
	}
	scsi_free_scsi_task(task);
	printf("[OK]\n");
	return 0;
}

int testunitready_nomedium(struct iscsi_context *iscsi, int lun)
{
	struct scsi_task *task;

	printf("Send TESTUNITREADY (expecting NOT_READY/MEDIUM_NOT_PRESENT) ... ");
	task = iscsi_testunitready_sync(iscsi, lun);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send TESTUNITREADY command: %s\n",
		    iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status        != SCSI_STATUS_CHECK_CONDITION
	    || task->sense.key  != SCSI_SENSE_NOT_READY
	    || (task->sense.ascq != SCSI_SENSE_ASCQ_MEDIUM_NOT_PRESENT
	        && task->sense.ascq != SCSI_SENSE_ASCQ_MEDIUM_NOT_PRESENT_TRAY_OPEN
	        && task->sense.ascq != SCSI_SENSE_ASCQ_MEDIUM_NOT_PRESENT_TRAY_CLOSED)) {
		printf("[FAILED]\n");
		printf("TESTUNITREADY Should have failed with NOT_READY/MEDIUM_NOT_PRESENT*\n");
		scsi_free_scsi_task(task);
		return -1;
	}	
	scsi_free_scsi_task(task);
	printf("[OK]\n");
	return 0;
}

int testunitready_conflict(struct iscsi_context *iscsi, int lun)
{
	struct scsi_task *task;

	printf("Send TESTUNITREADY (expecting RESERVATION_CONFLICT) ... ");
	task = iscsi_testunitready_sync(iscsi, lun);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send TESTUNITREADY command: %s\n",
		    iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status != SCSI_STATUS_RESERVATION_CONFLICT) {
		printf("[FAILED]\n");
		printf("Expected RESERVATION CONFLICT\n");
		return -1;
	}
	scsi_free_scsi_task(task);
	printf("[OK]\n");
	return 0;
}

int prefetch10(struct iscsi_context *iscsi, int lun, uint32_t lba, int num, int immed, int group)
{
	struct scsi_task *task;

	printf("Send PREFETCH10 LBA:%d Count:%d IMEMD:%d GROUP:%d ... ", lba, num, immed, group);
	task = iscsi_prefetch10_sync(iscsi, lun, lba, num, immed, group);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send PREFETCH10 command: %s\n", iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status        == SCSI_STATUS_CHECK_CONDITION
	    && task->sense.key  == SCSI_SENSE_ILLEGAL_REQUEST
	    && task->sense.ascq == SCSI_SENSE_ASCQ_INVALID_OPERATION_CODE) {
		printf("[SKIPPED]\n");
		printf("PREFETCH10 is not implemented on target\n");
		scsi_free_scsi_task(task);
		return -2;
	}
	if (task->status != SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("PREFETCH10 command: failed with sense. %s\n", iscsi_get_error(iscsi));
		scsi_free_scsi_task(task);
		return -1;
	}
	scsi_free_scsi_task(task);
	printf("[OK]\n");
	return 0;
}

int prefetch10_lbaoutofrange(struct iscsi_context *iscsi, int lun, uint32_t lba, int num, int immed, int group)
{
	struct scsi_task *task;

	printf("Send PREFETCH10 LBA:%d Count:%d IMEMD:%d GROUP:%d (expecting ILLEGAL_REQUEST/LBA_OUT_OF_RANGE) ... ", lba, num, immed, group);
	task = iscsi_prefetch10_sync(iscsi, lun, lba, num, immed, group);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send PREFETCH10 command: %s\n", iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status        == SCSI_STATUS_CHECK_CONDITION
	    && task->sense.key  == SCSI_SENSE_ILLEGAL_REQUEST
	    && task->sense.ascq == SCSI_SENSE_ASCQ_INVALID_OPERATION_CODE) {
		printf("[SKIPPED]\n");
		printf("PREFETCH10 is not implemented on target\n");
		scsi_free_scsi_task(task);
		return -2;
	}
	if (task->status        != SCSI_STATUS_CHECK_CONDITION
		|| task->sense.key  != SCSI_SENSE_ILLEGAL_REQUEST
		|| task->sense.ascq != SCSI_SENSE_ASCQ_LBA_OUT_OF_RANGE) {
		printf("[FAILED]\n");
		printf("PREFETCH10 should have failed with ILLEGAL_REQUEST/LBA_OUT_OF_RANGE. Sense:%s\n", iscsi_get_error(iscsi));
		scsi_free_scsi_task(task);
		return -1;
	}

	printf("[OK]\n");
	scsi_free_scsi_task(task);
	return 0;
}

int prefetch10_nomedium(struct iscsi_context *iscsi, int lun, uint32_t lba, int num, int immed, int group)
{
	struct scsi_task *task;

	printf("Send PREFETCH10 LBA:%d Count:%d IMEMD:%d GROUP:%d (expecting NOT_READY/MEDIUM_NOT_PRESENT) ... ", lba, num, immed, group);
	task = iscsi_prefetch10_sync(iscsi, lun, lba, num, immed, group);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send PREFETCH10 command: %s\n", iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status        == SCSI_STATUS_CHECK_CONDITION
	    && task->sense.key  == SCSI_SENSE_ILLEGAL_REQUEST
	    && task->sense.ascq == SCSI_SENSE_ASCQ_INVALID_OPERATION_CODE) {
		printf("[SKIPPED]\n");
		printf("PREFETCH10 is not implemented on target\n");
		scsi_free_scsi_task(task);
		return -2;
	}
	if (task->status        != SCSI_STATUS_CHECK_CONDITION
	    || task->sense.key  != SCSI_SENSE_NOT_READY
	    || (task->sense.ascq != SCSI_SENSE_ASCQ_MEDIUM_NOT_PRESENT
	        && task->sense.ascq != SCSI_SENSE_ASCQ_MEDIUM_NOT_PRESENT_TRAY_OPEN
	        && task->sense.ascq != SCSI_SENSE_ASCQ_MEDIUM_NOT_PRESENT_TRAY_CLOSED)) {
		printf("[FAILED]\n");
		printf("PREFETCH10 after eject failed with the wrong sense code. Should fail with NOT_READY/MEDIUM_NOT_PRESENT*\n");
		scsi_free_scsi_task(task);
		return -1;
	}	

	printf("[OK]\n");
	scsi_free_scsi_task(task);
	return 0;
}

int prefetch16(struct iscsi_context *iscsi, int lun, uint64_t lba, int num, int immed, int group)
{
	struct scsi_task *task;

	printf("Send PREFETCH16 LBA:%" PRIu64 " Count:%d IMEMD:%d GROUP:%d ... ", lba, num, immed, group);
	task = iscsi_prefetch16_sync(iscsi, lun, lba, num, immed, group);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send PREFETCH16 command: %s\n", iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status        == SCSI_STATUS_CHECK_CONDITION
	    && task->sense.key  == SCSI_SENSE_ILLEGAL_REQUEST
	    && task->sense.ascq == SCSI_SENSE_ASCQ_INVALID_OPERATION_CODE) {
		printf("[SKIPPED]\n");
		printf("PREFETCH16 is not implemented on target\n");
		scsi_free_scsi_task(task);
		return -2;
	}
	if (task->status != SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("PREFETCH16 command: failed with sense. %s\n", iscsi_get_error(iscsi));
		scsi_free_scsi_task(task);
		return -1;
	}
	scsi_free_scsi_task(task);
	printf("[OK]\n");
	return 0;
}

int prefetch16_lbaoutofrange(struct iscsi_context *iscsi, int lun, uint64_t lba, int num, int immed, int group)
{
	struct scsi_task *task;

	printf("Send PREFETCH16 LBA:%" PRIu64 " Count:%d IMEMD:%d GROUP:%d (expecting ILLEGAL_REQUEST/LBA_OUT_OF_RANGE) ... ", lba, num, immed, group);
	task = iscsi_prefetch16_sync(iscsi, lun, lba, num, immed, group);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send PREFETCH16 command: %s\n", iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status        == SCSI_STATUS_CHECK_CONDITION
	    && task->sense.key  == SCSI_SENSE_ILLEGAL_REQUEST
	    && task->sense.ascq == SCSI_SENSE_ASCQ_INVALID_OPERATION_CODE) {
		printf("[SKIPPED]\n");
		printf("PREFETCH16 is not implemented on target\n");
		scsi_free_scsi_task(task);
		return -2;
	}
	if (task->status        != SCSI_STATUS_CHECK_CONDITION
		|| task->sense.key  != SCSI_SENSE_ILLEGAL_REQUEST
		|| task->sense.ascq != SCSI_SENSE_ASCQ_LBA_OUT_OF_RANGE) {
		printf("[FAILED]\n");
		printf("PREFETCH16 should have failed with ILLEGAL_REQUEST/LBA_OUT_OF_RANGE. Sense:%s\n", iscsi_get_error(iscsi));
		scsi_free_scsi_task(task);
		return -1;
	}

	printf("[OK]\n");
	scsi_free_scsi_task(task);
	return 0;
}

int prefetch16_nomedium(struct iscsi_context *iscsi, int lun, uint64_t lba, int num, int immed, int group)
{
	struct scsi_task *task;

	printf("Send PREFETCH16 LBA:%" PRIu64 " Count:%d IMEMD:%d GROUP:%d (expecting NOT_READY/MEDIUM_NOT_PRESENT) ... ", lba, num, immed, group);
	task = iscsi_prefetch16_sync(iscsi, lun, lba, num, immed, group);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send PREFETCH16 command: %s\n", iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status        == SCSI_STATUS_CHECK_CONDITION
	    && task->sense.key  == SCSI_SENSE_ILLEGAL_REQUEST
	    && task->sense.ascq == SCSI_SENSE_ASCQ_INVALID_OPERATION_CODE) {
		printf("[SKIPPED]\n");
		printf("PREFETCH16 is not implemented on target\n");
		scsi_free_scsi_task(task);
		return -2;
	}
	if (task->status        != SCSI_STATUS_CHECK_CONDITION
	    || task->sense.key  != SCSI_SENSE_NOT_READY
	    || (task->sense.ascq != SCSI_SENSE_ASCQ_MEDIUM_NOT_PRESENT
	        && task->sense.ascq != SCSI_SENSE_ASCQ_MEDIUM_NOT_PRESENT_TRAY_OPEN
	        && task->sense.ascq != SCSI_SENSE_ASCQ_MEDIUM_NOT_PRESENT_TRAY_CLOSED)) {
		printf("[FAILED]\n");
		printf("PREFETCH16 after eject failed with the wrong sense code. Should fail with NOT_READY/MEDIUM_NOT_PRESENT*\n");
		scsi_free_scsi_task(task);
		return -1;
	}	

	printf("[OK]\n");
	scsi_free_scsi_task(task);
	return 0;
}

int verify10(struct iscsi_context *iscsi, int lun, unsigned char *data, uint32_t datalen, uint32_t lba, int vprotect, int dpo, int bytchk, int blocksize)
{
	struct scsi_task *task;

	printf("Send VERIFY10 LBA:%d blocks:%d vprotect:%d dpo:%d bytchk:%d ... ", lba, datalen / blocksize, vprotect, dpo, bytchk);
	task = iscsi_verify10_sync(iscsi, lun, data, datalen, lba, vprotect, dpo, bytchk, blocksize);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send VERIFY10 command: %s\n", iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status        == SCSI_STATUS_CHECK_CONDITION
	    && task->sense.key  == SCSI_SENSE_ILLEGAL_REQUEST
	    && task->sense.ascq == SCSI_SENSE_ASCQ_INVALID_OPERATION_CODE) {
		printf("[SKIPPED]\n");
		printf("VERIFY10 is not implemented on target\n");
		scsi_free_scsi_task(task);
		return -2;
	}
	if (task->status != SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("VERIFY10 command: failed with sense. %s\n", iscsi_get_error(iscsi));
		scsi_free_scsi_task(task);
		return -1;
	}

	printf("[OK]\n");
	scsi_free_scsi_task(task);
	return 0;
}

int verify10_nomedium(struct iscsi_context *iscsi, int lun, unsigned char *data, uint32_t datalen, uint32_t lba, int vprotect, int dpo, int bytchk, int blocksize)
{
	struct scsi_task *task;

	printf("Send VERIFY10 LBA:%d blocks:%d vprotect:%d dpo:%d bytchk:%d (expecting NOT_READY/MEDIUM_NOT_PRESENT) ... ", lba, datalen / blocksize, vprotect, dpo, bytchk);
	task = iscsi_verify10_sync(iscsi, lun, data, datalen, lba, vprotect, dpo, bytchk, blocksize);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send VERIFY10 command: %s\n", iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status        == SCSI_STATUS_CHECK_CONDITION
	    && task->sense.key  == SCSI_SENSE_ILLEGAL_REQUEST
	    && task->sense.ascq == SCSI_SENSE_ASCQ_INVALID_OPERATION_CODE) {
		printf("[SKIPPED]\n");
		printf("VERIFY10 is not implemented on target\n");
		scsi_free_scsi_task(task);
		return -2;
	}
	if (task->status        != SCSI_STATUS_CHECK_CONDITION
	    || task->sense.key  != SCSI_SENSE_NOT_READY
	    || (task->sense.ascq != SCSI_SENSE_ASCQ_MEDIUM_NOT_PRESENT
	        && task->sense.ascq != SCSI_SENSE_ASCQ_MEDIUM_NOT_PRESENT_TRAY_OPEN
	        && task->sense.ascq != SCSI_SENSE_ASCQ_MEDIUM_NOT_PRESENT_TRAY_CLOSED)) {
		printf("[FAILED]\n");
		printf("VERIFY10 after eject failed with the wrong sense code. Should fail with NOT_READY/MEDIUM_NOT_PRESENT*\n");
		scsi_free_scsi_task(task);
		return -1;
	}	

	printf("[OK]\n");
	scsi_free_scsi_task(task);
	return 0;
}

int verify10_miscompare(struct iscsi_context *iscsi, int lun, unsigned char *data, uint32_t datalen, uint32_t lba, int vprotect, int dpo, int bytchk, int blocksize)
{
	struct scsi_task *task;

	printf("Send VERIFY10 LBA:%d blocks:%d vprotect:%d dpo:%d bytchk:%d (expecting MISCOMPARE) ... ", lba, datalen / blocksize, vprotect, dpo, bytchk);
	task = iscsi_verify10_sync(iscsi, lun, data, datalen, lba, vprotect, dpo, bytchk, blocksize);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send VERIFY10 command: %s\n", iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status        == SCSI_STATUS_CHECK_CONDITION
	    && task->sense.key  == SCSI_SENSE_ILLEGAL_REQUEST
	    && task->sense.ascq == SCSI_SENSE_ASCQ_INVALID_OPERATION_CODE) {
		printf("[SKIPPED]\n");
		printf("VERIFY10 is not implemented on target\n");
		scsi_free_scsi_task(task);
		return -2;
	}
	if (task->status == SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("VERIFY10 command successful but should have failed with MISCOMPARE\n");
		scsi_free_scsi_task(task);
		return -1;
	}
	if (task->sense.key != SCSI_SENSE_MISCOMPARE) {
		printf("[FAILED]\n");
		printf("VERIFY10 command returned wrong sense key. MISCOMPARE MISCOMPARE 0x%x expected but got key 0x%x. Sense:%s\n", SCSI_SENSE_MISCOMPARE, task->sense.key, iscsi_get_error(iscsi)); 
		scsi_free_scsi_task(task);
		return -1;
	}

	printf("[OK]\n");
	scsi_free_scsi_task(task);
	return 0;
}

int verify10_lbaoutofrange(struct iscsi_context *iscsi, int lun, unsigned char *data, uint32_t datalen, uint32_t lba, int vprotect, int dpo, int bytchk, int blocksize)
{
	struct scsi_task *task;

	printf("Send VERIFY10 LBA:%d blocks:%d vprotect:%d dpo:%d bytchk:%d (expecting LBA_OUT_OF_RANGE) ... ", lba, datalen / blocksize, vprotect, dpo, bytchk);
	task = iscsi_verify10_sync(iscsi, lun, data, datalen, lba, vprotect, dpo, bytchk, blocksize);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send VERIFY10 command: %s\n", iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status        == SCSI_STATUS_CHECK_CONDITION
	    && task->sense.key  == SCSI_SENSE_ILLEGAL_REQUEST
	    && task->sense.ascq == SCSI_SENSE_ASCQ_INVALID_OPERATION_CODE) {
		printf("[SKIPPED]\n");
		printf("VERIFY10 is not implemented on target\n");
		scsi_free_scsi_task(task);
		return -2;
	}
	if (task->status == SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("VERIFY10 command successful but should have failed with LBA_OUT_OF_RANGE\n");
		scsi_free_scsi_task(task);
		return -1;
	}
	if (task->status        != SCSI_STATUS_CHECK_CONDITION
		|| task->sense.key  != SCSI_SENSE_ILLEGAL_REQUEST
		|| task->sense.ascq != SCSI_SENSE_ASCQ_LBA_OUT_OF_RANGE) {
		printf("[FAILED]\n");
		printf("VERIFY10 should have failed with ILLEGAL_REQUEST/LBA_OUT_OF_RANGE. Sense:%s\n", iscsi_get_error(iscsi));
		scsi_free_scsi_task(task);
		return -1;
	}

	printf("[OK]\n");
	scsi_free_scsi_task(task);
	return 0;
}

int verify12(struct iscsi_context *iscsi, int lun, unsigned char *data, uint32_t datalen, uint32_t lba, int vprotect, int dpo, int bytchk, int blocksize)
{
	struct scsi_task *task;

	printf("Send VERIFY12 LBA:%d blocks:%d vprotect:%d dpo:%d bytchk:%d ... ", lba, datalen / blocksize, vprotect, dpo, bytchk);
	task = iscsi_verify12_sync(iscsi, lun, data, datalen, lba, vprotect, dpo, bytchk, blocksize);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send VERIFY12 command: %s\n", iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status        == SCSI_STATUS_CHECK_CONDITION
	    && task->sense.key  == SCSI_SENSE_ILLEGAL_REQUEST
	    && task->sense.ascq == SCSI_SENSE_ASCQ_INVALID_OPERATION_CODE) {
		printf("[SKIPPED]\n");
		printf("VERIFY12 is not implemented on target\n");
		scsi_free_scsi_task(task);
		return -2;
	}
	if (task->status != SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("VERIFY12 command: failed with sense. %s\n", iscsi_get_error(iscsi));
		scsi_free_scsi_task(task);
		return -1;
	}

	printf("[OK]\n");
	scsi_free_scsi_task(task);
	return 0;
}

int verify12_nomedium(struct iscsi_context *iscsi, int lun, unsigned char *data, uint32_t datalen, uint32_t lba, int vprotect, int dpo, int bytchk, int blocksize)
{
	struct scsi_task *task;

	printf("Send VERIFY12 LBA:%d blocks:%d vprotect:%d dpo:%d bytchk:%d (expecting NOT_READY/MEDIUM_NOT_PRESENT) ... ", lba, datalen / blocksize, vprotect, dpo, bytchk);
	task = iscsi_verify12_sync(iscsi, lun, data, datalen, lba, vprotect, dpo, bytchk, blocksize);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send VERIFY12 command: %s\n", iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status        == SCSI_STATUS_CHECK_CONDITION
	    && task->sense.key  == SCSI_SENSE_ILLEGAL_REQUEST
	    && task->sense.ascq == SCSI_SENSE_ASCQ_INVALID_OPERATION_CODE) {
		printf("[SKIPPED]\n");
		printf("VERIFY12 is not implemented on target\n");
		scsi_free_scsi_task(task);
		return -2;
	}
	if (task->status        != SCSI_STATUS_CHECK_CONDITION
	    || task->sense.key  != SCSI_SENSE_NOT_READY
	    || (task->sense.ascq != SCSI_SENSE_ASCQ_MEDIUM_NOT_PRESENT
	        && task->sense.ascq != SCSI_SENSE_ASCQ_MEDIUM_NOT_PRESENT_TRAY_OPEN
	        && task->sense.ascq != SCSI_SENSE_ASCQ_MEDIUM_NOT_PRESENT_TRAY_CLOSED)) {
		printf("[FAILED]\n");
		printf("VERIFY12 after eject failed with the wrong sense code. Should fail with NOT_READY/MEDIUM_NOT_PRESENT*\n");
		scsi_free_scsi_task(task);
		return -1;
	}	

	printf("[OK]\n");
	scsi_free_scsi_task(task);
	return 0;
}

int verify12_miscompare(struct iscsi_context *iscsi, int lun, unsigned char *data, uint32_t datalen, uint32_t lba, int vprotect, int dpo, int bytchk, int blocksize)
{
	struct scsi_task *task;

	printf("Send VERIFY12 LBA:%d blocks:%d vprotect:%d dpo:%d bytchk:%d (expecting MISCOMPARE) ... ", lba, datalen / blocksize, vprotect, dpo, bytchk);
	task = iscsi_verify12_sync(iscsi, lun, data, datalen, lba, vprotect, dpo, bytchk, blocksize);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send VERIFY12 command: %s\n", iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status        == SCSI_STATUS_CHECK_CONDITION
	    && task->sense.key  == SCSI_SENSE_ILLEGAL_REQUEST
	    && task->sense.ascq == SCSI_SENSE_ASCQ_INVALID_OPERATION_CODE) {
		printf("[SKIPPED]\n");
		printf("VERIFY12 is not implemented on target\n");
		scsi_free_scsi_task(task);
		return -2;
	}
	if (task->status == SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("VERIFY12 command successful but should have failed with MISCOMPARE\n");
		scsi_free_scsi_task(task);
		return -1;
	}
	if (task->sense.key != SCSI_SENSE_MISCOMPARE) {
		printf("[FAILED]\n");
		printf("VERIFY12 command returned wrong sense key. MISCOMPARE MISCOMPARE 0x%x expected but got key 0x%x. Sense:%s\n", SCSI_SENSE_MISCOMPARE, task->sense.key, iscsi_get_error(iscsi)); 
		scsi_free_scsi_task(task);
		return -1;
	}

	printf("[OK]\n");
	scsi_free_scsi_task(task);
	return 0;
}

int verify12_lbaoutofrange(struct iscsi_context *iscsi, int lun, unsigned char *data, uint32_t datalen, uint32_t lba, int vprotect, int dpo, int bytchk, int blocksize)
{
	struct scsi_task *task;

	printf("Send VERIFY12 LBA:%d blocks:%d vprotect:%d dpo:%d bytchk:%d (expecting LBA_OUT_OF_RANGE) ... ", lba, datalen / blocksize, vprotect, dpo, bytchk);
	task = iscsi_verify12_sync(iscsi, lun, data, datalen, lba, vprotect, dpo, bytchk, blocksize);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send VERIFY12 command: %s\n", iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status        == SCSI_STATUS_CHECK_CONDITION
	    && task->sense.key  == SCSI_SENSE_ILLEGAL_REQUEST
	    && task->sense.ascq == SCSI_SENSE_ASCQ_INVALID_OPERATION_CODE) {
		printf("[SKIPPED]\n");
		printf("VERIFY12 is not implemented on target\n");
		scsi_free_scsi_task(task);
		return -2;
	}
	if (task->status == SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("VERIFY12 command successful but should have failed with LBA_OUT_OF_RANGE\n");
		scsi_free_scsi_task(task);
		return -1;
	}
	if (task->status        != SCSI_STATUS_CHECK_CONDITION
		|| task->sense.key  != SCSI_SENSE_ILLEGAL_REQUEST
		|| task->sense.ascq != SCSI_SENSE_ASCQ_LBA_OUT_OF_RANGE) {
		printf("[FAILED]\n");
		printf("VERIFY12 should have failed with ILLEGAL_REQUEST/LBA_OUT_OF_RANGE. Sense:%s\n", iscsi_get_error(iscsi));
		scsi_free_scsi_task(task);
		return -1;
	}

	printf("[OK]\n");
	scsi_free_scsi_task(task);
	return 0;
}

int verify16(struct iscsi_context *iscsi, int lun, unsigned char *data, uint32_t datalen, uint64_t lba, int vprotect, int dpo, int bytchk, int blocksize)
{
	struct scsi_task *task;

	printf("Send VERIFY16 LBA:%" PRIu64 " blocks:%d vprotect:%d dpo:%d bytchk:%d ... ", lba, datalen / blocksize, vprotect, dpo, bytchk);
	task = iscsi_verify16_sync(iscsi, lun, data, datalen, lba, vprotect, dpo, bytchk, blocksize);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send VERIFY16 command: %s\n", iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status        == SCSI_STATUS_CHECK_CONDITION
	    && task->sense.key  == SCSI_SENSE_ILLEGAL_REQUEST
	    && task->sense.ascq == SCSI_SENSE_ASCQ_INVALID_OPERATION_CODE) {
		printf("[SKIPPED]\n");
		printf("VERIFY16 is not implemented on target\n");
		scsi_free_scsi_task(task);
		return -2;
	}
	if (task->status != SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("VERIFY16 command: failed with sense. %s\n", iscsi_get_error(iscsi));
		scsi_free_scsi_task(task);
		return -1;
	}

	printf("[OK]\n");
	scsi_free_scsi_task(task);
	return 0;
}

int verify16_nomedium(struct iscsi_context *iscsi, int lun, unsigned char *data, uint32_t datalen, uint64_t lba, int vprotect, int dpo, int bytchk, int blocksize)
{
	struct scsi_task *task;

	printf("Send VERIFY16 LBA:%" PRIu64 " blocks:%d vprotect:%d dpo:%d bytchk:%d (expecting NOT_READY/MEDIUM_NOT_PRESENT) ... ", lba, datalen / blocksize, vprotect, dpo, bytchk);
	task = iscsi_verify16_sync(iscsi, lun, data, datalen, lba, vprotect, dpo, bytchk, blocksize);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send VERIFY16 command: %s\n", iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status        == SCSI_STATUS_CHECK_CONDITION
	    && task->sense.key  == SCSI_SENSE_ILLEGAL_REQUEST
	    && task->sense.ascq == SCSI_SENSE_ASCQ_INVALID_OPERATION_CODE) {
		printf("[SKIPPED]\n");
		printf("VERIFY16 is not implemented on target\n");
		scsi_free_scsi_task(task);
		return -2;
	}
	if (task->status        != SCSI_STATUS_CHECK_CONDITION
	    || task->sense.key  != SCSI_SENSE_NOT_READY
	    || (task->sense.ascq != SCSI_SENSE_ASCQ_MEDIUM_NOT_PRESENT
	        && task->sense.ascq != SCSI_SENSE_ASCQ_MEDIUM_NOT_PRESENT_TRAY_OPEN
	        && task->sense.ascq != SCSI_SENSE_ASCQ_MEDIUM_NOT_PRESENT_TRAY_CLOSED)) {
		printf("[FAILED]\n");
		printf("VERIFY16 after eject failed with the wrong sense code. Should fail with NOT_READY/MEDIUM_NOT_PRESENT*\n");
		scsi_free_scsi_task(task);
		return -1;
	}	

	printf("[OK]\n");
	scsi_free_scsi_task(task);
	return 0;
}

int verify16_miscompare(struct iscsi_context *iscsi, int lun, unsigned char *data, uint32_t datalen, uint64_t lba, int vprotect, int dpo, int bytchk, int blocksize)
{
	struct scsi_task *task;

	printf("Send VERIFY16 LBA:%" PRIu64 " blocks:%d vprotect:%d dpo:%d bytchk:%d (expecting MISCOMPARE) ... ", lba, datalen / blocksize, vprotect, dpo, bytchk);
	task = iscsi_verify16_sync(iscsi, lun, data, datalen, lba, vprotect, dpo, bytchk, blocksize);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send VERIFY16 command: %s\n", iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status        == SCSI_STATUS_CHECK_CONDITION
	    && task->sense.key  == SCSI_SENSE_ILLEGAL_REQUEST
	    && task->sense.ascq == SCSI_SENSE_ASCQ_INVALID_OPERATION_CODE) {
		printf("[SKIPPED]\n");
		printf("VERIFY16 is not implemented on target\n");
		scsi_free_scsi_task(task);
		return -2;
	}
	if (task->status == SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("VERIFY16 command successful but should have failed with MISCOMPARE\n");
		scsi_free_scsi_task(task);
		return -1;
	}
	if (task->sense.key != SCSI_SENSE_MISCOMPARE) {
		printf("[FAILED]\n");
		printf("VERIFY16 command returned wrong sense key. MISCOMPARE MISCOMPARE 0x%x expected but got key 0x%x. Sense:%s\n", SCSI_SENSE_MISCOMPARE, task->sense.key, iscsi_get_error(iscsi)); 
		scsi_free_scsi_task(task);
		return -1;
	}

	printf("[OK]\n");
	scsi_free_scsi_task(task);
	return 0;
}

int verify16_lbaoutofrange(struct iscsi_context *iscsi, int lun, unsigned char *data, uint32_t datalen, uint64_t lba, int vprotect, int dpo, int bytchk, int blocksize)
{
	struct scsi_task *task;

	printf("Send VERIFY16 LBA:%" PRIu64 " blocks:%d vprotect:%d dpo:%d bytchk:%d (expecting LBA_OUT_OF_RANGE) ... ", lba, datalen / blocksize, vprotect, dpo, bytchk);
	task = iscsi_verify16_sync(iscsi, lun, data, datalen, lba, vprotect, dpo, bytchk, blocksize);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send VERIFY16 command: %s\n", iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status        == SCSI_STATUS_CHECK_CONDITION
	    && task->sense.key  == SCSI_SENSE_ILLEGAL_REQUEST
	    && task->sense.ascq == SCSI_SENSE_ASCQ_INVALID_OPERATION_CODE) {
		printf("[SKIPPED]\n");
		printf("VERIFY16 is not implemented on target\n");
		scsi_free_scsi_task(task);
		return -2;
	}
	if (task->status == SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("VERIFY16 command successful but should have failed with LBA_OUT_OF_RANGE\n");
		scsi_free_scsi_task(task);
		return -1;
	}
	if (task->status        != SCSI_STATUS_CHECK_CONDITION
		|| task->sense.key  != SCSI_SENSE_ILLEGAL_REQUEST
		|| task->sense.ascq != SCSI_SENSE_ASCQ_LBA_OUT_OF_RANGE) {
		printf("[FAILED]\n");
		printf("VERIFY16 should have failed with ILLEGAL_REQUEST/LBA_OUT_OF_RANGE. Sense:%s\n", iscsi_get_error(iscsi));
		scsi_free_scsi_task(task);
		return -1;
	}

	printf("[OK]\n");
	scsi_free_scsi_task(task);
	return 0;
}

int inquiry(struct iscsi_context *iscsi, int lun, int evpd, int page_code, int maxsize)
{
	struct scsi_task *task;

	printf("Send INQUIRY evpd:%d page_code:%d ... ", evpd, page_code);
	task = iscsi_inquiry_sync(iscsi, lun, evpd, page_code, maxsize);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send INQUIRY command: %s\n", iscsi_get_error(iscsi));
		return -1;
	}
	if (task->status != SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("INQUIRY command: failed with sense. %s\n", iscsi_get_error(iscsi));
		scsi_free_scsi_task(task);
		return -1;
	}

	printf("[OK]\n");
	scsi_free_scsi_task(task);
	return 0;
}


int main(int argc, const char *argv[])
{
	poptContext pc;
	const char **extra_argv;
	int extra_argc = 0;
	const char *url = NULL;
	int show_help = 0, show_usage = 0, list_tests = 0;
	int res, num_failed, num_skipped;
	struct scsi_test *test;
	char *testname = NULL;
	char *skipname = NULL;
	int lun;
	struct iscsi_context *iscsi;
	struct scsi_task *task;
	struct scsi_readcapacity10 *rc10;
	struct scsi_readcapacity16 *rc16;
	struct scsi_inquiry_standard *inq;
	int full_size;

	struct poptOption popt_options[] = {
		{ "help", '?', POPT_ARG_NONE, &show_help, 0, "Show this help message", NULL },
		{ "usage", 0, POPT_ARG_NONE, &show_usage, 0, "Display brief usage message", NULL },
		{ "list", 'l', POPT_ARG_NONE, &list_tests, 0, "List all tests", NULL },
		{ "initiator-name", 'i', POPT_ARG_STRING, &initiatorname1, 0, "Initiatorname to use", "iqn-name" },
		{ "initiator-name-2", 'I', POPT_ARG_STRING, &initiatorname2, 0, "Second initiatorname to use for tests using more than one session", "iqn-name" },
		{ "test", 't', POPT_ARG_STRING, &testname, 0, "Which test to run", "testname" },
		{ "skip", 's', POPT_ARG_STRING, &skipname, 0, "Which test to skip", "skipname" },
		{ "info", 0, POPT_ARG_NONE, &show_info, 0, "Show information about the test", "testname" },
		{ "dataloss", 0, POPT_ARG_NONE, &data_loss, 0, "Allow destructuve tests", NULL },
		POPT_TABLEEND
	};

	real_iscsi_queue_pdu = dlsym(RTLD_NEXT, "iscsi_queue_pdu");

	pc = poptGetContext(argv[0], argc, argv, popt_options, POPT_CONTEXT_POSIXMEHARDER);
	if ((res = poptGetNextOpt(pc)) < -1) {
		fprintf(stderr, "Failed to parse option : %s %s\n",
			poptBadOption(pc, 0), poptStrerror(res));
		exit(10);
	}
	extra_argv = poptGetArgs(pc);
	if (extra_argv) {
		url = strdup(*extra_argv);
		extra_argv++;
		while (extra_argv[extra_argc]) {
			extra_argc++;
		}
	}

	if (show_help != 0) {
		print_help();
		exit(0);
	}

	if (show_usage != 0) {
		print_usage();
		exit(0);
	}

	if (list_tests != 0) {
		for (test = &tests[0]; test->name; test++) {
			printf("%s\n", test->name);
			if (show_info) {
				test->test(initiatorname1, url);
			}
		}
		exit(0);
	}
	poptFreeContext(pc);

	if (url == NULL) {
		fprintf(stderr, "You must specify the URL\n");
		print_usage();
		free(skipname);
		free(testname);
		exit(10);
	}


	iscsi = iscsi_context_login(initiatorname1, url, &lun);
	if (iscsi == NULL) {
		printf("Failed to login to target\n");
		return -1;
	}

	/* find the size of the LUN
	   All devices support readcapacity10 but only some support
	   readcapacity16
	*/
	task = iscsi_readcapacity10_sync(iscsi, lun, 0, 0);
	if (task == NULL) {
		printf("Failed to send READCAPACITY10 command: %s\n", iscsi_get_error(iscsi));
		iscsi_destroy_context(iscsi);
		return -1;
	}
	if (task->status != SCSI_STATUS_GOOD) {
		printf("READCAPACITY10 command: failed with sense. %s\n", iscsi_get_error(iscsi));
		scsi_free_scsi_task(task);
		iscsi_destroy_context(iscsi);
		return -1;
	}
	rc10 = scsi_datain_unmarshall(task);
	if (rc10 == NULL) {
		printf("failed to unmarshall READCAPACITY10 data. %s\n", iscsi_get_error(iscsi));
		scsi_free_scsi_task(task);
		iscsi_destroy_context(iscsi);
		return -1;
	}
	block_size = rc10->block_size;
	num_blocks = rc10->lba;
	scsi_free_scsi_task(task);

	task = iscsi_readcapacity16_sync(iscsi, lun);
	if (task == NULL) {
		printf("Failed to send READCAPACITY16 command: %s\n", iscsi_get_error(iscsi));
		iscsi_destroy_context(iscsi);
		return -1;
	}
	if (task->status == SCSI_STATUS_GOOD) {
		rc16 = scsi_datain_unmarshall(task);
		if (rc16 == NULL) {
			printf("failed to unmarshall READCAPACITY16 data. %s\n", iscsi_get_error(iscsi));
			scsi_free_scsi_task(task);
			iscsi_destroy_context(iscsi);
			return -1;
		}
		block_size = rc16->block_length;
		num_blocks = rc16->returned_lba;
		lbpme      = rc16->lbpme;
		lbppb      = 1 << rc16->lbppbe;
		lbpme      = rc16->lbpme;

		scsi_free_scsi_task(task);
	}


	task = iscsi_inquiry_sync(iscsi, lun, 0, 0, 64);
	if (task == NULL || task->status != SCSI_STATUS_GOOD) {
		printf("Inquiry command failed : %s\n", iscsi_get_error(iscsi));
		return -1;
	}
	full_size = scsi_datain_getfullsize(task);
	if (full_size > task->datain.size) {
		scsi_free_scsi_task(task);

		/* we need more data for the full list */
		if ((task = iscsi_inquiry_sync(iscsi, lun, 0, 0, full_size)) == NULL) {
			printf("Inquiry command failed : %s\n", iscsi_get_error(iscsi));
			return -1;
		}
	}
	inq = scsi_datain_unmarshall(task);
	if (inq == NULL) {
		printf("failed to unmarshall inquiry datain blob\n");
		scsi_free_scsi_task(task);
		return -1;
	}
	removable   = inq->rmb;
	device_type = inq->device_type;
	sccs        = inq->sccs;
	encserv     = inq->encserv;
	scsi_free_scsi_task(task);


	iscsi_logout_sync(iscsi);
	iscsi_destroy_context(iscsi);


	num_failed = num_skipped = 0;
	for (test = &tests[0]; test->name; test++) {
		if (testname != NULL && fnmatch(testname, test->name, 0)) {
			continue;
		}

		if (skipname != NULL) {
			char * pchr = skipname;
			char * pchr2 = NULL;
			int skip = 0;
			do {
				pchr2 = strchr(pchr,',');
				if (pchr2) pchr2[0]=0x00;
				if (!fnmatch(pchr, test->name, 0)) {
					skip = 1;
				}
				if (pchr2) {pchr2[0]=',';pchr=pchr2+1;}
			} while (pchr2);
			if (skip) continue;
		}

		res = test->test(initiatorname1, url);
		if (res == 0) {
			printf("TEST %s [OK]\n", test->name);
		} else if (res == -2) {
			printf("TEST %s [SKIPPED]\n", test->name);
			num_skipped++;
		} else {
			printf("TEST %s [FAILED]\n", test->name);
			num_failed++;
		}
		printf("\n");
	}

	free(skipname);
	free(testname);
	free(discard_const(url));

	return num_failed ? num_failed : num_skipped ? 77 : 0;
}
