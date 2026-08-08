#include "app.h"
#include "ui/icons.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#endif

#include <vector>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// OS Framebuffer Callback: Forces a synchronous frame render during Win32 modal resize loops
static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    if (width <= 0 || height <= 0)
        return;

    if (GraphLab::App::HasInstance() && GraphLab::App::Get().GetNativeWindow() == window) {
        GraphLab::App::Get().RenderFrame();
    }
}

static std::string GetExecutableDir() {
#if defined(_WIN32)
    char path[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (len > 0) {
        std::string strPath(path, len);
        size_t pos = strPath.find_last_of("\\/");
        return (pos != std::string::npos) ? strPath.substr(0, pos) : "";
    }
#elif defined(__linux__)
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    if (count > 0) {
        std::string strPath(path, count);
        size_t pos = strPath.find_last_of('/');
        return (pos != std::string::npos) ? strPath.substr(0, pos) : "";
    }
#endif
    return "";
}

static std::string GetAssetPath(const std::string& relativePath) {
    std::string exeDir = GetExecutableDir();
    if (!exeDir.empty()) {
        std::string path = exeDir + "/" + relativePath;
        if (FILE* f = fopen(path.c_str(), "rb")) {
            fclose(f);
            return path;
        }
    }
    return relativePath;
}

namespace GraphLab {
    App* App::s_Instance = nullptr;

    App::App(const AppSpec& spec) : m_Spec(spec) {
        s_Instance = this;
    }

    App::~App() {
        Shutdown();
        s_Instance = nullptr;
    }

    int App::Run() {
        if (!Init())
            return -1;

        m_Running = true;

        while (m_Running && !glfwWindowShouldClose(m_Window)) {
            glfwPollEvents();

            if (glfwGetWindowAttrib(m_Window, GLFW_ICONIFIED) != 0) {
                ImGui_ImplGlfw_Sleep(10);
                continue;
            }

            RenderFrame();
        }

        Shutdown();
        return 0;
    }

    void App::RenderFrame() {
        if (!m_Window)
            return;

        int window_w = 0, window_h = 0;
        int display_w = 0, display_h = 0;
        glfwGetWindowSize(m_Window, &window_w, &window_h);
        glfwGetFramebufferSize(m_Window, &display_w, &display_h);

        if (window_w <= 0 || window_h <= 0 || display_w <= 0 || display_h <= 0)
            return;

        bool isExporting = !m_PendingExportImagePath.empty();
        if (isExporting) {
            m_GraphCanvas.SetHideOverlayUI(true);
        }

        // Synchronize ImGui viewport display size with actual window dimensions
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)window_w, (float)window_h);
        io.DisplayFramebufferScale = ImVec2((float)display_w / (float)window_w, (float)display_h / (float)window_h);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        OnUpdate();
        OnRenderUI();

        ImGui::Render();
        glViewport(0, 0, display_w, display_h);

        ImVec4 clear_color = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
                     clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (isExporting) {
            m_MainMenuBar.PerformExportImage(m_PendingExportImagePath);
            m_PendingExportImagePath.clear();
            m_GraphCanvas.SetHideOverlayUI(false);
        }

        glfwSwapBuffers(m_Window);
    }

    bool App::Init() {
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
            return false;

        const char* glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        m_Window = glfwCreateWindow((int)m_Spec.Width,
                                    (int)m_Spec.Height,
                                    m_Spec.Title.c_str(),
                                    nullptr, nullptr);

        if (m_Window == nullptr)
            return false;

        // Center window on the primary monitor work area
        GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();
        if (primary_monitor) {
            int monitor_x = 0, monitor_y = 0, monitor_w = 0, monitor_h = 0;
            glfwGetMonitorWorkarea(primary_monitor, &monitor_x, &monitor_y, &monitor_w, &monitor_h);
            int pos_x = monitor_x + (monitor_w - (int)m_Spec.Width) / 2;
            int pos_y = monitor_y + (monitor_h - (int)m_Spec.Height) / 2;
            glfwSetWindowPos(m_Window, pos_x, pos_y);
        }

        glfwSetFramebufferSizeCallback(m_Window, framebuffer_size_callback);

        glfwMakeContextCurrent(m_Window);
        glfwSwapInterval(m_Spec.VSync ? 1 : 0);

        SetAppIcon();

        if (!ImGui::CreateContext())
            return false;

        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.IniFilename = nullptr; // Disable imgui.ini generation
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        ImGui::StyleColorsDark();

        float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(main_scale);
        style.WindowRounding = 8.0f;
        style.ChildRounding = 8.0f;
        style.FrameRounding = 5.0f;
        style.PopupRounding = 8.0f;
        style.GrabRounding = 5.0f;
        style.TabRounding = 5.0f;
        style.ScrollbarRounding = 6.0f;
        style.AntiAliasedLines = true;
        style.AntiAliasedLinesUseTex = true;

        // Load Inter-Medium font with full Vietnamese Unicode glyph ranges
        const ImWchar* vietnamese_ranges = io.Fonts->GetGlyphRangesVietnamese();
        std::string inter_path = GetAssetPath("assets/fonts/Inter_18pt-Medium.ttf");
        float font_size = 18.0f * main_scale;
        io.Fonts->AddFontFromFileTTF(inter_path.c_str(), font_size, nullptr, vietnamese_ranges);

        // Load FontAwesome 6 Free Solid icons merged with primary font
        static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        icons_config.GlyphOffset = ImVec2(0.0f, -1.0f);
        std::string icon_font_path = GetAssetPath("assets/fonts/fa-solid-900.ttf");
        io.Fonts->AddFontFromFileTTF(icon_font_path.c_str(), font_size * 0.85f, &icons_config, icons_ranges);

        ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);

        return true;
    }

    void App::SetAppIcon() {
#if defined(_WIN32)
        // 1. Native Windows Win32 Icon (Embedded Resource ID 1)
        HWND hwnd = glfwGetWin32Window(m_Window);
        HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(1));
        if (hIcon) {
            SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        }
#endif

        // 2. Cross-platform GLFW Window Icon fallback
        std::string icon_png_path = GetAssetPath("assets/icon.png");
        int w = 0, h = 0, channels = 0;
        unsigned char* pixels = stbi_load(icon_png_path.c_str(), &w, &h, &channels, 4);
        if (pixels) {
            GLFWimage images[1];
            images[0].width = w;
            images[0].height = h;
            images[0].pixels = pixels;
            glfwSetWindowIcon(m_Window, 1, images);
            stbi_image_free(pixels);
        }
    }

    void App::OnUpdate() { }

    void App::OnRenderUI() {
        // 1. Render Top Main Navigation Menu Bar
        m_MainMenuBar.OnRenderUI();

        // 2. Render Sidebar Panel
        m_SidebarPanel.OnRenderUI();

        // 3. Render Graph Canvas
        m_GraphCanvas.OnRenderUI();
    }

    void App::Shutdown() {
        if (m_Window == nullptr)
            return;

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(m_Window);
        glfwTerminate();
        m_Window = nullptr;
    }

} // namespace GraphLab
