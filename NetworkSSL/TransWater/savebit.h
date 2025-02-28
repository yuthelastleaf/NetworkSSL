#pragma once

#include <windows.h>
#include <string>
using namespace std;

class ImageHelper
{
public:
    static bool SaveBitmapToFile(HBITMAP bitmap, const string& filename); //����λͼ���ļ�

private:
    static WORD GetBitmapBitCount(); //����λͼ�ļ�ÿ��������ռ�ֽ���
    static void ProcessPalette(HBITMAP hBitmap, const BITMAP& bitmap,
        DWORD paletteSize, LPBITMAPINFOHEADER lpBmpInfoHeader); //�����ɫ��
};
