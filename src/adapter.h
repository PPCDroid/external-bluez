/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2006-2007  Nokia Corporation
 *  Copyright (C) 2004-2008  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <bluetooth/bluetooth.h>
#include <glib.h>

#define ADAPTER_INTERFACE	"org.bluez.Adapter"

/* Discover types */
#define DISCOVER_TYPE_NONE	0x00
#define STD_INQUIRY		0x01
#define PERIODIC_INQUIRY	0x02

/* Actions executed after inquiry complete */
#define RESOLVE_NAME		0x10

typedef enum {
	NAME_ANY,
	NAME_NOT_REQUIRED, /* used by get remote name without name resolving */
	NAME_REQUIRED,      /* remote name needs be resolved       */
	NAME_REQUESTED,    /* HCI remote name request was sent    */
	NAME_SENT          /* D-Bus signal RemoteNameUpdated sent */
} name_status_t;

typedef enum {
	AUTH_TYPE_PINCODE,
	AUTH_TYPE_PASSKEY,
	AUTH_TYPE_CONFIRM,
	AUTH_TYPE_NOTIFY,
} auth_type_t;

struct remote_dev_info {
	bdaddr_t bdaddr;
	int8_t rssi;
	name_status_t name_status;
};

struct bonding_request_info {
	DBusConnection *conn;
	DBusMessage *msg;
	struct btd_adapter *adapter;
	bdaddr_t bdaddr;
	GIOChannel *io;
	guint io_id;
	guint listener_id;
	int hci_status;
	int cancel;
	int auth_active;
};

struct pending_auth_info {
	auth_type_t type;
	bdaddr_t bdaddr;
	gboolean replied;	/* If we've already replied to the request */
	struct agent *agent;    /* Agent associated with the request */
};

struct active_conn_info {
	bdaddr_t bdaddr;
	uint16_t handle;
};

struct hci_dev {
	int ignore;

	uint8_t  features[8];
	uint8_t  lmp_ver;
	uint16_t lmp_subver;
	uint16_t hci_rev;
	uint16_t manufacturer;

	uint8_t  ssp_mode;
	uint8_t  name[248];
	uint8_t  class[3];
};

int adapter_start(struct btd_adapter *adapter);

int adapter_stop(struct btd_adapter *adapter);

int adapter_update(struct btd_adapter *adapter);

int adapter_get_class(struct btd_adapter *adapter, uint8_t *cls);

int adapter_set_class(struct btd_adapter *adapter, uint8_t *cls);

int adapter_update_ssp_mode(struct btd_adapter *adapter, int dd, uint8_t mode);

struct btd_device *adapter_get_device(DBusConnection *conn,
				struct btd_adapter *adapter, const char *address);

struct btd_device *adapter_find_device(struct btd_adapter *adapter, const char *dest);

void adapter_remove_device(DBusConnection *conn, struct btd_adapter *adapter,
				struct btd_device *device);
struct btd_device *adapter_create_device(DBusConnection *conn,
				struct btd_adapter *adapter, const char *address);

int pending_remote_name_cancel(struct btd_adapter *adapter);

void remove_pending_device(struct btd_adapter *adapter);

struct pending_auth_info *adapter_find_auth_request(struct btd_adapter *adapter,
							bdaddr_t *dba);
void adapter_remove_auth_request(struct btd_adapter *adapter, bdaddr_t *dba);
struct pending_auth_info *adapter_new_auth_request(struct btd_adapter *adapter,
							bdaddr_t *dba,
							auth_type_t type);
struct btd_adapter *adapter_create(DBusConnection *conn, int id);
void adapter_remove(struct btd_adapter *adapter);
uint16_t adapter_get_dev_id(struct btd_adapter *adapter);
const gchar *adapter_get_path(struct btd_adapter *adapter);
void adapter_get_address(struct btd_adapter *adapter, bdaddr_t *bdaddr);
void adapter_remove(struct btd_adapter *adapter);
void adapter_set_discov_timeout(struct btd_adapter *adapter, guint interval);
void adapter_remove_discov_timeout(struct btd_adapter *adapter);
void adapter_set_scan_mode(struct btd_adapter *adapter, uint8_t scan_mode);
uint8_t adapter_get_scan_mode(struct btd_adapter *adapter);
void adapter_set_mode(struct btd_adapter *adapter, uint8_t mode);
uint8_t adapter_get_mode(struct btd_adapter *adapter);
void adapter_set_state(struct btd_adapter *adapter, int state);
int adapter_get_state(struct btd_adapter *adapter);
struct remote_dev_info *adapter_search_found_devices(struct btd_adapter *adapter,
						struct remote_dev_info *match);
int adapter_add_found_device(struct btd_adapter *adapter, bdaddr_t *bdaddr,
				int8_t rssi, name_status_t name_status);
int adapter_remove_found_device(struct btd_adapter *adapter, bdaddr_t *bdaddr);
void adapter_update_oor_devices(struct btd_adapter *adapter);
void adapter_remove_oor_device(struct btd_adapter *adapter, char *peer_addr);
void adapter_mode_changed(struct btd_adapter *adapter, uint8_t scan_mode);
struct agent *adapter_get_agent(struct btd_adapter *adapter);
void adapter_add_active_conn(struct btd_adapter *adapter, bdaddr_t *bdaddr,
				uint16_t handle);
void adapter_remove_active_conn(struct btd_adapter *adapter,
				struct active_conn_info *dev);
void adapter_update_devices(struct btd_adapter *adapter);
struct active_conn_info *adapter_search_active_conn_by_bdaddr(struct btd_adapter *adapter,
						    bdaddr_t *bda);
struct active_conn_info *adapter_search_active_conn_by_handle(struct btd_adapter *adapter,
							uint16_t handle);
void adapter_free_bonding_request(struct btd_adapter *adapter);
struct bonding_request_info *adapter_get_bonding_info(struct btd_adapter *adapter);
gboolean adapter_has_discov_sessions(struct btd_adapter *adapter);

struct btd_adapter_driver {
	const char *name;
	int (*probe) (struct btd_adapter *adapter);
	void (*remove) (struct btd_adapter *adapter);
};

typedef void (*service_auth_cb) (DBusError *derr, void *user_data);

int btd_register_adapter_driver(struct btd_adapter_driver *driver);
void btd_unregister_adapter_driver(struct btd_adapter_driver *driver);
int btd_request_authorization(const bdaddr_t *src, const bdaddr_t *dst,
		const char *uuid, service_auth_cb cb, void *user_data);
int btd_cancel_authorization(const bdaddr_t *src, const bdaddr_t *dst);