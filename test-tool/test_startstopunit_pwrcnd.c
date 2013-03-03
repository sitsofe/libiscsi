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

#include <CUnit/CUnit.h>

#include "iscsi.h"
#include "scsi-lowlevel.h"
#include "iscsi-test-cu.h"

void
test_startstopunit_pwrcnd(void)
{ 
	int i, ret;

	logging(LOG_VERBOSE, LOG_BLANK_LINE);
	logging(LOG_VERBOSE, "Test STARTSTOPUNIT PowerCondition");
	if (!removable) {
		logging(LOG_VERBOSE, "[SKIPPED] LUN is not removalbe. "
			"Skipping test.");
		return;
	}

	logging(LOG_VERBOSE, "Test that media is not ejected when PC!=0");
	for (i = 1; i < 16; i++) {
		ret = startstopunit(iscsic, tgt_lun,
				    1, 0, i, 0, 1, 0);
		CU_ASSERT_EQUAL(ret, 0);

		logging(LOG_VERBOSE, "Test TESTNUITREADY that medium is not ejected.");
		ret = testunitready(iscsic, tgt_lun);
		CU_ASSERT_EQUAL(ret, 0);
	}

	logging(LOG_VERBOSE, "In case the target did eject the medium, load it again.");
	startstopunit(iscsic, tgt_lun, 1, 0, 0, 0, 1, 1);
}
