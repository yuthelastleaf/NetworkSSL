#include "savebit.h"
#include <shlwapi.h>


bool ImageHelper::SaveBitmapToFile(HBITMAP hBitmap, const string& filename)
{
    //1. 创建位图文件
    const auto file = CreateFileA(filename.c_str(), GENERIC_WRITE,
        0, nullptr, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    //2. 计算位图文件每个像素所占字节数
    const auto bitCount = GetBitmapBitCount();

    //3. 获取位图结构
    BITMAP bitmap;
    ::GetObject(hBitmap, sizeof(bitmap), reinterpret_cast<LPSTR>(&bitmap));

    //位图中像素字节大小(32字节对齐)
    const DWORD bmBitsSize = ((bitmap.bmWidth * bitCount + 31) / 32) * 4 * bitmap.bmHeight;

    //调色板大小
    const DWORD paletteSize = 0;

    //4. 构造位图信息头
    BITMAPINFOHEADER  bmpInfoHeader; //位图信息头结构
    bmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfoHeader.biWidth = bitmap.bmWidth;
    bmpInfoHeader.biHeight = bitmap.bmHeight;
    bmpInfoHeader.biPlanes = 1;
    bmpInfoHeader.biBitCount = bitCount;
    bmpInfoHeader.biCompression = BI_RGB;
    bmpInfoHeader.biSizeImage = 0;
    bmpInfoHeader.biXPelsPerMeter = 0;
    bmpInfoHeader.biYPelsPerMeter = 0;
    bmpInfoHeader.biClrImportant = 0;
    bmpInfoHeader.biClrUsed = 0;

    //5. 构造位图文件头
    BITMAPFILEHEADER bmpFileHeader;
    bmpFileHeader.bfType = 0x4D42; //"BM"
    //位图文件大小
    const DWORD dibSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + paletteSize + bmBitsSize;
    bmpFileHeader.bfSize = dibSize;
    bmpFileHeader.bfReserved1 = 0;
    bmpFileHeader.bfReserved2 = 0;
    bmpFileHeader.bfOffBits = static_cast<DWORD>(sizeof(BITMAPFILEHEADER))
        + static_cast<DWORD>(sizeof(BITMAPINFOHEADER)) + paletteSize;

    //6. 为位图内容分配内存 
    const auto dib = GlobalAlloc(GHND, bmBitsSize + paletteSize + sizeof(BITMAPINFOHEADER)); //内存句柄
    const auto lpBmpInfoHeader = static_cast<LPBITMAPINFOHEADER>(GlobalLock(dib)); //指向位图信息头结构
    *lpBmpInfoHeader = bmpInfoHeader;

    //7. 处理调色板
    ProcessPalette(hBitmap, bitmap, paletteSize, lpBmpInfoHeader);

    //8. 写入文件
    DWORD written = 0; //写入文件字节数   
    WriteFile(file, reinterpret_cast<LPSTR>(&bmpFileHeader), sizeof(BITMAPFILEHEADER),
        &written, nullptr); //写入位图文件头
    WriteFile(file, reinterpret_cast<LPSTR>(lpBmpInfoHeader), dibSize,
        &written, nullptr); //写入位图文件其余内容

    //9. 清理资源
    GlobalUnlock(dib);
    GlobalFree(dib);
    CloseHandle(file);

    return true;
}

//计算位图文件每个像素所占字节数
WORD ImageHelper::GetBitmapBitCount()
{
    const auto dc = ::CreateDCA("DISPLAY", nullptr, nullptr, nullptr);
    //当前分辨率下每像素所占字节数
    const auto bits = ::GetDeviceCaps(dc, BITSPIXEL) * GetDeviceCaps(dc, PLANES);
    ::DeleteDC(dc);

    //位图中每像素所占字节数
    WORD bitCount;
    if (bits <= 1)
        bitCount = 1;
    else if (bits <= 4)
        bitCount = 4;
    else if (bits <= 8)
        bitCount = 8;
    else
        bitCount = 24;

    return bitCount;
}

//处理调色板
void ImageHelper::ProcessPalette(HBITMAP hBitmap, const BITMAP& bitmap,
    DWORD paletteSize, LPBITMAPINFOHEADER lpBmpInfoHeader)
{
    HANDLE oldPalette = nullptr;
    HDC dc = nullptr;
    const auto palette = GetStockObject(DEFAULT_PALETTE);
    if (palette != nullptr)
    {
        dc = ::GetDC(nullptr);
        oldPalette = ::SelectPalette(dc, static_cast<HPALETTE>(palette), FALSE);
        ::RealizePalette(dc); //实现设备调色板
    }

    //获取该调色板下新的像素值
    GetDIBits(dc, hBitmap, 0, static_cast<UINT>(bitmap.bmHeight),
        reinterpret_cast<LPSTR>(lpBmpInfoHeader) + sizeof(BITMAPINFOHEADER) + paletteSize,
        reinterpret_cast<BITMAPINFO*>(lpBmpInfoHeader), DIB_RGB_COLORS);

    //恢复调色板
    if (oldPalette != nullptr)
    {
        ::SelectPalette(dc, static_cast<HPALETTE>(oldPalette), TRUE);
        ::RealizePalette(dc);
        ::ReleaseDC(nullptr, dc);
    }
}