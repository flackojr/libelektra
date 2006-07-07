/***************************************************************************
                   kdb_wrapper.c  -  The server for the daemon backend
                             -------------------
    copyright            : (C) 2006 by Yannick Lecaillez
    email                : sizon5@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the BSD License (revised).                      *
 *                                                                         *
 ***************************************************************************/


/* Subversion stuff

$Id$

*/


#include <stdlib.h> /* malloc */
#include <sys/types.h>
#include <pwd.h>

#include "kdbbackend.h"

#include "datatype.h"
#include "message.h"

Message *wrapper_kdbOpen(KDBHandle *handle, Message *request, uid_t euid, gid_t egid)
{
	struct	passwd	*user;
//	char		*userName;
	int		ret, error;
	Message		*reply;
	
	/* Sanity check */
	error = 0;
	if ( messageGetNbArgs(request) != 0 ) {
		fprintf(stderr, "BAD POUET\n");
		return NULL;
	}

	/* Opens the default backend for daemon */
	ret = kdbOpenBackend(handle,"ddefault");
	error = errno;
	kdbhSetUID(*handle, euid);
	kdbhSetGID(*handle, egid);

	reply = messageNew(MESSAGE_REPLY, KDB_BE_OPEN,
			DATATYPE_INTEGER, &ret,
			DATATYPE_INTEGER, &error,
			DATATYPE_LAST);
	if ( reply == NULL ) {
		fprintf(stderr, "Nul answer\n");
	}

	return reply;	
}

Message *wrapper_kdbClose(KDBHandle *handle, Message *request)
{
	int		error, ret;
	Message		*reply;

	error = 0;
	if ( messageGetNbArgs(request) != 0 )
		return NULL;
	
	ret = kdbClose(handle);
	error = errno;
	reply = messageNew(MESSAGE_REPLY, KDB_BE_CLOSE,
			DATATYPE_INTEGER, &ret,
			DATATYPE_INTEGER, &error,
			DATATYPE_LAST);

	return reply;
}

Message *wrapper_kdbStatKey(KDBHandle handle, Message *request)
{
	Key		key;
	int             error, ret;
	Message		*reply;
	
	keyInit(&key);
	if ( messageExtractArgs(request,
			DATATYPE_KEY, key,
			DATATYPE_LAST) ) {
		keyClose(&key);
		return NULL;
	}
	
	ret = kdbStatKey(handle, &key);
	error = errno;
	
	reply = messageNew(MESSAGE_REPLY, KDB_BE_STATKEY,
			DATATYPE_INTEGER, &ret,
			DATATYPE_INTEGER, &error,
			DATATYPE_KEY, &key,
			DATATYPE_LAST);
	
	keyClose(&key);
	
	return reply;
}

Message *wrapper_kdbGetKey(KDBHandle handle, Message *request)
{
	Key		key;
	int		error, ret;
	Message		*reply;

	keyInit(&key);
	error = 0;
	
	if ( messageExtractArgs(request,
				DATATYPE_KEY, &key,
				DATATYPE_LAST) ) {
		keyClose(&key);
		return NULL;
	}

	ret = kdbGetKey(handle, &key);
	error = errno;

	reply = messageNew(MESSAGE_REPLY, KDB_BE_GETKEY,
			DATATYPE_INTEGER, &ret,
			DATATYPE_INTEGER, &error,
			DATATYPE_KEY, &key,
			DATATYPE_LAST);

	keyClose(&key);
	
	return reply;
}

Message *wrapper_kdbSetKey(KDBHandle handle, void *request)
{
	Key             key;
	int             error, ret;
	Message         *reply;
	
	keyInit(&key);
	error = 0;
	
	if ( messageExtractArgs(request,
			DATATYPE_KEY, &key,
			DATATYPE_LAST) ) {
		keyClose(&key);
		return NULL;
	}
	
	ret = kdbSetKey(handle, &key);
	error = errno;
	
	reply = messageNew(MESSAGE_REPLY, KDB_BE_SETKEY,
			DATATYPE_INTEGER, &ret,
			DATATYPE_INTEGER, &error,
			DATATYPE_KEY, &key,
			DATATYPE_LAST);
	
	keyClose(&key);
	
	return reply;
}

Message *wrapper_kdbSetKeys(KDBHandle handle, void *request)
{
	KeySet		*ks;
	int             error, ret;
	Message         *reply;
	
	if ( (ks = ksNew()) == NULL )
		return NULL;
	
	if ( messageExtractArgs(request, 
			DATATYPE_KEYSET, ks,
			DATATYPE_LAST) ) {
		ksDel(ks);
		return NULL;
	}

	ret = kdbSetKeys(handle, ks);
	error = errno;
	reply = messageNew(MESSAGE_REPLY, KDB_BE_SETKEYS,
			DATATYPE_INTEGER, &ret,
			DATATYPE_INTEGER, &error,
			DATATYPE_KEYSET, ks,
			DATATYPE_LAST);

	ksDel(ks);

	return reply;
}

Message *wrapper_kdbRename(KDBHandle handle, void *request)
{
	Key             key;
	char		*newKeyName;
	int             error, ret;
	Message         *reply;

	keyInit(&key);
	if ( messageExtractArgs(request,
			DATATYPE_KEY, &key,
			DATATYPE_STRING, &newKeyName,
			DATATYPE_LAST) ) {
		keyClose(&key);
		return NULL;
	}

	ret = kdbRename(handle, &key, newKeyName);
	error = errno;
	free(newKeyName);
	keyClose(&key);

	reply = messageNew(MESSAGE_REPLY, KDB_BE_RENAME,
			DATATYPE_INTEGER, &ret,
			DATATYPE_INTEGER, &error,
			DATATYPE_LAST);

	return reply;
}

Message *wrapper_kdbRemoveKey(KDBHandle handle, Message *request)
{
	Key             key;
	int             error, ret;
	Message         *reply;
	
	keyInit(&key);
	if ( messageExtractArgs(request,
			DATATYPE_KEY, &key,
			DATATYPE_LAST) ) {
		keyClose(&key);
		return NULL;
	}
	
	ret = kdbRemoveKey(handle, &key);
	error = errno;
	keyClose(&key);
	
	reply = messageNew(MESSAGE_REPLY, KDB_BE_REMOVEKEY,
			DATATYPE_INTEGER, &ret,
			DATATYPE_INTEGER, &error,
			DATATYPE_LAST);
	
	return reply;
}

Message *wrapper_kdbGetChild(KDBHandle handle, Message *request)
{
	Key             parentKey;
	KeySet		*ks;
	int             error, ret;
	unsigned long	options;
	Message		*reply;
	
	keyInit(&parentKey);

	if ( messageExtractArgs(request,
			DATATYPE_KEY, &parentKey,
			DATATYPE_ULONG, &options,
			DATATYPE_LAST) ) {
		keyClose(&parentKey);
		return NULL;
	}
	
	ks = ksNew();
	ret = kdbGetKeyChildKeys(handle, &parentKey, ks, options);
	error = errno;
	keyClose(&parentKey);
	
	reply = messageNew(MESSAGE_REPLY, KDB_BE_GETCHILD,
			DATATYPE_INTEGER, &ret,
			DATATYPE_INTEGER, &error,
			DATATYPE_KEYSET, ks,
			DATATYPE_LAST);
	ksDel(ks);

	return reply;
}

Message *wrapper_kdbMonitorKey(KDBHandle handle, Message *request)
{
	Key		key;
	int             error;
	unsigned long   diffMask, iterations, sleep, ret;
	Message         *reply;
	
	keyInit(&key);
	
	if ( messageExtractArgs(request,
			DATATYPE_KEY, &key,
			DATATYPE_ULONG, &diffMask,
			DATATYPE_ULONG, &iterations,
			DATATYPE_ULONG, &sleep,
			DATATYPE_LAST) ) {
		keyClose(&key);
		return NULL;
	}
	
	ret = kdbMonitorKey(handle, &key, diffMask, iterations, sleep);
	error = errno;
	
	reply = messageNew(MESSAGE_REPLY, KDB_BE_MONITORKEY,
			DATATYPE_ULONG, &ret,
			DATATYPE_INTEGER, &error,
			DATATYPE_KEY, &key,
			DATATYPE_LAST);
	
	keyClose(&key);
	
	return reply;
}

Message *wrapper_kdbMonitorKeys(KDBHandle handle, Message *request)
{
	KeySet		*ks;
	int             error;
	unsigned long	diffMask, iterations, sleep, ret;
	Message         *reply;
	
	if ( (ks = ksNew()) == NULL )
		return NULL;
	
	if ( messageExtractArgs(request,
			DATATYPE_KEYSET, ks,
			DATATYPE_ULONG, &diffMask,
			DATATYPE_ULONG, &iterations,
			DATATYPE_ULONG, &sleep,
			DATATYPE_LAST) ) {
		ksDel(ks);
		return NULL;
	}
	
	ret = kdbMonitorKeys(handle, ks, diffMask, iterations, sleep);
	error = errno;
	
	reply = messageNew(MESSAGE_REPLY, KDB_BE_MONITORKEYS,
			DATATYPE_ULONG, &ret,
			DATATYPE_INTEGER, &error,
			DATATYPE_KEYSET, ks,
			DATATYPE_LAST);
	
	ksDel(ks);
	
	return reply;	
	
}

