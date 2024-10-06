#include "getline.h"

#include <stdlib.h>
#include <errno.h>

#ifdef _UCRT
#  ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#ifndef _IOERROR
#  define _IOERROR	(0x0010)
#endif

struct __crt_stdio_stream_data
{
    union
    {
        FILE  _public_file;
        char* _ptr;
    };

    char*            _base;
    int              _cnt;
    long             _flags;
    long             _file;
    int              _charbuf;
    int              _bufsiz;
    char*            _tmpfname;
    CRITICAL_SECTION _lock;
};
#endif

static void fseterr(FILE *fp)
{
#ifdef _UCRT
    // UCRT specific implementation
    _lock_file(fp);
    _InterlockedOr(&(((struct __crt_stdio_stream_data *)fp)->_flags), _IOERROR);
    _unlock_file(fp);
#else
#  ifdef __MINGW32__
    // MinGW specific implementation
    ((struct _iobuf *)fp)->_flag |= _IOERR;
#  else
    // MSVC specific implementation
    struct file { // Undocumented implementation detail
        unsigned char *_ptr;
        unsigned char *_base;
        int _cnt;
        int _flag;
        int _file;
        int _charbuf;
        int _bufsiz;
    };
    #define _IOERR 0x10

    ((struct file *)fp)->_flag |= _IOERR;
#  endif
#endif
}

ssize_t getdelim(char **restrict lineptr, size_t *restrict n, int delim, FILE *restrict stream)
{
    if (lineptr == NULL || n == NULL || stream == NULL || (*lineptr == NULL && *n != 0)) {
        errno = EINVAL;
        return -1;
    }
    if (feof(stream) || ferror(stream)) {
        return -1;
    }

    if (*lineptr == NULL) {
        *n = 256;
        *lineptr = malloc(*n);
        if (*lineptr == NULL) {
            fseterr(stream);
            errno = ENOMEM;
            return -1;
        }
    }
    ssize_t nread = 0;
    int c = EOF;
    while (c != delim) {
        c = fgetc(stream);
        if (c == EOF) {
            break;
        }
        if (nread >= (ssize_t)(*n - 1)) {
            size_t newn = *n * 2;
            char *newptr = realloc(*lineptr, newn);
            if (newptr == NULL) {
                fseterr(stream);
                errno = ENOMEM;
                return -1;
            }
            *lineptr = newptr;
            *n = newn;
        }
        (*lineptr)[nread++] = c;
    }
    if (c == EOF && nread == 0) {
        return -1;
    }
    (*lineptr)[nread] = 0;
    return nread;
}

#ifdef __MINGW32__
ssize_t msvc_getline(char **restrict lineptr, size_t *restrict n, FILE *restrict stream)
#else
ssize_t getline(char **restrict lineptr, size_t *restrict n, FILE *restrict stream)
#endif
{
    return getdelim(lineptr, n, '\n', stream);
}

