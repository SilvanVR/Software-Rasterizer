#include "stdafx.h"
#include "timer/platform_timer.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

struct ColorB
{
    unsigned char r, g, b, a;

public:
    ColorB(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 0xff)
        : r(r), g(g), b(b), a(a)
    {
    }

    ColorB(int val)
    {
        r = (val & 0x00ff0000) >> 16;
        g = (val & 0x0000ff00) >> 8;
        b = (val & 0x000000ff) >> 0;
        a = (val & 0xff000000) >> 24;
    }

    ColorB operator*(f32 val) const
    {
        return ColorB{ unsigned char(r * val), unsigned char(g * val), unsigned char(b * val), unsigned char(a * val) };
    }

    ColorB operator+(ColorB c) const
    {
        return ColorB{ unsigned char(r + c.r), unsigned char(g + c.g), unsigned char(b + c.b), unsigned char(a + c.a) };
    }
};


struct Point2D
{
    f32 x;
    f32 y;
};

struct Vec3
{
    f32 x, y, z;
};

struct ScanRange
{
    uint32 min;
    uint32 max;
};

struct Triangle2D
{
    Point2D p0;
    Point2D p1;
    Point2D p2;

    ColorB c0;
    ColorB c1;
    ColorB c2;

    Vec3 ComputeBarycentricCoordinates(const Point2D& p) const
    {
        f32 w0 = ((p1.y - p2.y)*(p.x - p2.x) + (p2.x - p1.x)*(p.y - p2.y)) / ((p1.y - p2.y)*(p0.x-p2.x)+(p2.x-p1.x)*(p0.y-p2.y));
        f32 w1 = ((p2.y - p0.y)*(p.x - p2.x) + (p0.x - p2.x)*(p.y - p2.y)) / ((p1.y - p2.y)*(p0.x - p2.x) + (p2.x - p1.x)*(p0.y - p2.y));
        f32 w2 = 1 - w1 - w0;
        return Vec3{ w0, w1, w2 };
    }
};

bool g_bSaveImageToFile = false;

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
        printf("WM_PAINT\n");
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (wParam == 0x53)
            g_bSaveImageToFile = true;
        
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

        m_hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, strClassName, strTitle, windowStyle, x, y, w, h, NULL, NULL, m_hInstance, NULL);

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
        m_pPixels = (ColorB*)calloc(width * height, sizeof(ColorB));
        m_scanBuffer = (ScanRange*)calloc(height, sizeof(ScanRange));
    }
    ~Bitmap() { free(m_pPixels); free(m_scanBuffer); }

    inline void Clear()
    {
        memset(m_pPixels, 0, m_width * m_height * sizeof(ColorB));
    }

    inline void Clear(ColorB col)
    {
        for (uint32 x = 0; x < m_width; x++)
            for (uint32 y = 0; y < m_height; y++)
                SetPixel(x, y, col);
    }

    inline void SetPixel(uint32 x, uint32 y, ColorB col)
    {
        int index = y * m_width + x;
        m_pPixels[index] = col;
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
    inline void* GetData() const { return m_pPixels; }
    std::vector<char> GetDataRGB() const
    { 
        std::vector<char> pPixels(m_width * m_height * 3, 0);
        for (uint32 x = 0; x < m_width; x++)
        {
            for (uint32 y = 0; y < m_height; y++)
            {
                ColorB col = m_pPixels[m_width * y + x];
                int index = m_width * y + x;
                pPixels[3 * index    ] = col.b;
                pPixels[3 * index + 1] = col.g;
                pPixels[3 * index + 2] = col.r;
            }
        }
        return pPixels;
    }

    void FillTriangle(const Triangle2D& t)
    {
        //ResetScanbuffer();

        Point2D min{};
        Point2D max{};

        min.x = t.p0.x < t.p1.x ? (t.p0.x < t.p2.x ? t.p0.x : t.p2.x) : (t.p1.x < t.p2.x ? t.p1.x : t.p2.x);
        min.y = t.p0.y < t.p1.y ? (t.p0.y < t.p2.y ? t.p0.y : t.p2.y) : (t.p1.y < t.p2.y ? t.p1.y : t.p2.y);

        max.x = t.p0.x > t.p1.x ? (t.p0.x > t.p2.x ? t.p0.x : t.p2.x) : (t.p1.x > t.p2.x ? t.p1.x : t.p2.x);
        max.y = t.p0.y > t.p1.y ? (t.p0.y > t.p2.y ? t.p0.y : t.p2.y) : (t.p1.y > t.p2.y ? t.p1.y : t.p2.y);

        for (int x = min.x; x < max.x; x++)
        {
            for (int y = min.y; y < max.y; y++)
            {
                Point2D p{ x, y };
                Vec3 bary = t.ComputeBarycentricCoordinates(p);

                if (bary.x < -0.00001f || bary.y < -0.00001f || bary.z < -0.00001f)
                    continue;

                ColorB col = t.c0 * bary.x + t.c1 * bary.y + t.c2 * bary.z;
                SetPixel(p.x, p.y, col);
            }
        }
    }

    void DrawLine(Point2D p0, Point2D p1, ColorB col)
    {
        // Bresenham's line algorithm
        const bool steep = (fabs(p1.y - p0.y) > fabs(p1.x - p0.x));
        if (steep)
        {
            std::swap(p0.x, p0.y);
            std::swap(p1.x, p1.y);
        }

        if (p0.x > p1.x)
        {
            std::swap(p0.x, p1.x);
            std::swap(p0.y, p1.y);
        }

        const float dx = p1.x - p0.x;
        const float dy = fabs(p1.y - p0.y);

        float error = dx / 2.0f;
        const int ystep = (p0.y < p1.y) ? 1 : -1;
        int y = (int)p0.y;

        const int maxX = (int)p1.x;

        for (int x = (int)p0.x; x < maxX; x++)
        {
            if (steep)
                SetPixel(y, x, col);
            else
                SetPixel(x, y, col);

            error -= dy;
            if (error < 0)
            {
                y += ystep;
                error += dx;
            }
        }
    }

private:
    uint32     m_width;
    uint32     m_height;
    HDC        m_hdc;
    ColorB    *m_pPixels;
    ScanRange *m_scanBuffer;

    void FillScanline(ColorB col)
    {
        for (uint32 y = 0; y < m_height; y++)
        {
            if (m_scanBuffer[y].min < m_scanBuffer[y].max)
            {
                for (uint32 xMin = m_scanBuffer[y].min; xMin < m_scanBuffer[y].max; xMin++)
                    SetPixel(xMin, y, col);
            }
        }
    }

    void ResetScanbuffer()
    {
        for (uint32 y = 0; y < m_height; y++)
        {
            m_scanBuffer[y].min = UINT_MAX;
            m_scanBuffer[y].max = 0;
        }
    }
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
        f32 halfWidth = bitmap.GetWidth() / 2.0f;
        f32 halfHeight = bitmap.GetHeight() / 2.0f;
        for (int i = 0; i < m_numStars; i++)
        {
            m_starZ[i] -= delta * m_speed;
            if (m_starZ[i] <= 0)
                InitStar(i);

            uint32 x = (int)((m_starX[i]/m_starZ[i]) * halfWidth + halfWidth);
            uint32 y = (int)((m_starY[i]/m_starZ[i]) * halfHeight + halfHeight);

            if (x < 0 || x >= bitmap.GetWidth() ||
                y < 0 || y >= bitmap.GetHeight())
            {
                InitStar(i);
            }
            else
            {
                bitmap.SetPixel(x, y, ColorB(0xffffff));
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

        bitmap.Clear();

        // Draw
        //for (uint32 x = 0; x < width; x++)
        //{
        //    for (uint32 y = 0; y < height; y++)
        //    {
        //        bitmap.SetPixel(x, y, 0xffff0000);
        //    }
        //}

        //stars.UpdateAndRender(bitmap, delta / 1000.0);

        #define PROTOS_HEAD_ICOUNT 32178
        #define PROTOS_HEAD_VCOUNT 5584
        extern f32 arrProtosTexCoordsHead[];
        extern uint16 arrProtosIndicesHead[];
        extern f32 arrSplitMap_BrowL[]; extern f32 arrSplitMap_BrowR[];
        extern f32 arrSplitMap_EyeL[]; extern f32 arrSplitMap_EyeR[];
        extern f32 arrSplitMap_Nose[];
        extern f32 arrSplitMap_EarL[]; extern f32 arrSplitMap_EarR[];
        extern f32 arrSplitMap_CheekL[]; extern f32 arrSplitMap_CheekR[];
        extern f32 arrSplitMap_Mouth[];
        extern f32 arrSplitMap_Jaw[];
        extern f32 arrSplitMap_NeckHead[];
        f32* pSplitMaps[] = { arrSplitMap_BrowL, arrSplitMap_BrowR, arrSplitMap_EyeL, arrSplitMap_EyeR, arrSplitMap_Nose, arrSplitMap_EarL, arrSplitMap_EarR, 
                              arrSplitMap_CheekL, arrSplitMap_CheekR, arrSplitMap_Mouth, arrSplitMap_Jaw, arrSplitMap_NeckHead };
        //for (uint32 vert = 0; vert < PROTOS_HEAD_VCOUNT; vert++)
        //{
        //    Point2D pixelCoord{ arrProtosTexCoordsHead[2*vert] * bitmap.GetWidth(), arrProtosTexCoordsHead[2* vert+1] * bitmap.GetHeight() };

        //    f32 splitMapVal = arrSplitMap_Mouth[vert];
        //    char v = char(splitMapVal * 255);
        //    bitmap.SetPixel(pixelCoord.x, pixelCoord.y, v, v, v);
        //}

        for (uint16 idx = 0; idx < PROTOS_HEAD_ICOUNT; idx += 3)
        {
            uint16 v0 = arrProtosIndicesHead[idx];
            uint16 v1 = arrProtosIndicesHead[idx+1];
            uint16 v2 = arrProtosIndicesHead[idx+2];

            Point2D p1{ arrProtosTexCoordsHead[2 * v0], arrProtosTexCoordsHead[2 * v0 + 1] };
            Point2D p2{ arrProtosTexCoordsHead[2 * v1], arrProtosTexCoordsHead[2 * v1 + 1] };
            Point2D p3{ arrProtosTexCoordsHead[2 * v2], arrProtosTexCoordsHead[2 * v2 + 1] };

            Point2D pC1{ p1.x * bitmap.GetWidth(), p1.y * bitmap.GetHeight() };
            Point2D pC2{ p2.x * bitmap.GetWidth(), p2.y * bitmap.GetHeight() };
            Point2D pC3{ p3.x * bitmap.GetWidth(), p3.y * bitmap.GetHeight() };

            unsigned char col0 = unsigned char(arrSplitMap_BrowL[v0] * 255);
            unsigned char col1 = unsigned char(arrSplitMap_BrowL[v1] * 255);
            unsigned char col2 = unsigned char(arrSplitMap_BrowL[v2] * 255);

            ColorB c0{ col0, col0, col0 };
            ColorB c1{ col1, col1, col1 };
            ColorB c2{ col2, col2, col2 };

            bitmap.FillTriangle({ pC1, pC2, pC3, c0, c1, c2});

            //bitmap.DrawLine(pC1, pC2, 0);
            //bitmap.DrawLine(pC2, pC3, 0);
            //bitmap.DrawLine(pC3, pC1, 0);
        }

        //Point2D p0{0, 0};
        //Point2D p1{bitmap.GetWidth(), 0};
        //Point2D p2{0, bitmap.GetHeight()};
        //bitmap.FillTriangle({ p0, p1, p2, 0xff, 0xff00, 0xff0000 });

        if (g_bSaveImageToFile)
        {
            g_bSaveImageToFile = false;

            const char* path = "test.png";
            int row = bitmap.GetWidth() * 3;
            printf("Save image to file: %s...\n", path);
            stbi_write_png(path, bitmap.GetWidth(), bitmap.GetHeight(), 3, bitmap.GetDataRGB().data(), row);

            //const char* path = "test.bmp";
            //printf("Save image to file: %s...\n", path);
            //int res = stbi_write_bmp(path, bitmap.GetWidth(), bitmap.GetHeight(), 3, bitmap.GetDataRGB().data());
            //printf("Result: %d\n", res);
        }

        bitmap.SwapBuffers();
    }

    return 0;
}