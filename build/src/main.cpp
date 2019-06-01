#include "stdafx.h"
#include "timer/platform_timer.h"

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_PAINT:
    {
        printf("WM_PAINT");
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

class Window
{
public:
    HWND m_hwnd;
    HINSTANCE m_hInstance;

    Window(uint32 width, uint32 height, bool bCenter)
    {
        LPCWSTR strClassName = L"Software Rasterizer";
        LPCWSTR strTitle = L"Software Rasterizer";

        //Step 1: Registering the Window Class
        WNDCLASSEX wc;
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = 0;
        wc.lpfnWndProc = WndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = m_hInstance;
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = NULL;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = strClassName;
        wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
        RegisterClassEx(&wc);

        UINT x = CW_USEDEFAULT;
        UINT y = CW_USEDEFAULT;
        if (bCenter)
        {
            UINT screenResX = GetSystemMetrics(SM_CXSCREEN);
            UINT screenResY = GetSystemMetrics(SM_CYSCREEN);
            x = (UINT)(screenResX * 0.5f - (width * 0.5f));
            y = (UINT)(screenResY * 0.5f - (height * 0.5f));
        }
        DWORD windowStyle = WS_OVERLAPPEDWINDOW;

        // Calculate real window size (with borders)
        RECT r{ 0, 0, (LONG)width, (LONG)height };
        AdjustWindowRect(&r, windowStyle, FALSE);
        UINT w = r.right - r.left;
        UINT h = r.bottom - r.top;

        m_hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, strClassName, strTitle, windowStyle, x, y, width, height, NULL, NULL, m_hInstance, NULL);

        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);
    }

    void SetTitle(const wchar_t* newTitle, ...)
    {
        WCHAR buffer[256]{};
        va_list args;
        va_start(args, newTitle);
        vswprintf(buffer, 256, newTitle, args);
        va_end(args);
        SetWindowText(m_hwnd, buffer);
    }
};

class Bitmap
{
public:
    Bitmap(HWND hwnd, uint32 width, uint32 height)
        : m_width(width), m_height(height)
    {
        m_hdc = GetDC(hwnd);
        m_pPixels = (COLORREF*)calloc(width * height, sizeof(COLORREF));
    }
    ~Bitmap() { free(m_pPixels); }

    inline void Clear()
    {
        memset(m_pPixels, 0, m_width * m_height * sizeof(COLORREF));
    }

    inline void Clear(char r, char g, char b)
    {
        for (uint32 x = 0; x < m_width; x++)
            for (uint32 y = 0; y < m_height; y++)
                SetPixel(x, y, r, g, b);
    }

    inline void SetPixel(uint32 x, uint32 y, char r, char g, char b)
    {
        #define RGB2(r, g, b) RGB(b, g, r)
        int index = y * m_width + x;
        m_pPixels[index] = RGB2(r, g, b);
    }

    inline void SwapBuffers()
    {
        HBITMAP bitmap = CreateBitmap(m_width, m_height, 1, 32, (void*)m_pPixels);

        HDC src = CreateCompatibleDC(m_hdc);
        SelectObject(src, bitmap);

        BitBlt(m_hdc, 0, 0, m_width, m_height, src, 0, 0, SRCCOPY);

        DeleteDC(src);
        DeleteObject(bitmap);
    }

    inline uint32 GetWidth() const { return m_width; }
    inline uint32 GetHeight() const { return m_height; }

private:
    uint32    m_width;
    uint32    m_height;
    HDC       m_hdc;
    COLORREF *m_pPixels;
};

class Stars3D
{
public:
    Stars3D(int numStars, f32 spread, f32 speed)
        : m_numStars(numStars), m_spread(spread), m_speed(speed)
    {
        m_starX = new f32[numStars];
        m_starY = new f32[numStars];
        m_starZ = new f32[numStars];
        for (int i = 0; i < numStars; i++)
            InitStar(i);
    }

    ~Stars3D()
    {
        delete[] m_starX;
        delete[] m_starY;
        delete[] m_starZ;
    }

    void InitStar(int index)
    {
        m_starX[index] = ((rand() / (f32)RAND_MAX) - 0.5f) * 2.0f * m_spread;
        m_starY[index] = ((rand() / (f32)RAND_MAX) - 0.5f) * 2.0f * m_spread;
        m_starZ[index] = ((rand() / (f32)RAND_MAX) + 0.00001f) * m_spread;
    }

    void UpdateAndRender(Bitmap &bitmap, f32 delta)
    {
        bitmap.Clear();

        f32 halfWidth = bitmap.GetWidth() / 2.0f;
        f32 halfHeight = bitmap.GetHeight() / 2.0f;
        for (int i = 0; i < m_numStars; i++)
        {
            m_starZ[i] -= delta * m_speed;
            if (m_starZ[i] <= 0)
                InitStar(i);

            int x = (int)((m_starX[i]/m_starZ[i]) * halfWidth + halfWidth);
            int y = (int)((m_starY[i]/m_starZ[i]) * halfHeight + halfHeight);

            if (x < 0 || x >= bitmap.GetWidth() ||
                y < 0 || y >= bitmap.GetHeight())
            {
                InitStar(i);
            }
            else
            {
                bitmap.SetPixel(x, y, 255, 255, 255);
            }
        }
    }

private:
    int m_numStars;
    f32 m_spread;
    f32 m_speed;
    f32 *m_starX;
    f32 *m_starY;
    f32 *m_starZ;
};

int main()
{
    uint32 width = 1024;
    uint32 height = 1024;

    Window window(width, height, false);
    Bitmap bitmap(window.m_hwnd, width, height);

    uint64 prevTicks = OS::PlatformTimer::getTicks();

    uint64 frames = 0;
    uint64 fps = 0;
    f32 fpsTimer = 0.0;
    f32 titleTimer = 0.0;

    Stars3D stars(4096, 64.0f, 20.0f);

    bool bClose = false;
    while (!bClose)
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                bClose = true;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        frames++;

        uint64 curTicks = OS::PlatformTimer::getTicks();
        uint64 deltaTicks = curTicks - prevTicks;
        prevTicks = curTicks;

        f32 delta = (f32)OS::PlatformTimer::ticksToMilliSeconds(deltaTicks);

        fpsTimer += delta;
        if (fpsTimer > 1000.0)
            fpsTimer -= 1000.0, fps = frames, frames = 0;

        titleTimer += delta;
        if (titleTimer > 40.0)
        {
            titleTimer -= 40.0;
            window.SetTitle(L"%fms (%d FPS)", delta, fps);
        }

        // Draw
        //for (uint32 x = 0; x < width; x++)
        //{
        //    for (uint32 y = 0; y < height; y++)
        //    {
        //        bitmap.SetPixel(x, y, 0, 0, 0);
        //    }
        //}

        stars.UpdateAndRender(bitmap, delta / 1000.0);

        bitmap.SwapBuffers();
    }

    return 0;
}