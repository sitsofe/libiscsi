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

#include <CUnit/CUnit.h>

#include "iscsi.h"
#include "scsi-lowlevel.h"
#include "iscsi-support.h"
#include "iscsi-test-cu.h"


void
test_writesame10_unmap(void)
{
	int i, ret;
	unsigned int j;

	CHECK_FOR_DATALOSS;
	CHECK_FOR_THIN_PROVISIONING;
	CHECK_FOR_LBPWS10;
	CHECK_FOR_SBC;

	logging(LOG_VERBOSE, LOG_BLANK_LINE);
	logging(LOG_VERBOSE, "Test WRITESAME10 of 1-256 blocks at the start of the LUN");
	for (i = 1; i <= 256; i++) {
		unsigned char *buf = malloc(block_size * i);

		logging(LOG_VERBOSE, "Write %d blocks of 0xFF", i);
		memset(buf, 0xff, block_size * i);
		ret = write10(iscsic, tgt_lun, 0,
			      i * block_size, block_size,
			      0, 0, 0, 0, 0, buf);
		CU_ASSERT_EQUAL(ret, 0);

		logging(LOG_VERBOSE, "Unmap %d blocks using WRITESAME10", i);
		ret = writesame10(iscsic, tgt_lun, 0,
				  block_size, i,
				  0, 1, 0, 0, NULL);
		CU_ASSERT_EQUAL(ret, 0);

		logging(LOG_VERBOSE, "Read %d blocks and verify they are now zero", i);
		ret = read10(iscsic, tgt_lun, 0,
			     i * block_size, block_size,
			     0, 0, 0, 0, 0, buf);
		for (j = 0; j < block_size * i; j++) {
			if (buf[j] != 0) {
				CU_ASSERT_EQUAL(buf[j], 0);
			}
		}
		free(buf);
	}


	logging(LOG_VERBOSE, "Test WRITESAME10 of 1-256 blocks at the end of the LUN");
	for (i = 1; i <= 256; i++) {
		unsigned char *buf = malloc(block_size * i);

		logging(LOG_VERBOSE, "Write %d blocks of 0xFF", i);
		memset(buf, 0xff, block_size * i);
		ret = write10(iscsic, tgt_lun, num_blocks - i,
			      i * block_size, block_size,
			      0, 0, 0, 0, 0, buf);
		CU_ASSERT_EQUAL(ret, 0);

		logging(LOG_VERBOSE, "Unmap %d blocks using WRITESAME10", i);
		ret = writesame10(iscsic, tgt_lun, num_blocks - i,
				  block_size, i,
				  0, 1, 0, 0, buf);
		CU_ASSERT_EQUAL(ret, 0);

		logging(LOG_VERBOSE, "Read %d blocks and verify they are now zero", i);
		ret = read10(iscsic, tgt_lun, num_blocks - i,
			     i * block_size, block_size,
			     0, 0, 0, 0, 0, buf);
		for (j = 0; j < block_size * i; j++) {
			if (buf[j] != 0) {
				CU_ASSERT_EQUAL(buf[j], 0);
			}
		}
		free(buf);
	}

	logging(LOG_VERBOSE, "Verify that WRITESAME10 ANCHOR==1 + UNMAP==0 is invalid");
	ret = writesame10_invalidfieldincdb(iscsic, tgt_lun, 0,
					    block_size, 1,
					    1, 0, 0, 0, NULL);
	CU_ASSERT_EQUAL(ret, 0);



	if (inq_lbp->anc_sup) {
		logging(LOG_VERBOSE, "Test WRITESAME10 ANCHOR==1 + UNMAP==0");
		ret = writesame10(iscsic, tgt_lun, 0,
				  block_size, 1,
				  1, 1, 0, 0, NULL);
	} else {
		logging(LOG_VERBOSE, "Test WRITESAME10 ANCHOR==1 + UNMAP==0 no ANC_SUP so expecting to fail");
		ret = writesame10_invalidfieldincdb(iscsic, tgt_lun, 0,
						    block_size, 1,
						    1, 1, 0, 0, NULL);
	}

	CU_ASSERT_EQUAL(ret, 0);

}
