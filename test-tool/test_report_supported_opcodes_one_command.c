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
test_report_supported_opcodes_one_command(void)
{
	int i, ret;
	struct scsi_task *rso_task;
	struct scsi_task *one_task;
	struct scsi_report_supported_op_codes *rsoc;
	struct scsi_report_supported_op_codes_one_command *rsoc_one;

	logging(LOG_VERBOSE, LOG_BLANK_LINE);
	logging(LOG_VERBOSE, "Test READ_SUPPORTED_OPCODES reading one-command");


	logging(LOG_VERBOSE, "Fetch list of all supported opcodes");
	ret = report_supported_opcodes(iscsic, tgt_lun,
		0, SCSI_REPORT_SUPPORTING_OPS_ALL, 0, 0,
		65535, &rso_task);
	if (ret == -2) {
		logging(LOG_NORMAL, "[SKIPPED] READ_SUPPORTED_OPCODES is not "
			"implemented.");
		CU_PASS("READ_SUPPORTED_OPCODES is not implemented.");
		return;
	}
	CU_ASSERT_EQUAL(ret, 0);
	if (ret != 0) {
		return;
	}
	
	logging(LOG_VERBOSE, "Unmarshall the DATA-IN buffer");
	rsoc = scsi_datain_unmarshall(rso_task);
	CU_ASSERT_NOT_EQUAL(rsoc, NULL);


	logging(LOG_VERBOSE, "Verify read one-command works for all supported "
		"opcodes");
	for (i = 0; i < rsoc->num_descriptors; i++) {
		logging(LOG_VERBOSE, "Check opcode:0x%02x ServiceAction:0x%02x",
			rsoc->descriptors[i].opcode,
			rsoc->descriptors[i].sa);
		if (rsoc->descriptors[i].servactv) {
			logging(LOG_VERBOSE, "This opcode has service actions. "
				"Reporting Options 001b should fail");
			ret = report_supported_opcodes_invalidfieldincdb(
				iscsic, tgt_lun,
				0, SCSI_REPORT_SUPPORTING_OPCODE,
				rsoc->descriptors[i].opcode,
				rsoc->descriptors[i].sa,
				65535, NULL);
		} else {
			logging(LOG_VERBOSE, "This opcode does not have "
				"service actions. Reporting Options 001b "
				"should work");
			ret = report_supported_opcodes(
				iscsic, tgt_lun,
				0, SCSI_REPORT_SUPPORTING_OPCODE,
				rsoc->descriptors[i].opcode,
				rsoc->descriptors[i].sa,
				65535, NULL);
		}
		CU_ASSERT_EQUAL(ret, 0);

		if (rsoc->descriptors[i].servactv) {
			logging(LOG_VERBOSE, "This opcode has service actions. "
				"Reporting Options 002b should work");
			ret = report_supported_opcodes(
				iscsic, tgt_lun,
				0, SCSI_REPORT_SUPPORTING_SERVICEACTION,
				rsoc->descriptors[i].opcode,
				rsoc->descriptors[i].sa,
				65535, NULL);
		} else {
			logging(LOG_VERBOSE, "This opcode does not have "
				"service actions. Reporting Options 002b "
				"should fail");
			ret = report_supported_opcodes_invalidfieldincdb(
				iscsic, tgt_lun,
				0, SCSI_REPORT_SUPPORTING_SERVICEACTION,
				rsoc->descriptors[i].opcode,
				rsoc->descriptors[i].sa,
				65535, NULL);
		}
		CU_ASSERT_EQUAL(ret, 0);
	}


	logging(LOG_VERBOSE, "Verify read one-command CDB looks sane");
	for (i = 0; i < rsoc->num_descriptors; i++) {
		logging(LOG_VERBOSE, "Check CDB for opcode:0x%02x "
			"ServiceAction:0x%02x",
			rsoc->descriptors[i].opcode,
			rsoc->descriptors[i].sa);
		ret = report_supported_opcodes(
			iscsic, tgt_lun,
			0,
			rsoc->descriptors[i].servactv ?
				SCSI_REPORT_SUPPORTING_SERVICEACTION :
				SCSI_REPORT_SUPPORTING_OPCODE,
			rsoc->descriptors[i].opcode,
			rsoc->descriptors[i].sa,
			65535, &one_task);

		logging(LOG_VERBOSE, "Unmarshall the DATA-IN buffer");
		rsoc_one = scsi_datain_unmarshall(one_task);
		CU_ASSERT_NOT_EQUAL(rsoc_one, NULL);

		logging(LOG_VERBOSE, "Verify CDB length is not 0");
		CU_ASSERT_NOT_EQUAL(rsoc_one->cdb_length, 0);
		if (rsoc_one->cdb_length != 0) {
			logging(LOG_NORMAL, "[FAILED] CDB length is 0");
		}
		
		logging(LOG_VERBOSE, "Verify CDB[0] Usage Data is 0xFF");
		CU_ASSERT_EQUAL(rsoc_one->cdb_usage_data[0], 0xff);
		if (rsoc_one->cdb_usage_data[0] != 0xff) {
			logging(LOG_NORMAL, "[FAILED] CDB[0] Usage Data is "
				"not 0xFF");
		}
	}


	scsi_free_scsi_task(rso_task);
}
