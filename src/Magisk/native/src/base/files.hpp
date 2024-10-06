#pragma once

#include <sys/stat.h>
#include <functional>
#include <string_view>
#include <string>
#include <vector>

#ifdef __linux__
#include <linux/fs.h>
#elif defined(__APPLE__)
#  include <sys/disk.h>
#elif defined(__CYGWIN__)
#  include <cygwin/fs.h>
#elif defined(_WIN32)
#  include <winsup/dirent_compat.h>

#  define closedir _closedir_stub
#  define DIR _DIR_stub
#  define opendir _opendir_stub
#endif
#include "misc.hpp"

template <typename T>
static inline T align_to(T v, int a) {
    static_assert(std::is_integral<T>::value);
    return (v + a - 1) / a * a;
}

template <typename T>
static inline T align_padding(T v, int a) {
    return align_to(v, a) - v;
}

struct mmap_data : public byte_data {
#ifdef __linux__
    static_assert((sizeof(void *) == 8 && BLKGETSIZE64 == 0x80081272) ||
                  (sizeof(void *) == 4 && BLKGETSIZE64 == 0x80041272));
#endif
    ALLOW_MOVE_ONLY(mmap_data)

    explicit mmap_data(const char *name, bool rw = false);
    mmap_data(int fd, size_t sz, bool rw = false);
    ~mmap_data();
};

extern "C" {

int mkdirs(const char *path, mode_t mode);
ssize_t canonical_path(const char * __restrict__ path, char * __restrict__ buf, size_t bufsiz);
bool rm_rf(const char *path);
bool frm_rf(int dirfd);
void cp_afc(const char *src, const char *dest);
void mv_path(const char *src, const char *dest);
void link_path(const char *src, const char *dest);
void clone_attr(const char *src, const char *dest);
void fclone_attr(int src, int dest);

} // extern "C"

int fd_pathat(int dirfd, const char *name, char *path, size_t size);
static inline ssize_t realpath(
        const char * __restrict__ path, char * __restrict__ buf, size_t bufsiz) {
    return canonical_path(path, buf, bufsiz);
}
void full_read(int fd, std::string &str);
void full_read(const char *filename, std::string &str);
std::string full_read(int fd);
std::string full_read(const char *filename);
void write_zero(int fd, size_t size);
void file_readline(bool trim, FILE *fp, const std::function<bool(std::string_view)> &fn);
void file_readline(bool trim, const char *file, const std::function<bool(std::string_view)> &fn);
void file_readline(const char *file, const std::function<bool(std::string_view)> &fn);
void parse_prop_file(FILE *fp, const std::function<bool(std::string_view, std::string_view)> &fn);
void parse_prop_file(const char *file,
        const std::function<bool(std::string_view, std::string_view)> &fn);
std::string resolve_preinit_dir(const char *base_dir);

using sFILE = std::unique_ptr<FILE, decltype(&fclose)>;
#ifdef _WIN32
using sDIR = std::unique_ptr<DIR, int (*)(DIR*)>;
#else
using sDIR = std::unique_ptr<DIR, decltype(&closedir)>;
#endif
sDIR make_dir(DIR *dp);
sFILE make_file(FILE *fp);

static inline sDIR open_dir(const char *path) {
    return make_dir(opendir(path));
}

static inline sDIR xopen_dir(const char *path) {
    return make_dir(xopendir(path));
}

static inline sDIR xopen_dir(int dirfd) {
    return make_dir(xfdopendir(dirfd));
}

static inline sFILE open_file(const char *path, const char *mode) {
    return make_file(fopen(path, mode));
}

static inline sFILE xopen_file(const char *path, const char *mode) {
    return make_file(xfopen(path, mode));
}

static inline sFILE xopen_file(int fd, const char *mode) {
    return make_file(xfdopen(fd, mode));
}
