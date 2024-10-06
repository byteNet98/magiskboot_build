
#include <windows.h>
#include <errno.h>
#include <io.h>
#include <stdbool.h>

#include "mman.h"

#ifndef FILE_MAP_EXECUTE
#define FILE_MAP_EXECUTE    0x0020
#endif /* FILE_MAP_EXECUTE */

#ifdef WITH_WINSUP
// this function is defined in src/libc-compat/winsup/internal/errno.c
extern void __set_errno_via_winerr(DWORD winerr);
#else
static int __map_mman_error(const DWORD err, const int deferr)
{
    if (err == 0)
        return 0;
    //TODO: implement
    return err;
}
#endif

static DWORD __map_mmap_prot_page(const int prot, const int flags)
{
    DWORD protect = 0;
    
    if (prot == PROT_NONE)
        return protect;
        
    if ((prot & PROT_EXEC) != 0)
    {
        protect = ((prot & PROT_WRITE) != 0) ? 
                    (((flags & MAP_PRIVATE) != 0) ? PAGE_EXECUTE_WRITECOPY : PAGE_EXECUTE_READWRITE) : PAGE_EXECUTE_READ;
    }
    else
    {
        protect = ((prot & PROT_WRITE) != 0) ?
                    (((flags & MAP_PRIVATE) != 0) ? PAGE_WRITECOPY : PAGE_READWRITE) : PAGE_READONLY;
    }
    
    return protect;
}

static DWORD __map_mmap_prot_file(const int prot, const int flags)
{
    DWORD desiredAccess = 0;
    
    if (prot == PROT_NONE)
        return desiredAccess;
    
    bool map_read = (prot & PROT_READ) != 0;
    bool map_write = (prot & PROT_WRITE) != 0;
    bool map_exec = (prot & PROT_EXEC) != 0;
    bool map_private = (flags & MAP_PRIVATE) != 0;
    bool map_copy = map_write && map_private;
    
    // ref: https://stackoverflow.com/questions/55018806/copy-on-write-file-mapping-on-windows
    if (map_read && !map_copy)
        desiredAccess |= FILE_MAP_READ;
    if (map_write)
        desiredAccess |= map_private ? FILE_MAP_COPY : FILE_MAP_WRITE;
    if (map_exec)
        desiredAccess |= FILE_MAP_EXECUTE;
    
    return desiredAccess;
}

void* mmap(void *addr, size_t len, int prot, int flags, int fildes, OffsetType off)
{
    HANDLE fm, h;
    
    void * map = MAP_FAILED;
    
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4293)
#endif

    const DWORD dwFileOffsetLow = (sizeof(OffsetType) <= sizeof(DWORD)) ?
                    (DWORD)off : (DWORD)(off & 0xFFFFFFFFL);
    const DWORD dwFileOffsetHigh = (sizeof(OffsetType) <= sizeof(DWORD)) ?
                    (DWORD)0 : (DWORD)((off >> 32) & 0xFFFFFFFFL);
    const DWORD protect = __map_mmap_prot_page(prot, flags);
    const DWORD desiredAccess = __map_mmap_prot_file(prot, flags);

    const OffsetType maxSize = off + (OffsetType)len;

    const DWORD dwMaxSizeLow = (sizeof(OffsetType) <= sizeof(DWORD)) ?
                    (DWORD)maxSize : (DWORD)(maxSize & 0xFFFFFFFFL);
    const DWORD dwMaxSizeHigh = (sizeof(OffsetType) <= sizeof(DWORD)) ?
                    (DWORD)0 : (DWORD)((maxSize >> 32) & 0xFFFFFFFFL);

#ifdef _MSC_VER
#pragma warning(pop)
#endif

    errno = 0;
    
    if (len == 0 
        /* Usupported protection combinations */
        || prot == PROT_EXEC)
    {
        errno = EINVAL;
        return MAP_FAILED;
    }
    
    h = ((flags & MAP_ANONYMOUS) == 0) ? 
                    (HANDLE)_get_osfhandle(fildes) : INVALID_HANDLE_VALUE;

    if ((flags & MAP_ANONYMOUS) == 0 && h == INVALID_HANDLE_VALUE)
    {
        errno = EBADF;
        return MAP_FAILED;
    }

    fm = CreateFileMapping(h, NULL, protect, dwMaxSizeHigh, dwMaxSizeLow, NULL);

    if (fm == NULL)
    {
#ifdef WITH_WINSUP
        __set_errno_via_winerr(GetLastError());
#else
        errno = __map_mman_error(GetLastError(), EPERM);
#endif
        return MAP_FAILED;
    }
  
    if ((flags & MAP_FIXED) == 0)
    {
        map = MapViewOfFile(fm, desiredAccess, dwFileOffsetHigh, dwFileOffsetLow, len);
    }
    else
    {
        map = MapViewOfFileEx(fm, desiredAccess, dwFileOffsetHigh, dwFileOffsetLow, len, addr);
    }

    CloseHandle(fm);
  
    if (map == NULL)
    {
#ifdef WITH_WINSUP
        __set_errno_via_winerr(GetLastError());
#else
        errno = __map_mman_error(GetLastError(), EPERM);
#endif
        return MAP_FAILED;
    }

    return map;
}

int munmap(void *addr, size_t len)
{
    if (UnmapViewOfFile(addr))
        return 0;
        
#ifdef WITH_WINSUP
    __set_errno_via_winerr(GetLastError());
#else
    errno =  __map_mman_error(GetLastError(), EPERM);
#endif
    
    return -1;
}

// TODO: not portable, use VirtualQuery to get old protection instead
int _mprotect_np(void *addr, size_t len, int prot, int map_flags)
{
    DWORD newProtect = __map_mmap_prot_page(prot, map_flags);
    DWORD oldProtect = 0;
    
    if (VirtualProtect(addr, len, newProtect, &oldProtect))
        return 0;
    
#ifdef WITH_WINSUP
    __set_errno_via_winerr(GetLastError());
#else
    errno =  __map_mman_error(GetLastError(), EPERM);
#endif
    
    return -1;
}

int msync(void *addr, size_t len, int flags)
{
    if (FlushViewOfFile(addr, len))
        return 0;
    
#ifdef WITH_WINSUP
    __set_errno_via_winerr(GetLastError());
#else
    errno =  __map_mman_error(GetLastError(), EPERM);
#endif
    
    return -1;
}

int mlock(const void *addr, size_t len)
{
    if (VirtualLock((LPVOID)addr, len))
        return 0;
        
#ifdef WITH_WINSUP
    __set_errno_via_winerr(GetLastError());
#else
    errno =  __map_mman_error(GetLastError(), EPERM);
#endif
    
    return -1;
}

int munlock(const void *addr, size_t len)
{
    if (VirtualUnlock((LPVOID)addr, len))
        return 0;
        
#ifdef WITH_WINSUP
    __set_errno_via_winerr(GetLastError());
#else
    errno =  __map_mman_error(GetLastError(), EPERM);
#endif
    
    return -1;
}
