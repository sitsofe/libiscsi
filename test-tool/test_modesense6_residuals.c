/* 
   Copyright (C) 2013 by Ronnie Sahlberg <ronniesahlberg@gmail.com>
   
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

#include <CUnit/CUnit.h>

#include "iscsi.h"
#include "scsi-lowlevel.h"
#include "iscsi-support.h"
#include "iscsi-test-cu.h"

void
test_modesense6_residuals(void)
{
	logging(LOG_VERBOSE, LOG_BLANK_LINE);
	logging(LOG_VERBOSE, "Test of MODESENSE6 Residuals");

	logging(LOG_VERBOSE, "MODESENSE6 command should not result in any "
		"residuals");

	
	logging(LOG_VERBOSE, "Try a MODESENSE6 command with 4 bytes of "
		"transfer length and verify that we dont get residuals.");
	if (task != NULL) {
		scsi_free_scsi_task(task);
		task = NULL;
	}
	task = iscsi_modesense6_sync(iscsic, tgt_lun, 0,
				     SCSI_MODESENSE_PC_CURRENT,
				     SCSI_MODESENSE_PAGECODE_RETURN_ALL_PAGES,
				     0, 4);
	if (task == NULL || task->status != SCSI_STATUS_GOOD) {
		logging(LOG_VERBOSE, "[FAILED] Failed to send MODE_SENSE6 "
			"command:%s",
			iscsi_get_error(iscsic));
		CU_FAIL("[FAILED] Failed to fetch the All Pages page.");
		return;
	}
	logging(LOG_VERBOSE, "[SUCCESS] All Pages fetched.");


	logging(LOG_VERBOSE, "Verify that we got at most 4 bytes of DATA-IN");
	if (task->datain.size > 4) {
		logging(LOG_NORMAL, "[FAILED] got more than 4 bytes of "
			"DATA-IN.");
	} else {
		logging(LOG_VERBOSE, "[SUCCESS] <= 4 bytes of DATA-IN "
			"received.");
	}
	CU_ASSERT_TRUE(task->datain.size <= 4);


	logging(LOG_VERBOSE, "Verify residual overflow flag not set");
	if (task->residual_status == SCSI_RESIDUAL_OVERFLOW) {
		logging(LOG_VERBOSE, "[FAILED] Target set residual "
			"overflow flag");
	}
	CU_ASSERT_NOT_EQUAL(task->residual_status, SCSI_RESIDUAL_OVERFLOW);



	logging(LOG_VERBOSE, "Try a MODESENSE6 command with 255 bytes of "
		"transfer length and verify that we get residuals if the target returns less than the requested amount of data.");
	if (task != NULL) {
		scsi_free_scsi_task(task);
		task = NULL;
	}
	task = iscsi_modesense6_sync(iscsic, tgt_lun, 0,
				     SCSI_MODESENSE_PC_CURRENT,
				     SCSI_MODESENSE_PAGECODE_RETURN_ALL_PAGES,
				     0, 255);
	if (task == NULL || task->status != SCSI_STATUS_GOOD) {
		logging(LOG_VERBOSE, "[FAILED] Failed to send MODE_SENSE6 "
			"command:%s",
			iscsi_get_error(iscsic));
		CU_FAIL("[FAILED] Failed to fetch the All Pages page.");
		return;
	}
	logging(LOG_VERBOSE, "[SUCCESS] All Pages fetched.");


	if (task->datain.size == 255) {
		logging(LOG_VERBOSE, "We got all 255 bytes of data back "
			"from the target. Verify that underflow is not set.");

		if (task->residual_status == SCSI_RESIDUAL_UNDERFLOW) {
			logging(LOG_VERBOSE, "[FAILED] Target set residual "
				"underflow flag");
		} else {
			logging(LOG_VERBOSE, "[SUCCESS] Residual underflow "
				"is not set");
		}
		CU_ASSERT_NOT_EQUAL(task->residual_status,
				SCSI_RESIDUAL_UNDERFLOW);
	} else {
		logging(LOG_VERBOSE, "We got less than the requested 255 bytes "
			"from the target. Verify that underflow is set.");

		if (task->residual_status != SCSI_RESIDUAL_UNDERFLOW) {
			logging(LOG_VERBOSE, "[FAILED] Target did not set "
				"residual underflow flag");
		} else {
			logging(LOG_VERBOSE, "[SUCCESS] Residual underflow "
				"is set");
		}
		CU_ASSERT_EQUAL(task->residual_status,
				SCSI_RESIDUAL_UNDERFLOW);
	}


	if (task != NULL) {
		scsi_free_scsi_task(task);
		task = NULL;
	}
}
