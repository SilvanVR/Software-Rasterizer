#include "stdafx.h"
#include "timer/platform_timer.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#include "rasterizer/rasterizer.h"


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
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        //if (wParam == 0x53) // S key
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}


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

            uint32 x = (int)((m_starX[i] / m_starZ[i]) * halfWidth + halfWidth);
            uint32 y = (int)((m_starY[i] / m_starZ[i]) * halfHeight + halfHeight);

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

    Window window(width, height, false, WndProc);
    Rasterizer rasterizer(window);

    uint64 prevTicks = OS::PlatformTimer::getTicks();

    uint64 frames = 0;
    uint64 fps = 0;
    f32 fpsTimer = 0.0;
    f32 titleTimer = 0.0;

    Stars3D stars(4096, 64.0f, 20.0f);

    while (window.HandleMessages())
    {
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

        rasterizer.Clear();

        // Draw
        //for (uint32 x = 0; x < width; x++)
        //{
        //    for (uint32 y = 0; y < height; y++)
        //    {
        //        rasterizer.GetBitmap().SetPixel(x, y, ColorB{ 128, 128, 128 });
        //    }
        //}

        stars.UpdateAndRender(rasterizer.GetBitmap(), delta / 1000.0f);

        //Point2D p0{0, 0};
        //Point2D p1{(f32)window.GetWidth(), 0};
        //Point2D p2{0, (f32)window.GetHeight()};
        //rasterizer.FillTriangle({ p0, p1, p2, 0xff, 0xff00, 0xff0000 });

        rasterizer.SwapBuffers();
    }

    return 0;
}



#define NUM_REGIONS   12

#define REGION_BROWS  12
#define REGION_EYES   13
#define REGION_EARS   14
#define REGION_CHEEKS 15

#define NUM_EXTENDED_REGIONS 16

#define PROTOS_HEAD_ICOUNT 32178
#define PROTOS_HEAD_VCOUNT 5584

extern f32 arrProtosTexCoordsHead[];
extern uint16 arrProtosIndicesHead[];
extern f64 arrSplitMap_BrowL[]; extern f64 arrSplitMap_BrowR[];
extern f64 arrSplitMap_EyeL[]; extern f64 arrSplitMap_EyeR[];
extern f64 arrSplitMap_Nose[];
extern f64 arrSplitMap_EarL[]; extern f64 arrSplitMap_EarR[];
extern f64 arrSplitMap_CheekL[]; extern f64 arrSplitMap_CheekR[];
extern f64 arrSplitMap_Mouth[];
extern f64 arrSplitMap_Jaw[];
extern f64 arrSplitMap_NeckHead[];
f64* pSplitMaps[NUM_REGIONS] = { arrSplitMap_BrowL, arrSplitMap_BrowR, arrSplitMap_EyeL, arrSplitMap_EyeR, arrSplitMap_Nose, arrSplitMap_EarL, arrSplitMap_EarR,
                                 arrSplitMap_CheekL, arrSplitMap_CheekR, arrSplitMap_Mouth, arrSplitMap_Jaw, arrSplitMap_NeckHead };

void RasterizeRegions(Rasterizer& rasterizer, std::vector<int> regions, int nDrawMode)
{
    uint32 width = rasterizer.GetBitmap().GetWidth();
    uint32 height = rasterizer.GetBitmap().GetHeight();
    for (uint16 idx = 0; idx < PROTOS_HEAD_ICOUNT; idx += 3)
    {
        uint16 v0 = arrProtosIndicesHead[idx];
        uint16 v1 = arrProtosIndicesHead[idx + 1];
        uint16 v2 = arrProtosIndicesHead[idx + 2];

        Point2D p1{ arrProtosTexCoordsHead[2 * v0], arrProtosTexCoordsHead[2 * v0 + 1] };
        Point2D p2{ arrProtosTexCoordsHead[2 * v1], arrProtosTexCoordsHead[2 * v1 + 1] };
        Point2D p3{ arrProtosTexCoordsHead[2 * v2], arrProtosTexCoordsHead[2 * v2 + 1] };

        Point2D pC1{ p1.x * width, p1.y * height };
        Point2D pC2{ p2.x * width, p2.y * height };
        Point2D pC3{ p3.x * width, p3.y * height };

        f64 fSplitVal0 = 0.0f;
        f64 fSplitVal1 = 0.0f;
        f64 fSplitVal2 = 0.0f;
        for (int reg : regions)
        {
            fSplitVal0 += pSplitMaps[reg][v0];
            fSplitVal1 += pSplitMaps[reg][v1];
            fSplitVal2 += pSplitMaps[reg][v2];
        }

        unsigned char col0 = unsigned char(fSplitVal0 * 255);
        unsigned char col1 = unsigned char(fSplitVal1 * 255);
        unsigned char col2 = unsigned char(fSplitVal2 * 255);

        if (col0 == 0 && col1 == 0 && col2 == 0)
            continue;

        ColorB c0{ col0, col0, col0, col0 };
        ColorB c1{ col1, col1, col1, col1 };
        ColorB c2{ col2, col2, col2, col2 };

        if (nDrawMode == 0 || nDrawMode == 1)
            rasterizer.FillTriangle({ pC1, pC2, pC3, c0, c1, c2 });

        if (nDrawMode == 1 || nDrawMode == 2)
        {
            rasterizer.DrawLine(pC1, pC2, c0);
            rasterizer.DrawLine(pC2, pC3, c1);
            rasterizer.DrawLine(pC3, pC1, c2);
        }
        if (nDrawMode == 3)
        {
            rasterizer.GetBitmap().SetPixel((uint32)pC1.x, (uint32)pC1.y, c0);
            rasterizer.GetBitmap().SetPixel((uint32)pC2.x, (uint32)pC2.y, c1);
            rasterizer.GetBitmap().SetPixel((uint32)pC3.x, (uint32)pC3.y, c2);
        }
    }
}

//int main(int argc, char *argv[])
//{
//    if (argc == 2)
//    {
//        string str(argv[1]);
//        if (str == "--help")
//        {
//            printf("Call .exe with these arguments:\n 1: Width \n 2: Height\n 3: Drawmode (0: Fill | 1: Fill+Wireframe | 2: Wireframe | 3: Points) \n Example: call ***.exe 512 512 1\n");
//            system("pause");
//            return 0;
//        }
//    }
//
//    uint32 width = 1024;
//    uint32 height = 1024;
//
//    if (argc >= 3)
//    {
//        width = std::stoi(argv[1]);
//        height = std::stoi(argv[2]);
//    }
//
//    int nDrawMode = 2;
//    if (argc >= 4)
//    {
//        nDrawMode = std::stoi(argv[3]);
//    }
//
//    Window window(width, height, false, WndProc);
//    Rasterizer rasterizer(window);
//
//    int region = 15;
//
//    uint64 prevTicks = OS::PlatformTimer::getTicks();
//
//    uint64 frames = 0;
//    uint64 fps = 0;
//    f32 fpsTimer = 0.0;
//    f32 titleTimer = 0.0;
//
//    while (window.HandleMessages())
//    {
//        frames++;
//
//        uint64 curTicks = OS::PlatformTimer::getTicks();
//        uint64 deltaTicks = curTicks - prevTicks;
//        prevTicks = curTicks;
//
//        f32 delta = (f32)OS::PlatformTimer::ticksToMilliSeconds(deltaTicks);
//
//        fpsTimer += delta;
//        if (fpsTimer > 1000.0)
//            fpsTimer -= 1000.0, fps = frames, frames = 0;
//
//        titleTimer += delta;
//        if (titleTimer > 40.0)
//        {
//            titleTimer -= 40.0;
//            window.SetTitle(L"%fms (%d FPS)", delta, fps);
//        }
//
//        rasterizer.Clear();
//
//        if (region < NUM_REGIONS)
//        {
//            RasterizeRegions(rasterizer, { region }, nDrawMode);
//        }
//        else
//        {
//            // Symmetrical regions
//            switch (region)
//            {
//            case REGION_BROWS:  RasterizeRegions(rasterizer, { 0, 1 }, nDrawMode); break;
//            case REGION_EYES:   RasterizeRegions(rasterizer, { 2, 3 }, nDrawMode); break;
//            case REGION_EARS:   RasterizeRegions(rasterizer, { 5, 6 }, nDrawMode); break;
//            case REGION_CHEEKS: RasterizeRegions(rasterizer, { 7, 8 }, nDrawMode); break;
//            }
//        }
//
//        rasterizer.SwapBuffers();
//    }
//;
//
//    //for (int region = 0; region < NUM_EXTENDED_REGIONS; region++)
//    //{
//    //    rasterizer.Clear();
//
//    //    if (region < NUM_REGIONS)
//    //    {
//    //        RasterizeRegions(rasterizer, { region }, nDrawMode);
//    //    }
//    //    else
//    //    {
//    //        // Symmetrical regions
//    //        switch (region)
//    //        {
//    //        case REGION_BROWS:  RasterizeRegions(rasterizer, { 0, 1 }, nDrawMode); break;
//    //        case REGION_EYES:   RasterizeRegions(rasterizer, { 2, 3 }, nDrawMode); break;
//    //        case REGION_EARS:   RasterizeRegions(rasterizer, { 5, 6 }, nDrawMode); break;
//    //        case REGION_CHEEKS: RasterizeRegions(rasterizer, { 7, 8 }, nDrawMode); break;
//    //        }
//    //    }
//
//    //    string filename = "";
//    //    switch (region)
//    //    {
//    //    case 0:  filename = "BrowL";    break;
//    //    case 1:  filename = "BrowR";    break;
//    //    case 2:  filename = "EyeL";     break;
//    //    case 3:  filename = "EyeR";     break;
//    //    case 4:  filename = "Nose";     break;
//    //    case 5:  filename = "EarL";     break;
//    //    case 6:  filename = "EarR";     break;
//    //    case 7:  filename = "CheekL";   break;
//    //    case 8:  filename = "CheekR";   break;
//    //    case 9:  filename = "Mouth";    break;
//    //    case 10: filename = "Jaw";      break;
//    //    case 11: filename = "Neckhead"; break;
//    //    case 12: filename = "Brows";    break;
//    //    case 13: filename = "Eyes";     break;
//    //    case 14: filename = "Ears";     break;
//    //    case 15: filename = "Cheeks";   break;
//    //    }
//
//    //    string ext = ".png";
//    //    string path = filename + ext;
//    //    int row = width * 3;
//    //    printf("Saving image to file: %s...\n", path.c_str());
//    //    stbi_write_png(path.c_str(), width, height, 3, rasterizer.GetBitmap().GetDataRGB().data(), row);
//
//    //    rasterizer.SwapBuffers();
//    //}
//
//    return 0;
//}