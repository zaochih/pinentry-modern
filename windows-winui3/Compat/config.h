#pragma once

#define HAVE_CONFIG_H 1
#define HAVE_W32_SYSTEM 1
#define HAVE_DOSISH_SYSTEM 1

#define PACKAGE_NAME "pinentry-modern"
#define PACKAGE_VERSION "1.0.1"
#define PACKAGE_BUGREPORT "https://github.com/stapxs/pinentry-mac-modern/issues"
#define VERSION PACKAGE_VERSION

#define strcasecmp _stricmp
#define strncasecmp _strnicmp
