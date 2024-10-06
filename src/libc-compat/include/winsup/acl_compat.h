#ifndef _MAGISKBOOT_BUILD_WINSUP_ACL_COMPAT
#define _MAGISKBOOT_BUILD_WINSUP_ACL_COMPAT

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

typedef short uid_t, gid_t;

int     chown (const char *path, uid_t owner, gid_t group);
int     fchmod (int fd, mode_t mode);
int     fchown (int fildes, uid_t owner, gid_t group);
int     lchown (const char *path, uid_t owner, gid_t group);

#ifdef __cplusplus
}
#endif

#endif /* _MAGISKBOOT_BUILD_WINSUP_ACL_COMPAT */
