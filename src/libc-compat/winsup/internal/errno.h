#ifndef _MAGISKBOOT_BUILD_WINSUP_INTERNAL_ERRNO
#define _MAGISKBOOT_BUILD_WINSUP_INTERNAL_ERRNO

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

void __set_errno_via_winerr(DWORD winerr);

#ifdef WIN32_LEAN_AND_MEAN
#  undef WIN32_LEAN_AND_MEAN
#endif

#endif /* _MAGISKBOOT_BUILD_WINSUP_INTERNAL_ERRNO */
