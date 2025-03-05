#include <opencv2/opencv.hpp>
#include <windows.h>
#include <vector>
#include <string>
#include <GdiPlus.h>
#include <atlimage.h>

#include "savebit.h"

using namespace cv;

#pragma comment(lib,"Gdiplus.lib")

#define RGBA(r,g,b,a)          (COLORREF)(((BYTE)(r) |((WORD)((BYTE)(g)) << 8)) |(((DWORD)((BYTE)(b)) << 16)) |(((DWORD)((BYTE)(a)) << 24)))

// 全局变量
const wchar_t* g_ClassName = L"TransparentWindow";
const UINT_PTR TIMER_ID = 1;
const UINT_PTR UPDATE_ID = 2;
Mat g_DisplayBuffer; // 显示缓冲

// 全局初始化
ULONG_PTR gdiplusToken;
Gdiplus::GdiplusStartupInput gdiplusStartupInput;


void UpdateDisplayBuffer(HWND hwnd);



auto ConvertCVMatToBMP(cv::Mat frame) -> HBITMAP
{
    auto convertOpenCVBitDepthToBits = [](const int32_t value)
        {
            auto regular = 0u;

            switch (value)
            {
            case CV_8U:
            case CV_8S:
                regular = 8u;
                break;

            case CV_16U:
            case CV_16S:
                regular = 16u;
                break;

            case CV_32S:
            case CV_32F:
                regular = 32u;
                break;

            case CV_64F:
                regular = 64u;
                break;

            default:
                regular = 0u;
                break;
            }

            return regular;
        };

    auto imageSize = frame.size();
    assert(imageSize.width && "invalid size provided by frame");
    assert(imageSize.height && "invalid size provided by frame");

    if (imageSize.width && imageSize.height)
    {
        auto headerInfo = BITMAPINFOHEADER{};
        ZeroMemory(&headerInfo, sizeof(headerInfo));

        headerInfo.biSize = sizeof(headerInfo);
        headerInfo.biWidth = imageSize.width;
        headerInfo.biHeight = -(imageSize.height); // negative otherwise it will be upsidedown
        headerInfo.biPlanes = 1;// must be set to 1 as per documentation frame.channels();

        const auto bits = convertOpenCVBitDepthToBits(frame.depth());
        headerInfo.biBitCount = frame.channels() * bits;

        auto bitmapInfo = BITMAPINFO{};
        ZeroMemory(&bitmapInfo, sizeof(bitmapInfo));

        bitmapInfo.bmiHeader = headerInfo;
        bitmapInfo.bmiColors->rgbBlue = 0;
        bitmapInfo.bmiColors->rgbGreen = 0;
        bitmapInfo.bmiColors->rgbRed = 0;
        bitmapInfo.bmiColors->rgbReserved = 0;

        auto dc = GetDC(nullptr);
        assert(dc != nullptr && "Failure to get DC");
        auto bmp = CreateDIBitmap(dc,
            &headerInfo,
            CBM_INIT,
            frame.data,
            &bitmapInfo,
            DIB_RGB_COLORS);
        assert(bmp != nullptr && "Failure creating bitmap from captured frame");

        return bmp;
    }
    else
    {
        return nullptr;
    }
}

void ConvertMatToCImageWithAlpha(const cv::Mat& src, CImage& dst) {
    // 确保 Mat 是四通道 BGRA 格式
    CV_Assert(src.type() == CV_8UC4);

    // 创建 32 位 CImage（负高度确保 top-down 布局）
    dst.Create(src.cols, -src.rows, 32);
    dst.SetHasAlphaChannel(true);
    if (dst.IsNull()) {
        throw std::runtime_error("Failed to create CImage");
    }

    // 获取 CImage 像素数据指针和步长
    BYTE* pDst = reinterpret_cast<BYTE*>(dst.GetBits());
    const int dstStride = dst.GetPitch();
    const int srcStride = src.step;
    int cns = dst.GetTransparentColor();

    // 直接内存复制（若步长一致）
    if (srcStride == dstStride) {
        memcpy(pDst, src.data, src.rows * srcStride);
    }
    else {
        // 逐行拷贝处理步长差异
        for (int y = 0; y < src.rows; ++y) {
            
            const BYTE* pSrcRow = src.ptr<BYTE>(y);
            BYTE* pDstRow = pDst + y * dstStride;
            memcpy(pDstRow, pSrcRow, src.cols * 4); // 每像素 4 字节
        }
    }
}


//Gdiplus::Bitmap* CreateBitmapFromHBITMAP(IN HBITMAP hBitmap)
//{
//    BITMAP bmp = { 0 };
//    if (0 == GetObject(hBitmap, sizeof(BITMAP), (LPVOID)&bmp))
//    {
//        return FALSE;
//    }
//
//    // Although we can get bitmap data address by bmp.bmBits member of BITMAP 
//    // which is got by GetObject function sometime,
//    // we can determine the bitmap data in the HBITMAP is arranged bottom-up 
//    // or top-down, so we should always use GetDIBits to get bitmap data.
//    BYTE* piexlsSrc = NULL;
//    LONG cbSize = bmp.bmWidthBytes * bmp.bmHeight;
//    piexlsSrc = new BYTE[cbSize];
//
//    BITMAPINFO bmpInfo = { 0 };
//    // We should initialize the first six members of BITMAPINFOHEADER structure.
//    // A bottom-up DIB is specified by setting the height to a positive number, 
//    // while a top-down DIB is specified by setting the height to a negative number.
//    bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
//    bmpInfo.bmiHeader.biWidth = bmp.bmWidth;
//    bmpInfo.bmiHeader.biHeight = bmp.bmHeight; // 正数，说明数据从下到上，如未负数，则从上到下
//    bmpInfo.bmiHeader.biPlanes = bmp.bmPlanes;
//    bmpInfo.bmiHeader.biBitCount = bmp.bmBitsPixel;
//    bmpInfo.bmiHeader.biCompression = BI_RGB;
//
//    HDC hdcScreen = CreateDC(L"DISPLAY", NULL, NULL, NULL);
//    LONG cbCopied = GetDIBits(hdcScreen, hBitmap, 0, bmp.bmHeight,
//        piexlsSrc, &bmpInfo, DIB_RGB_COLORS);
//    DeleteDC(hdcScreen);
//    if (0 == cbCopied)
//    {
//        delete[] piexlsSrc;
//        return FALSE;
//    }
//
//    // Create an GDI+ Bitmap has the same dimensions with hbitmap
//    Gdiplus::Bitmap* pBitmap = new Gdiplus::Bitmap(bmp.bmWidth, bmp.bmHeight, PixelFormat32bppPARGB);
//
//    // Access to the Gdiplus::Bitmap's pixel data
//    Gdiplus::BitmapData bitmapData;
//    Rect rect(0, 0, bmp.bmWidth, bmp.bmHeight);
//    if (Gdiplus::Ok != pBitmap->LockBits(&rect, Gdiplus::ImageLockModeRead,
//        PixelFormat32bppPARGB, &bitmapData))
//    {   
//        return NULL;
//    }
//
//    BYTE* pixelsDest = (BYTE*)bitmapData.Scan0;
//    int nLinesize = bmp.bmWidth * sizeof(UINT);
//    int nHeight = bmp.bmHeight;
//
//    // Copy pixel data from HBITMAP by bottom-up.
//    for (int y = 0; y < nHeight; y++)
//    {
//        // 从下到上复制数据，因为前面设置高度时是正数。
//        memcpy_s(
//            (pixelsDest + y * nLinesize),
//            nLinesize,
//            (piexlsSrc + (nHeight - y - 1) * nLinesize),
//            nLinesize);
//    }
//
//    // Copy the data in temporary buffer to pBitmap
//    if (Gdiplus::Ok != pBitmap->UnlockBits(&bitmapData))
//    {
//        delete pBitmap;
//    }
//
//    delete[] piexlsSrc;
//    return pBitmap;
//}

bool DrawFullText(Mat& drawmat, Size spacing, String text, HersheyFonts font, double fontsize, Scalar color,
    int thickness = 1, int lineType = LINE_8) {
    std::vector<Point> positions;

    int h = drawmat.rows;
    int w = drawmat.cols;

    int baseLine = 0;
    Size textSize = getTextSize(text, font,
        fontsize, thickness, &baseLine);

    int spaceh = textSize.height + spacing.height;
    int spacew = textSize.width + spacing.width;

    for (int i = h; i > 0; i -= spaceh) {
        for (int j = 0; j < w; j += spacew) {
            putText(drawmat, text,
                Point(j, i),
                font,
                fontsize,
                color,
                thickness, lineType);
        }
    }

    return true;
}

//Gdiplus::Bitmap Mat2HBitmap(Mat& mat)
//{
//    //MAT类的TYPE=（nChannels-1+ CV_8U）<<3int nChannels=(mat.type()>>3)-CV_8U+1;
//
//    /*int iSize = mat.cols * mat.rows * mat.channels();
//    hBmp = CreateBitmap(mat.cols, mat.rows, 1, mat.channels() * 8, mat.data);*/
//
//    /*int size = ((((mat.cols * 32) + 31) & ~31) >> 3) * mat.rows;
//
//    Gdiplus::Bitmap* bitmap = new Gdiplus::Bitmap(mat.cols, mat.rows, PixelFormat32bppARGB);
//    Gdiplus::BitmapData data;
//    Gdiplus::Rect rect = { 0, 0, mat.cols, mat.rows };
//    bitmap->LockBits(&rect,
//        Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &data);
//    memcpy(data.Scan0, mat.data, size);
//    bitmap->UnlockBits(&data);
//    bitmap->GetHBITMAP(0, &hBmp);*/
//
//    // Gdiplus::Bitmap bitmap(mat.cols, mat.rows, mat.step1(), PixelFormat24bppRGB, mat.data);
//    Size size = mat.size();
//    Gdiplus::Bitmap bitmap(size.width, size.height, mat.step1(), PixelFormat24bppRGB, mat.data);
//    
//    return bitmap;
//
//}


int Mat2CImage(Mat* mat, CImage& img) {
    if (!mat || mat->empty())
        return -1;
    int nBPP = mat->channels() * 8;
    img.Create(mat->cols, mat->rows, nBPP);
    if (nBPP == 8)
    {
        static RGBQUAD pRGB[256];
        for (int i = 0; i < 256; i++)
            pRGB[i].rgbBlue = pRGB[i].rgbGreen = pRGB[i].rgbRed = i;
        img.SetColorTable(0, 256, pRGB);
    }
    uchar* psrc = mat->data;
    uchar* pdst = (uchar*)img.GetBits();
    int imgPitch = img.GetPitch();
    for (int y = 0; y < mat->rows; y++)
    {
        memcpy(pdst, psrc, mat->cols * mat->channels());//mat->step is incorrect for those images created by roi (sub-images!)
        psrc += mat->step;
        pdst += imgPitch;
    }

    return 0;
}

// 更新窗口内容（关键函数）
void UpdateWindowContent(HWND hwnd) {
    /*if (g_DisplayBuffer.empty() || g_DisplayBuffer.type() != CV_8UC4) {
        return;
    }*/
    UpdateDisplayBuffer(hwnd);

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    ps.fErase = true;

    // 初始化背景为透明（可选）
    CImage image;
    /*image.Create(GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN), 32);*/
    Mat2CImage(&g_DisplayBuffer, image);
    
    
    // image.Draw(hdc, { 0, 0,GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) });

    // graphics.Clear(Gdiplus::Color(0, 0, 0, 0));  // ARGB(0,0,0,0) 全透明


    HDC hdcscreen = GetDC(NULL);
    // 创建内存 DC 并初始化
    // HDC hdcMem = CreateCompatibleDC(hdc);
    HDC hdcMem = CreateCompatibleDC(hdc);

    // 创建兼容位图并清空背景
    HBITMAP hbmpMem = CreateCompatibleBitmap(hdc, GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN));
    ::SetBkMode(hdcMem, TRANSPARENT);


    /*Size size = g_DisplayBuffer.size();
    Gdiplus::Bitmap bitmap(size.width, size.height, g_DisplayBuffer.step1(), PixelFormat24bppRGB, g_DisplayBuffer.data);

    int bwidth = bitmap.GetWidth();
    int bheight = bitmap.GetHeight();
    LPBYTE pBmpBits = NULL;
    BITMAPINFO bimpi = { 0 };
    bimpi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bimpi.bmiHeader.biWidth = bwidth;
    bimpi.bmiHeader.biHeight = -bheight;
    bimpi.bmiHeader.biPlanes = 1;
    bimpi.bmiHeader.biBitCount = 32;
    bimpi.bmiHeader.biCompression = BI_RGB;
    bimpi.bmiHeader.biSizeImage = bwidth * bheight * 4;
    HBITMAP hNewBMP = CreateDIBSection(NULL, &bimpi, DIB_RGB_COLORS, (void**)&pBmpBits, NULL, NULL);

    Gdiplus::BitmapData bitmapData;
    Gdiplus::Rect rect = { 0, 0, bwidth, bheight };
    bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, bitmap.GetPixelFormat(), &bitmapData);
    memcpy(pBmpBits, (unsigned char*)bitmapData.Scan0, bwidth * bheight * 4);
    bitmap.UnlockBits(&bitmapData);*/
    
    // Mat2HBitmap(hbmpMem, g_DisplayBuffer);
    CImage water_image;
    // ConvertMatToCImageWithAlpha(g_DisplayBuffer, water_image);
    


    // bmp.GetHBITMAP(0, &hbmpMem);

    // HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, hbmpMem);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, hbmpMem);
    // water_image.Draw(hdcMem, { 0, 0, water_image.GetWidth(), water_image.GetHeight() });
    image.Draw(hdcMem, { 0, 0, image.GetWidth(), image.GetHeight() });

    // CImage srcimage;

    // srcimage.Load(L"rotate.png");
    // srcimage.Draw(hdcMem, { 0, 0, srcimage.GetWidth(), srcimage.GetHeight() });
    // srcimage.Draw(hdc, { 0, 0, srcimage.GetWidth(), srcimage.GetHeight() });

    // RECT dcmem_rect = { 0, 0, bmp.GetWidth(), bmp.GetHeight() };
    // FillRect(hdc, &dcmem_rect, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
    // ::SetBkMode(hdc, TRANSPARENT);

    // 绘制 PNG 到内存 DC（保留 Alpha）
    // Gdiplus::Graphics graphics(hdcMem);
    // graphics.DrawImage(&bmp, 0, 0, bmp.GetWidth(), bmp.GetHeight());

    // 使用 AlphaBlend 传输到窗口 DC
    BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

     DWORD type = GetObjectType(hdcMem);
     type = GetObjectType(hdc);
    /*AlphaBlend(
        hdc, 0, 0, bmp.GetWidth(), bmp.GetHeight(),
        hdcMem, 0, 0, bmp.GetWidth(), bmp.GetHeight(),
        bf
    );*/

    // 设置混合参数
    BLENDFUNCTION blend = { 0 };
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 128; // 全局透明度
    blend.AlphaFormat = AC_SRC_ALPHA; // 关键参数
    POINT ptDst = { 0, 0 }; // 窗口左上角屏幕坐标
    SIZE sizeWnd = { g_DisplayBuffer.cols, g_DisplayBuffer.rows };
    POINT ptSrc = { 0, 0 }; // 源图像起始点
    COLORREF laycolor = RGBA(0, 0, 0, 20);
    BOOL bRet = UpdateLayeredWindow(
        hwnd,          // 目标窗口句柄
        hdc,           // 目标DC（必须为屏幕DC）
        &ptDst,        // 窗口新位置
        &sizeWnd,      // 窗口新尺寸
        hdcMem,        // 源DC
        &ptSrc,        // 源起始点
        laycolor,             // 颜色键（不使用）
        &blend,        // 混合参数
        ULW_ALPHA      // 使用Alpha通道
    );
    // BitBlt(hdc, 0, 0, bmp.GetWidth(), bmp.GetHeight(), hdcMem, 0, 0, SRCCOPY);

    // 清理资源
    SelectObject(hdcMem, hOldBmp);
    DeleteObject(hbmpMem);
    DeleteDC(hdcMem);
    EndPaint(hwnd, &ps);

    //PAINTSTRUCT ps;
    //HDC hdc = BeginPaint(hwnd, &ps);

    //HDC hdcScreen = GetDC(hwnd);

    //HDC hdcMem = CreateCompatibleDC(hdcScreen);
    //if (!hdcMem) {
    //    ReleaseDC(NULL, hdc);
    //    return;
    //}

    //Gdiplus::Bitmap bmp(L"wode.png");
    //HBITMAP hbmp;
    //bmp.GetHBITMAP({ 0, 0, 0, 0 }, &hbmp);
    //SelectObject(hdcMem, hbmp);
    //::TransparentBlt(hdc, 0, 0, bmp.GetWidth(), bmp.GetHeight(),
    //    hdcMem, 0, 0, bmp.GetWidth(), bmp.GetHeight(), 0);

    //DeleteDC(hdcMem);
    //// ReleaseDC(NULL, hdcScreen);
    //EndPaint(hwnd, &ps);
    //return;
    // ::SetBkMode(hdcMem, TRANSPARENT);


    // 获取屏幕DC（必须用GetDC(NULL)获取桌面DC）
    /*HDC hdcScreen = GetDC(hwnd);
    if (!hdcScreen) {
        return;
    }*/

    // 创建兼容内存DC
    /*HDC hdcMem = CreateCompatibleDC(hdcScreen);
    if (!hdcMem) {
        ReleaseDC(NULL, hdcScreen);
        return;
    }
    ::SetBkMode(hdcMem, TRANSPARENT);*/

    //// 创建32位ARGB DIB
    //BITMAPINFOHEADER bmi = { 0 };
    //bmi.biSize = sizeof(BITMAPINFOHEADER);
    //bmi.biWidth = g_DisplayBuffer.cols;
    //bmi.biHeight = -g_DisplayBuffer.rows; // 正向DIB
    //bmi.biPlanes = 1;
    //bmi.biBitCount = 32;
    //bmi.biCompression = BI_RGB;

    //void* pPixels = nullptr;
    //HBITMAP hBitmap = CreateDIBSection(hdcScreen, (BITMAPINFO*)&bmi,
    //    DIB_RGB_COLORS, &pPixels, NULL, 0);
    //if (!hBitmap || !pPixels) {
    //    DeleteDC(hdcMem);
    //    ReleaseDC(NULL, hdcScreen);
    //    MessageBox(NULL, L"创建DIB失败", L"错误", MB_ICONERROR);
    //    return;
    //}

    //// 拷贝OpenCV数据到DIB
    //memcpy(pPixels, g_DisplayBuffer.data,
    //    g_DisplayBuffer.total() * g_DisplayBuffer.elemSize());
    //int chan = g_DisplayBuffer.channels();
    // HBITMAP hBitmap = CreateBitmap(g_DisplayBuffer.cols, g_DisplayBuffer.rows, 1, g_DisplayBuffer.channels() * 8, g_DisplayBuffer.data);

   
    // HBITMAP transbit = ConvertCVMatToBMP(g_DisplayBuffer);
    
    // ImageHelper::SaveBitmapToFile(hBitmap, "test.bmp");
    // ImageHelper::SaveBitmapToFile(transbit, "test.bmp");
    // imwrite("test.png", g_DisplayBuffer);

    /*CImage image;
    image.Load(L"test.png");
    int length = image.GetWidth();
    length = image.GetHeight();*/
    //COLORREF color = image.GetPixel(1080, 1920);
    // void* bits = image.GetBits();


   

    // image.Draw(hdcMem, 0, 0, g_DisplayBuffer.cols, g_DisplayBuffer.rows);


    // Gdiplus::Graphics graphics(hdcMem);
    // Gdiplus::Image gdi_image(L"test.png");
    // graphics.DrawImage(&gdi_image, 0, 0, gdi_image.GetWidth(), gdi_image.GetHeight());
    
    
    // image.Draw(hdc, 0, 0, g_DisplayBuffer.cols, g_DisplayBuffer.rows);
    // image.BitBlt(hdc, { 0, 0, g_DisplayBuffer.cols, g_DisplayBuffer.rows }, { 0, 0 });
    /*BLENDFUNCTION bf;

    bf.BlendOp = AC_SRC_OVER;
    bf.BlendFlags = 0;
    bf.SourceConstantAlpha = 0xff;
    bf.AlphaFormat = AC_SRC_ALPHA;*/
    /*::AlphaBlend(hdc, 0, 0, g_DisplayBuffer.cols, g_DisplayBuffer.rows,
        hdcMem, 0, 0, g_DisplayBuffer.cols, g_DisplayBuffer.rows, bf);*/
    
    // BitBlt(hdc, 0, 0, g_DisplayBuffer.cols, g_DisplayBuffer.rows, hdcMem, 0, 0, SRCCOPY);
        
     // Gdiplus::Bitmap gdi_bitmap(g_DisplayBuffer.cols, g_DisplayBuffer.rows, g_DisplayBuffer.step1(), PixelFormat32bppARGB, g_DisplayBuffer.data);
     /*Gdiplus::Bitmap gdi_bitmap(g_DisplayBuffer.cols, g_DisplayBuffer.rows, PixelFormat32bppARGB);
     HBITMAP testbit;
     gdi_bitmap.GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &testbit);
     ImageHelper::SaveBitmapToFile(testbit, "test2.bmp");*/
     
     // Gdiplus::Bitmap my_bit(L"test.bmp");
     // my_bit.FromFile(L"test.png");
     // Gdiplus::Graphics baseGraph(hdcMem);
     // baseGraph.DrawImage(&my_bit, Gdiplus::Rect(0, 0, g_DisplayBuffer.cols, g_DisplayBuffer.rows));

    // HBITMAP testbit;
    // my_bit.GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &testbit);

    //// 选择位图到内存DC
    //HGDIOBJ hOld = SelectObject(hdcMem, testbit);
    //if (hOld == NULL || GetLastError() != ERROR_SUCCESS) {
    //    DeleteObject(testbit);
    //    DeleteDC(hdcMem);
    //    ReleaseDC(NULL, hdcScreen);
    //    return;
    //}

    //// 设置混合参数
    //BLENDFUNCTION blend = { 0 };
    //blend.BlendOp = AC_SRC_OVER;
    //blend.SourceConstantAlpha = 128; // 全局透明度
    //blend.AlphaFormat = AC_SRC_ALPHA; // 关键参数

    //::AlphaBlend(hdcScreen, 0, 0, g_DisplayBuffer.cols, g_DisplayBuffer.rows, hdcMem, 0, 0,
    //    g_DisplayBuffer.cols, g_DisplayBuffer.rows, blend);

    //// 窗口位置和尺寸
    //POINT ptDst = { 0, 0 }; // 窗口左上角屏幕坐标
    //SIZE sizeWnd = { g_DisplayBuffer.cols, g_DisplayBuffer.rows };
    //POINT ptSrc = { 0, 0 }; // 源图像起始点

    // BitBlt(hdcScreen, 0, 0, g_DisplayBuffer.cols, g_DisplayBuffer.rows, hdcMem, 0, 0, SRCCOPY);

    

    // CImage image;

    // ConvertMatToCImageWithAlpha(g_DisplayBuffer, image);

    //创建CImage对象附加图像，需与源图像大小类型一致
    // image.Create(g_DisplayBuffer.cols, g_DisplayBuffer.rows, 8 * g_DisplayBuffer.channels());

    /*
    if (src.channels() == 1)
    {
        //将源位图转成八位灰度图时，CImage对象需用到颜色表，需定义一个RGBQUAD数组，并填充该数组
        RGBQUAD* colorTable = new RGBQUAD[256];
        for (int i = 0; i < 256; i++)
        {
            colorTable[i].rgbRed = i;
            colorTable[i].rgbGreen = i;
            colorTable[i].rgbBlue = i;
        }

        //设置颜色表RGB分量值
        dst.SetColorTable(0, 255, colorTable);
    }
    */

    // 将 image 内存复制，但是会异常
    //int rows = g_DisplayBuffer.rows;
    //int cols = g_DisplayBuffer.cols;
    //uchar channels = g_DisplayBuffer.channels();

    //LONG cns = image.IsTransparencySupported();
    ////内存中的数据传送，注意这里是逐行传送。
    //for (int i = 0; i < rows; i++)
    //{
    //    memcpy(image.GetPixelAddress(0, i), g_DisplayBuffer.ptr<uchar>(i), cols * channels);
    //}
    //// LONG cns = image.GetTransparentColor();
    // image.TransparentBlt(hdcMem, { 0, 0, g_DisplayBuffer.cols, g_DisplayBuffer.rows });
    // BitBlt(hdcScreen, 0, 0, g_DisplayBuffer.cols, g_DisplayBuffer.rows, hdcMem, 0, 0, SRCCOPY);

    
    //// 调用关键API
    //BOOL bRet = UpdateLayeredWindow(
    //    hwnd,          // 目标窗口句柄
    //    hdcScreen,     // 目标DC（必须为屏幕DC）
    //    &ptDst,        // 窗口新位置
    //    &sizeWnd,      // 窗口新尺寸
    //    hdcMem,        // 源DC
    //    &ptSrc,        // 源起始点
    //    0,             // 颜色键（不使用）
    //    &blend,        // 混合参数
    //    ULW_ALPHA      // 使用Alpha通道
    //);

    //// 错误处理
    //if (!bRet) {
    //    DWORD dwError = GetLastError();
    //    TCHAR szMsg[256];
    //    wsprintf(szMsg, L"UpdateLayeredWindow失败 (错误码:0x%08X)", dwError);
    //    MessageBox(NULL, szMsg, L"错误", MB_ICONERROR);
    //}

    // 清理资源
    // SelectObject(hdcMem, hOld);
    // DeleteObject(testbit);
    
}


std::vector<Point> calculateTilingPositions(Size screenSize, Size unitSize) {
    std::vector<Point> positions;

    // 横向平铺
    for (int x = -unitSize.width; x < screenSize.width + unitSize.width;
        x += unitSize.width)
    {
        // 纵向平铺
        for (int y = -unitSize.height; y < screenSize.height + unitSize.height;
            y += unitSize.height)
        {
            positions.emplace_back(x, y);
        }
    }
    return positions;
}


Mat CreateRotatedWatermark(
    const String& text,
    double fontSize,
    Scalar color,
    double angleDegree,
    Size jianju,
    Size baseSize
) {
    // 角度转弧度
    double θ = angleDegree * CV_PI / 180.0;

    // 原始文字尺寸
    int baseLine = 0;
    int w = baseSize.width;
    int h = baseSize.height;

    Size offsize = { 0, 0 };

    // 计算扩展画布尺寸
    double val_sin = abs(sin(θ)), val_cos = abs(cos(θ));

    int w1 = static_cast<int>((double)w * val_cos + (double)h * val_sin);
    int h1 = static_cast<int>((double)h * val_cos + (double)w * val_sin);

    int w2 = w + static_cast<int>(2.0 * (double)h * val_sin * val_cos);
    int h2 = h + static_cast<int>(2.0 * (double)w * val_sin * val_cos);

    if (w2 < w1) {
        offsize.width = (w1 - w2) / 2;
        w2 = w1;
    }

    if (h2 < h1) {
        offsize.height = (h1 - h2) / 2;
        h2 = h1;
    }

    int pposx = static_cast<int>((double)w * val_cos * val_sin);
    int pposy = static_cast<int>((double)h * val_cos * val_sin);

    // 创建扩展画布
    Mat extendedCanvasL(h2, w2, CV_8UC4, Scalar(0, 0, 0, 0));

    DrawFullText(extendedCanvasL, jianju, text, FONT_HERSHEY_SIMPLEX, fontSize, color);
    // 计算旋转中心（扩展画布中心）
    Point2f center(extendedCanvasL.cols / 2, extendedCanvasL.rows / 2);

    // 执行旋转
    Mat rotMat = getRotationMatrix2D(center, angleDegree, 1.0);

    warpAffine(extendedCanvasL, extendedCanvasL, rotMat, extendedCanvasL.size(),
        INTER_LINEAR, BORDER_TRANSPARENT); 

    Mat rot_res = extendedCanvasL.rowRange(pposx + offsize.height, pposx + offsize.height + h)
        .colRange(pposy + offsize.width, pposy + offsize.width + w);

    return rot_res;
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    RECT* pRect = (RECT*)dwData;
    //保存显示器信息
    MONITORINFO monitorinfo;
    monitorinfo.cbSize = sizeof(MONITORINFO);

    //获得显示器信息，将信息保存到monitorinfo中
    GetMonitorInfo(hMonitor, &monitorinfo);

    if (pRect->top > monitorinfo.rcWork.top)
    {
        pRect->top = monitorinfo.rcWork.top;
    }
    if (pRect->left > monitorinfo.rcWork.left)
    {
        pRect->left = monitorinfo.rcWork.left;
    }
    if (pRect->right < monitorinfo.rcWork.right)
    {
        pRect->right = monitorinfo.rcWork.right;
    }
    if (pRect->bottom < monitorinfo.rcWork.bottom)
    {
        pRect->bottom = monitorinfo.rcWork.bottom;
    }
    return TRUE;
}

// 窗口消息处理函数
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        // 创建定时器
        
        return 0;
    case WM_TIMER:
        //if (wParam == TIMER_ID) {
        //    RECT rect;
        //    GetWindowRect(hwnd, &rect);
        //    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, LPARAM(&rect));

        //    HWND h_prewnd = GetWindow(hwnd, GW_HWNDFIRST);
        //    ::SetWindowPos(hwnd, h_prewnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, NULL);
        //}
        //else if (wParam == UPDATE_ID) {
        //    // UpdateDisplayBuffer(hwnd);
        //    UpdateWindowContent(hwnd);
        //}   
        return 0;
    case WM_PAINT: {

        // UpdateWindowContent(hwnd);
        // UpdateWindowContent(hwnd);

        /*PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);*/

        //// 将OpenCV缓冲数据绘制到窗口
        //if (!g_DisplayBuffer.empty()) {
        //    BITMAPINFO bmi = { 0 };
        //    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        //    bmi.bmiHeader.biWidth = g_DisplayBuffer.cols;
        //    bmi.bmiHeader.biHeight = -g_DisplayBuffer.rows; // 正向DIB
        //    bmi.bmiHeader.biPlanes = 1;
        //    bmi.bmiHeader.biBitCount = 32;

        //    SetDIBitsToDevice(
        //        hdc,
        //        0, 0,                   // 目标起始位置
        //        g_DisplayBuffer.cols,    // 图像宽度
        //        g_DisplayBuffer.rows,    // 图像高度
        //        0, 0,                   // 源起始位置
        //        0,                      // 起始扫描线
        //        g_DisplayBuffer.rows,    // 扫描线数量
        //        g_DisplayBuffer.data,    // OpenCV数据指针
        //        &bmi,
        //        DIB_RGB_COLORS
        //    );
        //}

        // EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        // InvalidateRect(hwnd, NULL, TRUE);
        return 0; // 直接返回，禁止擦除
    case WM_DESTROY:
        KillTimer(hwnd, TIMER_ID);
        KillTimer(hwnd, UPDATE_ID);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

// 创建透明窗口
HWND CreateTransparentWindow(int width, int height) {
    // 注册窗口类
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = g_ClassName;
    RegisterClassEx(&wc);

    // 创建窗口
    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
        g_ClassName,
        L"Transparent Window",
        WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );

    // 设置窗口透明度
    // SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

    // 设置窗口透明度//从任务栏中去掉.
    /*SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE)
        & ~WS_EX_APPWINDOW | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_TOPMOST);*/
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE)
        & ~WS_EX_APPWINDOW | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_OVERLAPPED);

    // ::SetLayeredWindowAttributes(hwnd, GetSysColor(CTLCOLOR_DLG), 128, 2 | LWA_COLORKEY);
    // SetLayeredWindowAttributes(hwnd, 0, 176, LWA_ALPHA);
    UpdateWindowContent(hwnd);

    // 显示窗口
    ShowWindow(hwnd, SW_SHOW);

    UpdateWindow(hwnd);

    return hwnd;
}

// 使用OpenCV绘制文字到缓冲
void UpdateDisplayBuffer(HWND hwnd) {
    //  获取当前系统时间
    time_t now = time(nullptr);
    tm localTime;
    localtime_s(&localTime, &now);  // 安全版的时间转换

    // 格式化时间字符串
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y/%m/%d %H:%M:%S", &localTime);

    // 根据秒数计算旋转角度（每秒转6度，每分钟360度）
    // double angle = (double)localTime.tm_sec;

    // 创建透明背景
    /*Mat canvas(GetSystemMetrics(SM_CYSCREEN),
        GetSystemMetrics(SM_CXSCREEN),
        CV_8UC4, Scalar(0, 0, 0, 0));*/


    //// 绘制时间文字（在屏幕右下角显示）
    //int baseY = GetSystemMetrics(SM_CYSCREEN) - 100; // 离底部100像素
    //int baseX = 100;                                 // 离左边100像素

    //int baseLine = 0;
    //Size textSize = getTextSize(timeStr, FONT_HERSHEY_SIMPLEX,
    //    2.0, 2, &baseLine);

    /*DrawFullText(canvas, Size(100, 100), timeStr, FONT_HERSHEY_SIMPLEX, 2.0,
        Scalar(255, 255, 255, 255), 2, LINE_AA);*/

    //std::vector<Point> point_draw = calculateTilingPositions(Size(GetSystemMetrics(SM_CYSCREEN),
    //    GetSystemMetrics(SM_CXSCREEN)), textSize);

    //putText(canvas, timeStr,
    //    Point(baseX, baseY),
    //    FONT_HERSHEY_SIMPLEX,
    //    2.0,
    //    Scalar(255, 255, 255, 255), // 白色文字，完全不透明
    //    3, LINE_AA);

    //putText(canvas, timeStr,
    //    Point(textSize.width + 200, baseY),
    //    FONT_HERSHEY_SIMPLEX,
    //    2.0,
    //    Scalar(255, 255, 255, 255), // 白色文字，完全不透明
    //    3, LINE_AA);

    //putText(canvas, timeStr,
    //    Point(2 * textSize.width + 300, baseY),
    //    FONT_HERSHEY_SIMPLEX,
    //    2.0,
    //    Scalar(255, 255, 255, 255), // 白色文字，完全不透明
    //    3, LINE_AA);

    /*Point respos;
    respos.x = canvas.cols / 2;
    respos.y = canvas.rows / 2;
    

    Mat rotMat = getRotationMatrix2D(respos, 30, 1.0);
    Mat rotated;
    warpAffine(canvas, rotated, rotMat, canvas.size(),
        INTER_LINEAR, BORDER_TRANSPARENT);*/

    Mat rotmat = CreateRotatedWatermark(timeStr, 2.0, Scalar(128, 128, 128, 255), 170,
        Size(100, 100), Size(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)));

    ///*Mat trans = getRotationMatrix2D(Point(0, canvas.rows), angle, 1);
    //Mat res()
    //warpAffine(canvas, ;*/
    //rotmat.copyTo(g_DisplayBuffer(
    //    Rect(respos.x, respos.y,
    //        rotmat.cols, rotmat.rows)
    //));
    // 更新全局缓冲
    // g_DisplayBuffer = rotmat;
    // rotmat.copyTo(g_DisplayBuffer);

    g_DisplayBuffer = rotmat;

    // 请求重绘
    // InvalidateRect(hwnd, NULL, FALSE);
}

int main() {

    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
    // 创建窗口
    HWND hwnd = CreateTransparentWindow(
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN)
    );

    SetTimer(hwnd, TIMER_ID, 100, NULL);
    SetTimer(hwnd, UPDATE_ID, 1000, NULL);

    // 初始化OpenCV缓冲
    // UpdateDisplayBuffer(hwnd);
    // UpdateWindowContent(hwnd);
    

    // 消息循环
    MSG msg;
    while (true) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            switch (msg.message) {
            case WM_TIMER:
                if (msg.wParam == TIMER_ID) {
                    /*RECT rect;
                    GetWindowRect(hwnd, &rect);
                    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, LPARAM(&rect));

                    HWND h_prewnd = GetWindow(hwnd, GW_HWNDFIRST);
                    ::SetWindowPos(hwnd, h_prewnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, NULL);*/
                }
                else if (msg.wParam == UPDATE_ID) {
                    // UpdateDisplayBuffer(hwnd);
                    UpdateWindowContent(hwnd);
                }
            }
        }
        else {
            // 空闲时处理其他任务（可选）
        }
    }

    /*Mat rotmat = CreateRotatedWatermark("wodeshijie hahaha", 2.0, Scalar(255, 255, 255, 255), 30,
        Size(100, 100), Size(GetSystemMetrics(SM_CYSCREEN), GetSystemMetrics(SM_CXSCREEN)));*/

    Gdiplus::GdiplusShutdown(gdiplusToken);


    return 0;
}
