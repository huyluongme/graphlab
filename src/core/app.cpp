#include "app.h"
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
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// OS Framebuffer Callback: Forces a synchronous frame render during Win32 modal resize loops
static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    if (width <= 0 || height <= 0)
        return;

    if (GraphLab::App::HasInstance() && GraphLab::App::Get().GetNativeWindow() == window)
        GraphLab::App::Get().RenderFrame();
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
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        ImGui::StyleColorsDark();

        float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(main_scale);

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
        const char* paths[] = { "assets/icon.png", "../assets/icon.png", "../../assets/icon.png" };
        unsigned char* pixels = nullptr;
        int w = 0, h = 0, channels = 0;

        for (const char* path : paths) {
            pixels = stbi_load(path, &w, &h, &channels, 4);
            if (pixels)
                break;
        }

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

        // 2. Render Main Workspace Window
        ImGuiIO& io = ImGui::GetIO();

        // Enforce full-viewport coverage for the main workspace window
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(io.DisplaySize);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration |
                                        ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_NoSavedSettings |
                                        ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15.0f, 15.0f));

        ImGui::Begin("GraphLab Workspace", nullptr, window_flags);
        ImGui::PopStyleVar(3);

        ImGui::Dummy(ImVec2(0.0f, 20.0f)); // Spacer for top menu bar
        ImGui::Text("Welcome to GraphLab - Graphing Calculator");
        ImGui::Separator();
        ImGui::Text("Status: Main Menu Bar Integrated!");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

        ImGui::End();
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
