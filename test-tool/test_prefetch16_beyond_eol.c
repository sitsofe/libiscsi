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
#include "iscsi-test-cu.h"


void
test_prefetch16_beyond_eol(void)
{ 
	int i, ret;

	logging(LOG_VERBOSE, "");
	logging(LOG_VERBOSE, "Test PREFETCH16 1-256 blocks one block beyond the end");
	for (i = 1; i <= 256; i++) {
		ret = prefetch16_lbaoutofrange(iscsic, tgt_lun, num_blocks + 1 - i,
					       i, 0, 0);
		if (ret == -2) {
			CU_PASS("[SKIPPED] Target does not support PREFETCH16. Skipping test");
			return;
		}
		CU_ASSERT_EQUAL(ret, 0);
	}


	logging(LOG_VERBOSE, "Test PREFETCH16 1-256 blocks at LBA==2^63");
	for (i = 1; i <= 256; i++) {
		ret = prefetch16_lbaoutofrange(iscsic, tgt_lun, 0x8000000000000000,
					       i, 0, 0);
		CU_ASSERT_EQUAL(ret, 0);
	}


	logging(LOG_VERBOSE, "Test PREFETCH16 1-256 blocks at LBA==-1");
	for (i = 1; i <= 256; i++) {
		ret = prefetch16_lbaoutofrange(iscsic, tgt_lun, -1,
					       i, 0, 0);
		CU_ASSERT_EQUAL(ret, 0);
	}


	logging(LOG_VERBOSE, "Test PREFETCH16 2-256 blocks all but one block beyond the end");
	for (i = 2; i <= 256; i++) {
		ret = prefetch16_lbaoutofrange(iscsic, tgt_lun, num_blocks - 1,
					       i, 0, 0);
		CU_ASSERT_EQUAL(ret, 0);
	}
}
