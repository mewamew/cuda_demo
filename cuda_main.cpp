#include "particles.h"
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <vector>
#include <random>
#include <cmath>

// CUDA 主机函数声明
extern "C" void InitializeParticles(Particle* h_particles, int count);
extern "C" void UpdateParticles(int count, Particle* h_particles, float dt);
extern "C" void AllocateDeviceMemory(int count);
extern "C" void CleanupCUDA();

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")



std::vector<Particle> particles;

// 函数声明
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void DrawParticles(HDC hdc);
void SetupOpenGL(HWND hwnd);
void InitializeParticlesHost();

int main(int argc, char** argv) {
    // 注册窗口类
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"ParticleExplosionCUDAClass";
    RegisterClassW(&wc);

    // 创建窗口
    HWND hwnd = CreateWindowExW(
        0,
        L"ParticleExplosionCUDAClass",
        L"Particle Explosion (CUDA)",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL,
        NULL,
        wc.hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    // 设置 OpenGL
    SetupOpenGL(hwnd);

    // 显示窗口
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // 初始化粒子
    particles.resize(PARTICLE_COUNT);
    AllocateDeviceMemory(PARTICLE_COUNT);
    InitializeParticlesHost();

    // 消息循环
    MSG msg = {};
    while (true) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                CleanupCUDA();
                return 0;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // 更新粒子
        float dt = 1.0f / 60.0f;  // 假设60FPS
        UpdateParticles(PARTICLE_COUNT, particles.data(), dt);

        // 绘制粒子
        InvalidateRect(hwnd, NULL, FALSE);

        // 控制帧率（可选）
        //Sleep(16); // ~60 FPS
    }

    return 0;
}

void InitializeParticlesHost() {
    InitializeParticles(particles.data(), PARTICLE_COUNT);
}

void DrawParticles(HDC hdc) {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    for (const auto& p : particles) {
        float size = PARTICLE_SIZE * sqrtf(p.mass);  // 根据质量调整大小
        glPointSize(size);
        glBegin(GL_POINTS);
        glColor3f(p.color[0], p.color[1], p.color[2]);  // 使用粒子的颜色
        glVertex2f(p.x / (WINDOW_WIDTH / 2.0f) - 1.0f,
                   1.0f - p.y / (WINDOW_HEIGHT / 2.0f));
        glEnd();
    }

    SwapBuffers(hdc);
}

void SetupOpenGL(HWND hwnd) {
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        32,
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24,
        8,
        0,
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };

    HDC hdc = GetDC(hwnd);
    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pixelFormat, &pfd);
    HGLRC hrc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, hrc);

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // 黑色背景
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        return 0;
    case WM_DESTROY:
        CleanupCUDA();
        PostQuitMessage(0);
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        DrawParticles(hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_KEYDOWN:
        if (wParam == VK_SPACE) {
            InitializeParticlesHost();
        }
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}