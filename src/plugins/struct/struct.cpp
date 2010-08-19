/***************************************************************************
                     struct.c  -  Skeleton of a plugin
                             -------------------
    begin                : Fri May 21 2010
    copyright            : (C) 2010 by Markus Raab
    email                : elektra@markus-raab.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the BSD License (revised).                      *
 *                                                                         *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This is the skeleton of the methods you'll have to implement in order *
 *   to provide a valid plugin.                                            *
 *   Simple fill the empty functions with your code and you are            *
 *   ready to go.                                                          *
 *                                                                         *
 ***************************************************************************/


#include "struct.hpp"

#include <key>
#include <keyset>

//TODO: fix opening of modules (show warnings)
#include <iostream>

#include "factory.hpp"

using namespace ckdb;
#include <kdberrors.h>

extern "C"
{

int elektraStructOpen(ckdb::Plugin *handle, ckdb::Key *errorKey)
{
	/* plugin initialization logic */
	kdb::KeySet config (elektraPluginGetConfig(handle));

	try {
		elektra::Checker *c = static_cast<elektra::Checker*>(elektra::buildChecker(config));
		config.release();
		// std::cout << "got back [open] " << c << std::endl;
		elektraPluginSetData (handle, c);
	}
	catch (const char* msg)
	{
		config.release();

		// TODO: warnings are not passed when plugin creation failed!
		std::cout << "opening of plugin struct failed with msg: " << msg << std::endl;
		ELEKTRA_ADD_WARNING (58, errorKey, msg);
		return -1;
	}


	return 1; /* success */
}

int elektraStructClose(ckdb::Plugin *handle, ckdb::Key *)
{
	/* free all plugin resources and shut it down */
	elektra::Checker *c = static_cast<elektra::Checker*>(elektraPluginGetData (handle));

	// std::cout << "got back [close] " << c << std::endl;
	delete c;

	return 1; /* success */
}

int elektraStructGet(ckdb::Plugin *, ckdb::KeySet *returned, ckdb::Key *)
{
	/* configuration only */
	KeySet *n;
	ksAppend (returned, n=ksNew (30,
		keyNew ("system/elektra/modules/struct",
			KEY_VALUE, "struct plugin waits for your orders", KEY_END),
		keyNew ("system/elektra/modules/struct/exports", KEY_END),
		keyNew ("system/elektra/modules/struct/exports/open",
			KEY_FUNC, elektraStructOpen,
			KEY_END),
		keyNew ("system/elektra/modules/struct/exports/close",
			KEY_FUNC, elektraStructClose,
			KEY_END),
		keyNew ("system/elektra/modules/struct/exports/get",
			KEY_FUNC, elektraStructGet,
			KEY_END),
		keyNew ("system/elektra/modules/struct/exports/set",
			KEY_FUNC, elektraStructSet,
			KEY_END),
		keyNew ("system/elektra/modules/struct/infos",
			KEY_VALUE, "All information you want to know", KEY_END),
		keyNew ("system/elektra/modules/struct/infos/author",
			KEY_VALUE, "Markus Raab <elektra@markus-raab.org>", KEY_END),
		keyNew ("system/elektra/modules/struct/infos/licence",
			KEY_VALUE, "BSD", KEY_END),
		keyNew ("system/elektra/modules/struct/infos/description",
			KEY_VALUE, "Copies meta data to keys using structbing", KEY_END),
		keyNew ("system/elektra/modules/struct/infos/provides",
			KEY_VALUE, "struct", KEY_END),
		keyNew ("system/elektra/modules/struct/infos/placements",
			KEY_VALUE, "presetstorage", KEY_END),
		keyNew ("system/elektra/modules/struct/infos/needs",
			KEY_VALUE, "", KEY_END),
		keyNew ("system/elektra/modules/struct/infos/version",
			KEY_VALUE, PLUGINVERSION, KEY_END),
		KS_END));
	ksDel (n);

	return 1; /* success */
}

int elektraStructSet(ckdb::Plugin *handle, ckdb::KeySet *returned, ckdb::Key *parentKey)
{
	/* set all keys */

	try {
		elektra::Checker *c = static_cast<elektra::Checker*>(elektraPluginGetData (handle));
		// std::cout << "got back [set] " << c << std::endl;
		doCheck (c, returned);
	}
	catch (const char* msg)
	{
		ELEKTRA_SET_ERROR (53, parentKey, msg);
		return -1;
	}

	return 1; /* success */
}

ckdb::Plugin *ELEKTRA_PLUGIN_EXPORT(struct)
{
	return elektraPluginExport("struct",
		ELEKTRA_PLUGIN_OPEN,	&elektraStructOpen,
		ELEKTRA_PLUGIN_CLOSE,	&elektraStructClose,
		ELEKTRA_PLUGIN_GET,	&elektraStructGet,
		ELEKTRA_PLUGIN_SET,	&elektraStructSet,
		ELEKTRA_PLUGIN_END);
}

}
