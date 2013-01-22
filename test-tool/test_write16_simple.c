
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
test_write16_simple(void)
{
	int i, ret;

	CHECK_FOR_DATALOSS;
	CHECK_FOR_SBC;

	logging(LOG_VERBOSE, "");
	logging(LOG_VERBOSE, "Test WRITE16 of 1-256 blocks at the start of the LUN");

	for (i = 1; i <= 256; i++) {
		unsigned char *buf = malloc(block_size * i);

		ret = write16(iscsic, tgt_lun, 0, i * block_size,
		    block_size, 0, 0, 0, 0, 0, buf);
		free(buf);
		CU_ASSERT_EQUAL(ret, 0);
	}

	logging(LOG_VERBOSE, "Test WRITE16 of 1-256 blocks at the end of the LUN");
	for (i = 1; i <= 256; i++) {
		unsigned char *buf = malloc(block_size * i);

		ret = write16(iscsic, tgt_lun, num_blocks +1 - i,
		    i * block_size, block_size, 0, 0, 0, 0, 0, buf);
		free(buf);
		CU_ASSERT_EQUAL(ret, 0);
	}

}
