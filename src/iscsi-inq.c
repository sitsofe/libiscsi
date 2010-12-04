/* 
   Copyright (C) 2010 by Ronnie Sahlberg <ronniesahlberg@gmail.com>
   
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
#include <stdint.h>
#include <string.h>
#include <poll.h>
#include <popt.h>
#include "iscsi.h"
#include "scsi-lowlevel.h"

char *initiator = "iqn.2010-11.ronnie:iscsi-inq";


void inquiry_block_device_characteristics(struct scsi_inquiry_block_device_characteristics *inq)
{
	printf("Medium Rotation Rate:%dRPM\n", inq->medium_rotation_rate);	
}

void inquiry_device_identification(struct scsi_inquiry_device_identification *inq)
{
	struct scsi_inquiry_device_designator *dev;
	int i;

	printf("Peripheral Qualifier:%s\n",
		scsi_devqualifier_to_str(inq->periperal_qualifier));
	printf("Peripheral Device Type:%s\n",
		scsi_devtype_to_str(inq->periperal_device_type));
	printf("Page Code:(0x%02x) %s\n",
		inq->pagecode, scsi_inquiry_pagecode_to_str(inq->pagecode));

	for (i=0, dev = inq->designators; dev; i++, dev = dev->next) {
		printf("DEVICE DESIGNATOR #%d\n", i);
		if (dev->piv != 0) {
			printf("Device Protocol Identifier:(%d) %s\n", dev->protocol_identifier, scsi_protocol_identifier_to_str(dev->protocol_identifier));
		}
		printf("Code Set:(%d) %s\n", dev->code_set, scsi_codeset_to_str(dev->code_set));
		printf("PIV:%d\n", dev->piv);
		printf("Association:(%d) %s\n", dev->association, scsi_association_to_str(dev->association));
		printf("Designator Type:(%d) %s\n", dev->designator_type, scsi_designator_type_to_str(dev->designator_type));
		printf("Designator:[%s]\n", dev->designator);
	}
}

void inquiry_unit_serial_number(struct scsi_inquiry_unit_serial_number *inq)
{
	printf("Unit Serial Number:[%s]\n", inq->usn);
}

void inquiry_supported_pages(struct scsi_inquiry_supported_pages *inq)
{
	int i;

	for (i = 0; i < inq->num_pages; i++) {
		printf("Page:0x%02x %s\n", inq->pages[i], scsi_inquiry_pagecode_to_str(inq->pages[i]));
	}
}

void inquiry_standard(struct scsi_inquiry_standard *inq)
{
	printf("Peripheral Qualifier:%s\n",
		scsi_devqualifier_to_str(inq->periperal_qualifier));
	printf("Peripheral Device Type:%s\n",
		scsi_devtype_to_str(inq->periperal_device_type));
	printf("Removable:%d\n", inq->rmb);
	printf("Version:%d %s\n", inq->version, scsi_version_to_str(inq->version));
	printf("NormACA:%d\n", inq->normaca);
	printf("HiSup:%d\n", inq->hisup);
	printf("ReponseDataFormat:%d\n", inq->response_data_format);
	printf("SCCS:%d\n", inq->sccs);
	printf("ACC:%d\n", inq->acc);
	printf("TPGS:%d\n", inq->tpgs);
	printf("3PC:%d\n", inq->threepc);
	printf("Protect:%d\n", inq->protect);
	printf("EncServ:%d\n", inq->encserv);
	printf("MultiP:%d\n", inq->multip);
	printf("SYNC:%d\n", inq->sync);
	printf("CmdQue:%d\n", inq->cmdque);
	printf("Vendor:%s\n", inq->vendor_identification);
	printf("Product:%s\n", inq->product_identification);
	printf("Revision:%s\n", inq->product_revision_level);
}

void do_inquiry(struct iscsi_context *iscsi, int lun, int evpd, int pc)
{
	struct scsi_task *task;
	int full_size;
	void *inq;

	/* See how big this inquiry data is */
	task = iscsi_inquiry_sync(iscsi, lun, evpd, pc, 64);
	if (task == NULL || task->status != SCSI_STATUS_GOOD) {
		fprintf(stderr, "Inquiry command failed : %s\n", iscsi_get_error(iscsi));
		exit(10);
	}

	full_size = scsi_datain_getfullsize(task);
	if (full_size > task->datain.size) {
		scsi_free_scsi_task(task);

		/* we need more data for the full list */
		if ((task = iscsi_inquiry_sync(iscsi, lun, evpd, pc, full_size)) == NULL) {
			fprintf(stderr, "Inquiry command failed : %s\n", iscsi_get_error(iscsi));
			exit(10);
		}
	}

	inq = scsi_datain_unmarshall(task);
	if (inq == NULL) {
		fprintf(stderr, "failed to unmarshall inquiry datain blob\n");
		exit(10);
	}

	if (evpd == 0) {
		inquiry_standard(inq);
	} else {
		switch (pc) {
		case SCSI_INQUIRY_PAGECODE_SUPPORTED_VPD_PAGES:
			inquiry_supported_pages(inq);
			break;
		case SCSI_INQUIRY_PAGECODE_UNIT_SERIAL_NUMBER:
			inquiry_unit_serial_number(inq);
			break;
		case SCSI_INQUIRY_PAGECODE_DEVICE_IDENTIFICATION:
			inquiry_device_identification(inq);
			break;
		case SCSI_INQUIRY_PAGECODE_BLOCK_DEVICE_CHARACTERISTICS:
			inquiry_block_device_characteristics(inq);
			break;
		default:
			fprintf(stderr, "Usupported pagecode:0x%02x\n", pc);
		}
	}
	scsi_free_scsi_task(task);
}



int main(int argc, const char *argv[])
{
	poptContext pc;
	struct iscsi_context *iscsi;
	char *portal = NULL;
	char *target = NULL;
	int lun = -1, evpd = 0, pagecode = 0;
	int res;

	struct poptOption popt_options[] = {
		POPT_AUTOHELP
		{ "initiator-name", 'i', POPT_ARG_STRING, &initiator, 0, "Initiatorname to use", "iqn-name" },
		{ "portal", 'p', POPT_ARG_STRING, &portal, 0, "Target portal", "address[:port]" },
		{ "target", 't', POPT_ARG_STRING, &target, 0, "Target name", "iqn-name" },
		{ "lun", 'l', POPT_ARG_INT, &lun, 0, "LUN", "integer" },
		{ "evpd", 'e', POPT_ARG_INT, &evpd, 0, "evpd", "integer" },
		{ "pagecode", 'c', POPT_ARG_INT, &pagecode, 0, "page code", "integer" },
		POPT_TABLEEND
	};

	pc = poptGetContext(argv[0], argc, argv, popt_options, POPT_CONTEXT_POSIXMEHARDER);
	if ((res = poptGetNextOpt(pc)) < -1) {
		fprintf(stderr, "Failed to parse option : %s %s\n",
			poptBadOption(pc, 0), poptStrerror(res));
		exit(10);
	}
	poptFreeContext(pc);

	if (portal == NULL) {
		fprintf(stderr, "You must specify target portal\n");
		exit(10);
	}

	if (target == NULL) {
		fprintf(stderr, "You must specify target name\n");
		exit(10);
	}

	if (lun == -1) {
		fprintf(stderr, "You must specify LUN\n");
		exit(10);
	}

	iscsi = iscsi_create_context(initiator);
	if (iscsi == NULL) {
		fprintf(stderr, "Failed to create context\n");
		exit(10);
	}

	iscsi_set_targetname(iscsi, target);
	iscsi_set_session_type(iscsi, ISCSI_SESSION_NORMAL);
	iscsi_set_header_digest(iscsi, ISCSI_HEADER_DIGEST_NONE_CRC32C);

	if (iscsi_full_connect_sync(iscsi, portal, lun) != 0) {
		fprintf(stderr, "Failed to log in to target %s\n", iscsi_get_error(iscsi));
		exit(10);
	}

	do_inquiry(iscsi, lun, evpd, pagecode);

	iscsi_logout_sync(iscsi);
	iscsi_destroy_context(iscsi);
	return 0;
}

