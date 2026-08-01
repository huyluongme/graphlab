#include "utils/file_dialog.h"
#include <GLFW/glfw3.h>
#include <cstring>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

namespace GraphLab::Utils {

    std::string FileDialog::Open(GLFWwindow* window, const char* filter) {
#ifdef _WIN32
        OPENFILENAMEA ofn;
        char szFile[260] = { 0 };
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = window ? glfwGetWin32Window(window) : NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameA(&ofn) == TRUE) {
            return ofn.lpstrFile;
        }
#else
        // Linux / POSIX GTK/KDE file chooser via zenity or kdialog
        char path[1024] = {0};
        FILE* f = popen("zenity --file-selection 2>/dev/null || kdialog --getopenfilename . 2>/dev/null", "r");
        if (f) {
            if (fgets(path, sizeof(path), f)) {
                path[strcspn(path, "\r\n")] = 0;
                pclose(f);
                return std::string(path);
            }
            pclose(f);
        }
#endif
        return "";
    }

    std::string FileDialog::Save(GLFWwindow* window, const char* filter, const char* defaultExt) {
#ifdef _WIN32
        OPENFILENAMEA ofn;
        char szFile[260] = { 0 };
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = window ? glfwGetWin32Window(window) : NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.lpstrDefExt = defaultExt;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

        if (GetSaveFileNameA(&ofn) == TRUE) {
            return ofn.lpstrFile;
        }
#else
        // Linux / POSIX GTK/KDE file saver via zenity or kdialog
        char path[1024] = {0};
        FILE* f = popen("zenity --file-selection --save --confirm-overwrite 2>/dev/null || kdialog --getsavefilename . 2>/dev/null", "r");
        if (f) {
            if (fgets(path, sizeof(path), f)) {
                path[strcspn(path, "\r\n")] = 0;
                pclose(f);
                return std::string(path);
            }
            pclose(f);
        }
#endif
        return "";
    }

} // namespace GraphLab::Utils
