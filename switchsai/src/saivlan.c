/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <saivlan.h>
#include "saiinternal.h"
#include <switchapi/switch_vlan.h>
#include <switchapi/switch_handle.h>
#include <switchapi/switch_hostif.h>

extern switch_status_t
switch_api_interface_get_port_handle(switch_handle_t intf_handle,
                                     switch_handle_t *port_handle);
static sai_api_t api_id = SAI_API_VLAN;
#define VLAN_MEMBER_HANDLE_FROM_PORT_VLAN(port_id, vlan_id) (0 | (port_id & \
                                                                      0xFFF) | \
                                                                 (vlan_id << 12) \
                                                                 | \
                                                                 (SWITCH_HANDLE_TYPE_VLAN_MEMBER \
                                                                  << \
                                                                  HANDLE_TYPE_SHIFT))

/*
* Routine Description:
*    Create a VLAN
*
* Arguments:
*    [in] vlan_id - VLAN id
*
* Return Values:
*    SAI_STATUS_SUCCESS on success
*    Failure status code on error
*/
sai_status_t sai_create_vlan_entry(_In_ sai_vlan_id_t vlan_id) {
  SAI_LOG_ENTER();

  switch_handle_t vlan_handle = SWITCH_API_INVALID_HANDLE;

  sai_status_t status = SAI_STATUS_SUCCESS;
  vlan_handle = switch_api_vlan_create(device, (switch_vlan_t)vlan_id);

  status = (vlan_handle == SWITCH_API_INVALID_HANDLE) ? SAI_STATUS_FAILURE
                                                      : SAI_STATUS_SUCCESS;

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR(
        "failed to create vlan %d: %s", vlan_id, sai_status_to_string(status));
  } else {
    /* enable IGMP and MLD snooping by default */
    switch_status_t switch_status;
    switch_status =
        switch_api_vlan_igmp_snooping_enabled_set(vlan_handle, true);
    assert(switch_status == SWITCH_STATUS_SUCCESS);
    switch_status = switch_api_vlan_mld_snooping_enabled_set(vlan_handle, true);
    assert(switch_status == SWITCH_STATUS_SUCCESS);
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
* Routine Description:
*    Remove a VLAN
*
* Arguments:
*    [in] vlan_id - VLAN id
*
* Return Values:
*    SAI_STATUS_SUCCESS on success
*    Failure status code on error
*/
sai_status_t sai_remove_vlan_entry(_In_ sai_vlan_id_t vlan_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_handle_t vlan_handle = 0;

  switch_status =
      switch_api_vlan_id_to_handle_get((switch_vlan_t)vlan_id, &vlan_handle);
  status = sai_switch_status_to_sai_status(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR(
        "failed to remove vlan %d: %s", vlan_id, sai_status_to_string(status));
    return status;
  }

  switch_status = switch_api_vlan_delete(device, vlan_handle);
  status = sai_switch_status_to_sai_status(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR(
        "failed to remove vlan %d: %s", vlan_id, sai_status_to_string(status));
    return status;
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
* Routine Description:
*    Set VLAN attribute Value
*
* Arguments:
*    [in] vlan_id - VLAN id
*    [in] attr - attribute
*
* Return Values:
*    SAI_STATUS_SUCCESS on success
*    Failure status code on error
*/
sai_status_t sai_set_vlan_entry_attribute(_In_ sai_vlan_id_t vlan_id,
                                          _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_status_to_string(status));
    return status;
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
* Routine Description:
*    Get VLAN attribute Value
*
* Arguments:
*    [in] vlan_id - VLAN id
*    [in] attr_count - number of attributes
*    [inout] attr_list - array of attributes
*
* Return Values:
*    SAI_STATUS_SUCCESS on success
*    Failure status code on error
*/
sai_status_t sai_get_vlan_entry_attribute(_In_ sai_vlan_id_t vlan_id,
                                          _In_ uint32_t attr_count,
                                          _Inout_ sai_attribute_t *attr_list) {

  unsigned int i=0;
  switch_handle_t vlan_handle = 0;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  short unsigned int mbr_count=0;
  switch_vlan_interface_t *mbrs=NULL;
  sai_status_t status = SAI_STATUS_SUCCESS;

  SAI_LOG_ENTER();

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s", sai_status_to_string(status));
    return status;
  }

  if (!attr_list) {
        status = SAI_STATUS_INVALID_PARAMETER;
        SAI_LOG_ERROR("null attribute list: %s",
                      sai_status_to_string(status));
        return status;
  }

  switch_status = switch_api_vlan_id_to_handle_get((switch_vlan_t) vlan_id, &vlan_handle);
    
  for(i=0;i<attr_count;i++) {
      switch(attr_list[i].id) {
        // Handle list of members associated with VLAN
        case SAI_VLAN_ATTR_MEMBER_LIST:
          switch_status = switch_api_vlan_interfaces_get(device,
                                                         vlan_handle,
                                                         &mbr_count,
                                                         &mbrs);
	  if(switch_status == SWITCH_STATUS_SUCCESS) {
		  for(short unsigned int j=0;j<mbr_count;j++) {
			  switch_handle_t port_handle=0;
			  switch_handle_t vlan_member_id=0;
			  // locate the port from interface and create the object ID of the
			  // member handle
			  switch_api_interface_get_port_handle(mbrs[j].intf_handle,
					  &port_handle);
			  vlan_member_id = VLAN_MEMBER_HANDLE_FROM_PORT_VLAN(port_handle, vlan_id);
			  attr_list[i].value.objlist.list[j] = vlan_member_id;
		  }
		  attr_list[i].value.objlist.count = mbr_count;
	  }
          break;
      }
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
* Routine Description:
*    Remove VLAN configuration (remove all VLANs).
*
* Arguments:
*    None
*
* Return Values:
*    SAI_STATUS_SUCCESS on success
*    Failure status code on error
*/
sai_status_t sai_remove_all_vlans(void) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
    \brief Create VLAN Member
    \param[out] vlan_member_id VLAN member ID
    \param[in] attr_count number of attributes
    \param[in] attr_list array of attributes
    \return Success: SAI_STATUS_SUCCESS
            Failure: failure status code on error
*/
sai_status_t sai_create_vlan_member(_Out_ sai_object_id_t *vlan_member_id,
                                    _In_ uint32_t attr_count,
                                    _In_ const sai_attribute_t *attr_list) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  SAI_LOG_ENTER();
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_handle_t vlan_handle = 0;
  switch_vlan_port_t switch_port;
  unsigned int index = 0;
  sai_vlan_tagging_mode_t tag_mode = SAI_VLAN_PORT_UNTAGGED;
  unsigned short vlan_id = 0;
  switch_handle_t port_id = 0;

  memset(vlan_member_id, 0, sizeof(sai_object_id_t));
  memset(&switch_port, 0, sizeof(switch_port));
  for (index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      case SAI_VLAN_MEMBER_ATTR_VLAN_ID:
        vlan_id = attr_list[index].value.u16;
        break;
      case SAI_VLAN_MEMBER_ATTR_PORT_ID:
        port_id = (switch_handle_t)attr_list[index].value.oid;
        break;
      case SAI_VLAN_MEMBER_ATTR_TAGGING_MODE:
        tag_mode = attr_list[index].value.s32;
        break;
      default:
        // NEED error checking here!
        break;
    }
  }
    switch_status = switch_api_vlan_id_to_handle_get((switch_vlan_t) vlan_id, &vlan_handle);
    status = sai_switch_status_to_sai_status(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to add ports to vlan %d: %s",
                      vlan_id, sai_status_to_string(status));
        return status;
    }

    switch_port.handle = port_id;
    switch_port.tagging_mode = tag_mode;
    switch_status = switch_api_vlan_ports_add(device, vlan_handle, 1, &switch_port);
    status = sai_switch_status_to_sai_status(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to add ports to vlan %d: %s",
                      vlan_id, sai_status_to_string(status));
        return status;
    }

    // make it from port and vlan (no storage of handle)
    *vlan_member_id = VLAN_MEMBER_HANDLE_FROM_PORT_VLAN(port_id, vlan_id);

    SAI_LOG_EXIT();

    return (sai_status_t) status;
}

/*
    \brief Remove VLAN Member
    \param[in] vlan_member_id VLAN member ID
    \return Success: SAI_STATUS_SUCCESS
            Failure: failure status code on error
*/
sai_status_t sai_remove_vlan_member(_In_ sai_object_id_t vlan_member_id) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  SAI_LOG_ENTER();
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_handle_t vlan_handle = 0;
  switch_vlan_port_t switch_port;
  // sai_vlan_tagging_mode_t tag_mode=SAI_VLAN_PORT_UNTAGGED;
  unsigned short vlan_id;

  memset(&switch_port, 0, sizeof(switch_port));
  // check the OID for handle type?
  switch_port.handle =
      id_to_handle(SWITCH_HANDLE_TYPE_PORT, (vlan_member_id & 0xFFF));
  vlan_id = (vlan_member_id >> 12) & 0xFFF;
  switch_status =
      switch_api_vlan_id_to_handle_get((switch_vlan_t)vlan_id, &vlan_handle);
  status = sai_switch_status_to_sai_status(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to del port from vlan (no VLAN) %d: %s",
                  vlan_id,
                  sai_status_to_string(status));
    return status;
  }

  switch_status =
      switch_api_vlan_ports_remove(device, vlan_handle, 1, &switch_port);
  status = sai_switch_status_to_sai_status(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to del ports from vlan %d: %s",
                  vlan_id,
                  sai_status_to_string(status));
    return status;
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
    \brief Set VLAN Member Attribute
    \param[in] vlan_member_id VLAN member ID
    \param[in] attr attribute structure containing ID and value
    \return Success: SAI_STATUS_SUCCESS
            Failure: failure status code on error
*/
sai_status_t sai_set_vlan_member_attribute(_In_ sai_object_id_t vlan_member_id,
                                           _In_ const sai_attribute_t *attr) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  SAI_LOG_ENTER();

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
    \brief Get VLAN Member Attribute
    \param[in] vlan_member_id VLAN member ID
    \param[in] attr_count number of attributes
    \param[in,out] attr_list list of attribute structures containing ID and
   value
    \return Success: SAI_STATUS_SUCCESS
            Failure: failure status code on error
*/
sai_status_t sai_get_vlan_member_attribute(_In_ sai_object_id_t vlan_member_id,
                                           _In_ const uint32_t attr_count,
                                           _Inout_ sai_attribute_t *attr_list) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  SAI_LOG_ENTER();

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/**
 * Routine Description:
 *   @brief Clear vlan statistics counters.
 *
 * Arguments:
 *    @param[in] vlan_id - vlan id
 *    @param[in] counter_ids - specifies the array of counter ids
 *    @param[in] number_of_counters - number of counters in the array
 *
 * Return Values:
 *    @return SAI_STATUS_SUCCESS on success
 *            Failure status code on error
 */
sai_status_t sai_clear_vlan_stats(
    _In_ sai_vlan_id_t vlan_id,
    _In_ const sai_vlan_stat_counter_t *counter_ids,
    _In_ uint32_t number_of_counters) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  SAI_LOG_ENTER();

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

static sai_status_t switch_vlan_counters_to_sai_vlan_counters(
    _In_ uint32_t number_of_counters,
    _In_ const sai_vlan_stat_counter_t *counter_ids,
    _In_ switch_counter_t *switch_counters,
    _Out_ uint64_t *counters) {
  uint32_t index = 0;
  for (index = 0; index < number_of_counters; index++) {
    switch (counter_ids[index]) {
      case SAI_VLAN_STAT_IN_OCTETS:
        counters[index] = switch_counters[SWITCH_BD_STATS_IN_UCAST].num_bytes +
                          switch_counters[SWITCH_BD_STATS_IN_MCAST].num_bytes +
                          switch_counters[SWITCH_BD_STATS_IN_BCAST].num_bytes;
        break;
      case SAI_VLAN_STAT_IN_UCAST_PKTS:
        counters[index] = switch_counters[SWITCH_BD_STATS_IN_UCAST].num_packets;
        break;
      case SAI_VLAN_STAT_IN_NON_UCAST_PKTS:
        counters[index] =
            switch_counters[SWITCH_BD_STATS_IN_MCAST].num_packets +
            switch_counters[SWITCH_BD_STATS_IN_BCAST].num_packets;
        break;
      case SAI_VLAN_STAT_IN_DISCARDS:
      case SAI_VLAN_STAT_IN_ERRORS:
      case SAI_VLAN_STAT_IN_UNKNOWN_PROTOS:
        counters[index] = 0;
        break;
      case SAI_VLAN_STAT_OUT_OCTETS:
        counters[index] = switch_counters[SWITCH_BD_STATS_OUT_UCAST].num_bytes +
                          switch_counters[SWITCH_BD_STATS_OUT_MCAST].num_bytes +
                          switch_counters[SWITCH_BD_STATS_OUT_BCAST].num_bytes;
        break;
      case SAI_VLAN_STAT_OUT_UCAST_PKTS:
        counters[index] =
            switch_counters[SWITCH_BD_STATS_OUT_UCAST].num_packets;
        break;
      case SAI_VLAN_STAT_OUT_NON_UCAST_PKTS:
        counters[index] =
            switch_counters[SWITCH_BD_STATS_OUT_MCAST].num_packets +
            switch_counters[SWITCH_BD_STATS_OUT_BCAST].num_packets;
        break;
      case SAI_VLAN_STAT_OUT_DISCARDS:
      case SAI_VLAN_STAT_OUT_ERRORS:
      case SAI_VLAN_STAT_OUT_QLEN:
        counters[index] = 0;
        break;
      case SAI_VLAN_STAT_IN_PACKETS:
      case SAI_VLAN_STAT_OUT_PACKETS:
        SAI_LOG_WARN("Unsupported attribute");
        break;
    }
  }
  return SAI_STATUS_SUCCESS;
}

/*
* Routine Description:
*   Get vlan statistics counters.
*
* Arguments:
*    [in] vlan_id - VLAN id
*    [in] counter_ids - specifies the array of counter ids
*    [in] number_of_counters - number of counters in the array
*    [out] counters - array of resulting counter values.
*
* Return Values:
*    SAI_STATUS_SUCCESS on success
*    Failure status code on error
*/
sai_status_t sai_get_vlan_stats(_In_ sai_vlan_id_t vlan_id,
                                _In_ const sai_vlan_stat_counter_t *counter_ids,
                                _In_ uint32_t number_of_counters,
                                _Out_ uint64_t *counters) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_counter_t *switch_counters = NULL;
  switch_bd_stats_id_t *vlan_stat_ids = NULL;
  switch_handle_t vlan_handle = 0;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  uint32_t index = 0;

  switch_status =
      switch_api_vlan_id_to_handle_get((switch_vlan_t)vlan_id, &vlan_handle);
  status = sai_switch_status_to_sai_status(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to rremove ports from vlan %d: %s",
                  vlan_id,
                  sai_status_to_string(status));
    return status;
  }

  switch_counters = SAI_MALLOC(sizeof(switch_counter_t) * SWITCH_BD_STATS_MAX);
  if (!switch_counters) {
    status = SAI_STATUS_NO_MEMORY;
    SAI_LOG_ERROR("failed to get vlan stats %d: %s",
                  vlan_id,
                  sai_status_to_string(status));
    return status;
  }

  vlan_stat_ids =
      SAI_MALLOC(sizeof(switch_bd_stats_id_t) * SWITCH_BD_STATS_MAX);
  if (!vlan_stat_ids) {
    status = SAI_STATUS_NO_MEMORY;
    SAI_LOG_ERROR("failed to get vlan stats %d: %s",
                  vlan_id,
                  sai_status_to_string(status));
    SAI_FREE(switch_counters);
    return status;
  }

  for (index = 0; index < SWITCH_BD_STATS_MAX; index++) {
    vlan_stat_ids[index] = index;
  }

  switch_status = switch_api_vlan_stats_get(
      device, vlan_handle, SWITCH_BD_STATS_MAX, vlan_stat_ids, switch_counters);
  status = sai_switch_status_to_sai_status(switch_status);

  if (status != SWITCH_STATUS_SUCCESS) {
    status = SAI_STATUS_NO_MEMORY;
    SAI_LOG_ERROR("failed to get vlan stats %d: %s",
                  vlan_id,
                  sai_status_to_string(status));
    SAI_FREE(vlan_stat_ids);
    SAI_FREE(switch_counters);
    return status;
  }

  switch_vlan_counters_to_sai_vlan_counters(
      number_of_counters, counter_ids, switch_counters, counters);

  SAI_FREE(vlan_stat_ids);
  SAI_FREE(switch_counters);

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
* VLAN methods table retrieved with sai_api_query()
*/
sai_vlan_api_t vlan_api = {
    .create_vlan = sai_create_vlan_entry,
    .remove_vlan = sai_remove_vlan_entry,
    .set_vlan_attribute = sai_set_vlan_entry_attribute,
    .get_vlan_attribute = sai_get_vlan_entry_attribute,
    .create_vlan_member = sai_create_vlan_member,
    .remove_vlan_member = sai_remove_vlan_member,
    .set_vlan_member_attribute = sai_set_vlan_member_attribute,
    .get_vlan_member_attribute = sai_get_vlan_member_attribute,
    .get_vlan_stats = sai_get_vlan_stats,
    .clear_vlan_stats = sai_clear_vlan_stats};

sai_status_t sai_vlan_initialize(sai_api_service_t *sai_api_service) {
  sai_api_service->vlan_api = vlan_api;
  return SAI_STATUS_SUCCESS;
}
