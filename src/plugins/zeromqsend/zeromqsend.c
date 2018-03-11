/**
 * @file
 *
 * @brief Source for zeromqsend plugin
 *
 * @copyright BSD License (see LICENSE.md or https://www.libelektra.org)
 *
 */

#ifndef HAVE_KDBCONFIG
#include "kdbconfig.h"
#endif

#include "zeromqsend.h"

#include <kdbhelper.h>
#include <kdblogger.h>

int elektraZeroMqSendOpen (Plugin * handle, Key * errorKey ELEKTRA_UNUSED)
{
	Key * endpointKey = ksLookupByName (elektraPluginGetConfig (handle), "/config/endpoint", 0);
	const char * endpoint;
	if (endpointKey)
	{
		endpoint = keyString (endpointKey);
	}
	else
	{
		endpoint = ELEKTRA_ZEROMQ_DEFAULT_PUB_ENDPOINT;
	}

	ElektraZeroMqSendPluginData * data = elektraPluginGetData (handle);
	if (!data)
	{
		data = elektraMalloc (sizeof (*data));
		data->zmqContext = NULL;
		data->zmqPublisher = NULL;
		data->endpoint = endpoint;
	}
	elektraPluginSetData (handle, data);

	// create connection
	if (!elektraZeroMqSendConnect (data))
	{
		ELEKTRA_LOG_WARNING ("could not connect to endpoint");
	}

	return 1; /* success */
}

int elektraZeroMqSendGet (Plugin * handle ELEKTRA_UNUSED, KeySet * returned, Key * parentKey)
{
	if (!strcmp (keyName (parentKey), "system/elektra/modules/zeromqsend"))
	{
		KeySet * contract = ksNew (
			30, keyNew ("system/elektra/modules/zeromqsend", KEY_VALUE, "zeromqsend plugin waits for your orders", KEY_END),
			keyNew ("system/elektra/modules/zeromqsend/exports", KEY_END),
			keyNew ("system/elektra/modules/zeromqsend/exports/open", KEY_FUNC, elektraZeroMqSendOpen, KEY_END),
			keyNew ("system/elektra/modules/zeromqsend/exports/get", KEY_FUNC, elektraZeroMqSendGet, KEY_END),
			keyNew ("system/elektra/modules/zeromqsend/exports/set", KEY_FUNC, elektraZeroMqSendSet, KEY_END),
			keyNew ("system/elektra/modules/zeromqsend/exports/close", KEY_FUNC, elektraZeroMqSendClose, KEY_END),
#include ELEKTRA_README (zeromqsend)
			keyNew ("system/elektra/modules/zeromqsend/infos/version", KEY_VALUE, PLUGINVERSION, KEY_END), KS_END);
		ksAppend (returned, contract);
		ksDel (contract);

		return 1; /* success */
	}

	return 1; /* success */
}

int elektraZeroMqSendSet (Plugin * handle, KeySet * returned ELEKTRA_UNUSED, Key * parentKey ELEKTRA_UNUSED)
{
	ElektraZeroMqSendPluginData * pluginData = elektraPluginGetData (handle);
	ELEKTRA_NOT_NULL (pluginData);

	elektraZeroMqSendPublish ("Commit", keyName (parentKey), pluginData);

	return 1; /* success */
}

int elektraZeroMqSendClose (Plugin * handle, Key * parentKey ELEKTRA_UNUSED)
{
	ElektraZeroMqSendPluginData * pluginData = elektraPluginGetData (handle);
	if (pluginData == NULL)
	{
		return 1;
	}

	if (pluginData->zmqPublisher)
	{
		zmq_close (pluginData->zmqPublisher);
		pluginData->zmqPublisher = NULL;
	}

	if (pluginData->zmqContext)
	{
		zmq_ctx_destroy (pluginData->zmqContext);
		pluginData->zmqContext = NULL;
	}

	elektraFree (pluginData);
	elektraPluginSetData (handle, NULL);

	return 1; /* success */
}

Plugin * ELEKTRA_PLUGIN_EXPORT (zeromqsend)
{
	// clang-format off
	return elektraPluginExport("zeromqsend",
		ELEKTRA_PLUGIN_OPEN,	&elektraZeroMqSendOpen,
		ELEKTRA_PLUGIN_GET,	&elektraZeroMqSendGet,
		ELEKTRA_PLUGIN_SET,	&elektraZeroMqSendSet,
		ELEKTRA_PLUGIN_CLOSE,	&elektraZeroMqSendClose,
		ELEKTRA_PLUGIN_END);
}
