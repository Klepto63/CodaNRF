#ifndef APP_VERSION_H
#define APP_VERSION_H

#include <stdint.h>

static const uint16_t app_version_release_major     = !VERSION_MAJOR!;
static const uint16_t app_version_release_minor     = !VERSION_MINOR!;
static const uint16_t app_version_release_debug     = !VERSION_DEBUG!;
static const uint16_t app_version_release_magic     = !VERSION_MAGIC!;
static const char app_version_release_str[]         = "!VERSION_STR!";
static const char app_version_release_tag[]         = "!VERSION_TAG!";
static const char app_version_release_commit_id[]   = "!VERSION_COMMIT_ID!";
static const char app_version_release_datetime[]    = "!VERSION_DATETIME!";

#endif
