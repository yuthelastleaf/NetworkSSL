#include <windows.h>
#include <winternl.h>
#include <stdio.h>

// ���ȱʧ�Ķ��������
#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

// KEY_VALUE_INFORMATION_CLASS ö��
typedef enum _KEY_VALUE_INFORMATION_CLASS {
    KeyValueBasicInformation,
    KeyValueFullInformation,
    KeyValuePartialInformation,
    KeyValueFullInformationAlign64,
    KeyValuePartialInformationAlign64,
    KeyValueLayerInformation,
    MaxKeyValueInfoClass
} KEY_VALUE_INFORMATION_CLASS;

// KEY_VALUE_FULL_INFORMATION �ṹ
typedef struct _KEY_VALUE_FULL_INFORMATION {
    ULONG   TitleIndex;
    ULONG   Type;
    ULONG   DataOffset;
    ULONG   DataLength;
    ULONG   NameLength;
    WCHAR   Name[1];
} KEY_VALUE_FULL_INFORMATION, * PKEY_VALUE_FULL_INFORMATION;

// ���ȱʧ��ע�����
#ifndef REG_CREATED_NEW_KEY
#define REG_CREATED_NEW_KEY     0x00000001L
#endif

#ifndef REG_OPENED_EXISTING_KEY
#define REG_OPENED_EXISTING_KEY 0x00000002L
#endif

#ifndef REG_OPTION_NON_VOLATILE
#define REG_OPTION_NON_VOLATILE 0x00000000L
#endif

#ifndef OBJ_CASE_INSENSITIVE
#define OBJ_CASE_INSENSITIVE    0x00000040L
#endif

// ����Ȩ�޶���
#ifndef KEY_ALL_ACCESS
#define KEY_ALL_ACCESS          0xF003F
#endif

#ifndef KEY_READ
#define KEY_READ                0x20019
#endif

// OBJECT_ATTRIBUTES �ṹ�����û�ж��壩
#ifndef InitializeObjectAttributes
#define InitializeObjectAttributes(p, n, a, r, s) { \
    (p)->Length = sizeof(OBJECT_ATTRIBUTES);          \
    (p)->RootDirectory = r;                           \
    (p)->Attributes = a;                              \
    (p)->ObjectName = n;                              \
    (p)->SecurityDescriptor = s;                      \
    (p)->SecurityQualityOfService = NULL;             \
}
#endif

// �ֶ�����NT API����������ʹ�ø߼�API
typedef NTSTATUS(NTAPI* pNtOpenKey)(
    PHANDLE KeyHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes
    );

typedef NTSTATUS(NTAPI* pNtCreateKey)(
    PHANDLE KeyHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    ULONG TitleIndex,
    PUNICODE_STRING Class,
    ULONG CreateOptions,
    PULONG Disposition
    );

typedef NTSTATUS(NTAPI* pNtQueryValueKey)(
    HANDLE KeyHandle,
    PUNICODE_STRING ValueName,
    KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    PVOID KeyValueInformation,
    ULONG Length,
    PULONG ResultLength
    );

typedef NTSTATUS(NTAPI* pNtSetValueKey)(
    HANDLE KeyHandle,
    PUNICODE_STRING ValueName,
    ULONG TitleIndex,
    ULONG Type,
    PVOID Data,
    ULONG DataSize
    );

typedef NTSTATUS(NTAPI* pNtDeleteValueKey)(
    HANDLE KeyHandle,
    PUNICODE_STRING ValueName
    );

typedef NTSTATUS(NTAPI* pNtDeleteKey)(
    HANDLE KeyHandle
    );

typedef NTSTATUS(NTAPI* pNtClose)(
    HANDLE Handle
    );

typedef NTSTATUS(NTAPI* pllRtlInitUnicodeString)(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
    );

// ȫ�ֺ���ָ��
pNtOpenKey NtOpenKey = NULL;
pNtCreateKey NtCreateKey = NULL;
pNtQueryValueKey NtQueryValueKey = NULL;
pNtSetValueKey NtSetValueKey = NULL;
pNtDeleteValueKey NtDeleteValueKey = NULL;
pNtDeleteKey NtDeleteKey = NULL;
pNtClose llNtClose = NULL;
pllRtlInitUnicodeString llRtlInitUnicodeString = NULL;

// ��ʼ��NT API����
BOOL InitializeNtApi()
{
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll) {
        printf("Failed to get ntdll.dll handle\n");
        return FALSE;
    }

    NtOpenKey = (pNtOpenKey)GetProcAddress(hNtdll, "NtOpenKey");
    NtCreateKey = (pNtCreateKey)GetProcAddress(hNtdll, "NtCreateKey");
    NtQueryValueKey = (pNtQueryValueKey)GetProcAddress(hNtdll, "NtQueryValueKey");
    NtSetValueKey = (pNtSetValueKey)GetProcAddress(hNtdll, "NtSetValueKey");
    NtDeleteValueKey = (pNtDeleteValueKey)GetProcAddress(hNtdll, "NtDeleteValueKey");
    NtDeleteKey = (pNtDeleteKey)GetProcAddress(hNtdll, "NtDeleteKey");
    llNtClose = (pNtClose)GetProcAddress(hNtdll, "NtClose");
    llRtlInitUnicodeString = (pllRtlInitUnicodeString)GetProcAddress(hNtdll, "llRtlInitUnicodeString");

    if (!NtOpenKey || !NtCreateKey || !NtQueryValueKey || !NtSetValueKey ||
        !NtDeleteValueKey || !NtDeleteKey || !llNtClose || !llRtlInitUnicodeString) {
        printf("Failed to get NT API addresses\n");
        return FALSE;
    }

    printf("Successfully initialized NT API functions\n");
    return TRUE;
}

// ����1: ֱ��ʹ��NT API����ԭʼ·��
BOOL BypassMethod1_DirectNtApi()
{
    printf("\n=== Method 1: Direct NT API Access ===\n");

    HANDLE hKey = NULL;
    NTSTATUS status;
    OBJECT_ATTRIBUTES objAttr;
    UNICODE_STRING keyPath;

    // ֱ�ӷ���ԭʼ·�����ƹ��߼�API
    llRtlInitUnicodeString(&keyPath, L"\\Registry\\Machine\\Software\\test");

    InitializeObjectAttributes(&objAttr,
        &keyPath,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    // ʹ��NT APIֱ�Ӵ򿪼�
    status = NtOpenKey(&hKey, KEY_ALL_ACCESS, &objAttr);

    if (NT_SUCCESS(status)) {
        printf("[+] Successfully opened key using direct NT API\n");
        printf("[+] This bypassed the registry redirection!\n");

        // ���Զ�ȡֵ
        UNICODE_STRING valueName;
        llRtlInitUnicodeString(&valueName, L"TestValue");

        UCHAR buffer[256];
        ULONG resultLength;

        status = NtQueryValueKey(hKey, &valueName, KeyValueFullInformation,
            buffer, sizeof(buffer), &resultLength);

        if (NT_SUCCESS(status)) {
            PKEY_VALUE_FULL_INFORMATION keyValueInfo = (PKEY_VALUE_FULL_INFORMATION)buffer;
            printf("[+] Successfully read value from original location\n");
            printf("[+] Value data: %.*S\n",
                keyValueInfo->DataLength / sizeof(WCHAR),
                (PWCHAR)((PUCHAR)keyValueInfo + keyValueInfo->DataOffset));
        }
        else {
            printf("[-] Failed to read value: 0x%X\n", status);
        }

        llNtClose(hKey);
        return TRUE;
    }
    else {
        printf("[-] Failed to open key: 0x%X\n", status);
        return FALSE;
    }
}

// ����2: ʹ�ñ���·����ʽ
BOOL BypassMethod2_AlternatePath()
{
    printf("\n=== Method 2: Alternate Path Format ===\n");

    HANDLE hKey = NULL;
    NTSTATUS status;
    OBJECT_ATTRIBUTES objAttr;
    UNICODE_STRING keyPath;

    // ʹ�ò�ͬ��·����ʾ����
    llRtlInitUnicodeString(&keyPath, L"\\Registry\\Machine\\SOFTWARE\\test"); // ��д

    InitializeObjectAttributes(&objAttr,
        &keyPath,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    status = NtOpenKey(&hKey, KEY_ALL_ACCESS, &objAttr);

    if (NT_SUCCESS(status)) {
        printf("[+] Opened key using alternate path format\n");
        llNtClose(hKey);
        return TRUE;
    }
    else {
        printf("[-] Alternate path failed: 0x%X\n", status);
        return FALSE;
    }
}

// ����3: �ȴ򿪸���������Է����Ӽ�
BOOL BypassMethod3_RelativeAccess()
{
    printf("\n=== Method 3: Relative Access ===\n");

    HANDLE hParentKey = NULL, hChildKey = NULL;
    NTSTATUS status;
    OBJECT_ATTRIBUTES objAttr;
    UNICODE_STRING parentPath, childPath;

    // �ȴ򿪸���
    llRtlInitUnicodeString(&parentPath, L"\\Registry\\Machine\\Software");

    InitializeObjectAttributes(&objAttr,
        &parentPath,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    status = NtOpenKey(&hParentKey, KEY_ALL_ACCESS, &objAttr);

    if (!NT_SUCCESS(status)) {
        printf("[-] Failed to open parent key: 0x%X\n", status);
        return FALSE;
    }

    // ����ڸ��������Ӽ�
    llRtlInitUnicodeString(&childPath, L"test");

    InitializeObjectAttributes(&objAttr,
        &childPath,
        OBJ_CASE_INSENSITIVE,
        hParentKey,  // ʹ�ø�����Ϊ��
        NULL);

    status = NtOpenKey(&hChildKey, KEY_ALL_ACCESS, &objAttr);

    if (NT_SUCCESS(status)) {
        printf("[+] Successfully accessed via relative path\n");
        llNtClose(hChildKey);
        llNtClose(hParentKey);
        return TRUE;
    }
    else {
        printf("[-] Relative access failed: 0x%X\n", status);
        llNtClose(hParentKey);
        return FALSE;
    }
}

// ����4: ʹ��Ԥ�ȴ򿪵ľ��
BOOL BypassMethod4_PreOpenedHandle()
{
    printf("\n=== Method 4: Pre-opened Handle ===\n");

    // ģ������������ǰ�ʹ򿪵ľ��
    printf("[+] This method requires opening handle before driver loads\n");
    printf("[+] In real scenario, you would:\n");
    printf("    1. Open registry key handle before loading protection driver\n");
    printf("    2. Keep handle open across driver load\n");
    printf("    3. Use cached handle to bypass new registrations\n");

    return TRUE;
}

// ����5: �����µļ������Դ����������ƹ�
BOOL BypassMethod5_CreateKey()
{
    printf("\n=== Method 5: Create Key Bypass ===\n");

    HANDLE hKey = NULL;
    NTSTATUS status;
    OBJECT_ATTRIBUTES objAttr;
    UNICODE_STRING keyPath;
    ULONG disposition;

    // ���Դ���һ���µ��Ӽ�
    llRtlInitUnicodeString(&keyPath, L"\\Registry\\Machine\\Software\\test\\bypass_test");

    InitializeObjectAttributes(&objAttr,
        &keyPath,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    status = NtCreateKey(&hKey, KEY_ALL_ACCESS, &objAttr, 0, NULL,
        REG_OPTION_NON_VOLATILE, &disposition);

    if (NT_SUCCESS(status)) {
        printf("[+] Successfully created key using NT API\n");
        printf("[+] Disposition: %s\n",
            disposition == REG_CREATED_NEW_KEY ? "Created new" : "Opened existing");

        // ����һ��ֵ������
        UNICODE_STRING valueName;
        llRtlInitUnicodeString(&valueName, L"BypassValue");

        WCHAR data[] = L"Bypassed!";
        status = NtSetValueKey(hKey, &valueName, 0, REG_SZ,
            data, sizeof(data));

        if (NT_SUCCESS(status)) {
            printf("[+] Successfully set value in bypassed key\n");
        }

        llNtClose(hKey);
        return TRUE;
    }
    else {
        printf("[-] Failed to create key: 0x%X\n", status);
        return FALSE;
    }
}

// ����6: ʹ�÷������ӻ��豸·��
BOOL BypassMethod6_DevicePath()
{
    printf("\n=== Method 6: Device Path Access ===\n");

    // ע�⣺���ַ���ͨ����Ҫ���ߵ�Ȩ��
    HANDLE hKey = NULL;
    NTSTATUS status;
    OBJECT_ATTRIBUTES objAttr;
    UNICODE_STRING keyPath;

    // ����ʹ���豸·������ҪSeBackupPrivilege�ȣ�
    llRtlInitUnicodeString(&keyPath, L"\\??\\GLOBALROOT\\Registry\\Machine\\Software\\test");

    InitializeObjectAttributes(&objAttr,
        &keyPath,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    status = NtOpenKey(&hKey, KEY_ALL_ACCESS, &objAttr);

    if (NT_SUCCESS(status)) {
        printf("[+] Accessed via device path\n");
        llNtClose(hKey);
        return TRUE;
    }
    else {
        printf("[-] Device path access failed: 0x%X (expected, needs privileges)\n", status);
        return FALSE;
    }
}

// ���Գ���API�Ƿ��ض���
BOOL TestRegularAPI()
{
    printf("\n=== Testing Regular API (Should be redirected) ===\n");

    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\test",
        0, KEY_READ, &hKey);

    if (result == ERROR_SUCCESS) {
        printf("[+] Regular API can access key\n");

        WCHAR buffer[256];
        DWORD bufferSize = sizeof(buffer);
        DWORD type;

        result = RegQueryValueExW(hKey, L"TestValue", NULL, &type,
            (LPBYTE)buffer, &bufferSize);

        if (result == ERROR_SUCCESS) {
            printf("[+] Regular API read value: %S\n", buffer);
            printf("    (This should be from redirected location)\n");
        }

        RegCloseKey(hKey);
        return TRUE;
    }
    else {
        printf("[-] Regular API failed: %d\n", result);
        return FALSE;
    }
}

int main()
{
    printf("Registry Redirection Bypass Test\n");
    printf("================================\n");

    if (!InitializeNtApi()) {
        printf("Failed to initialize NT API\n");
        return 1;
    }

    // ���Ȳ��Գ���API��Ӧ�ñ��ض���
    TestRegularAPI();

    // ���Ը����ƹ�����
    BypassMethod1_DirectNtApi();
    BypassMethod2_AlternatePath();
    BypassMethod3_RelativeAccess();
    BypassMethod4_PreOpenedHandle();
    BypassMethod5_CreateKey();
    BypassMethod6_DevicePath();

    printf("\n=== Summary ===\n");
    printf("If any bypass method succeeded, it indicates that the\n");
    printf("registry redirection driver has limitations that can be exploited.\n");
    printf("\nTo improve protection, consider:\n");
    printf("1. Handling more registry operation types\n");
    printf("2. Normalizing path formats before comparison\n");
    printf("3. Checking caller context and privileges\n");
    printf("4. Implementing multiple callback layers\n");

    return 0;
}

// �������� (ʹ��Visual Studio������ʾ��):
// cl /nologo registry_bypass.c /link ntdll.lib advapi32.lib

// ����ʹ��MinGW:
// gcc -o registry_bypass.exe registry_bypass.c -lntdll -ladvapi32