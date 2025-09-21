#include <windows.h>
#include <winldap.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <conio.h>
#include <locale.h>

#pragma comment(lib, "wldap32.lib")

// 用户信息结构
typedef struct UserInfo {
    WCHAR samAccountName[256];
    WCHAR displayName[256];
    WCHAR userPrincipalName[256];
} UserInfo;

// 组信息结构
typedef struct GroupInfo {
    WCHAR groupName[256];
    WCHAR description[256];
    WCHAR groupType[64];
    WCHAR distinguishedName[512];
} GroupInfo;

// 全局变量
UserInfo* g_users = NULL;
int g_userCount = 0;
int g_userCapacity = 0;

GroupInfo* g_groups = NULL;
int g_groupCount = 0;
int g_groupCapacity = 0;

// 函数声明
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

// 添加用户到列表
void AddUser(const WCHAR* sam, const WCHAR* display, const WCHAR* upn) {
    if (g_userCount >= g_userCapacity) {
        g_userCapacity = g_userCapacity == 0 ? 1000 : g_userCapacity * 2;
        UserInfo* newUsers = (UserInfo*)realloc(g_users, g_userCapacity * sizeof(UserInfo));
        if (!newUsers) {
            wprintf(L"内存分配失败\n");
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

// 添加组到列表
void AddGroup(const WCHAR* name, const WCHAR* desc, const WCHAR* type, const WCHAR* dn) {
    if (g_groupCount >= g_groupCapacity) {
        g_groupCapacity = g_groupCapacity == 0 ? 500 : g_groupCapacity * 2;
        GroupInfo* newGroups = (GroupInfo*)realloc(g_groups, g_groupCapacity * sizeof(GroupInfo));
        if (!newGroups) {
            wprintf(L"组列表内存分配失败\n");
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

// 获取当前域名
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

// 构建DN字符串
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

// 解析组类型
void ParseGroupType(DWORD groupType, WCHAR* typeStr, DWORD size) {
    if (groupType & 0x00000002) {
        wcscpy_s(typeStr, size, L"全局");
    }
    else if (groupType & 0x00000004) {
        wcscpy_s(typeStr, size, L"域本地");
    }
    else if (groupType & 0x00000008) {
        wcscpy_s(typeStr, size, L"通用");
    }
    else {
        wcscpy_s(typeStr, size, L"未知");
    }

    if (groupType & 0x80000000) {
        wcscat_s(typeStr, size, L"安全组");
    }
    else {
        wcscat_s(typeStr, size, L"通讯组");
    }
}

// 执行LDAP用户查询
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
            wprintf(L"警告: 无法获取当前域名，使用localhost\n");
        }
    }

    wprintf(L"正在连接到域: %s\n", targetDomain);

    pLdap = ldap_initW(targetDomain, LDAP_PORT);
    if (!pLdap) {
        wprintf(L"错误: LDAP初始化失败\n");
        return FALSE;
    }

    ULONG version = LDAP_VERSION3;
    result = ldap_set_optionW(pLdap, LDAP_OPT_PROTOCOL_VERSION, &version);

    LDAP_TIMEVAL timeout = { 30, 0 };
    ldap_set_optionW(pLdap, LDAP_OPT_TIMELIMIT, &timeout);

    wprintf(L"正在进行身份验证...\n");

    result = ldap_bind_sW(pLdap, NULL, NULL, LDAP_AUTH_NEGOTIATE);
    if (result != LDAP_SUCCESS) {
        PWCHAR errorMsg = ldap_err2stringW(result);
        wprintf(L"错误: LDAP绑定失败 - %s (代码: %lu)\n", errorMsg, result);
        ldap_unbind(pLdap);
        return FALSE;
    }

    wprintf(L"身份验证成功！\n");

    BuildBaseDN(targetDomain, baseDN, 512);
    wprintf(L"搜索基础DN: %s\n", baseDN);

    static WCHAR filter[] = L"(&(objectClass=user)(objectCategory=person)(!(userAccountControl:1.2.840.113556.1.4.803:=2)))";

    wprintf(L"开始搜索用户...\n");

    result = ldap_search_sW(pLdap, baseDN, LDAP_SCOPE_SUBTREE, filter, attributes, 0, &pResult);

    if (result != LDAP_SUCCESS) {
        PWCHAR errorMsg = ldap_err2stringW(result);
        wprintf(L"错误: LDAP搜索失败 - %s (代码: %lu)\n", errorMsg, result);
        ldap_unbind(pLdap);
        return FALSE;
    }

    ULONG entryCount = ldap_count_entries(pLdap, pResult);
    wprintf(L"找到 %lu 个用户条目\n", entryCount);

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

    wprintf(L"LDAP用户查询完成！总共获取到 %d 个用户\n", g_userCount);
    return TRUE;
}

// 执行LDAP组查询
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
            wprintf(L"警告: 无法获取当前域名，使用localhost\n");
        }
    }

    wprintf(L"正在连接到域查询组: %s\n", targetDomain);

    pLdap = ldap_initW(targetDomain, LDAP_PORT);
    if (!pLdap) {
        wprintf(L"错误: LDAP初始化失败(组查询)\n");
        return FALSE;
    }

    ULONG version = LDAP_VERSION3;
    result = ldap_set_optionW(pLdap, LDAP_OPT_PROTOCOL_VERSION, &version);

    LDAP_TIMEVAL timeout = { 30, 0 };
    ldap_set_optionW(pLdap, LDAP_OPT_TIMELIMIT, &timeout);

    wprintf(L"正在进行组查询身份验证...\n");

    result = ldap_bind_sW(pLdap, NULL, NULL, LDAP_AUTH_NEGOTIATE);
    if (result != LDAP_SUCCESS) {
        PWCHAR errorMsg = ldap_err2stringW(result);
        wprintf(L"错误: LDAP绑定失败(组查询) - %s (代码: %lu)\n", errorMsg, result);
        ldap_unbind(pLdap);
        return FALSE;
    }

    wprintf(L"组查询身份验证成功！\n");

    BuildBaseDN(targetDomain, baseDN, 512);
    wprintf(L"组搜索基础DN: %s\n", baseDN);

    static WCHAR filter[] = L"(objectClass=group)";

    wprintf(L"开始搜索组...\n");

    result = ldap_search_sW(pLdap, baseDN, LDAP_SCOPE_SUBTREE, filter, attributes, 0, &pResult);

    if (result != LDAP_SUCCESS) {
        PWCHAR errorMsg = ldap_err2stringW(result);
        wprintf(L"错误: LDAP组搜索失败 - %s (代码: %lu)\n", errorMsg, result);
        ldap_unbind(pLdap);
        return FALSE;
    }

    ULONG entryCount = ldap_count_entries(pLdap, pResult);
    wprintf(L"找到 %lu 个组条目\n", entryCount);

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

    wprintf(L"LDAP组查询完成！总共获取到 %d 个组\n", g_groupCount);
    return TRUE;
}

// 显示用户列表
void DisplayUsers(int maxCount) {
    wprintf(L"\n============ 用户列表 ============\n");
    wprintf(L"总用户数: %d\n", g_userCount);

    int displayCount = (maxCount > 0 && maxCount < g_userCount) ? maxCount : g_userCount;
    wprintf(L"显示前 %d 个用户:\n", displayCount);
    wprintf(L"----------------------------------\n");

    for (int i = 0; i < displayCount; i++) {
        wprintf(L"[%d] 用户名: %s", i + 1, g_users[i].samAccountName);

        if (wcslen(g_users[i].displayName) > 0) {
            wprintf(L", 显示名: %s", g_users[i].displayName);
        }

        if (wcslen(g_users[i].userPrincipalName) > 0) {
            wprintf(L", UPN: %s", g_users[i].userPrincipalName);
        }

        wprintf(L"\n");
    }

    if (g_userCount > displayCount) {
        wprintf(L"... 还有 %d 个用户未显示\n", g_userCount - displayCount);
    }
}

// 显示组列表
void DisplayGroups(int maxCount) {
    wprintf(L"\n============ 组列表 ============\n");
    wprintf(L"总组数: %d\n", g_groupCount);

    int displayCount = (maxCount > 0 && maxCount < g_groupCount) ? maxCount : g_groupCount;
    wprintf(L"显示前 %d 个组:\n", displayCount);
    wprintf(L"----------------------------------\n");

    for (int i = 0; i < displayCount; i++) {
        wprintf(L"[%d] 组名: %s", i + 1, g_groups[i].groupName);

        if (wcslen(g_groups[i].groupType) > 0) {
            wprintf(L", 类型: %s", g_groups[i].groupType);
        }

        if (wcslen(g_groups[i].description) > 0) {
            wprintf(L", 描述: %s", g_groups[i].description);
        }

        wprintf(L"\n");
    }

    if (g_groupCount > displayCount) {
        wprintf(L"... 还有 %d 个组未显示\n", g_groupCount - displayCount);
    }
}

// 显示帮助信息
void ShowUsage(void) {
    wprintf(L"LDAP域用户和组查询测试程序\n");
    wprintf(L"用法:\n");
    wprintf(L"  LDAPTest.exe [域名] [显示数量] [查询类型]\n");
    wprintf(L"\n");
    wprintf(L"参数:\n");
    wprintf(L"  域名       - 要查询的域名 (可选，默认为当前域)\n");
    wprintf(L"  显示数量   - 显示的条目数量 (可选，默认50)\n");
    wprintf(L"  查询类型   - users|groups|both (可选，默认both)\n");
    wprintf(L"\n");
    wprintf(L"示例:\n");
    wprintf(L"  LDAPTest.exe\n");
    wprintf(L"  LDAPTest.exe contoso.com\n");
    wprintf(L"  LDAPTest.exe contoso.com 100\n");
    wprintf(L"  LDAPTest.exe contoso.com 50 users\n");
    wprintf(L"  LDAPTest.exe contoso.com 50 groups\n");
    wprintf(L"  LDAPTest.exe contoso.com 50 both\n");
}

// 主函数
int wmain(int argc, wchar_t* argv[]) {
    setlocale(LC_ALL, "chs");

    SetConsoleOutputCP(CP_UTF8);

    wprintf(L"========================================\n");
    wprintf(L"   LDAP域用户和组查询测试程序\n");
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

    wprintf(L"配置信息:\n");
    wprintf(L"- 目标域: %s\n", domainName);
    wprintf(L"- 显示数量: %d\n", displayCount);
    wprintf(L"- 查询类型: %s\n", queryType);
    wprintf(L"- 使用端口: 389 (LDAP)\n");
    wprintf(L"========================================\n");

    WCHAR userName[256];
    DWORD userNameSize = 256;
    if (GetUserNameW(userName, &userNameSize)) {
        wprintf(L"当前用户: %s\n", userName);
    }

    wprintf(L"开始LDAP查询...\n");

    DWORD startTime = GetTickCount();
    BOOL userSuccess = TRUE;
    BOOL groupSuccess = TRUE;

    if (wcscmp(queryType, L"users") == 0) {
        wprintf(L"=== 仅查询用户 ===\n");
        userSuccess = QueryUsersViaLDAP(domainName);
        groupSuccess = TRUE;
    }
    else if (wcscmp(queryType, L"groups") == 0) {
        wprintf(L"=== 仅查询组 ===\n");
        groupSuccess = QueryGroupsViaLDAP(domainName);
        userSuccess = TRUE;
    }
    else {
        wprintf(L"=== 查询用户和组 ===\n");
        userSuccess = QueryUsersViaLDAP(domainName);
        if (userSuccess) {
            wprintf(L"\n");
            groupSuccess = QueryGroupsViaLDAP(domainName);
        }
    }

    DWORD endTime = GetTickCount();

    if (userSuccess && groupSuccess) {
        wprintf(L"\n查询成功完成！\n");
        wprintf(L"总用时: %lu 毫秒\n", endTime - startTime);

        if (wcscmp(queryType, L"groups") != 0 && g_userCount > 0) {
            DisplayUsers(displayCount);
        }

        if (wcscmp(queryType, L"users") != 0 && g_groupCount > 0) {
            DisplayGroups(displayCount);
        }

        wprintf(L"\n============ 统计信息 ============\n");
        if (wcscmp(queryType, L"groups") != 0) {
            wprintf(L"用户总数: %d\n", g_userCount);
        }
        if (wcscmp(queryType, L"users") != 0) {
            wprintf(L"组总数: %d\n", g_groupCount);
        }
        wprintf(L"查询耗时: %lu 毫秒\n", endTime - startTime);

        wprintf(L"\n========================================\n");
        wprintf(L"测试完成！按任意键退出...\n");

    }
    else {
        wprintf(L"\n查询失败！\n");
        wprintf(L"可能的原因:\n");
        wprintf(L"1. 网络连接问题\n");
        wprintf(L"2. 域控制器不可达\n");
        wprintf(L"3. 身份验证失败\n");
        wprintf(L"4. 权限不足\n");
        wprintf(L"5. 防火墙阻止端口389\n");
        wprintf(L"6. DNS解析失败\n");

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