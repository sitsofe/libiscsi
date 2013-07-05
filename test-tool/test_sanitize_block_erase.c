/* 
   Copyright (C) 2013 Ronnie Sahlberg <ronniesahlberg@gmail.com>
   
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
#include <stdlib.h>
#include <string.h>
#include <alloca.h>

#include <CUnit/CUnit.h>

#include "iscsi.h"
#include "scsi-lowlevel.h"
#include "iscsi-test-cu.h"

static void
check_wabereq(void)
{
	struct scsi_task *task_ret;

	logging(LOG_VERBOSE, "Read one block from LBA 0");
	task_ret = malloc(sizeof(struct scsi_task));
	CU_ASSERT_PTR_NOT_NULL(task_ret);
	memset(task_ret, 0, sizeof(struct scsi_task));
	task_ret->cdb[0] = SCSI_OPCODE_READ10;
	task_ret->cdb[8] = 1;
	task_ret->cdb_size = 10;
	task_ret->xfer_dir = SCSI_XFER_READ;
	task_ret->expxferlen = 0;
	task_ret = iscsi_scsi_command_sync(iscsic, tgt_lun, task_ret, NULL);
	CU_ASSERT_PTR_NOT_NULL(task_ret);
	CU_ASSERT_NOT_EQUAL(task_ret->status, SCSI_STATUS_CANCELLED);

	switch (inq_bdc->wabereq) {
	case 0:
		logging(LOG_NORMAL, "[FAILED] SANITIZE BLOCK ERASE "
			"opcode is supported but WABEREQ is 0");
		CU_FAIL("[FAILED] SANITIZE BLOCK ERASE "
			"opcode is supported but WABEREQ is 0");
		break;
	case 1:
		logging(LOG_VERBOSE, "WABEREQ==1. Reads from the "
			"device should be successful.");
		if (task_ret->status == SCSI_STATUS_GOOD) {
			logging(LOG_VERBOSE, "[SUCCESS] Read was "
				"successful after SANITIZE");
			break;
		}
		logging(LOG_NORMAL, "[FAILED] Read after "
			"SANITIZE failed but WABEREQ is 1");
		CU_FAIL("[FAILED] Read after SANITIZE failed "
			"but WABEREQ is 1");
		break;
	case 2:
		logging(LOG_VERBOSE, "WABEREQ==2. Reads from the "
			"device should fail.");
		if (task_ret->status        == SCSI_STATUS_CHECK_CONDITION
		    && task_ret->sense.key  == SCSI_SENSE_MEDIUM_ERROR
		    && task_ret->sense.ascq != SCSI_SENSE_ASCQ_WRITE_AFTER_SANITIZE_REQUIRED) {
			logging(LOG_VERBOSE, "[SUCCESS] Read failed "
				"with CHECK_CONDITION/MEDIUM_ERROR/"
				"!WRITE_AFTER_SANITIZE_REQUIRED");
			break;
		}
		logging(LOG_VERBOSE, "[FAILED] Read should have failed "
			"with CHECK_CONDITION/MEDIUM_ERROR/"
			"!WRITE_AFTER_SANITIZE_REQUIRED");
		CU_FAIL("[FAILED] Read should have failed "
			"with CHECK_CONDITION/MEDIUM_ERROR/"
			"!WRITE_AFTER_SANITIZE_REQUIRED");
		break;
	case 3:
		logging(LOG_VERBOSE, "WABEREQ==3. Reads from the "
			"device should fail.");
		if (task_ret->status        == SCSI_STATUS_CHECK_CONDITION
		    && task_ret->sense.key  == SCSI_SENSE_MEDIUM_ERROR
		    && task_ret->sense.ascq == SCSI_SENSE_ASCQ_WRITE_AFTER_SANITIZE_REQUIRED) {
			logging(LOG_VERBOSE, "[SUCCESS] Read failed "
				"with CHECK_CONDITION/MEDIUM_ERROR/"
				"WRITE_AFTER_SANITIZE_REQUIRED");
			break;
		}
		logging(LOG_VERBOSE, "[FAILED] Read should have failed "
			"with CHECK_CONDITION/MEDIUM_ERROR/"
			"WRITE_AFTER_SANITIZE_REQUIRED");
		CU_FAIL("[FAILED] Read should have failed "
			"with CHECK_CONDITION/MEDIUM_ERROR/"
			"WRITE_AFTER_SANITIZE_REQUIRED");
		break;
	}

	scsi_free_scsi_task(task_ret);
}

void
test_sanitize_block_erase(void)
{ 
	int ret;
	struct iscsi_data data;
	struct scsi_command_descriptor *cd;

	logging(LOG_VERBOSE, LOG_BLANK_LINE);
	logging(LOG_VERBOSE, "Test SANITIZE BLOCK ERASE");

	CHECK_FOR_SANITIZE;

	logging(LOG_NORMAL, "Check that SANITIZE BLOCK_ERASE is supported "
		"in REPORT_SUPPORTED_OPCODES");
	cd = get_command_descriptor(SCSI_OPCODE_SANITIZE,
				    SCSI_SANITIZE_BLOCK_ERASE);
	if (cd == NULL) {
		logging(LOG_VERBOSE, "Opcode is not supported. Verify that "
			"WABEREQ is zero.");
		if (inq_bdc && inq_bdc->wabereq) {
			logging(LOG_NORMAL, "[FAILED] WABEREQ is not 0 but "
				"SANITIZE BLOCK ERASE opcode is not supported");
			CU_FAIL("[FAILED] WABEREQ is not 0 but BLOCK ERASE "
				"is not supported.");
		}
	}
	if (cd == NULL) {
		logging(LOG_NORMAL, "[SKIPPED] SANITIZE BLOCK_ERASE is not "
			"implemented according to REPORT_SUPPORTED_OPCODES.");
		CU_PASS("SANITIZE is not implemented.");
		return;
	}

	logging(LOG_VERBOSE, "Verify that we have BlockDeviceCharacteristics "
		"VPD page.");
	if (inq_bdc == NULL) {
		logging(LOG_NORMAL, "[FAILED] SANITIZE BLOCK ERASE opcode is "
			"supported but BlockDeviceCharacteristics VPD page is "
			"missing");
		CU_FAIL("[FAILED] BlockDeviceCharacteristics VPD "
			"page is missing");
	}

	logging(LOG_VERBOSE, "Check MediumRotationRate whether this is a HDD "
		"or a SSD device.");
	if (inq_bdc && inq_bdc->medium_rotation_rate != 0) {
		logging(LOG_NORMAL, "This is a HDD device");
		logging(LOG_NORMAL, "[WARNING] SANITIZE BLOCK ERASE opcode is "
			"supported but MediumRotationRate is not 0 "
			"indicating that this is a HDD. Only SSDs should "
			"implement BLOCK ERASE");
	} else {
		logging(LOG_NORMAL, "This is a HDD device");
	}

	logging(LOG_VERBOSE, "Test we can perform basic BLOCK ERASE SANITIZE");

	ret = sanitize(iscsic, tgt_lun,
		       0, 0, SCSI_SANITIZE_BLOCK_ERASE, 0, NULL);
	CU_ASSERT_EQUAL(ret, 0);

	data.size = 8;
	data.data = alloca(data.size);
	memset(data.data, 0, data.size);

	logging(LOG_VERBOSE, "BLOCK_ERASE parameter list length must be 0");
	logging(LOG_VERBOSE, "Test that non-zero param length is an error for "
		"BLOCK ERASE");
	ret = sanitize_invalidfieldincdb(iscsic, tgt_lun,
		       0, 0, SCSI_SANITIZE_BLOCK_ERASE, 8, &data);
	CU_ASSERT_EQUAL(ret, 0);

	if (inq_bdc) {
		logging(LOG_VERBOSE, "Check WABEREQ setting and that READ "
			"after SANITIZE works correctly.");
		check_wabereq();
	}
}
