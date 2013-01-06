/*
   iscsi-test tool

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

#ifndef	_ISCSI_TEST_CU_H_
#define	_ISCSI_TEST_CU_H_

#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "iscsi-support.h"

/* globals between setup, tests, and teardown */
extern struct iscsi_context *iscsic;
extern int tgt_lun;
extern struct scsi_task *task;

int test_setup(void);
int test_teardown(void);

void test_testunitready_simple(void);

void test_read10_simple(void);
void test_read10_beyond_eol(void);
void test_read10_0blocks(void);
void test_read10_rdprotect(void);
void test_read10_flags(void);
void test_read10_invalid(void);


#endif	/* _ISCSI_TEST_CU_H_ */
