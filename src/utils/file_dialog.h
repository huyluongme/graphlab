#pragma once

#include <string>

struct GLFWwindow;

namespace GraphLab::Utils {
    /**
     * @brief Cross-platform native OS file dialog helper (Windows Win32 & Linux GTK/KDE).
     */
    class FileDialog {
    public:
        /**
         * @brief Opens a native file open dialog window.
         * @param window Pointer to the GLFW window context.
         * @param filter File extension filter string (e.g. "GraphLab Project (*.graphlab)\0*.graphlab\0").
         * @return Selected file path string, or empty string if cancelled.
         */
        static std::string Open(GLFWwindow* window, const char* filter);

        /**
         * @brief Opens a native file save dialog window.
         * @param window Pointer to the GLFW window context.
         * @param filter File extension filter string.
         * @param defaultExt Default file extension.
         * @return Selected target file path string, or empty string if cancelled.
         */
        static std::string Save(GLFWwindow* window, const char* filter, const char* defaultExt);
    };
}
