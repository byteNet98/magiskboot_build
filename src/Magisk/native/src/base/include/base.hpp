#pragma once

#include "../xwrap.hpp"
#include "../files.hpp"
#include "../misc.hpp"
#include "../logging.hpp"
#include "../base-rs.hpp"

#ifdef __linux__
using rust::xpipe2;
#endif
using rust::fd_path;