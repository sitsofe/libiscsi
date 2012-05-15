/*
   Copyright (C) 2010 by Ronnie Sahlberg <ronniesahlberg@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2.1 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.
*/

#if defined(WIN32)
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "slist.h"
#include "iscsi.h"
#include "iscsi-private.h"
#include "scsi-lowlevel.h"

struct connect_task {
	iscsi_command_cb cb;
	void *private_data;
	int lun;
};

static void
iscsi_testunitready_cb(struct iscsi_context *iscsi, int status,
		       void *command_data, void *private_data)
{
	struct connect_task *ct = private_data;
	struct scsi_task *task = command_data;

	if (status != 0) {
		if (task->sense.key == SCSI_SENSE_UNIT_ATTENTION
		    && task->sense.ascq == SCSI_SENSE_ASCQ_BUS_RESET) {
			/* This is just the normal unitattention/busreset
			 * you always get just after a fresh login. Try
			 * again.
			 */
			if (iscsi_testunitready_task(iscsi, ct->lun,
						      iscsi_testunitready_cb,
						      ct) == NULL) {
				iscsi_set_error(iscsi, "iscsi_testunitready "
						"failed.");
				ct->cb(iscsi, SCSI_STATUS_ERROR, NULL,
				       ct->private_data);
				free(ct);
			}
			scsi_free_scsi_task(task);
			return;
		}
	}

	ct->cb(iscsi, status?SCSI_STATUS_ERROR:SCSI_STATUS_GOOD, NULL,
	       ct->private_data);
	free(ct);
	scsi_free_scsi_task(task);
}

static void
iscsi_login_cb(struct iscsi_context *iscsi, int status, void *command_data _U_,
	       void *private_data)
{
	struct connect_task *ct = private_data;

	if (status != 0) {
		ct->cb(iscsi, SCSI_STATUS_ERROR, NULL, ct->private_data);
		free(ct);
		return;
	}

	if (iscsi_testunitready_task(iscsi, ct->lun,
				      iscsi_testunitready_cb, ct) == NULL) {
		iscsi_set_error(iscsi, "iscsi_testunitready_async failed.");
		ct->cb(iscsi, SCSI_STATUS_ERROR, NULL, ct->private_data);
		free(ct);
	}
}

static void
iscsi_connect_cb(struct iscsi_context *iscsi, int status, void *command_data _U_,
		 void *private_data)
{
	struct connect_task *ct = private_data;

	if (status != 0) {
		iscsi_set_error(iscsi, "Failed to connect to iSCSI socket. "
				"%s", iscsi_get_error(iscsi));
		ct->cb(iscsi, SCSI_STATUS_ERROR, NULL, ct->private_data);
		free(ct);
		return;
	}

	if (iscsi_login_async(iscsi, iscsi_login_cb, ct) != 0) {
		iscsi_set_error(iscsi, "iscsi_login_async failed.");
		ct->cb(iscsi, SCSI_STATUS_ERROR, NULL, ct->private_data);
		free(ct);
	}
}


int
iscsi_full_connect_async(struct iscsi_context *iscsi, const char *portal,
			 int lun, iscsi_command_cb cb, void *private_data)
{
	struct connect_task *ct;

	iscsi->lun = lun;
	iscsi->portal = strdup(portal);

	ct = malloc(sizeof(struct connect_task));
	if (ct == NULL) {
		iscsi_set_error(iscsi, "Out-of-memory. Failed to allocate "
				"connect_task structure.");
		return -ENOMEM;
	}
	ct->cb           = cb;
	ct->lun          = lun;
	ct->private_data = private_data;
	if (iscsi_connect_async(iscsi, portal, iscsi_connect_cb, ct) != 0) {
		free(ct);
		return -ENOMEM;
	}
	return 0;
}

int iscsi_reconnect(struct iscsi_context *old_iscsi)
{
	struct iscsi_context *iscsi;

try_again:

	iscsi = iscsi_create_context(old_iscsi->initiator_name);
	iscsi->is_reconnecting = 1;

	iscsi_set_targetname(iscsi, old_iscsi->target_name);

	iscsi_set_header_digest(iscsi, old_iscsi->want_header_digest);

	if (old_iscsi->user != NULL) {
		iscsi_set_initiator_username_pwd(iscsi, old_iscsi->user, old_iscsi->passwd);
	}

	iscsi_set_session_type(iscsi, ISCSI_SESSION_NORMAL);

	iscsi->lun = old_iscsi->lun;

	iscsi->portal = strdup(old_iscsi->portal);

	if (iscsi_full_connect_sync(iscsi, iscsi->portal, iscsi->lun) != 0) {
		iscsi_destroy_context(iscsi);
		sleep(1);
		goto try_again;
	}

	while (old_iscsi->waitpdu) {
		struct iscsi_pdu *pdu = old_iscsi->waitpdu;

		SLIST_REMOVE(&old_iscsi->waitpdu, pdu);
		if (pdu->itt == 0xffffffff) {
			continue;
		}

		pdu->itt   = iscsi->itt++;
		iscsi_pdu_set_itt(pdu, pdu->itt);

		pdu->cmdsn = iscsi->cmdsn++;
		iscsi_pdu_set_cmdsn(pdu, pdu->cmdsn);

		iscsi_pdu_set_expstatsn(pdu, iscsi->statsn);
		iscsi->statsn++;

		pdu->written = 0;
		SLIST_ADD_END(&iscsi->outqueue, pdu);
	}

	while (old_iscsi->outqueue) {
		struct iscsi_pdu *pdu = old_iscsi->outqueue;

		SLIST_REMOVE(&old_iscsi->outqueue, pdu);
		if (pdu->itt == 0xffffffff) {
			continue;
		}

		pdu->itt   = iscsi->itt++;
		iscsi_pdu_set_itt(pdu, pdu->itt);

		pdu->cmdsn = iscsi->cmdsn++;
		iscsi_pdu_set_cmdsn(pdu, pdu->cmdsn);

		iscsi_pdu_set_expstatsn(pdu, iscsi->statsn);
		iscsi->statsn++;

		pdu->written = 0;
		SLIST_ADD_END(&iscsi->outqueue, pdu);
	}

	if (dup2(iscsi->fd, old_iscsi->fd) == -1) {
		iscsi_destroy_context(iscsi);
		goto try_again;
	}
	

	free(discard_const(old_iscsi->initiator_name));
	free(discard_const(old_iscsi->target_name));
	free(discard_const(old_iscsi->target_address));
	free(discard_const(old_iscsi->alias));
	free(discard_const(old_iscsi->portal));
	if (old_iscsi->incoming != NULL) {
		iscsi_free_iscsi_in_pdu(old_iscsi->incoming);
	}
	if (old_iscsi->inqueue != NULL) {
		iscsi_free_iscsi_inqueue(old_iscsi->inqueue);
	}
	free(old_iscsi->error_string);
	free(discard_const(old_iscsi->user));
	free(discard_const(old_iscsi->passwd));
	free(discard_const(old_iscsi->chap_c));

	close(iscsi->fd);
	iscsi->fd = old_iscsi->fd;
	memcpy(old_iscsi, iscsi, sizeof(struct iscsi_context));
	free(iscsi);

	old_iscsi->is_reconnecting = 0;

	return 0;
}
