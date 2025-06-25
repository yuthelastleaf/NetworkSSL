#include <windows.h>
#include <winldap.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <conio.h>
#include <locale.h>

#pragma comment(lib, "wldap32.lib")

// �û���Ϣ�ṹ
typedef struct UserInfo {
    WCHAR samAccountName[256];
    WCHAR displayName[256];
    WCHAR userPrincipalName[256];
} UserInfo;

// ����Ϣ�ṹ
typedef struct GroupInfo {
    WCHAR groupName[256];
    WCHAR description[256];
    WCHAR groupType[64];
    WCHAR distinguishedName[512];
} GroupInfo;

// ȫ�ֱ���
UserInfo* g_users = NULL;
int g_userCount = 0;
int g_userCapacity = 0;

GroupInfo* g_groups = NULL;
int g_groupCount = 0;
int g_groupCapacity = 0;

// ��������
void AddUser(const WCHAR* sam, const WCHAR* display, const WCHAR* upn);
void AddGroup(const WCHAR* name, const WCHAR* desc, const WCHAR* type, const WCHAR* dn);
BOOL GetCurrentDomain(WCHAR* domainName, DWORD size);
void BuildBaseDN(const WCHAR* domain, WCHAR* baseDN, DWORD size);
void ParseGroupType(DWORD groupType, WCHAR* typeStr, DWORD size);
BOOL QueryUsersViaLDAP(const WCHAR* domainName);
BOOL QueryGroupsViaLDAP(const WCHAR* domainName);
void DisplayUsers(int maxCount);
void DisplayGroups(int maxCount);
void ShowUsage(void);

// ����û����б�
void AddUser(const WCHAR* sam, const WCHAR* display, const WCHAR* upn) {
    if (g_userCount >= g_userCapacity) {
        g_userCapacity = g_userCapacity == 0 ? 1000 : g_userCapacity * 2;
        UserInfo* newUsers = (UserInfo*)realloc(g_users, g_userCapacity * sizeof(UserInfo));
        if (!newUsers) {
            wprintf(L"�ڴ����ʧ��\n");
            return;
        }
        g_users = newUsers;
    }

    if (sam) {
        wcsncpy_s(g_users[g_userCount].samAccountName, 256, sam, _TRUNCATE);
    }
    else {
        g_users[g_userCount].samAccountName[0] = L'\0';
    }

    if (display) {
        wcsncpy_s(g_users[g_userCount].displayName, 256, display, _TRUNCATE);
    }
    else {
        g_users[g_userCount].displayName[0] = L'\0';
    }

    if (upn) {
        wcsncpy_s(g_users[g_userCount].userPrincipalName, 256, upn, _TRUNCATE);
    }
    else {
        g_users[g_userCount].userPrincipalName[0] = L'\0';
    }

    g_userCount++;
}

// ����鵽�б�
void AddGroup(const WCHAR* name, const WCHAR* desc, const WCHAR* type, const WCHAR* dn) {
    if (g_groupCount >= g_groupCapacity) {
        g_groupCapacity = g_groupCapacity == 0 ? 500 : g_groupCapacity * 2;
        GroupInfo* newGroups = (GroupInfo*)realloc(g_groups, g_groupCapacity * sizeof(GroupInfo));
        if (!newGroups) {
            wprintf(L"���б��ڴ����ʧ��\n");
            return;
        }
        g_groups = newGroups;
    }

    if (name) {
        wcsncpy_s(g_groups[g_groupCount].groupName, 256, name, _TRUNCATE);
    }
    else {
        g_groups[g_groupCount].groupName[0] = L'\0';
    }

    if (desc) {
        wcsncpy_s(g_groups[g_groupCount].description, 256, desc, _TRUNCATE);
    }
    else {
        g_groups[g_groupCount].description[0] = L'\0';
    }

    if (type) {
        wcsncpy_s(g_groups[g_groupCount].groupType, 64, type, _TRUNCATE);
    }
    else {
        g_groups[g_groupCount].groupType[0] = L'\0';
    }

    if (dn) {
        wcsncpy_s(g_groups[g_groupCount].distinguishedName, 512, dn, _TRUNCATE);
    }
    else {
        g_groups[g_groupCount].distinguishedName[0] = L'\0';
    }

    g_groupCount++;
}

// ��ȡ��ǰ����
BOOL GetCurrentDomain(WCHAR* domainName, DWORD size) {
    DWORD actualSize = size;
    if (GetComputerNameExW(ComputerNameDnsDomain, domainName, &actualSize)) {
        return TRUE;
    }

    if (GetEnvironmentVariableW(L"USERDNSDOMAIN", domainName, size) > 0) {
        return TRUE;
    }

    wcscpy_s(domainName, size, L"localhost");
    return FALSE;
}

// ����DN�ַ���
void BuildBaseDN(const WCHAR* domain, WCHAR* baseDN, DWORD size) {
    WCHAR tempDomain[256];
    wcscpy_s(tempDomain, 256, domain);

    wcscpy_s(baseDN, size, L"DC=");

    WCHAR* context = NULL;
    WCHAR* token = wcstok_s(tempDomain, L".", &context);
    BOOL first = TRUE;

    while (token != NULL) {
        if (!first) {
            wcscat_s(baseDN, size, L",DC=");
        }
        wcscat_s(baseDN, size, token);
        first = FALSE;
        token = wcstok_s(NULL, L".", &context);
    }
}

// ����������
void ParseGroupType(DWORD groupType, WCHAR* typeStr, DWORD size) {
    if (groupType & 0x00000002) {
        wcscpy_s(typeStr, size, L"ȫ��");
    }
    else if (groupType & 0x00000004) {
        wcscpy_s(typeStr, size, L"�򱾵�");
    }
    else if (groupType & 0x00000008) {
        wcscpy_s(typeStr, size, L"ͨ��");
    }
    else {
        wcscpy_s(typeStr, size, L"δ֪");
    }

    if (groupType & 0x80000000) {
        wcscat_s(typeStr, size, L"��ȫ��");
    }
    else {
        wcscat_s(typeStr, size, L"ͨѶ��");
    }
}

// ִ��LDAP�û���ѯ
BOOL QueryUsersViaLDAP(const WCHAR* domainName) {
    LDAP* pLdap = NULL;
    LDAPMessage* pResult = NULL;
    LDAPMessage* pEntry = NULL;
    ULONG result;
    WCHAR baseDN[512];
    WCHAR targetDomain[256];

    static WCHAR attr_sam[] = L"sAMAccountName";
    static WCHAR attr_display[] = L"displayName";
    static WCHAR attr_upn[] = L"userPrincipalName";
    PWCHAR attributes[4];
    attributes[0] = attr_sam;
    attributes[1] = attr_display;
    attributes[2] = attr_upn;
    attributes[3] = NULL;

    if (domainName && wcslen(domainName) > 0) {
        wcscpy_s(targetDomain, 256, domainName);
    }
    else {
        if (!GetCurrentDomain(targetDomain, 256)) {
            wprintf(L"����: �޷���ȡ��ǰ������ʹ��localhost\n");
        }
    }

    wprintf(L"�������ӵ���: %s\n", targetDomain);

    pLdap = ldap_initW(targetDomain, LDAP_PORT);
    if (!pLdap) {
        wprintf(L"����: LDAP��ʼ��ʧ��\n");
        return FALSE;
    }

    ULONG version = LDAP_VERSION3;
    result = ldap_set_optionW(pLdap, LDAP_OPT_PROTOCOL_VERSION, &version);

    LDAP_TIMEVAL timeout = { 30, 0 };
    ldap_set_optionW(pLdap, LDAP_OPT_TIMELIMIT, &timeout);

    wprintf(L"���ڽ��������֤...\n");

    result = ldap_bind_sW(pLdap, NULL, NULL, LDAP_AUTH_NEGOTIATE);
    if (result != LDAP_SUCCESS) {
        PWCHAR errorMsg = ldap_err2stringW(result);
        wprintf(L"����: LDAP��ʧ�� - %s (����: %lu)\n", errorMsg, result);
        ldap_unbind(pLdap);
        return FALSE;
    }

    wprintf(L"�����֤�ɹ���\n");

    BuildBaseDN(targetDomain, baseDN, 512);
    wprintf(L"��������DN: %s\n", baseDN);

    static WCHAR filter[] = L"(&(objectClass=user)(objectCategory=person)(!(userAccountControl:1.2.840.113556.1.4.803:=2)))";

    wprintf(L"��ʼ�����û�...\n");

    result = ldap_search_sW(pLdap, baseDN, LDAP_SCOPE_SUBTREE, filter, attributes, 0, &pResult);

    if (result != LDAP_SUCCESS) {
        PWCHAR errorMsg = ldap_err2stringW(result);
        wprintf(L"����: LDAP����ʧ�� - %s (����: %lu)\n", errorMsg, result);
        ldap_unbind(pLdap);
        return FALSE;
    }

    ULONG entryCount = ldap_count_entries(pLdap, pResult);
    wprintf(L"�ҵ� %lu ���û���Ŀ\n", entryCount);

    pEntry = ldap_first_entry(pLdap, pResult);
    while (pEntry) {
        PWCHAR samAccountName = NULL;
        PWCHAR displayName = NULL;
        PWCHAR userPrincipalName = NULL;

        PWCHAR* values = ldap_get_valuesW(pLdap, pEntry, attr_sam);
        if (values && values[0]) {
            samAccountName = values[0];
        }

        PWCHAR* displayValues = ldap_get_valuesW(pLdap, pEntry, attr_display);
        if (displayValues && displayValues[0]) {
            displayName = displayValues[0];
        }

        PWCHAR* upnValues = ldap_get_valuesW(pLdap, pEntry, attr_upn);
        if (upnValues && upnValues[0]) {
            userPrincipalName = upnValues[0];
        }

        if (samAccountName) {
            AddUser(samAccountName, displayName, userPrincipalName);
        }

        if (values) ldap_value_freeW(values);
        if (displayValues) ldap_value_freeW(displayValues);
        if (upnValues) ldap_value_freeW(upnValues);

        pEntry = ldap_next_entry(pLdap, pEntry);
    }

    if (pResult) ldap_msgfree(pResult);
    ldap_unbind(pLdap);

    wprintf(L"LDAP�û���ѯ��ɣ��ܹ���ȡ�� %d ���û�\n", g_userCount);
    return TRUE;
}

// ִ��LDAP���ѯ
BOOL QueryGroupsViaLDAP(const WCHAR* domainName) {
    LDAP* pLdap = NULL;
    LDAPMessage* pResult = NULL;
    LDAPMessage* pEntry = NULL;
    ULONG result;
    WCHAR baseDN[512];
    WCHAR targetDomain[256];

    static WCHAR attr_cn[] = L"cn";
    static WCHAR attr_desc[] = L"description";
    static WCHAR attr_type[] = L"groupType";
    static WCHAR attr_dn[] = L"distinguishedName";
    PWCHAR attributes[5];
    attributes[0] = attr_cn;
    attributes[1] = attr_desc;
    attributes[2] = attr_type;
    attributes[3] = attr_dn;
    attributes[4] = NULL;

    if (domainName && wcslen(domainName) > 0) {
        wcscpy_s(targetDomain, 256, domainName);
    }
    else {
        if (!GetCurrentDomain(targetDomain, 256)) {
            wprintf(L"����: �޷���ȡ��ǰ������ʹ��localhost\n");
        }
    }

    wprintf(L"�������ӵ����ѯ��: %s\n", targetDomain);

    pLdap = ldap_initW(targetDomain, LDAP_PORT);
    if (!pLdap) {
        wprintf(L"����: LDAP��ʼ��ʧ��(���ѯ)\n");
        return FALSE;
    }

    ULONG version = LDAP_VERSION3;
    result = ldap_set_optionW(pLdap, LDAP_OPT_PROTOCOL_VERSION, &version);

    LDAP_TIMEVAL timeout = { 30, 0 };
    ldap_set_optionW(pLdap, LDAP_OPT_TIMELIMIT, &timeout);

    wprintf(L"���ڽ������ѯ�����֤...\n");

    result = ldap_bind_sW(pLdap, NULL, NULL, LDAP_AUTH_NEGOTIATE);
    if (result != LDAP_SUCCESS) {
        PWCHAR errorMsg = ldap_err2stringW(result);
        wprintf(L"����: LDAP��ʧ��(���ѯ) - %s (����: %lu)\n", errorMsg, result);
        ldap_unbind(pLdap);
        return FALSE;
    }

    wprintf(L"���ѯ�����֤�ɹ���\n");

    BuildBaseDN(targetDomain, baseDN, 512);
    wprintf(L"����������DN: %s\n", baseDN);

    static WCHAR filter[] = L"(objectClass=group)";

    wprintf(L"��ʼ������...\n");

    result = ldap_search_sW(pLdap, baseDN, LDAP_SCOPE_SUBTREE, filter, attributes, 0, &pResult);

    if (result != LDAP_SUCCESS) {
        PWCHAR errorMsg = ldap_err2stringW(result);
        wprintf(L"����: LDAP������ʧ�� - %s (����: %lu)\n", errorMsg, result);
        ldap_unbind(pLdap);
        return FALSE;
    }

    ULONG entryCount = ldap_count_entries(pLdap, pResult);
    wprintf(L"�ҵ� %lu ������Ŀ\n", entryCount);

    pEntry = ldap_first_entry(pLdap, pResult);
    while (pEntry) {
        PWCHAR groupName = NULL;
        PWCHAR description = NULL;
        PWCHAR distinguishedName = NULL;
        WCHAR groupTypeStr[64] = { 0 };

        PWCHAR* values = ldap_get_valuesW(pLdap, pEntry, attr_cn);
        if (values && values[0]) {
            groupName = values[0];
        }

        PWCHAR* descValues = ldap_get_valuesW(pLdap, pEntry, attr_desc);
        if (descValues && descValues[0]) {
            description = descValues[0];
        }

        PWCHAR* typeValues = ldap_get_valuesW(pLdap, pEntry, attr_type);
        if (typeValues && typeValues[0]) {
            DWORD groupType = (DWORD)_wtol(typeValues[0]);
            ParseGroupType(groupType, groupTypeStr, 64);
        }

        PWCHAR* dnValues = ldap_get_valuesW(pLdap, pEntry, attr_dn);
        if (dnValues && dnValues[0]) {
            distinguishedName = dnValues[0];
        }

        if (groupName) {
            AddGroup(groupName, description, groupTypeStr, distinguishedName);
        }

        if (values) ldap_value_freeW(values);
        if (descValues) ldap_value_freeW(descValues);
        if (typeValues) ldap_value_freeW(typeValues);
        if (dnValues) ldap_value_freeW(dnValues);

        pEntry = ldap_next_entry(pLdap, pEntry);
    }

    if (pResult) ldap_msgfree(pResult);
    ldap_unbind(pLdap);

    wprintf(L"LDAP���ѯ��ɣ��ܹ���ȡ�� %d ����\n", g_groupCount);
    return TRUE;
}

// ��ʾ�û��б�
void DisplayUsers(int maxCount) {
    wprintf(L"\n============ �û��б� ============\n");
    wprintf(L"���û���: %d\n", g_userCount);

    int displayCount = (maxCount > 0 && maxCount < g_userCount) ? maxCount : g_userCount;
    wprintf(L"��ʾǰ %d ���û�:\n", displayCount);
    wprintf(L"----------------------------------\n");

    for (int i = 0; i < displayCount; i++) {
        wprintf(L"[%d] �û���: %s", i + 1, g_users[i].samAccountName);

        if (wcslen(g_users[i].displayName) > 0) {
            wprintf(L", ��ʾ��: %s", g_users[i].displayName);
        }

        if (wcslen(g_users[i].userPrincipalName) > 0) {
            wprintf(L", UPN: %s", g_users[i].userPrincipalName);
        }

        wprintf(L"\n");
    }

    if (g_userCount > displayCount) {
        wprintf(L"... ���� %d ���û�δ��ʾ\n", g_userCount - displayCount);
    }
}

// ��ʾ���б�
void DisplayGroups(int maxCount) {
    wprintf(L"\n============ ���б� ============\n");
    wprintf(L"������: %d\n", g_groupCount);

    int displayCount = (maxCount > 0 && maxCount < g_groupCount) ? maxCount : g_groupCount;
    wprintf(L"��ʾǰ %d ����:\n", displayCount);
    wprintf(L"----------------------------------\n");

    for (int i = 0; i < displayCount; i++) {
        wprintf(L"[%d] ����: %s", i + 1, g_groups[i].groupName);

        if (wcslen(g_groups[i].groupType) > 0) {
            wprintf(L", ����: %s", g_groups[i].groupType);
        }

        if (wcslen(g_groups[i].description) > 0) {
            wprintf(L", ����: %s", g_groups[i].description);
        }

        wprintf(L"\n");
    }

    if (g_groupCount > displayCount) {
        wprintf(L"... ���� %d ����δ��ʾ\n", g_groupCount - displayCount);
    }
}

// ��ʾ������Ϣ
void ShowUsage(void) {
    wprintf(L"LDAP���û������ѯ���Գ���\n");
    wprintf(L"�÷�:\n");
    wprintf(L"  LDAPTest.exe [����] [��ʾ����] [��ѯ����]\n");
    wprintf(L"\n");
    wprintf(L"����:\n");
    wprintf(L"  ����       - Ҫ��ѯ������ (��ѡ��Ĭ��Ϊ��ǰ��)\n");
    wprintf(L"  ��ʾ����   - ��ʾ����Ŀ���� (��ѡ��Ĭ��50)\n");
    wprintf(L"  ��ѯ����   - users|groups|both (��ѡ��Ĭ��both)\n");
    wprintf(L"\n");
    wprintf(L"ʾ��:\n");
    wprintf(L"  LDAPTest.exe\n");
    wprintf(L"  LDAPTest.exe contoso.com\n");
    wprintf(L"  LDAPTest.exe contoso.com 100\n");
    wprintf(L"  LDAPTest.exe contoso.com 50 users\n");
    wprintf(L"  LDAPTest.exe contoso.com 50 groups\n");
    wprintf(L"  LDAPTest.exe contoso.com 50 both\n");
}

// ������
int wmain(int argc, wchar_t* argv[]) {
    setlocale(LC_ALL, "chs");

    SetConsoleOutputCP(CP_UTF8);

    wprintf(L"========================================\n");
    wprintf(L"   LDAP���û������ѯ���Գ���\n");
    wprintf(L"========================================\n");

    WCHAR domainName[256] = { 0 };
    int displayCount = 50;
    WCHAR queryType[32] = L"both";

    if (argc > 1) {
        if (wcscmp(argv[1], L"/?") == 0 || wcscmp(argv[1], L"-h") == 0 || wcscmp(argv[1], L"--help") == 0) {
            ShowUsage();
            return 0;
        }
        wcscpy_s(domainName, 256, argv[1]);
    }

    if (argc > 2) {
        displayCount = _wtoi(argv[2]);
        if (displayCount <= 0) displayCount = 50;
    }

    if (argc > 3) {
        wcscpy_s(queryType, 32, argv[3]);
    }

    if (wcslen(domainName) == 0) {
        GetCurrentDomain(domainName, 256);
    }

    wprintf(L"������Ϣ:\n");
    wprintf(L"- Ŀ����: %s\n", domainName);
    wprintf(L"- ��ʾ����: %d\n", displayCount);
    wprintf(L"- ��ѯ����: %s\n", queryType);
    wprintf(L"- ʹ�ö˿�: 389 (LDAP)\n");
    wprintf(L"========================================\n");

    WCHAR userName[256];
    DWORD userNameSize = 256;
    if (GetUserNameW(userName, &userNameSize)) {
        wprintf(L"��ǰ�û�: %s\n", userName);
    }

    wprintf(L"��ʼLDAP��ѯ...\n");

    DWORD startTime = GetTickCount();
    BOOL userSuccess = TRUE;
    BOOL groupSuccess = TRUE;

    if (wcscmp(queryType, L"users") == 0) {
        wprintf(L"=== ����ѯ�û� ===\n");
        userSuccess = QueryUsersViaLDAP(domainName);
        groupSuccess = TRUE;
    }
    else if (wcscmp(queryType, L"groups") == 0) {
        wprintf(L"=== ����ѯ�� ===\n");
        groupSuccess = QueryGroupsViaLDAP(domainName);
        userSuccess = TRUE;
    }
    else {
        wprintf(L"=== ��ѯ�û����� ===\n");
        userSuccess = QueryUsersViaLDAP(domainName);
        if (userSuccess) {
            wprintf(L"\n");
            groupSuccess = QueryGroupsViaLDAP(domainName);
        }
    }

    DWORD endTime = GetTickCount();

    if (userSuccess && groupSuccess) {
        wprintf(L"\n��ѯ�ɹ���ɣ�\n");
        wprintf(L"����ʱ: %lu ����\n", endTime - startTime);

        if (wcscmp(queryType, L"groups") != 0 && g_userCount > 0) {
            DisplayUsers(displayCount);
        }

        if (wcscmp(queryType, L"users") != 0 && g_groupCount > 0) {
            DisplayGroups(displayCount);
        }

        wprintf(L"\n============ ͳ����Ϣ ============\n");
        if (wcscmp(queryType, L"groups") != 0) {
            wprintf(L"�û�����: %d\n", g_userCount);
        }
        if (wcscmp(queryType, L"users") != 0) {
            wprintf(L"������: %d\n", g_groupCount);
        }
        wprintf(L"��ѯ��ʱ: %lu ����\n", endTime - startTime);

        wprintf(L"\n========================================\n");
        wprintf(L"������ɣ���������˳�...\n");

    }
    else {
        wprintf(L"\n��ѯʧ�ܣ�\n");
        wprintf(L"���ܵ�ԭ��:\n");
        wprintf(L"1. ������������\n");
        wprintf(L"2. ����������ɴ�\n");
        wprintf(L"3. �����֤ʧ��\n");
        wprintf(L"4. Ȩ�޲���\n");
        wprintf(L"5. ����ǽ��ֹ�˿�389\n");
        wprintf(L"6. DNS����ʧ��\n");

        if (g_users) {
            free(g_users);
            g_users = NULL;
        }
        if (g_groups) {
            free(g_groups);
            g_groups = NULL;
        }
        return 1;
    }

    _getch();

    if (g_users) {
        free(g_users);
        g_users = NULL;
    }

    if (g_groups) {
        free(g_groups);
        g_groups = NULL;
    }

    return 0;
}