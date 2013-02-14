
/* 
   Copyright (C) 2012 by Lee Duncan <lee@gonzoleeman.net>
   
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
test_read10_simple(void)
{
	int i, ret;


	logging(LOG_VERBOSE, LOG_BLANK_LINE);
	logging(LOG_VERBOSE, "Test READ10 of 1-256 blocks at the start of the LUN");
	for (i = 1; i <= 256; i++) {
		ret = read10(iscsic, tgt_lun, 0, i * block_size,
		    block_size, 0, 0, 0, 0, 0, NULL);
		CU_ASSERT_EQUAL(ret, 0);
	}


	logging(LOG_VERBOSE, "Test READ10 of 1-256 blocks at the end of the LUN");
	for (i = 1; i <= 256; i++) {
		ret = read10(iscsic, tgt_lun, num_blocks - i,
		    i * block_size, block_size, 0, 0, 0, 0, 0, NULL);
		CU_ASSERT_EQUAL(ret, 0);
	}
}
