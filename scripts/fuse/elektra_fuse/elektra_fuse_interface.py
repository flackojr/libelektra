#stdlib imports
import errno, stat, time, re, psutil, logging, subprocess, os
from pathlib import Path

#fuse imports
from fuse import FUSE, FuseOSError, Operations, LoggingMixIn

#project imports
from elektra_util import *
import mock_context

startup_time = time.time()

#TODO: give appropriate error codes (instead of readonly fs) when non-authenticated user tries to do illegal things

def getattr(path, fh=None):

    is_value_of_file = Path(path).name == dir_file_special_name

    is_dir, is_file = key_type(path)

    if is_value_of_file:
        mode = stat.S_IFREG
        #resolve the liml (/.../@elektra.value) key to the real key (the parent)
        path = str(Path(path).parent)
    elif not is_dir and not is_file:
        mode = errno.ENOENT
    elif is_dir and is_file:
        mode = stat.S_IFDIR
    elif is_dir:
        mode = stat.S_IFDIR
    elif is_file and has_meta(path, "meta:/fuse-directory"):
        mode = stat.S_IFDIR
    elif is_file:
        mode = stat.S_IFREG

    if mode == stat.S_IFREG:
        try:
            filesize = size_of_file(path) 
        except KeyError: #key does not exist
            mode = errno.ENOENT


    try:
        kdb_file_stat = _stat_kdb_file(path)
        st_mode_only_permission_bits = kdb_file_stat.st_mode & 0o777
    except FileNotFoundError:
        # some keys, e.g. "system:/elektra" resolve to a file like "/etc/kdb/elektra.ecf", which does not exist
        # for the filesystem to remain useful, in this case dummy file attributes are used
        st_mode_only_permission_bits = 0o770
        kdb_file_stat = dict(
            st_ctime = startup_time,
            st_mtime = startup_time,
            st_atime = startup_time
        )
    
    

    #common attributes for files and directories
    key_stat = dict(
        st_mode = (mode | st_mode_only_permission_bits),

        #TODO: using the real timestamps results in vim complaining on write: "The file has been changed since reading it!!!"
        # => defaulting to a static timestamp for now
        #st_ctime = kdb_file_stat.st_ctime,
        #st_mtime = kdb_file_stat.st_mtime,
        #st_atime = kdb_file_stat.st_atime,
        st_ctime = startup_time,
        st_mtime = startup_time,
        st_atime = startup_time
    )

    if mode == stat.S_IFDIR:
        key_stat["st_nlink"] = 2
        return key_stat
    elif mode == stat.S_IFREG:
        key_stat["st_nlink"] = 1
        key_stat["st_size"] = filesize
        return key_stat
    else:
        raise OSError(mode)

#throws OSError when:
# -) `kdb file` does not return a path
# -) the returned path does not actually exist
def _stat_kdb_file(os_path):
    resolved_file_path = get_kdb_file(os_path)
    return os.stat(resolved_file_path)

def readdir(path, fh):
    if path == "/":
        return [".", "..", *elektra_namespaces]

    dir_set, file_set = ls(path)

    return ['.', '..', *dir_set, *file_set]


def read(path, size, offset, fh):
    return file_contents(path)[offset:offset+size]

def write(path, data, offset, fh):

    try:
        old_value = file_contents(path)
        new_value = old_value[:offset] + data + old_value[offset + len(data):]

        update_key_value(path, new_value)

        return len(data)
    except KeyError:
        raise OSError(errno.ENOENT)
    except kdb.KDBException:
        raise OSError(errno.EROFS) #TODO differentiate between validation error, write only keys etc


def truncate(path, length, fh=None):
    old_value = file_contents(path)
    new_value = old_value[:length].ljust(length, '\x00'.encode()) #if length increased, fill new space with zeros
    update_key_value(path, new_value)

def create(path, mode):
    if path.count('/') <= 1:
        raise OSError(errno.EROFS) #cannot create key in top level directory (reserved for /user:, /system: ...)

    create_key(path) #TODO: consider mode argument
    #TODO: maybe consider possible error codes as in https://linux.die.net/man/2/

def mkdir(path, mode):
    #TODO: think of a reasonable use for mode parameter
    create(path, mode)
    set_meta(path, "meta:/fuse-directory", "")  # 'hack' to enable creation of empty folders (these would otherwise automatically become files)


#append 'meta:/' is not already present
def _ensure_meta_prefix(name):
    return name if name.startswith("meta:/") else "meta:/" + name

#remove 'meta:/' if not already present
def _ensure_no_meta_prefix(name):
    return name[len("meta:/"):] if name.startswith("meta:/") else name
    #could use removeprefix, but that would require python 3.9+

def getxattr(path, name, position=0):
    name = _ensure_meta_prefix(name)
    try:
        return get_meta_map(path)[name].encode()
    except KeyError:
        raise OSError(errno.ENODATA)

def listxattr(path):
    try:
        meta_map = get_meta_map(path)
    except KeyError:
        return dict()
        # if key does not really exist (intermediate directories) return an empty map insted of an error
        # as to not confuse tools like xattr
    
    return [_ensure_no_meta_prefix(keyname) for keyname in get_meta_map(path).keys()]

def removexattr(path, name):
    try:
        meta_map = get_meta_map(path)
        name = _ensure_meta_prefix(name)
        del meta_map[name]
        update_meta_map(path, meta_map)
    except KeyError:
        raise OSError(errno.ENODATA)

def setxattr(path, name, value, options, position=0):
    
    #if key does not really exist (intermediate directories) key should be created (like kdb meta-set does)
    try:
        meta_map = get_meta_map(path)
    except KeyError:
        meta_map = dict()
        create(path, 0)
    
    name = _ensure_meta_prefix(name)

    try:
        meta_map[name] = value.decode()
    except UnicodeDecodeError:
        meta_map[name] = '' #meta keys cannot contain binary data (apparantly) (TODO: check)
    update_meta_map(path, meta_map)

def unlink(path):
    #delete_key(path) keyset.cut behaved unexpected and deleted child keys => using kdb directly

    returncode = subprocess.run(["kdb", "rm", os_path_to_elektra_path(path)]).returncode
    if returncode != 0:
        raise OSError(errno.EROFS) #TODO: differentiate between different error

def rmdir(path):
    if not is_directory_empty(path):
        raise OSError(errno.ENOTEMPTY)
    else:
        unlink(path)

def rename(old_path, new_path):

    if Path(old_path).name == dir_file_special_name:
        #TODO: refernece bug issue
        returncode = subprocess.run(["kdb", "mv", os_path_to_elektra_path(old_path), os_path_to_elektra_path(new_path)]).returncode
    else:
        #clumsy to implement using the python api => using kdb directly
        returncode = subprocess.run(["kdb", "mv", "-r", os_path_to_elektra_path(old_path), os_path_to_elektra_path(new_path)]).returncode
    if returncode != 0:
        raise OSError(errno.EROFS) #TODO: differentiate between different errors

# does not raise OSError(errno.EOPNOTSUPP), as this blocks tools like 'cp -r'
def chmod(path, mode):
    #TODO: maybe this can be handled better?
    return 0

def chown(path, uid, gid):
    return 0
