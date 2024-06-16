// Dear ImGui: standalone example application for Win32 + OpenGL 3

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// This is provided for completeness, however it is strongly recommended you use OpenGL with SDL or GLFW.

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/GL.h>
#include <tchar.h>
#include <spdlog/spdlog.h>
#include <chrono>
#include <thread>

// Data stored per platform window
struct WGL_WindowData
{
    HDC hDC;
};

// Data
static HGLRC g_hRC;
static WGL_WindowData g_MainWindow;
static int g_Width;
static int g_Height;

// Forward declarations of helper functions
bool CreateDeviceWGL(HWND hWnd, WGL_WindowData *data);
void CleanupDeviceWGL(HWND hWnd, WGL_WindowData *data);
void ResetDeviceWGL();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class UI_Controller
{
public:
    UI_Controller() = default;
    ~UI_Controller()
    {
        destroy();
    }

    int init();
    void destroy();
    void run();

    void RenderUI()
    {
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        ImGui::Begin("播放控制");

        if (ImGui::BeginTable("control buttons", 2))
        {
            const char *playBtnText = isPlaying ? "暂停" : "播放"; //"\u23f8" : "\u25b6";
            ImGui::TableNextColumn();
            if (ImGui::Button(playBtnText))
            {
                isPlaying = !isPlaying;
            }

            ImGui::TableNextColumn();
            const char *stopBtnText = "停止"; //"\u23f9";
            if (ImGui::Button(stopBtnText))
            {
            }
            ImGui::EndTable();
        }

        if (ImGui::SliderFloat("1", &currentTime, 0, 66.66))
        {
        }

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

private:
    WNDCLASSEXW _wc;
    HWND _hwnd = 0;


    bool show_demo_window = false;
    bool show_another_window = false;

    bool isPlaying = false;
    float currentTime = 0;
    float totalTime = 1;
};

int UI_Controller::init()
{
    // Create application window
    // ImGui_ImplWin32_EnableDpiAwareness();
    _wc = {sizeof(_wc), CS_OWNDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"SimplePlayer", nullptr};
    ::RegisterClassExW(&_wc);
    _hwnd = ::CreateWindowW(_wc.lpszClassName, L"Simple Player", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, _wc.hInstance, nullptr);

    // Initialize OpenGL
    if (!CreateDeviceWGL(_hwnd, &g_MainWindow))
    {
        CleanupDeviceWGL(_hwnd, &g_MainWindow);
        ::DestroyWindow(_hwnd);
        ::UnregisterClassW(_wc.lpszClassName, _wc.hInstance);
        return -1;
    }
    wglMakeCurrent(g_MainWindow.hDC, g_hRC);

    // Show the window
    ::ShowWindow(_hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(_hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsLight();
    // ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_InitForOpenGL(_hwnd);
    ImGui_ImplOpenGL3_Init();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // io.Fonts->AddFontDefault();
    // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    // 设置微软雅黑字体,并指定字体大小
    ImFont *font = io.Fonts->AddFontFromFileTTF(
        "C:/Windows/Fonts/msyh.ttc",
        14,
        nullptr,
        // 设置加载中文
        io.Fonts->GetGlyphRangesChineseFull());
    // 必须判断一下字体有没有加载成功
    IM_ASSERT(font != nullptr);

    return 0;
}

void UI_Controller::destroy()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceWGL(_hwnd, &g_MainWindow);
    wglDeleteContext(g_hRC);
    ::DestroyWindow(_hwnd);
    ::UnregisterClassW(_wc.lpszClassName, _wc.hInstance);
}

void UI_Controller::run() {
        // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        RenderUI();

        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        // Rendering
        ImGui::Render();
        glViewport(0, 0, g_Width, g_Height);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Present
        ::SwapBuffers(g_MainWindow.hDC);

        std::this_thread::sleep_for(std::chrono::microseconds(100000 / 60));
    }
}

// Main code
int main_win32(int argc, char *argv[])
{
    std::unique_ptr<UI_Controller> ui = std::make_unique<UI_Controller>();
    if (int ret = ui->init(); ret != 0)
        return ret;
    ui->run();
    ui.reset();

    return 0;
}

// Helper functions
bool CreateDeviceWGL(HWND hWnd, WGL_WindowData *data)
{
    HDC hDc = ::GetDC(hWnd);
    PIXELFORMATDESCRIPTOR pfd = {0};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;

    const int pf = ::ChoosePixelFormat(hDc, &pfd);
    if (pf == 0)
        return false;
    if (::SetPixelFormat(hDc, pf, &pfd) == FALSE)
        return false;
    ::ReleaseDC(hWnd, hDc);

    data->hDC = ::GetDC(hWnd);
    if (!g_hRC)
        g_hRC = wglCreateContext(data->hDC);
    return true;
}

void CleanupDeviceWGL(HWND hWnd, WGL_WindowData *data)
{
    wglMakeCurrent(nullptr, nullptr);
    ::ReleaseDC(hWnd, data->hDC);
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
            g_Width = LOWORD(lParam);
            g_Height = HIWORD(lParam);
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
