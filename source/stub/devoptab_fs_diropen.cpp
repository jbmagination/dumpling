#include "devoptab_fs.h"

DIR_ITER* __wut_fs_diropen(struct _reent* r, DIR_ITER* dirState, const char* path) {
    FSDirectoryHandle fd;
    FSStatus status;
    FSCmdBlock cmd;
    __wut_fs_dir_t* dir;

    if (r->deviceData == NULL || !((devoptab_data_t*)r->deviceData)->initialized) {
        r->_errno = EINVAL;
        return NULL;
    }
    devoptab_data_t* data = ((devoptab_data_t*)r->deviceData);

    if (!dirState || !path) {
        r->_errno = EINVAL;
        return NULL;
    }

    char* fixedPath = __wut_fs_fixpath(r, path);
    if (!fixedPath) {
        return NULL;
    }

    FSInitCmdBlock(&cmd);
    dir = (__wut_fs_dir_t*)(dirState->dirStruct);
    status = FSOpenDir(data->client, &cmd, fixedPath, &fd, FS_ERROR_FLAG_ALL);
    free(fixedPath);
    if (status < 0) {
        r->_errno = __wut_fs_translate_error(status);
        return NULL;
    }

    dir->magic = FS_DIRITER_MAGIC;
    dir->fd = fd;
    memset(&dir->entry_data, 0, sizeof(dir->entry_data));
    return dirState;
}
