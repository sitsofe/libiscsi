/* 
   Copyright (C) 2012 by Ronnie Sahlberg <ronniesahlberg@gmail.com>
   
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

#include <stdio.h>
#include <string.h>
#include "iscsi.h"
#include "scsi-lowlevel.h"
#include "iscsi-test.h"

int T0360_startstopunit_simple(const char *initiator, const char *url, int data_loss, int show_info)
{ 
	struct iscsi_context *iscsi;
	struct scsi_task *task;
	struct scsi_inquiry_standard *inq;
	int ret, lun, removable;
	int full_size;

	printf("0360_startstopunit_simple:\n");
	printf("===================\n");
	if (show_info) {
		printf("Test basic STARTSTOPUNIT functionality.\n");
		printf("1, Verify we can eject removable the media with IMMED==1\n");
		printf("2, Verify we can load the media back again with IMMED==1\n");
		printf("3, Verify we can eject removable the media with IMMED==0\n");
		printf("4, Verify we can load the media back again with IMMED==0\n");
		printf("\n");
		return 0;
	}

	iscsi = iscsi_context_login(initiator, url, &lun);
	if (iscsi == NULL) {
		printf("Failed to login to target\n");
		return -1;
	}

	/* See how big this inquiry data is */
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
	removable = inq->rmb;

	scsi_free_scsi_task(task);

	if (!data_loss) {
		printf("--dataloss flag is not set. Skipping test\n");
		ret = -2;
		goto finished;
	}
	

	ret = 0;


	if (removable) {
		printf("Media is removable. STARTSTOPUNIT should work\n");
	} else {
		printf("Media is not removable. STARTSTOPUNIT should fail\n");
	}


	printf("STARTSTOPUNIT try to eject the media with IMMED==1 ... ");
	task = iscsi_startstopunit_sync(iscsi, lun, 1, 0, 0, 0, 1, 0);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send STARTSTOPUNIT command: %s\n", iscsi_get_error(iscsi));
		ret = -1;
		goto finished;
	}
	if (task->status != SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("STARTSTOPUNIT command: failed with sense. %s\n", iscsi_get_error(iscsi));
		ret = -1;
		scsi_free_scsi_task(task);
		goto finished;
	}
	scsi_free_scsi_task(task);
	printf("[OK]\n");

	if (removable) {
		printf("Medium is removable. Check with TESTUNITREADY that was removed.\n");
		ret = testunitready_nomedium(iscsi, lun);
		if (ret != 0) {
			goto finished;
		}
	} else {
		printf("Medium is not removable. Check with TESTUNITREADY that medium is still present.\n");
		ret = testunitready(iscsi, lun);
		if (ret != 0) {
			goto finished;
		}
	}


	printf("STARTSTOPUNIT try to mount the media again with IMMED==1 ... ");
	task = iscsi_startstopunit_sync(iscsi, lun, 1, 0, 0, 0, 1, 1);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send STARTSTOPUNIT command: %s\n", iscsi_get_error(iscsi));
		ret = -1;
		goto finished;
	}
	if (task->status != SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("STARTSTOPUNIT command: failed with sense. %s\n", iscsi_get_error(iscsi));
		ret = -1;
		scsi_free_scsi_task(task);
		goto finished;
	}
	scsi_free_scsi_task(task);
	printf("[OK]\n");

	printf("Check with TESTUNITREADY that the medium is present.\n");
	ret = testunitready(iscsi, lun);
	if (ret != 0) {
		goto finished;
	}


	printf("STARTSTOPUNIT try to eject the media with IMMED==0 ... ");
	task = iscsi_startstopunit_sync(iscsi, lun, 0, 0, 0, 0, 1, 0);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send STARTSTOPUNIT command: %s\n", iscsi_get_error(iscsi));
		ret = -1;
		goto finished;
	}
	if (task->status != SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("STARTSTOPUNIT command: failed with sense. %s\n", iscsi_get_error(iscsi));
		ret = -1;
		scsi_free_scsi_task(task);
		goto finished;
	}
	scsi_free_scsi_task(task);
	printf("[OK]\n");

	if (removable) {
		printf("Medium is removable. Check with TESTUNITREADY that was removed.\n");
		ret = testunitready_nomedium(iscsi, lun);
		if (ret != 0) {
			goto finished;
		}
	} else {
		printf("Medium is not removable. Check with TESTUNITREADY that medium is still present.\n");
		ret = testunitready(iscsi, lun);
		if (ret != 0) {
			goto finished;
		}
	}


	printf("STARTSTOPUNIT try to mount the media again with IMMED==0 ... ");
	task = iscsi_startstopunit_sync(iscsi, lun, 0, 0, 0, 0, 1, 1);
	if (task == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send STARTSTOPUNIT command: %s\n", iscsi_get_error(iscsi));
		ret = -1;
		goto finished;
	}
	if (task->status != SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("STARTSTOPUNIT command: failed with sense. %s\n", iscsi_get_error(iscsi));
		ret = -1;
		scsi_free_scsi_task(task);
		goto finished;
	}
	scsi_free_scsi_task(task);
	printf("[OK]\n");


	printf("Check with TESTUNITREADY that the medium is present.\n");
	ret = testunitready(iscsi, lun);
	if (ret != 0) {
		goto finished;
	}


finished:
	iscsi_logout_sync(iscsi);
	iscsi_destroy_context(iscsi);
	return ret;
}
