#pragma once
#define STRINGIFY_IMPL(x) #x
#define STRINGIFY(x) STRINGIFY_IMPL(x)
#define FIRMWARE_VERSION \
    STRINGIFY(MAJOR_VERSION) "." \
    STRINGIFY(MINOR_VERSION) "." \
    STRINGIFY(PATCH_VERSION) "." \
    "rev" STRINGIFY(BUILD_NUMBER) "-" \
    STRINGIFY(COMMIT_HASH)

namespace quartz
{
    constexpr auto Version = FIRMWARE_VERSION;
}