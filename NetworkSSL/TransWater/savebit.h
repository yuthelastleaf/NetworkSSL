#pragma once

#include <windows.h>
#include <string>
using namespace std;

class ImageHelper
{
public:
    static bool SaveBitmapToFile(HBITMAP bitmap, const string& filename); //保存位图到文件

private:
    static WORD GetBitmapBitCount(); //计算位图文件每个像素所占字节数
    static void ProcessPalette(HBITMAP hBitmap, const BITMAP& bitmap,
        DWORD paletteSize, LPBITMAPINFOHEADER lpBmpInfoHeader); //处理调色板
};
