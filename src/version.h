#pragma once

#define GRAPHLAB_VERSION_MAJOR 1
#define GRAPHLAB_VERSION_MINOR 0
#define GRAPHLAB_VERSION_PATCH 0

// Macro stringification helpers
#define GRAPHLAB_STRINGIFY_HELPER(x) #x
#define GRAPHLAB_STRINGIFY(x) GRAPHLAB_STRINGIFY_HELPER(x)

// Concatenated version string automatically constructed from major, minor, patch defines
#define GRAPHLAB_VERSION_STRING \
    GRAPHLAB_STRINGIFY(GRAPHLAB_VERSION_MAJOR) "." \
    GRAPHLAB_STRINGIFY(GRAPHLAB_VERSION_MINOR) "." \
    GRAPHLAB_STRINGIFY(GRAPHLAB_VERSION_PATCH)

#define GRAPHLAB_APP_NAME "GraphLab"
#define GRAPHLAB_FULL_NAME "GraphLab - Graphing Calculator v" GRAPHLAB_VERSION_STRING
