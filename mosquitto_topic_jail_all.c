/*
Copyright (c) 2020-2021 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License 2.0
and Eclipse Distribution License v1.0 which accompany this distribution.

The Eclipse Public License is available at
   https://www.eclipse.org/legal/epl-2.0/
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.

SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

Contributors:
   Abilio Marques - initial implementation and documentation.
*/
/**
 * @file mosquitto_topic_jail.c
 * @brief plugin which jails clients to a topic
 * @author Abilio Marques - initial implementation and documentation.
 * @author Shawn Cicoria - modified to jail all clients to set of topics and other logic
 * @date 2023-03-22
 * @version 1.0
*/

/*
 * jails devices but not admin on different listeners 
 * be sure on how to configure before using this
 *
 * Two jailed clients cannot interact with each other. Admin clients can interact
 * with any jailed client by publishing or subscribing to the mounted topic.

 * Compile with:
 *   gcc -I<path to mosquitto-repo/include> -fPIC -shared mosquitto_topic_jail.c -o mosquitto_topic_jail.so
 *
 * Use in config with:
 *
 *   plugin /path/to/mosquitto_topic_jail.so
 *
 * Note that this only works on Mosquitto 2.0 or later.
 */
#include <stdio.h>
#include <string.h>

#include "mosquitto_broker.h"
#include "mosquitto_plugin.h"
#include "mosquitto.h"
#include "mqtt_protocol.h"
#include "openssl/ssl.h"

#define PLUGIN_NAME "topic-jail-all"
#define PLUGIN_VERSION "1.0"

#define UNUSED(A) (void)(A)

#define SLASH_Z sizeof((char)'/')

MOSQUITTO_PLUGIN_DECLARE_VERSION(5);

struct jail__config{
	char *username;
	char *get_topic;
	char *put_topic;
	char *sub_topic;
};

static struct jail__config jail_config;
static mosquitto_plugin_id_t* mosq_pid = NULL;

static bool is_jailed(const char* str)
{
	return !(strncmp(jail_config.username, str, strlen(jail_config.username)) == 0);
}


static int callback_message_in(int event, void* event_data, void* userdata)
{
	struct mosquitto_evt_message* ed = event_data;
	char* new_topic;
	size_t new_topic_len;

	UNUSED(event);
	UNUSED(userdata);

	const char* clientid = mosquitto_client_id(ed->client);

	if (!is_jailed(clientid)) {
		/* will only modify the topic of jailed clients */
		return MOSQ_ERR_SUCCESS;
	}

	/* put the clientid on front of the topic */
	/* calculate the length of the new payload */
	new_topic_len = strlen(clientid) + SLASH_Z + strlen(ed->topic) + 1;

	/* Allocate some memory - use
	 * mosquitto_calloc/mosquitto_malloc/mosquitto_strdup when allocating, to
	 * allow the broker to track memory usage */
	new_topic = mosquitto_calloc(1, new_topic_len);
	if (new_topic == NULL) {
		return MOSQ_ERR_NOMEM;
	}

	/* prepend the clientid to the topic */
	snprintf(new_topic, new_topic_len, "%s/%s", clientid, ed->topic);

	/* Assign the new topic to the event data structure. You
	 * must *not* free the original topic, it will be handled by the
	 * broker. */
	ed->topic = new_topic;

	return MOSQ_ERR_SUCCESS;
}

static int callback_message_out(int event, void* event_data, void* userdata)
{
	struct mosquitto_evt_message* ed = event_data;
	size_t clientid_len;

	UNUSED(event);
	UNUSED(userdata);

	const char* clientid = mosquitto_client_id(ed->client);

	if (!is_jailed(clientid))
	{
		/* will only modify the topic of jailed clients */
		return MOSQ_ERR_SUCCESS;
	}

	/* remove the clientid from the front of the topic */
	clientid_len = strlen(clientid);

	if (strlen(ed->topic) <= clientid_len + 1)
	{
		/* the topic is not long enough to contain the
		 * clientid + '/' */
		return MOSQ_ERR_SUCCESS;
	}

	if (!strncmp(clientid, ed->topic, clientid_len) && ed->topic[clientid_len] == '/')
	{
		/* Allocate some memory - use
		 * mosquitto_calloc/mosquitto_malloc/mosquitto_strdup when allocating, to
		 * allow the broker to track memory usage */

		 /* skip the clientid + '/' */
		char* new_topic = mosquitto_strdup(ed->topic + clientid_len + 1);

		if (new_topic == NULL)
		{
			return MOSQ_ERR_NOMEM;
		}

		/* Assign the new topic to the event data structure. You
		 * must *not* free the original topic, it will be handled by the
		 * broker. */
		ed->topic = new_topic;
	}

	return MOSQ_ERR_SUCCESS;
}

static int callback_subscribe(int event, void* event_data, void* userdata)
{
	struct mosquitto_evt_subscribe* ed = event_data;
	char* new_sub;
	size_t new_sub_len;

	UNUSED(event);
	UNUSED(userdata);

	const char* clientid = mosquitto_client_id(ed->client);

	if (!is_jailed(clientid))
	{
		/* will only modify the topic of jailed clients */
		return MOSQ_ERR_SUCCESS;
	}

	/* put the clientid on front of the topic */
	/* calculate the length of the new payload */
	new_sub_len = strlen(clientid) + SLASH_Z + strlen(ed->data.topic_filter) + 1;

	/* Allocate some memory - use
	 * mosquitto_calloc/mosquitto_malloc/mosquitto_strdup when allocating, to
	 * allow the broker to track memory usage */
	new_sub = mosquitto_calloc(1, new_sub_len);
	if (new_sub == NULL)
	{
		return MOSQ_ERR_NOMEM;
	}

	/* prepend the clientid to the subscription */
	snprintf(new_sub, new_sub_len, "%s/%s", clientid, ed->data.topic_filter);

	/* Assign the new topic to the event data structure. You
	 * must *not* free the original topic, it will be handled by the
	 * broker. */
	ed->data.topic_filter = new_sub;

	return MOSQ_ERR_SUCCESS;
}

static int callback_unsubscribe(int event, void* event_data, void* userdata)
{
	struct mosquitto_evt_unsubscribe* ed = event_data;
	char* new_sub;
	size_t new_sub_len;

	UNUSED(event);
	UNUSED(userdata);

	const char* clientid = mosquitto_client_id(ed->client);

	if (!is_jailed(clientid))
	{
		/* will only modify the topic of jailed clients */
		return MOSQ_ERR_SUCCESS;
	}

	/* put the clientid on front of the topic */
	/* calculate the length of the new payload */
	new_sub_len = strlen(clientid) + SLASH_Z + strlen(ed->data.topic_filter) + 1;

	/* Allocate some memory - use
	 * mosquitto_calloc/mosquitto_malloc/mosquitto_strdup when allocating, to
	 * allow the broker to track memory usage */
	new_sub = mosquitto_calloc(1, new_sub_len);
	if (new_sub == NULL)
	{
		return MOSQ_ERR_NOMEM;
	}

	/* prepend the clientid to the subscription */
	snprintf(new_sub, new_sub_len, "%s/%s", clientid, ed->data.topic_filter);

	/* Assign the new topic to the event data structure. You
	 * must *not* free the original topic, it will be handled by the
	 * broker. */
	ed->data.topic_filter = new_sub;

	return MOSQ_ERR_SUCCESS;
}

static int callback_acl_check(int event, void* event_data, void* userdata)
{
	struct mosquitto_evt_acl_check* ed = event_data;

	UNUSED(event);
	UNUSED(userdata);

	const char* clientid = mosquitto_client_id(ed->client);

	if (!is_jailed(clientid))
	{
		/* will only modify the topic of jailed clients */
		return MOSQ_ERR_SUCCESS;
	}

	size_t putLen = strlen(jail_config.put_topic) - 1;
	size_t getLen = strlen(jail_config.get_topic) - 1;
	size_t subLen = strlen(jail_config.sub_topic) - 1;

	char* new_topic;
	size_t new_topic_len;

	if (ed->access == MOSQ_ACL_SUBSCRIBE) {
		if (strncmp(ed->topic, jail_config.sub_topic, subLen) == 0) {
			return MOSQ_ERR_SUCCESS;
		}
	}
	// NOTE: during a inbond message with the prefix - it has the prefix
	// NOTE: this is the subscribe topic
	if (ed->access == MOSQ_ACL_READ) {
		new_topic_len = strlen(clientid) + SLASH_Z + subLen + 1;
		new_topic = mosquitto_calloc(1, new_topic_len);
		if (!new_topic) {
			return MOSQ_ERR_NOMEM;
		}
		snprintf(new_topic, new_topic_len, "%s/%s", clientid, jail_config.sub_topic);

		if (strncmp(ed->topic, new_topic, new_topic_len - 1) == 0) {
			return MOSQ_ERR_SUCCESS;
		}
	}

	if (ed->access == MOSQ_ACL_WRITE) {
		if (strncmp(ed->topic, jail_config.put_topic, putLen) == 0) {
			return MOSQ_ERR_SUCCESS;
		}

		if (strncmp(ed->topic, jail_config.get_topic, getLen) == 0) {
			return MOSQ_ERR_SUCCESS;
		}
	}

	return MOSQ_ERR_ACL_DENIED;
}

static int set_defaults(void)
{
	jail_config.username = mosquitto_strdup("admin");
	if(jail_config.username == NULL){
		return MOSQ_ERR_NOMEM;
	}
	jail_config.get_topic = mosquitto_strdup("$dps/registrations/GET/iotdps-get-operationstatus/");
	if(jail_config.get_topic == NULL){
		return MOSQ_ERR_NOMEM;
	}
	jail_config.put_topic = mosquitto_strdup("$dps/registrations/PUT/iotdps-register/");
	if(jail_config.put_topic == NULL){
		return MOSQ_ERR_NOMEM;
	}
	jail_config.sub_topic = mosquitto_strdup("$dps/registrations/res/#");
	if(jail_config.sub_topic == NULL){
		return MOSQ_ERR_NOMEM;
	}

	return MQTT_RC_SUCCESS;
}


int mosquitto_plugin_init(mosquitto_plugin_id_t* identifier, void** user_data, struct mosquitto_opt* options, int option_count)
{
	UNUSED(user_data);
	int i;
	int rc;

	mosq_pid = identifier;
	mosquitto_plugin_set_info(identifier, PLUGIN_NAME, PLUGIN_VERSION);

	memset(&jail_config, 0,sizeof(struct jail__config));
	
	if(set_defaults()){
		mosquitto_log_printf(MOSQ_LOG_ERR, "Error: jail_client: Unable to set default values or config");
		return MOSQ_ERR_NOMEM;
	}

	for(i=0; i<option_count; i++){
		if(!strcasecmp(options[i].key, "username")){
			jail_config.username = mosquitto_strdup(options[i].value);
			if(jail_config.username == NULL){
				return MOSQ_ERR_IMPLEMENTATION_SPECIFIC;
			}
		}else if(!strcasecmp(options[i].key, "get_topic")){
			jail_config.get_topic = mosquitto_strdup(options[i].value);
			if(jail_config.get_topic == NULL){
				return MOSQ_ERR_IMPLEMENTATION_SPECIFIC;
			}
		}else if(!strcasecmp(options[i].key, "put_topic")){
			jail_config.put_topic = mosquitto_strdup(options[i].value);
			if(jail_config.put_topic == NULL){
				return MOSQ_ERR_IMPLEMENTATION_SPECIFIC;
			}
		}else if(!strcasecmp(options[i].key, "sub_topic")){
			jail_config.sub_topic = mosquitto_strdup(options[i].value);
			if(jail_config.sub_topic == NULL){
				return MOSQ_ERR_IMPLEMENTATION_SPECIFIC;
			}
		}
	}

	mosquitto_log_printf(MOSQ_LOG_INFO, "jail config: \n\tusername:%s\n\tget_topic:%s\n\tput_topic:%s\n\tsub_topic:%s",
						jail_config.username, jail_config.get_topic, jail_config.put_topic, jail_config.sub_topic);

	rc = mosquitto_callback_register(mosq_pid, MOSQ_EVT_ACL_CHECK, callback_acl_check, NULL, NULL);
	if (rc)
		return rc;
	rc = mosquitto_callback_register(mosq_pid, MOSQ_EVT_MESSAGE_IN, callback_message_in, NULL, NULL);
	if (rc)
		return rc;
	rc = mosquitto_callback_register(mosq_pid, MOSQ_EVT_MESSAGE_OUT, callback_message_out, NULL, NULL);
	if (rc)
		return rc;
	rc = mosquitto_callback_register(mosq_pid, MOSQ_EVT_SUBSCRIBE, callback_subscribe, NULL, NULL);
	if (rc)
		return rc;
	rc = mosquitto_callback_register(mosq_pid, MOSQ_EVT_UNSUBSCRIBE, callback_unsubscribe, NULL, NULL);
	return rc;
}
