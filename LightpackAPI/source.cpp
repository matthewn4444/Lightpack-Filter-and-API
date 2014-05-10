// Copyright (C) 2006  Davis E. King (davis@dlib.net)
// License: Boost Software License   See LICENSE.txt for the full license.
#ifndef DLIB_ALL_SOURCe_
#define DLIB_ALL_SOURCe_

// ISO C++ code
#include <dlib\tokenizer\tokenizer_kernel_1.cpp>

#ifndef DLIB_ISO_CPP_ONLY
// Code that depends on OS specific APIs
// Editted - The non-commented includes are built for Windows

// include this first so that it can disable the older version
// of the winsock API when compiled in windows.
#include <dlib\sockets\sockets_kernel_1.cpp>
//#include "../bsp/bsp.cpp"

//#include "../dir_nav/dir_nav_kernel_1.cpp"
//#include "../dir_nav/dir_nav_kernel_2.cpp"
//#include "../dir_nav/dir_nav_extensions.cpp"
//#include "../linker/linker_kernel_1.cpp"
// #include "../logger/extra_logger_headers.cpp"
#include <dlib\logger\logger_kernel_1.cpp>
//#include "../logger/logger_config_file.cpp"
#include <dlib\misc_api\misc_api_kernel_1.cpp>
//#include "../misc_api/misc_api_kernel_2.cpp"
#include <dlib\sockets\sockets_extensions.cpp>
#include <dlib\sockets\sockets_kernel_2.cpp>
#include <dlib\sockstreambuf\sockstreambuf.cpp>
#include <dlib\sockstreambuf\sockstreambuf_unbuffered.cpp>
/*#include "../server/server_kernel.cpp"
#include "../server/server_iostream.cpp"
#include "../server/server_http.cpp"
*/
#include <dlib\threads\multithreaded_object_extension.cpp>
#include <dlib\threads\threaded_object_extension.cpp>
#include <dlib\threads\threads_kernel_1.cpp>
#include <dlib\threads\threads_kernel_2.cpp>
#include <dlib\threads\threads_kernel_shared.cpp>
#include <dlib\threads\thread_pool_extension.cpp>
#include <dlib\timer\timer.cpp>
#include <dlib\stack_trace.cpp>

#endif // DLIB_ISO_CPP_ONLY

#endif // DLIB_ALL_SOURCe_

