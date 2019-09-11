#pragma once

#if defined(__APPLE__)
  #include <TargetConditionals.h>
    #define OMIM_OS_MAC
    #define OMIM_OS_NAME "mac"
#else
  #define OMIM_OS_LINUX
  #define OMIM_OS_NAME "linux"
#endif
