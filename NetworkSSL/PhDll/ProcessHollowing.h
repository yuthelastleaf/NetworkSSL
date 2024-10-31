#pragma once
#include <Windows.h>

// Structure to store the address process infromation.
struct ProcessAddressInformation
{
	LPVOID lpProcessPEBAddress;
	LPVOID lpProcessImageBaseAddress;
};

//Structure relocation entry based on : https://docs.microsoft.com/fr-fr/windows/win32/debug/pe-format#the-reloc-section-image-only
typedef struct IMAGE_RELOCATION_ENTRY {
	WORD Offset : 12;
	WORD Type : 4;
} IMAGE_RELOCATION_ENTRY, * PIMAGE_RELOCATION_ENTRY;


#ifdef __cplusplus
extern "C" {
#endif



// ProcessHollowing Functions
HANDLE GetFileContent(int resourceId);
BOOL IsValidPE(const LPVOID lpImage);
BOOL IsPE32(const LPVOID lpImage);

ProcessAddressInformation GetProcessAddressInformation32(const PPROCESS_INFORMATION lpPI);
#ifdef _WIN64
ProcessAddressInformation GetProcessAddressInformation64(const PPROCESS_INFORMATION lpPI);
#endif
DWORD GetSubsytem32(const LPVOID lpImage);
DWORD GetSubsytem64(const LPVOID lpImage);
DWORD GetSubsystemEx32(const HANDLE hProcess, const LPVOID lpImageBaseAddress);
DWORD GetSubsystemEx64(const HANDLE hProcess, const LPVOID lpImageBaseAddress);
void CleanAndExitProcess(const LPPROCESS_INFORMATION lpPI, const HANDLE hFileContent);
void CleanProcess(const LPPROCESS_INFORMATION lpPI, const HANDLE hFileContent);
BOOL HasRelocation32(const LPVOID lpImage);
BOOL HasRelocation64(const LPVOID lpImage);
IMAGE_DATA_DIRECTORY GetRelocAddress32(const LPVOID lpImage);
IMAGE_DATA_DIRECTORY GetRelocAddress64(const LPVOID lpImage);
BOOL RunPE32(const LPPROCESS_INFORMATION lpPI, const LPVOID lpImage);
#ifdef _WIN64
BOOL RunPE64(const LPPROCESS_INFORMATION lpPI, const LPVOID lpImage);
#endif
BOOL RunPEReloc32(const LPPROCESS_INFORMATION lpPI, const LPVOID lpImage);
#ifdef _WIN64
BOOL RunPEReloc64(const LPPROCESS_INFORMATION lpPI, const LPVOID lpImage);
#endif
__declspec(dllexport) int RunPE(int resourceId);
#ifdef __cplusplus
}
#endif