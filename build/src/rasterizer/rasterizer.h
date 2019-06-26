#pragma once

////////////////////////////////////////////////////////////////////////////////////////////
struct ColorB
{
    unsigned char b, g, r, a;

public:
    ColorB(unsigned char b, unsigned char g, unsigned char r, unsigned char a = 0xff)
        : b(b), g(g), r(r), a(a)
    {
    }

    ColorB(int val)
    {
        b = (val & 0x000000ff) >> 0;
        g = (val & 0x0000ff00) >> 8;
        r = (val & 0x00ff0000) >> 16;
        a = (val & 0xff000000) >> 24;
    }

    ColorB operator*(f32 val) const
    {
        return ColorB{ unsigned char(b * val), unsigned char(g * val),  unsigned char(r * val), unsigned char(a * val) };
    }

    ColorB operator+(ColorB c) const
    {
        return ColorB{ unsigned char(b + c.b), unsigned char(g + c.g), unsigned char(r + c.r), unsigned char(a + c.a) };
    }

    ColorB operator+=(ColorB c) const
    {
        return *this + c;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////
struct Point2D
{
    f32 x;
    f32 y;
};

////////////////////////////////////////////////////////////////////////////////////////////
struct Vec3
{
    f32 x, y, z;
};

////////////////////////////////////////////////////////////////////////////////////////////
struct ScanRange
{
    uint32 min;
    uint32 max;
};

////////////////////////////////////////////////////////////////////////////////////////////
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
        f32 w0 = ((p1.y - p2.y)*(p.x - p2.x) + (p2.x - p1.x)*(p.y - p2.y)) / ((p1.y - p2.y)*(p0.x - p2.x) + (p2.x - p1.x)*(p0.y - p2.y));
        f32 w1 = ((p2.y - p0.y)*(p.x - p2.x) + (p0.x - p2.x)*(p.y - p2.y)) / ((p1.y - p2.y)*(p0.x - p2.x) + (p2.x - p1.x)*(p0.y - p2.y));
        f32 w2 = 1 - w1 - w0;
        return Vec3{ w0, w1, w2 };
    }
};

////////////////////////////////////////////////////////////////////////////////////////////
class Window
{
public:
    Window(uint32 width, uint32 height, bool bCenter, WNDPROC wndProc)
        : m_width(width), m_height(height)
    {
        LPCWSTR strClassName = L"Software Rasterizer";
        LPCWSTR strTitle = L"Software Rasterizer";

        //Step 1: Registering the Window Class
        WNDCLASSEX wc;
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = 0;
        wc.lpfnWndProc = wndProc;
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

        m_hdc = GetDC(m_hwnd);
    }

    inline uint32 GetWidth() const { return m_width; }
    inline uint32 GetHeight() const { return m_height; }

    void SetTitle(const wchar_t* newTitle, ...)
    {
        WCHAR buffer[256]{};
        va_list args;
        va_start(args, newTitle);
        vswprintf(buffer, 256, newTitle, args);
        va_end(args);
        SetWindowText(m_hwnd, buffer);
    }

    void SwapBuffers(const void* pPixels) const
    {
        HBITMAP bitmap = CreateBitmap(m_width, m_height, 1, 32, (void*)pPixels);

        HDC src = CreateCompatibleDC(m_hdc);
        SelectObject(src, bitmap);

        BitBlt(m_hdc, 0, 0, m_width, m_height, src, 0, 0, SRCCOPY);

        DeleteDC(src);
        DeleteObject(bitmap);
    }

    bool HandleMessages() const
    {
        bool bKeepRunning = true;

        MSG msg;
        while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                bKeepRunning = false;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        return bKeepRunning;
    }

private:
    uint32    m_width;
    uint32    m_height;
    HWND      m_hwnd;
    HINSTANCE m_hInstance;
    HDC       m_hdc;
};

class Bitmap
{
public:
    Bitmap(uint32 width, uint32 height)
        : m_width(width), m_height(height)
    {
        m_pPixels = (ColorB*)calloc(m_width * m_height, sizeof(ColorB));
    }
    ~Bitmap() { free(m_pPixels); }

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

    inline void BlendPixel(uint32 x, uint32 y, ColorB col)
    {
        int index = y * m_width + x;

        f32 a = col.a / 255.0f;
        m_pPixels[index].r = unsigned char(m_pPixels[index].r * (1.0f - a) + col.r * a);
        m_pPixels[index].g = unsigned char(m_pPixels[index].g * (1.0f - a) + col.g * a);
        m_pPixels[index].b = unsigned char(m_pPixels[index].b * (1.0f - a) + col.b * a);
    }

    inline void SetPixel(uint32 x, uint32 y, ColorB col)
    {
        int index = y * m_width + x;
        m_pPixels[index] = col;
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
                pPixels[3 * index] = col.b;
                pPixels[3 * index + 1] = col.g;
                pPixels[3 * index + 2] = col.r;
            }
        }
        return pPixels;
    }

private:
    uint32  m_width;
    uint32  m_height;
    ColorB *m_pPixels;
};


////////////////////////////////////////////////////////////////////////////////////////////
class Rasterizer
{
public:
    Rasterizer(Window& window)
        : m_window(window), m_bitmap(window.GetWidth(), window.GetHeight())
    {
        m_scanBuffer = (ScanRange*)calloc(window.GetHeight(), sizeof(ScanRange));
    }
    ~Rasterizer()
    {
        free(m_scanBuffer);
    }

    Bitmap& GetBitmap() { return m_bitmap; }
    Window& GetWindow() { return m_window; }

    void Clear() { m_bitmap.Clear(); }
    void SwapBuffers() const { m_window.SwapBuffers(m_bitmap.GetData()); }

    ///////////////////////////////////////////////////////////////////////////////////////
    void FillTriangle(const Triangle2D& t)
    {
        Point2D min{};
        Point2D max{};

        min.x = t.p0.x < t.p1.x ? (t.p0.x < t.p2.x ? t.p0.x : t.p2.x) : (t.p1.x < t.p2.x ? t.p1.x : t.p2.x);
        min.y = t.p0.y < t.p1.y ? (t.p0.y < t.p2.y ? t.p0.y : t.p2.y) : (t.p1.y < t.p2.y ? t.p1.y : t.p2.y);

        max.x = t.p0.x > t.p1.x ? (t.p0.x > t.p2.x ? t.p0.x : t.p2.x) : (t.p1.x > t.p2.x ? t.p1.x : t.p2.x);
        max.y = t.p0.y > t.p1.y ? (t.p0.y > t.p2.y ? t.p0.y : t.p2.y) : (t.p1.y > t.p2.y ? t.p1.y : t.p2.y);

        for (int x = (int)min.x; x <= (int)max.x; x++)
        {
            for (int y = (int)min.y; y <= (int)max.y; y++)
            {
                Point2D p{ (f32)x, (f32)y };
                Vec3 bary = t.ComputeBarycentricCoordinates(p);

                if (bary.x < -0.00001f || bary.y < -0.00001f || bary.z < -0.00001f)
                    continue;

                ColorB col = t.c0 * bary.x + t.c1 * bary.y + t.c2 * bary.z;
                m_bitmap.SetPixel((uint32)p.x, (uint32)p.y, col);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////
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
                m_bitmap.SetPixel(y, x, col);
            else
                m_bitmap.SetPixel(x, y, col);

            error -= dy;
            if (error < 0)
            {
                y += ystep;
                error += dx;
            }
        }
    }

private:
    Window&    m_window;
    Bitmap     m_bitmap;
    ScanRange *m_scanBuffer;

    void FillScanline(ColorB col)
    {
        for (uint32 y = 0; y < m_bitmap.GetHeight(); y++)
        {
            if (m_scanBuffer[y].min < m_scanBuffer[y].max)
            {
                for (uint32 xMin = m_scanBuffer[y].min; xMin < m_scanBuffer[y].max; xMin++)
                    m_bitmap.SetPixel(xMin, y, col);
            }
        }
    }

    void ResetScanbuffer()
    {
        for (uint32 y = 0; y < m_bitmap.GetHeight(); y++)
        {
            m_scanBuffer[y].min = UINT_MAX;
            m_scanBuffer[y].max = 0;
        }
    }
};