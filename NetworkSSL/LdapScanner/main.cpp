//#include <windows.h>
//#include <winldap.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <wchar.h>
//#include <conio.h>
//#include <locale.h>
//
//#pragma comment(lib, "wldap32.lib")
//
//// 用户信息结构 - 增加组成员信息
//typedef struct UserInfo {
//    WCHAR samAccountName[256];
//    WCHAR displayName[256];
//    WCHAR userPrincipalName[256];
//    WCHAR** memberOfGroups;  // 用户所属组的DN数组
//    int groupCount;          // 用户所属组的数量
//    int groupCapacity;       // 组数组容量
//} UserInfo;
//
//// 组信息结构 - 增加成员信息
//typedef struct GroupInfo {
//    WCHAR groupName[256];
//    WCHAR description[256];
//    WCHAR groupType[64];
//    WCHAR distinguishedName[512];
//    WCHAR** members;         // 组成员的DN数组
//    int memberCount;         // 组成员数量
//    int memberCapacity;      // 成员数组容量
//} GroupInfo;
//
//// 全局变量
//UserInfo* g_users = NULL;
//int g_userCount = 0;
//int g_userCapacity = 0;
//
//GroupInfo* g_groups = NULL;
//int g_groupCount = 0;
//int g_groupCapacity = 0;
//
//// 函数声明
//void AddUser(const WCHAR* sam, const WCHAR* display, const WCHAR* upn, PWCHAR* memberOf, int memberCount);
//void AddGroup(const WCHAR* name, const WCHAR* desc, const WCHAR* type, const WCHAR* dn, PWCHAR* members, int memberCount);
//BOOL GetCurrentDomain(WCHAR* domainName, DWORD size);
//void BuildBaseDN(const WCHAR* domain, WCHAR* baseDN, DWORD size);
//void ParseGroupType(DWORD groupType, WCHAR* typeStr, DWORD size);
//BOOL QueryUsersViaLDAP(const WCHAR* domainName);
//BOOL QueryGroupsViaLDAP(const WCHAR* domainName);
//void DisplayUsers(int maxCount);
//void DisplayGroups(int maxCount);
//void DisplayUserGroupMapping(int maxCount);
//void ShowUsage(void);
//void FreeUserInfo(UserInfo* user);
//void FreeGroupInfo(GroupInfo* group);
//WCHAR* ExtractCNFromDN(const WCHAR* dn);
//
//// 从DN中提取CN
//WCHAR* ExtractCNFromDN(const WCHAR* dn) {
//    if (!dn) return NULL;
//
//    static WCHAR cn[256];
//    const WCHAR* start = wcsstr(dn, L"CN=");
//    if (!start) return NULL;
//
//    start += 3; // 跳过 "CN="
//    const WCHAR* end = wcschr(start, L',');
//
//    if (end) {
//        size_t len = end - start;
//        if (len >= 256) len = 255;
//        wcsncpy_s(cn, 256, start, len);
//        cn[len] = L'\0';
//    }
//    else {
//        wcscpy_s(cn, 256, start);
//    }
//
//    return cn;
//}
//
//// 释放用户信息内存
//void FreeUserInfo(UserInfo* user) {
//    if (user->memberOfGroups) {
//        for (int i = 0; i < user->groupCount; i++) {
//            if (user->memberOfGroups[i]) {
//                free(user->memberOfGroups[i]);
//            }
//        }
//        free(user->memberOfGroups);
//        user->memberOfGroups = NULL;
//    }
//    user->groupCount = 0;
//    user->groupCapacity = 0;
//}
//
//// 释放组信息内存
//void FreeGroupInfo(GroupInfo* group) {
//    if (group->members) {
//        for (int i = 0; i < group->memberCount; i++) {
//            if (group->members[i]) {
//                free(group->members[i]);
//            }
//        }
//        free(group->members);
//        group->members = NULL;
//    }
//    group->memberCount = 0;
//    group->memberCapacity = 0;
//}
//
//// 添加用户到列表 - 包含组成员信息
//void AddUser(const WCHAR* sam, const WCHAR* display, const WCHAR* upn, PWCHAR* memberOf, int memberCount) {
//    if (g_userCount >= g_userCapacity) {
//        g_userCapacity = g_userCapacity == 0 ? 1000 : g_userCapacity * 2;
//        UserInfo* newUsers = (UserInfo*)realloc(g_users, g_userCapacity * sizeof(UserInfo));
//        if (!newUsers) {
//            wprintf(L"内存分配失败\n");
//            return;
//        }
//        g_users = newUsers;
//    }
//
//    UserInfo* user = &g_users[g_userCount];
//
//    // 初始化用户信息
//    if (sam) {
//        wcsncpy_s(user->samAccountName, 256, sam, _TRUNCATE);
//    }
//    else {
//        user->samAccountName[0] = L'\0';
//    }
//
//    if (display) {
//        wcsncpy_s(user->displayName, 256, display, _TRUNCATE);
//    }
//    else {
//        user->displayName[0] = L'\0';
//    }
//
//    if (upn) {
//        wcsncpy_s(user->userPrincipalName, 256, upn, _TRUNCATE);
//    }
//    else {
//        user->userPrincipalName[0] = L'\0';
//    }
//
//    // 初始化组信息
//    user->memberOfGroups = NULL;
//    user->groupCount = 0;
//    user->groupCapacity = 0;
//
//    // 添加组成员信息
//    if (memberOf && memberCount > 0) {
//        user->groupCapacity = memberCount;
//        user->memberOfGroups = (WCHAR**)malloc(memberCount * sizeof(WCHAR*));
//        if (user->memberOfGroups) {
//            for (int i = 0; i < memberCount; i++) {
//                if (memberOf[i]) {
//                    size_t len = wcslen(memberOf[i]) + 1;
//                    user->memberOfGroups[i] = (WCHAR*)malloc(len * sizeof(WCHAR));
//                    if (user->memberOfGroups[i]) {
//                        wcscpy_s(user->memberOfGroups[i], len, memberOf[i]);
//                        user->groupCount++;
//                    }
//                }
//            }
//        }
//    }
//
//    g_userCount++;
//}
//
//// 添加组到列表 - 包含成员信息
//void AddGroup(const WCHAR* name, const WCHAR* desc, const WCHAR* type, const WCHAR* dn, PWCHAR* members, int memberCount) {
//    if (g_groupCount >= g_groupCapacity) {
//        g_groupCapacity = g_groupCapacity == 0 ? 500 : g_groupCapacity * 2;
//        GroupInfo* newGroups = (GroupInfo*)realloc(g_groups, g_groupCapacity * sizeof(GroupInfo));
//        if (!newGroups) {
//            wprintf(L"组列表内存分配失败\n");
//            return;
//        }
//        g_groups = newGroups;
//    }
//
//    GroupInfo* group = &g_groups[g_groupCount];
//
//    if (name) {
//        wcsncpy_s(group->groupName, 256, name, _TRUNCATE);
//    }
//    else {
//        group->groupName[0] = L'\0';
//    }
//
//    if (desc) {
//        wcsncpy_s(group->description, 256, desc, _TRUNCATE);
//    }
//    else {
//        group->description[0] = L'\0';
//    }
//
//    if (type) {
//        wcsncpy_s(group->groupType, 64, type, _TRUNCATE);
//    }
//    else {
//        group->groupType[0] = L'\0';
//    }
//
//    if (dn) {
//        wcsncpy_s(group->distinguishedName, 512, dn, _TRUNCATE);
//    }
//    else {
//        group->distinguishedName[0] = L'\0';
//    }
//
//    // 初始化成员信息
//    group->members = NULL;
//    group->memberCount = 0;
//    group->memberCapacity = 0;
//
//    // 添加成员信息
//    if (members && memberCount > 0) {
//        group->memberCapacity = memberCount;
//        group->members = (WCHAR**)malloc(memberCount * sizeof(WCHAR*));
//        if (group->members) {
//            for (int i = 0; i < memberCount; i++) {
//                if (members[i]) {
//                    size_t len = wcslen(members[i]) + 1;
//                    group->members[i] = (WCHAR*)malloc(len * sizeof(WCHAR));
//                    if (group->members[i]) {
//                        wcscpy_s(group->members[i], len, members[i]);
//                        group->memberCount++;
//                    }
//                }
//            }
//        }
//    }
//
//    g_groupCount++;
//}
//
//// 获取当前域名
//BOOL GetCurrentDomain(WCHAR* domainName, DWORD size) {
//    DWORD actualSize = size;
//    if (GetComputerNameExW(ComputerNameDnsDomain, domainName, &actualSize)) {
//        return TRUE;
//    }
//
//    if (GetEnvironmentVariableW(L"USERDNSDOMAIN", domainName, size) > 0) {
//        return TRUE;
//    }
//
//    wcscpy_s(domainName, size, L"localhost");
//    return FALSE;
//}
//
//// 构建DN字符串
//void BuildBaseDN(const WCHAR* domain, WCHAR* baseDN, DWORD size) {
//    WCHAR tempDomain[256];
//    wcscpy_s(tempDomain, 256, domain);
//
//    wcscpy_s(baseDN, size, L"DC=");
//
//    WCHAR* context = NULL;
//    WCHAR* token = wcstok_s(tempDomain, L".", &context);
//    BOOL first = TRUE;
//
//    while (token != NULL) {
//        if (!first) {
//            wcscat_s(baseDN, size, L",DC=");
//        }
//        wcscat_s(baseDN, size, token);
//        first = FALSE;
//        token = wcstok_s(NULL, L".", &context);
//    }
//}
//
//// 解析组类型
//void ParseGroupType(DWORD groupType, WCHAR* typeStr, DWORD size) {
//    if (groupType & 0x00000002) {
//        wcscpy_s(typeStr, size, L"全局");
//    }
//    else if (groupType & 0x00000004) {
//        wcscpy_s(typeStr, size, L"域本地");
//    }
//    else if (groupType & 0x00000008) {
//        wcscpy_s(typeStr, size, L"通用");
//    }
//    else {
//        wcscpy_s(typeStr, size, L"未知");
//    }
//
//    if (groupType & 0x80000000) {
//        wcscat_s(typeStr, size, L"安全组");
//    }
//    else {
//        wcscat_s(typeStr, size, L"通讯组");
//    }
//}
//
//// 执行LDAP用户查询 - 包含memberOf属性
//BOOL QueryUsersViaLDAP(const WCHAR* domainName) {
//    LDAP* pLdap = NULL;
//    LDAPMessage* pResult = NULL;
//    LDAPMessage* pEntry = NULL;
//    ULONG result;
//    WCHAR baseDN[512];
//    WCHAR targetDomain[256];
//
//    static WCHAR attr_sam[] = L"sAMAccountName";
//    static WCHAR attr_display[] = L"displayName";
//    static WCHAR attr_upn[] = L"userPrincipalName";
//    static WCHAR attr_memberof[] = L"memberOf";
//    PWCHAR attributes[5];
//    attributes[0] = attr_sam;
//    attributes[1] = attr_display;
//    attributes[2] = attr_upn;
//    attributes[3] = attr_memberof;
//    attributes[4] = NULL;
//
//    if (domainName && wcslen(domainName) > 0) {
//        wcscpy_s(targetDomain, 256, domainName);
//    }
//    else {
//        if (!GetCurrentDomain(targetDomain, 256)) {
//            wprintf(L"警告: 无法获取当前域名，使用localhost\n");
//        }
//    }
//
//    wprintf(L"正在连接到域: %s\n", targetDomain);
//
//    pLdap = ldap_initW(targetDomain, LDAP_PORT);
//    if (!pLdap) {
//        wprintf(L"错误: LDAP初始化失败\n");
//        return FALSE;
//    }
//
//    ULONG version = LDAP_VERSION3;
//    result = ldap_set_optionW(pLdap, LDAP_OPT_PROTOCOL_VERSION, &version);
//
//    LDAP_TIMEVAL timeout = { 30, 0 };
//    ldap_set_optionW(pLdap, LDAP_OPT_TIMELIMIT, &timeout);
//
//    wprintf(L"正在进行身份验证...\n");
//
//    result = ldap_bind_sW(pLdap, NULL, NULL, LDAP_AUTH_NEGOTIATE);
//    if (result != LDAP_SUCCESS) {
//        PWCHAR errorMsg = ldap_err2stringW(result);
//        wprintf(L"错误: LDAP绑定失败 - %s (代码: %lu)\n", errorMsg, result);
//        ldap_unbind(pLdap);
//        return FALSE;
//    }
//
//    wprintf(L"身份验证成功！\n");
//
//    BuildBaseDN(targetDomain, baseDN, 512);
//    wprintf(L"搜索基础DN: %s\n", baseDN);
//
//    static WCHAR filter[] = L"(&(objectClass=user)(objectCategory=person)(!(userAccountControl:1.2.840.113556.1.4.803:=2)))";
//
//    wprintf(L"开始搜索用户(包含组成员信息)...\n");
//
//    result = ldap_search_sW(pLdap, baseDN, LDAP_SCOPE_SUBTREE, filter, attributes, 0, &pResult);
//
//    if (result != LDAP_SUCCESS) {
//        PWCHAR errorMsg = ldap_err2stringW(result);
//        wprintf(L"错误: LDAP搜索失败 - %s (代码: %lu)\n", errorMsg, result);
//        ldap_unbind(pLdap);
//        return FALSE;
//    }
//
//    ULONG entryCount = ldap_count_entries(pLdap, pResult);
//    wprintf(L"找到 %lu 个用户条目\n", entryCount);
//
//    pEntry = ldap_first_entry(pLdap, pResult);
//    while (pEntry) {
//        PWCHAR samAccountName = NULL;
//        PWCHAR displayName = NULL;
//        PWCHAR userPrincipalName = NULL;
//
//        PWCHAR* values = ldap_get_valuesW(pLdap, pEntry, attr_sam);
//        if (values && values[0]) {
//            samAccountName = values[0];
//        }
//
//        PWCHAR* displayValues = ldap_get_valuesW(pLdap, pEntry, attr_display);
//        if (displayValues && displayValues[0]) {
//            displayName = displayValues[0];
//        }
//
//        PWCHAR* upnValues = ldap_get_valuesW(pLdap, pEntry, attr_upn);
//        if (upnValues && upnValues[0]) {
//            userPrincipalName = upnValues[0];
//        }
//
//        // 获取memberOf属性
//        PWCHAR* memberOfValues = ldap_get_valuesW(pLdap, pEntry, attr_memberof);
//        int memberOfCount = 0;
//        if (memberOfValues) {
//            memberOfCount = ldap_count_valuesW(memberOfValues);
//        }
//
//        if (samAccountName) {
//            AddUser(samAccountName, displayName, userPrincipalName, memberOfValues, memberOfCount);
//        }
//
//        if (values) ldap_value_freeW(values);
//        if (displayValues) ldap_value_freeW(displayValues);
//        if (upnValues) ldap_value_freeW(upnValues);
//        if (memberOfValues) ldap_value_freeW(memberOfValues);
//
//        pEntry = ldap_next_entry(pLdap, pEntry);
//    }
//
//    if (pResult) ldap_msgfree(pResult);
//    ldap_unbind(pLdap);
//
//    wprintf(L"LDAP用户查询完成！总共获取到 %d 个用户\n", g_userCount);
//    return TRUE;
//}
//
//// 执行LDAP组查询 - 包含member属性
//BOOL QueryGroupsViaLDAP(const WCHAR* domainName) {
//    LDAP* pLdap = NULL;
//    LDAPMessage* pResult = NULL;
//    LDAPMessage* pEntry = NULL;
//    ULONG result;
//    WCHAR baseDN[512];
//    WCHAR targetDomain[256];
//
//    static WCHAR attr_cn[] = L"cn";
//    static WCHAR attr_desc[] = L"description";
//    static WCHAR attr_type[] = L"groupType";
//    static WCHAR attr_dn[] = L"distinguishedName";
//    static WCHAR attr_member[] = L"member";
//    PWCHAR attributes[6];
//    attributes[0] = attr_cn;
//    attributes[1] = attr_desc;
//    attributes[2] = attr_type;
//    attributes[3] = attr_dn;
//    attributes[4] = attr_member;
//    attributes[5] = NULL;
//
//    if (domainName && wcslen(domainName) > 0) {
//        wcscpy_s(targetDomain, 256, domainName);
//    }
//    else {
//        if (!GetCurrentDomain(targetDomain, 256)) {
//            wprintf(L"警告: 无法获取当前域名，使用localhost\n");
//        }
//    }
//
//    wprintf(L"正在连接到域查询组: %s\n", targetDomain);
//
//    pLdap = ldap_initW(targetDomain, LDAP_PORT);
//    if (!pLdap) {
//        wprintf(L"错误: LDAP初始化失败(组查询)\n");
//        return FALSE;
//    }
//
//    ULONG version = LDAP_VERSION3;
//    result = ldap_set_optionW(pLdap, LDAP_OPT_PROTOCOL_VERSION, &version);
//
//    LDAP_TIMEVAL timeout = { 30, 0 };
//    ldap_set_optionW(pLdap, LDAP_OPT_TIMELIMIT, &timeout);
//
//    wprintf(L"正在进行组查询身份验证...\n");
//
//    result = ldap_bind_sW(pLdap, NULL, NULL, LDAP_AUTH_NEGOTIATE);
//    if (result != LDAP_SUCCESS) {
//        PWCHAR errorMsg = ldap_err2stringW(result);
//        wprintf(L"错误: LDAP绑定失败(组查询) - %s (代码: %lu)\n", errorMsg, result);
//        ldap_unbind(pLdap);
//        return FALSE;
//    }
//
//    wprintf(L"组查询身份验证成功！\n");
//
//    BuildBaseDN(targetDomain, baseDN, 512);
//    wprintf(L"组搜索基础DN: %s\n", baseDN);
//
//    static WCHAR filter[] = L"(objectClass=group)";
//
//    wprintf(L"开始搜索组(包含成员信息)...\n");
//
//    result = ldap_search_sW(pLdap, baseDN, LDAP_SCOPE_SUBTREE, filter, attributes, 0, &pResult);
//
//    if (result != LDAP_SUCCESS) {
//        PWCHAR errorMsg = ldap_err2stringW(result);
//        wprintf(L"错误: LDAP组搜索失败 - %s (代码: %lu)\n", errorMsg, result);
//        ldap_unbind(pLdap);
//        return FALSE;
//    }
//
//    ULONG entryCount = ldap_count_entries(pLdap, pResult);
//    wprintf(L"找到 %lu 个组条目\n", entryCount);
//
//    pEntry = ldap_first_entry(pLdap, pResult);
//    while (pEntry) {
//        PWCHAR groupName = NULL;
//        PWCHAR description = NULL;
//        PWCHAR distinguishedName = NULL;
//        WCHAR groupTypeStr[64] = { 0 };
//
//        PWCHAR* values = ldap_get_valuesW(pLdap, pEntry, attr_cn);
//        if (values && values[0]) {
//            groupName = values[0];
//        }
//
//        PWCHAR* descValues = ldap_get_valuesW(pLdap, pEntry, attr_desc);
//        if (descValues && descValues[0]) {
//            description = descValues[0];
//        }
//
//        PWCHAR* typeValues = ldap_get_valuesW(pLdap, pEntry, attr_type);
//        if (typeValues && typeValues[0]) {
//            DWORD groupType = (DWORD)_wtol(typeValues[0]);
//            ParseGroupType(groupType, groupTypeStr, 64);
//        }
//
//        PWCHAR* dnValues = ldap_get_valuesW(pLdap, pEntry, attr_dn);
//        if (dnValues && dnValues[0]) {
//            distinguishedName = dnValues[0];
//        }
//
//        // 获取member属性
//        PWCHAR* memberValues = ldap_get_valuesW(pLdap, pEntry, attr_member);
//        int memberCount = 0;
//        if (memberValues) {
//            memberCount = ldap_count_valuesW(memberValues);
//        }
//
//        if (groupName) {
//            AddGroup(groupName, description, groupTypeStr, distinguishedName, memberValues, memberCount);
//        }
//
//        if (values) ldap_value_freeW(values);
//        if (descValues) ldap_value_freeW(descValues);
//        if (typeValues) ldap_value_freeW(typeValues);
//        if (dnValues) ldap_value_freeW(dnValues);
//        if (memberValues) ldap_value_freeW(memberValues);
//
//        pEntry = ldap_next_entry(pLdap, pEntry);
//    }
//
//    if (pResult) ldap_msgfree(pResult);
//    ldap_unbind(pLdap);
//
//    wprintf(L"LDAP组查询完成！总共获取到 %d 个组\n", g_groupCount);
//    return TRUE;
//}
//
//// 显示用户列表 - 包含组成员信息
//void DisplayUsers(int maxCount) {
//    wprintf(L"\n============ 用户列表(包含组信息) ============\n");
//    wprintf(L"总用户数: %d\n", g_userCount);
//
//    int displayCount = (maxCount > 0 && maxCount < g_userCount) ? maxCount : g_userCount;
//    wprintf(L"显示前 %d 个用户:\n", displayCount);
//    wprintf(L"--------------------------------------------\n");
//
//    for (int i = 0; i < displayCount; i++) {
//        wprintf(L"[%d] 用户名: %s", i + 1, g_users[i].samAccountName);
//
//        if (wcslen(g_users[i].displayName) > 0) {
//            wprintf(L", 显示名: %s", g_users[i].displayName);
//        }
//
//        if (wcslen(g_users[i].userPrincipalName) > 0) {
//            wprintf(L", UPN: %s", g_users[i].userPrincipalName);
//        }
//
//        wprintf(L"\n");
//
//        // 显示用户所属的组
//        if (g_users[i].groupCount > 0) {
//            wprintf(L"    所属组 (%d个):\n", g_users[i].groupCount);
//            for (int j = 0; j < g_users[i].groupCount; j++) {
//                WCHAR* groupCN = ExtractCNFromDN(g_users[i].memberOfGroups[j]);
//                if (groupCN) {
//                    wprintf(L"      - %s\n", groupCN);
//                }
//                else {
//                    wprintf(L"      - %s\n", g_users[i].memberOfGroups[j]);
//                }
//            }
//        }
//        else {
//            wprintf(L"    所属组: (无)\n");
//        }
//        wprintf(L"\n");
//    }
//
//    if (g_userCount > displayCount) {
//        wprintf(L"... 还有 %d 个用户未显示\n", g_userCount - displayCount);
//    }
//}
//
//// 显示组列表 - 包含成员信息
//void DisplayGroups(int maxCount) {
//    wprintf(L"\n============ 组列表(包含成员信息) ============\n");
//    wprintf(L"总组数: %d\n", g_groupCount);
//
//    int displayCount = (maxCount > 0 && maxCount < g_groupCount) ? maxCount : g_groupCount;
//    wprintf(L"显示前 %d 个组:\n", displayCount);
//    wprintf(L"--------------------------------------------\n");
//
//    for (int i = 0; i < displayCount; i++) {
//        wprintf(L"[%d] 组名: %s", i + 1, g_groups[i].groupName);
//
//        if (wcslen(g_groups[i].groupType) > 0) {
//            wprintf(L", 类型: %s", g_groups[i].groupType);
//        }
//
//        if (wcslen(g_groups[i].description) > 0) {
//            wprintf(L", 描述: %s", g_groups[i].description);
//        }
//
//        wprintf(L"\n");
//
//        // 显示组成员
//        if (g_groups[i].memberCount > 0) {
//            wprintf(L"    组成员 (%d个):\n", g_groups[i].memberCount);
//            for (int j = 0; j < g_groups[i].memberCount && j < 10; j++) { // 最多显示10个成员
//                WCHAR* memberCN = ExtractCNFromDN(g_groups[i].members[j]);
//                if (memberCN) {
//                    wprintf(L"      - %s\n", memberCN);
//                }
//                else {
//                    wprintf(L"      - %s\n", g_groups[i].members[j]);
//                }
//            }
//            if (g_groups[i].memberCount > 10) {
//                wprintf(L"      ... 还有 %d 个成员\n", g_groups[i].memberCount - 10);
//            }
//        }
//        else {
//            wprintf(L"    组成员: (无)\n");
//        }
//        wprintf(L"\n");
//    }
//
//    if (g_groupCount > displayCount) {
//        wprintf(L"... 还有 %d 个组未显示\n", g_groupCount - displayCount);
//    }
//}
//
//// 显示用户-组关系映射
//void DisplayUserGroupMapping(int maxCount) {
//    wprintf(L"\n============ 用户-组关系映射 ============\n");
//
//    int displayCount = (maxCount > 0 && maxCount < g_userCount) ? maxCount : g_userCount;
//    wprintf(L"显示前 %d 个用户的组关系:\n", displayCount);
//    wprintf(L"----------------------------------------\n");
//
//    for (int i = 0; i < displayCount; i++) {
//        wprintf(L"%s", g_users[i].samAccountName);
//        if (wcslen(g_users[i].displayName) > 0) {
//            wprintf(L" (%s)", g_users[i].displayName);
//        }
//        wprintf(L":\n");
//
//        if (g_users[i].groupCount > 0) {
//            for (int j = 0; j < g_users[i].groupCount; j++) {
//                WCHAR* groupCN = ExtractCNFromDN(g_users[i].memberOfGroups[j]);
//                if (groupCN) {
//                    wprintf(L"  ├─ %s\n", groupCN);
//                }
//            }
//        }
//        else {
//            wprintf(L"  └─ (无组成员关系)\n");
//        }
//        wprintf(L"\n");
//    }
//}
//
//// 显示帮助信息
//void ShowUsage(void) {
//    wprintf(L"LDAP域用户和组关联查询程序\n");
//    wprintf(L"用法:\n");
//    wprintf(L"  LDAPTest.exe [域名] [显示数量] [查询类型]\n");
//    wprintf(L"\n");
//    wprintf(L"参数:\n");
//    wprintf(L"  域名       - 要查询的域名 (可选，默认为当前域)\n");
//    wprintf(L"  显示数量   - 显示的条目数量 (可选，默认50)\n");
//    wprintf(L"  查询类型   - users|groups|both|mapping (可选，默认both)\n");
//    wprintf(L"\n");
//    wprintf(L"查询类型说明:\n");
//    wprintf(L"  users    - 仅查询用户(包含所属组信息)\n");
//}
//
//// 主函数
//int wmain(int argc, wchar_t* argv[]) {
//    setlocale(LC_ALL, "chs");
//
//    SetConsoleOutputCP(CP_UTF8);
//
//    wprintf(L"========================================\n");
//    wprintf(L"   LDAP域用户和组查询测试程序\n");
//    wprintf(L"========================================\n");
//
//    WCHAR domainName[256] = { 0 };
//    int displayCount = 50;
//    WCHAR queryType[32] = L"both";
//
//    if (argc > 1) {
//        if (wcscmp(argv[1], L"/?") == 0 || wcscmp(argv[1], L"-h") == 0 || wcscmp(argv[1], L"--help") == 0) {
//            ShowUsage();
//            return 0;
//        }
//        wcscpy_s(domainName, 256, argv[1]);
//    }
//
//    if (argc > 2) {
//        displayCount = _wtoi(argv[2]);
//        if (displayCount <= 0) displayCount = 50;
//    }
//
//    if (argc > 3) {
//        wcscpy_s(queryType, 32, argv[3]);
//    }
//
//    if (wcslen(domainName) == 0) {
//        GetCurrentDomain(domainName, 256);
//    }
//
//    wprintf(L"配置信息:\n");
//    wprintf(L"- 目标域: %s\n", domainName);
//    wprintf(L"- 显示数量: %d\n", displayCount);
//    wprintf(L"- 查询类型: %s\n", queryType);
//    wprintf(L"- 使用端口: 389 (LDAP)\n");
//    wprintf(L"========================================\n");
//
//    WCHAR userName[256];
//    DWORD userNameSize = 256;
//    if (GetUserNameW(userName, &userNameSize)) {
//        wprintf(L"当前用户: %s\n", userName);
//    }
//
//    wprintf(L"开始LDAP查询...\n");
//
//    DWORD startTime = GetTickCount();
//    BOOL userSuccess = TRUE;
//    BOOL groupSuccess = TRUE;
//
//    if (wcscmp(queryType, L"users") == 0) {
//        wprintf(L"=== 仅查询用户 ===\n");
//        userSuccess = QueryUsersViaLDAP(domainName);
//        groupSuccess = TRUE;
//    }
//    else if (wcscmp(queryType, L"groups") == 0) {
//        wprintf(L"=== 仅查询组 ===\n");
//        groupSuccess = QueryGroupsViaLDAP(domainName);
//        userSuccess = TRUE;
//    }
//    else {
//        wprintf(L"=== 查询用户和组 ===\n");
//        userSuccess = QueryUsersViaLDAP(domainName);
//        if (userSuccess) {
//            wprintf(L"\n");
//            groupSuccess = QueryGroupsViaLDAP(domainName);
//        }
//    }
//
//    DWORD endTime = GetTickCount();
//
//    if (userSuccess && groupSuccess) {
//        wprintf(L"\n查询成功完成！\n");
//        wprintf(L"总用时: %lu 毫秒\n", endTime - startTime);
//
//        if (wcscmp(queryType, L"groups") != 0 && g_userCount > 0) {
//            DisplayUsers(displayCount);
//        }
//
//        if (wcscmp(queryType, L"users") != 0 && g_groupCount > 0) {
//            DisplayGroups(displayCount);
//        }
//
//        wprintf(L"\n============ 统计信息 ============\n");
//        if (wcscmp(queryType, L"groups") != 0) {
//            wprintf(L"用户总数: %d\n", g_userCount);
//        }
//        if (wcscmp(queryType, L"users") != 0) {
//            wprintf(L"组总数: %d\n", g_groupCount);
//        }
//        wprintf(L"查询耗时: %lu 毫秒\n", endTime - startTime);
//
//        wprintf(L"\n========================================\n");
//        wprintf(L"测试完成！按任意键退出...\n");
//
//    }
//    else {
//        wprintf(L"\n查询失败！\n");
//        wprintf(L"可能的原因:\n");
//        wprintf(L"1. 网络连接问题\n");
//        wprintf(L"2. 域控制器不可达\n");
//        wprintf(L"3. 身份验证失败\n");
//        wprintf(L"4. 权限不足\n");
//        wprintf(L"5. 防火墙阻止端口389\n");
//        wprintf(L"6. DNS解析失败\n");
//
//        if (g_users) {
//            free(g_users);
//            g_users = NULL;
//        }
//        if (g_groups) {
//            free(g_groups);
//            g_groups = NULL;
//        }
//        return 1;
//    }
//
//    _getch();
//
//    if (g_users) {
//        free(g_users);
//        g_users = NULL;
//    }
//
//    if (g_groups) {
//        free(g_groups);
//        g_groups = NULL;
//    }
//
//    return 0;
//}


//#include <windows.h>
//#include <winldap.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <wchar.h>
//#include <conio.h>
//#include <locale.h>
//
//#pragma comment(lib, "wldap32.lib")
//
//// 用户信息结构 - 包含组成员信息
//typedef struct UserInfo {
//    WCHAR samAccountName[256];
//    WCHAR domainName[256];
//    BOOL isDisabled;         // 用户是否被禁用
//    WCHAR** memberOfGroups;  // 用户所属组的DN数组
//    int groupCount;          // 用户所属组的数量
//    int groupCapacity;       // 组数组容量
//} UserInfo;
//
//// 全局变量
//UserInfo* g_users = NULL;
//int g_userCount = 0;
//int g_userCapacity = 0;
//
//// 函数声明
//void AddUser(const WCHAR* sam, const WCHAR* domain, BOOL disabled, PWCHAR* memberOf, int memberCount);
//BOOL GetCurrentDomain(WCHAR* domainName, DWORD size);
//void BuildBaseDN(const WCHAR* domain, WCHAR* baseDN, DWORD size);
//BOOL QueryUsersViaLDAP(const WCHAR* domainName);
//void DisplayUsers(int maxCount);
//void DisplayUserGroupMapping(int maxCount);
//void ShowUsage(void);
//void FreeUserInfo(UserInfo* user);
//WCHAR* ExtractCNFromDN(const WCHAR* dn);
//
//// 从DN中提取CN
//WCHAR* ExtractCNFromDN(const WCHAR* dn) {
//    if (!dn) return NULL;
//
//    static WCHAR cn[256];
//    const WCHAR* start = wcsstr(dn, L"CN=");
//    if (!start) return NULL;
//
//    start += 3; // 跳过 "CN="
//    const WCHAR* end = wcschr(start, L',');
//
//    if (end) {
//        size_t len = end - start;
//        if (len >= 256) len = 255;
//        wcsncpy_s(cn, 256, start, len);
//        cn[len] = L'\0';
//    }
//    else {
//        wcscpy_s(cn, 256, start);
//    }
//
//    return cn;
//}
//
//// 释放用户信息内存
//void FreeUserInfo(UserInfo* user) {
//    if (user->memberOfGroups) {
//        for (int i = 0; i < user->groupCount; i++) {
//            if (user->memberOfGroups[i]) {
//                free(user->memberOfGroups[i]);
//            }
//        }
//        free(user->memberOfGroups);
//        user->memberOfGroups = NULL;
//    }
//    user->groupCount = 0;
//    user->groupCapacity = 0;
//}
//
//// 添加用户到列表 - 包含组成员信息
//void AddUser(const WCHAR* sam, const WCHAR* domain, BOOL disabled, PWCHAR* memberOf, int memberCount) {
//    if (g_userCount >= g_userCapacity) {
//        g_userCapacity = g_userCapacity == 0 ? 1000 : g_userCapacity * 2;
//        UserInfo* newUsers = (UserInfo*)realloc(g_users, g_userCapacity * sizeof(UserInfo));
//        if (!newUsers) {
//            wprintf(L"内存分配失败\n");
//            return;
//        }
//        g_users = newUsers;
//    }
//
//    UserInfo* user = &g_users[g_userCount];
//
//    // 初始化用户信息
//    if (sam) {
//        wcsncpy_s(user->samAccountName, 256, sam, _TRUNCATE);
//    }
//    else {
//        user->samAccountName[0] = L'\0';
//    }
//
//    if (domain) {
//        wcsncpy_s(user->domainName, 256, domain, _TRUNCATE);
//    }
//    else {
//        user->domainName[0] = L'\0';
//    }
//
//    user->isDisabled = disabled;
//
//    // 初始化组信息
//    user->memberOfGroups = NULL;
//    user->groupCount = 0;
//    user->groupCapacity = 0;
//
//    // 添加组成员信息
//    if (memberOf && memberCount > 0) {
//        user->groupCapacity = memberCount;
//        user->memberOfGroups = (WCHAR**)malloc(memberCount * sizeof(WCHAR*));
//        if (user->memberOfGroups) {
//            for (int i = 0; i < memberCount; i++) {
//                if (memberOf[i]) {
//                    size_t len = wcslen(memberOf[i]) + 1;
//                    user->memberOfGroups[i] = (WCHAR*)malloc(len * sizeof(WCHAR));
//                    if (user->memberOfGroups[i]) {
//                        wcscpy_s(user->memberOfGroups[i], len, memberOf[i]);
//                        user->groupCount++;
//                    }
//                }
//            }
//        }
//    }
//
//    g_userCount++;
//}
//
//// 获取当前域名
//BOOL GetCurrentDomain(WCHAR* domainName, DWORD size) {
//    DWORD actualSize = size;
//    if (GetComputerNameExW(ComputerNameDnsDomain, domainName, &actualSize)) {
//        return TRUE;
//    }
//
//    if (GetEnvironmentVariableW(L"USERDNSDOMAIN", domainName, size) > 0) {
//        return TRUE;
//    }
//
//    wcscpy_s(domainName, size, L"localhost");
//    return FALSE;
//}
//
//// 构建DN字符串
//void BuildBaseDN(const WCHAR* domain, WCHAR* baseDN, DWORD size) {
//    WCHAR tempDomain[256];
//    wcscpy_s(tempDomain, 256, domain);
//
//    wcscpy_s(baseDN, size, L"DC=");
//
//    WCHAR* context = NULL;
//    WCHAR* token = wcstok_s(tempDomain, L".", &context);
//    BOOL first = TRUE;
//
//    while (token != NULL) {
//        if (!first) {
//            wcscat_s(baseDN, size, L",DC=");
//        }
//        wcscat_s(baseDN, size, token);
//        first = FALSE;
//        token = wcstok_s(NULL, L".", &context);
//    }
//}
//
//// 执行LDAP用户查询 - 包含memberOf属性和userAccountControl
//BOOL QueryUsersViaLDAP(const WCHAR* domainName) {
//    LDAP* pLdap = NULL;
//    LDAPMessage* pResult = NULL;
//    LDAPMessage* pEntry = NULL;
//    ULONG result;
//    WCHAR baseDN[512];
//    WCHAR targetDomain[256];
//
//    static WCHAR attr_sam[] = L"sAMAccountName";
//    static WCHAR attr_uac[] = L"userAccountControl";
//    static WCHAR attr_memberof[] = L"memberOf";
//    PWCHAR attributes[4];
//    attributes[0] = attr_sam;
//    attributes[1] = attr_uac;
//    attributes[2] = attr_memberof;
//    attributes[3] = NULL;
//
//    if (domainName && wcslen(domainName) > 0) {
//        wcscpy_s(targetDomain, 256, domainName);
//    }
//    else {
//        if (!GetCurrentDomain(targetDomain, 256)) {
//            wprintf(L"警告: 无法获取当前域名，使用localhost\n");
//        }
//    }
//
//    wprintf(L"正在连接到域: %s\n", targetDomain);
//
//    pLdap = ldap_initW(targetDomain, LDAP_PORT);
//    if (!pLdap) {
//        wprintf(L"错误: LDAP初始化失败\n");
//        return FALSE;
//    }
//
//    ULONG version = LDAP_VERSION3;
//    result = ldap_set_optionW(pLdap, LDAP_OPT_PROTOCOL_VERSION, &version);
//
//    LDAP_TIMEVAL timeout = { 30, 0 };
//    ldap_set_optionW(pLdap, LDAP_OPT_TIMELIMIT, &timeout);
//
//    wprintf(L"正在进行身份验证...\n");
//
//    result = ldap_bind_sW(pLdap, NULL, NULL, LDAP_AUTH_NEGOTIATE);
//    if (result != LDAP_SUCCESS) {
//        PWCHAR errorMsg = ldap_err2stringW(result);
//        wprintf(L"错误: LDAP绑定失败 - %s (代码: %lu)\n", errorMsg, result);
//        ldap_unbind(pLdap);
//        return FALSE;
//    }
//
//    wprintf(L"身份验证成功！\n");
//
//    BuildBaseDN(targetDomain, baseDN, 512);
//    wprintf(L"搜索基础DN: %s\n", baseDN);
//
//    static WCHAR filter[] = L"(&(objectClass=user)(objectCategory=person))";
//
//    wprintf(L"开始搜索用户(包含禁用状态和组成员信息)...\n");
//
//    result = ldap_search_sW(pLdap, baseDN, LDAP_SCOPE_SUBTREE, filter, attributes, 0, &pResult);
//
//    if (result != LDAP_SUCCESS) {
//        PWCHAR errorMsg = ldap_err2stringW(result);
//        wprintf(L"错误: LDAP搜索失败 - %s (代码: %lu)\n", errorMsg, result);
//        ldap_unbind(pLdap);
//        return FALSE;
//    }
//
//    ULONG entryCount = ldap_count_entries(pLdap, pResult);
//    wprintf(L"找到 %lu 个用户条目\n", entryCount);
//
//    pEntry = ldap_first_entry(pLdap, pResult);
//    while (pEntry) {
//        PWCHAR samAccountName = NULL;
//        BOOL isDisabled = FALSE;
//
//        PWCHAR* values = ldap_get_valuesW(pLdap, pEntry, attr_sam);
//        if (values && values[0]) {
//            samAccountName = values[0];
//        }
//
//        // 获取userAccountControl属性来判断用户是否被禁用
//        PWCHAR* uacValues = ldap_get_valuesW(pLdap, pEntry, attr_uac);
//        if (uacValues && uacValues[0]) {
//            DWORD userAccountControl = (DWORD)_wtol(uacValues[0]);
//            // ADS_UF_ACCOUNTDISABLE = 0x2
//            isDisabled = (userAccountControl & 0x2) != 0;
//        }
//
//        // 获取memberOf属性
//        PWCHAR* memberOfValues = ldap_get_valuesW(pLdap, pEntry, attr_memberof);
//        int memberOfCount = 0;
//        if (memberOfValues) {
//            memberOfCount = ldap_count_valuesW(memberOfValues);
//        }
//
//        if (samAccountName) {
//            AddUser(samAccountName, targetDomain, isDisabled, memberOfValues, memberOfCount);
//        }
//
//        if (values) ldap_value_freeW(values);
//        if (uacValues) ldap_value_freeW(uacValues);
//        if (memberOfValues) ldap_value_freeW(memberOfValues);
//
//        pEntry = ldap_next_entry(pLdap, pEntry);
//    }
//
//    if (pResult) ldap_msgfree(pResult);
//    ldap_unbind(pLdap);
//
//    wprintf(L"LDAP用户查询完成！总共获取到 %d 个用户\n", g_userCount);
//    return TRUE;
//}
//
//// 显示用户列表 - 包含用户名、域名、禁用状态和组信息
//void DisplayUsers(int maxCount) {
//    wprintf(L"\n============ 用户列表 ============\n");
//    wprintf(L"总用户数: %d\n", g_userCount);
//
//    int displayCount = (maxCount > 0 && maxCount < g_userCount) ? maxCount : g_userCount;
//    wprintf(L"显示前 %d 个用户:\n", displayCount);
//    wprintf(L"--------------------------------------------\n");
//
//    for (int i = 0; i < displayCount; i++) {
//        wprintf(L"[%d] 用户名: %s", i + 1, g_users[i].samAccountName);
//        wprintf(L", 域名: %s", g_users[i].domainName);
//        wprintf(L", 状态: %s", g_users[i].isDisabled ? L"已禁用" : L"启用");
//        wprintf(L"\n");
//
//        // 显示用户所属的组
//        if (g_users[i].groupCount > 0) {
//            wprintf(L"    所属组 (%d个):\n", g_users[i].groupCount);
//            for (int j = 0; j < g_users[i].groupCount; j++) {
//                WCHAR* groupCN = ExtractCNFromDN(g_users[i].memberOfGroups[j]);
//                if (groupCN) {
//                    wprintf(L"      - %s\n", groupCN);
//                }
//                else {
//                    wprintf(L"      - %s\n", g_users[i].memberOfGroups[j]);
//                }
//            }
//        }
//        else {
//            wprintf(L"    所属组: (无)\n");
//        }
//        wprintf(L"\n");
//    }
//
//    if (g_userCount > displayCount) {
//        wprintf(L"... 还有 %d 个用户未显示\n", g_userCount - displayCount);
//    }
//
//    // 显示统计信息
//    int enabledCount = 0;
//    int disabledCount = 0;
//    for (int i = 0; i < g_userCount; i++) {
//        if (g_users[i].isDisabled) {
//            disabledCount++;
//        }
//        else {
//            enabledCount++;
//        }
//    }
//
//    wprintf(L"\n========== 用户状态统计 ==========\n");
//    wprintf(L"启用用户: %d\n", enabledCount);
//    wprintf(L"禁用用户: %d\n", disabledCount);
//    wprintf(L"总计用户: %d\n", g_userCount);
//}
//
//// 显示用户-组关系映射
//void DisplayUserGroupMapping(int maxCount) {
//    wprintf(L"\n============ 用户-组关系映射 ============\n");
//
//    int displayCount = (maxCount > 0 && maxCount < g_userCount) ? maxCount : g_userCount;
//    wprintf(L"显示前 %d 个用户的组关系:\n", displayCount);
//    wprintf(L"----------------------------------------\n");
//
//    for (int i = 0; i < displayCount; i++) {
//        wprintf(L"%s@%s", g_users[i].samAccountName, g_users[i].domainName);
//        wprintf(L" [%s]", g_users[i].isDisabled ? L"禁用" : L"启用");
//        wprintf(L":\n");
//
//        if (g_users[i].groupCount > 0) {
//            for (int j = 0; j < g_users[i].groupCount; j++) {
//                WCHAR* groupCN = ExtractCNFromDN(g_users[i].memberOfGroups[j]);
//                if (groupCN) {
//                    wprintf(L"  ├─ %s\n", groupCN);
//                }
//            }
//        }
//        else {
//            wprintf(L"  └─ (无组成员关系)\n");
//        }
//        wprintf(L"\n");
//    }
//}
//
//// 显示帮助信息
//void ShowUsage(void) {
//    wprintf(L"LDAP域用户查询程序\n");
//    wprintf(L"用法:\n");
//    wprintf(L"  LDAPUserQuery.exe [域名] [显示数量] [显示类型]\n");
//    wprintf(L"\n");
//    wprintf(L"参数:\n");
//    wprintf(L"  域名       - 要查询的域名 (可选，默认为当前域)\n");
//    wprintf(L"  显示数量   - 显示的用户数量 (可选，默认50)\n");
//    wprintf(L"  显示类型   - users|mapping (可选，默认users)\n");
//    wprintf(L"\n");
//    wprintf(L"显示类型说明:\n");
//    wprintf(L"  users    - 显示用户详细信息(包含所属组)\n");
//    wprintf(L"  mapping  - 显示用户-组关系映射\n");
//    wprintf(L"\n");
//    wprintf(L"示例:\n");
//    wprintf(L"  LDAPUserQuery.exe                    # 查询当前域的前50个用户\n");
//    wprintf(L"  LDAPUserQuery.exe example.com        # 查询指定域的前50个用户\n");
//    wprintf(L"  LDAPUserQuery.exe example.com 100    # 查询指定域的前100个用户\n");
//    wprintf(L"  LDAPUserQuery.exe localhost 20 mapping  # 显示前20个用户的组映射\n");
//}
//
//// 主函数
//int wmain(int argc, wchar_t* argv[]) {
//    setlocale(LC_ALL, "chs");
//    SetConsoleOutputCP(CP_UTF8);
//
//    wprintf(L"========================================\n");
//    wprintf(L"      LDAP域用户查询程序\n");
//    wprintf(L"========================================\n");
//
//    WCHAR domainName[256] = { 0 };
//    int displayCount = 50;
//    WCHAR displayType[32] = L"users";
//
//    // 解析命令行参数
//    if (argc > 1) {
//        if (wcscmp(argv[1], L"/?") == 0 || wcscmp(argv[1], L"-h") == 0 || wcscmp(argv[1], L"--help") == 0) {
//            ShowUsage();
//            return 0;
//        }
//        wcscpy_s(domainName, 256, argv[1]);
//    }
//
//    if (argc > 2) {
//        displayCount = _wtoi(argv[2]);
//        if (displayCount <= 0) displayCount = 50;
//    }
//
//    if (argc > 3) {
//        wcscpy_s(displayType, 32, argv[3]);
//    }
//
//    // 获取域名
//    if (wcslen(domainName) == 0) {
//        GetCurrentDomain(domainName, 256);
//    }
//
//    // 显示配置信息
//    wprintf(L"配置信息:\n");
//    wprintf(L"- 目标域: %s\n", domainName);
//    wprintf(L"- 显示数量: %d\n", displayCount);
//    wprintf(L"- 显示类型: %s\n", displayType);
//    wprintf(L"- 使用端口: 389 (LDAP)\n");
//    wprintf(L"========================================\n");
//
//    // 显示当前用户
//    WCHAR userName[256];
//    DWORD userNameSize = 256;
//    if (GetUserNameW(userName, &userNameSize)) {
//        wprintf(L"当前用户: %s\n", userName);
//    }
//
//    wprintf(L"开始LDAP用户查询...\n");
//
//    DWORD startTime = GetTickCount();
//    BOOL success = QueryUsersViaLDAP(domainName);
//    DWORD endTime = GetTickCount();
//
//    if (success) {
//        wprintf(L"\n查询成功完成！\n");
//        wprintf(L"总用时: %lu 毫秒\n", endTime - startTime);
//
//        // 根据显示类型显示结果
//        if (wcscmp(displayType, L"mapping") == 0) {
//            DisplayUserGroupMapping(displayCount);
//        }
//        else {
//            DisplayUsers(displayCount);
//        }
//
//        wprintf(L"\n============ 统计信息 ============\n");
//        wprintf(L"用户总数: %d\n", g_userCount);
//        wprintf(L"查询耗时: %lu 毫秒\n", endTime - startTime);
//
//        wprintf(L"\n========================================\n");
//        wprintf(L"查询完成！按任意键退出...\n");
//    }
//    else {
//        wprintf(L"\n查询失败！\n");
//        wprintf(L"可能的原因:\n");
//        wprintf(L"1. 网络连接问题\n");
//        wprintf(L"2. 域控制器不可达\n");
//        wprintf(L"3. 身份验证失败\n");
//        wprintf(L"4. 权限不足\n");
//        wprintf(L"5. 防火墙阻止端口389\n");
//        wprintf(L"6. DNS解析失败\n");
//
//        // 清理内存
//        if (g_users) {
//            for (int i = 0; i < g_userCount; i++) {
//                FreeUserInfo(&g_users[i]);
//            }
//            free(g_users);
//            g_users = NULL;
//        }
//        return 1;
//    }
//
//    _getch();
//
//    // 清理内存
//    if (g_users) {
//        for (int i = 0; i < g_userCount; i++) {
//            FreeUserInfo(&g_users[i]);
//        }
//        free(g_users);
//        g_users = NULL;
//    }
//
//    return 0;
//}

//#include <windows.h>
//#include <lm.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <wchar.h>
//#include <conio.h>
//#include <locale.h>
//#include <time.h>
//#include <process.h>
//
//#pragma comment(lib, "netapi32.lib")
//
//// 线程统计信息结构
//typedef struct ThreadStats {
//    int threadId;
//    int userCount;
//    int groupQueryCount;
//    DWORD startTime;
//    DWORD endTime;
//    DWORD totalTime;
//    BOOL success;
//    NET_API_STATUS lastError;
//    int retryCount;
//} ThreadStats;
//
//// 测试配置结构
//typedef struct LoadTestConfig {
//    WCHAR serverName[256];
//    int threadCount;
//    int iterationsPerThread;
//    int delayBetweenIterations;  // 毫秒
//    BOOL enableGroupQuery;
//    BOOL enableRetry;
//    int maxRetries;
//    BOOL verboseOutput;
//} LoadTestConfig;
//
//// 全局变量
//LoadTestConfig g_config = { 0 };
//ThreadStats* g_threadStats = NULL;
//HANDLE* g_threadHandles = NULL;
//CRITICAL_SECTION g_printLock;
//volatile LONG g_totalRequests = 0;
//volatile LONG g_successfulRequests = 0;
//volatile LONG g_failedRequests = 0;
//
//// 函数声明
//unsigned int __stdcall WorkerThread(void* param);
//BOOL QueryUsersWithRetry(const WCHAR* serverName, ThreadStats* stats);
//BOOL GetUserGroupsCount(const WCHAR* serverName, const WCHAR* userName, int* groupCount);
//void PrintThreadMessage(int threadId, const WCHAR* message, ...);
//void DisplayTestResults(void);
//void DisplayRealTimeStats(void);
//void ShowLoadTestUsage(void);
//BOOL ParseCommandLine(int argc, wchar_t* argv[]);
//
//// 线程安全的打印函数
//void PrintThreadMessage(int threadId, const WCHAR* message, ...) {
//    if (!g_config.verboseOutput) return;
//
//    EnterCriticalSection(&g_printLock);
//
//    SYSTEMTIME st;
//    GetLocalTime(&st);
//
//    wprintf(L"[%02d:%02d:%02d.%03d][线程%02d] ",
//        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, threadId);
//
//    va_list args;
//    va_start(args, message);
//    vwprintf(message, args);
//    va_end(args);
//
//    wprintf(L"\n");
//
//    LeaveCriticalSection(&g_printLock);
//}
//
//// 获取用户组数量（简化版本，减少查询开销）
//BOOL GetUserGroupsCount(const WCHAR* serverName, const WCHAR* userName, int* groupCount) {
//    PGROUP_USERS_INFO_0 groupInfo = NULL;
//    DWORD entriesRead = 0;
//    DWORD totalEntries = 0;
//    NET_API_STATUS status;
//
//    *groupCount = 0;
//
//    status = NetUserGetGroups(serverName, userName, 0, (LPBYTE*)&groupInfo,
//        MAX_PREFERRED_LENGTH, &entriesRead, &totalEntries);
//
//    if (status == NERR_Success) {
//        *groupCount = (int)entriesRead;
//        if (groupInfo) {
//            NetApiBufferFree(groupInfo);
//        }
//        return TRUE;
//    }
//
//    return FALSE;
//}
//
//// 带重试机制的用户查询
//BOOL QueryUsersWithRetry(const WCHAR* serverName, ThreadStats* stats) {
//    PUSER_INFO_1 userInfo = NULL;
//    DWORD entriesRead = 0;
//    DWORD totalEntries = 0;
//    DWORD resumeHandle = 0;
//    NET_API_STATUS status;
//    int attempt = 0;
//
//    stats->userCount = 0;
//    stats->groupQueryCount = 0;
//    stats->retryCount = 0;
//
//    do {
//        attempt++;
//        InterlockedIncrement(&g_totalRequests);
//
//        status = NetUserEnum(
//            wcslen(serverName) > 0 ? serverName : NULL,
//            1,  // 使用USER_INFO_1减少数据传输
//            FILTER_NORMAL_ACCOUNT,
//            (LPBYTE*)&userInfo,
//            MAX_PREFERRED_LENGTH,
//            &entriesRead,
//            &totalEntries,
//            &resumeHandle
//        );
//
//        if (status == NERR_Success || status == ERROR_MORE_DATA) {
//            stats->userCount += (int)entriesRead;
//
//            // 如果启用组查询，为部分用户查询组信息
//            if (g_config.enableGroupQuery && entriesRead > 0) {
//                int groupQueryLimit = min(entriesRead, 10); // 限制组查询数量以减少负载
//                for (DWORD i = 0; i < groupQueryLimit; i++) {
//                    int groupCount = 0;
//                    if (GetUserGroupsCount(wcslen(serverName) > 0 ? serverName : NULL,
//                        userInfo[i].usri1_name, &groupCount)) {
//                        stats->groupQueryCount++;
//                    }
//                }
//            }
//
//            PrintThreadMessage(stats->threadId, L"查询成功: %lu 用户, 总计: %d",
//                entriesRead, stats->userCount);
//
//            if (userInfo) {
//                NetApiBufferFree(userInfo);
//                userInfo = NULL;
//            }
//
//            if (status == NERR_Success) {
//                InterlockedIncrement(&g_successfulRequests);
//                return TRUE;
//            }
//        }
//        else {
//            stats->lastError = status;
//            PrintThreadMessage(stats->threadId, L"查询失败 (尝试 %d): 错误 %lu", attempt, status);
//
//            if (g_config.enableRetry && attempt < g_config.maxRetries) {
//                stats->retryCount++;
//                Sleep(100 * attempt); // 指数退避
//                PrintThreadMessage(stats->threadId, L"等待 %d 毫秒后重试...", 100 * attempt);
//            }
//            else {
//                InterlockedIncrement(&g_failedRequests);
//                return FALSE;
//            }
//        }
//
//        if (userInfo) {
//            NetApiBufferFree(userInfo);
//            userInfo = NULL;
//        }
//
//    } while (status == ERROR_MORE_DATA);
//
//    InterlockedIncrement(&g_failedRequests);
//    return FALSE;
//}
//
//// 工作线程函数
//unsigned int __stdcall WorkerThread(void* param) {
//    ThreadStats* stats = (ThreadStats*)param;
//
//    PrintThreadMessage(stats->threadId, L"开始执行，计划 %d 次迭代", g_config.iterationsPerThread);
//
//    stats->startTime = GetTickCount();
//
//    for (int i = 0; i < g_config.iterationsPerThread; i++) {
//        PrintThreadMessage(stats->threadId, L"开始第 %d/%d 次查询", i + 1, g_config.iterationsPerThread);
//
//        DWORD queryStart = GetTickCount();
//
//        if (QueryUsersWithRetry(g_config.serverName, stats)) {
//            DWORD queryTime = GetTickCount() - queryStart;
//            PrintThreadMessage(stats->threadId, L"第 %d 次查询完成，耗时: %lu 毫秒，用户数: %d",
//                i + 1, queryTime, stats->userCount);
//            stats->success = TRUE;
//        }
//        else {
//            PrintThreadMessage(stats->threadId, L"第 %d 次查询失败，错误: %lu", i + 1, stats->lastError);
//        }
//
//        // 迭代间延迟
//        if (i < g_config.iterationsPerThread - 1 && g_config.delayBetweenIterations > 0) {
//            PrintThreadMessage(stats->threadId, L"等待 %d 毫秒...", g_config.delayBetweenIterations);
//            Sleep(g_config.delayBetweenIterations);
//        }
//    }
//
//    stats->endTime = GetTickCount();
//    stats->totalTime = stats->endTime - stats->startTime;
//
//    PrintThreadMessage(stats->threadId, L"完成所有查询，总耗时: %lu 毫秒", stats->totalTime);
//
//    return 0;
//}
//
//// 显示实时统计
//void DisplayRealTimeStats(void) {
//    wprintf(L"\n========== 实时统计 ==========\n");
//    wprintf(L"总请求数: %ld\n", g_totalRequests);
//    wprintf(L"成功请求: %ld\n", g_successfulRequests);
//    wprintf(L"失败请求: %ld\n", g_failedRequests);
//    if (g_totalRequests > 0) {
//        wprintf(L"成功率: %.2f%%\n", (double)g_successfulRequests / g_totalRequests * 100);
//    }
//    wprintf(L"=============================\n");
//}
//
//// 显示测试结果
//void DisplayTestResults(void) {
//    wprintf(L"\n============ 负载测试结果 ============\n");
//
//    DWORD totalTime = 0;
//    int totalUsers = 0;
//    int totalGroupQueries = 0;
//    int successfulThreads = 0;
//    int totalRetries = 0;
//
//    for (int i = 0; i < g_config.threadCount; i++) {
//        ThreadStats* stats = &g_threadStats[i];
//
//        wprintf(L"线程 %02d: ", stats->threadId);
//        if (stats->success) {
//            wprintf(L"成功");
//            successfulThreads++;
//        }
//        else {
//            wprintf(L"失败 (错误: %lu)", stats->lastError);
//        }
//
//        wprintf(L", 耗时: %5lu ms, 用户: %4d, 组查询: %3d, 重试: %d\n",
//            stats->totalTime, stats->userCount, stats->groupQueryCount, stats->retryCount);
//
//        totalTime += stats->totalTime;
//        totalUsers += stats->userCount;
//        totalGroupQueries += stats->groupQueryCount;
//        totalRetries += stats->retryCount;
//    }
//
//    wprintf(L"\n========== 汇总统计 ==========\n");
//    wprintf(L"测试配置:\n");
//    wprintf(L"  - 目标服务器: %s\n", wcslen(g_config.serverName) > 0 ? g_config.serverName : L"本地域控制器");
//    wprintf(L"  - 并发线程数: %d\n", g_config.threadCount);
//    wprintf(L"  - 每线程迭代数: %d\n", g_config.iterationsPerThread);
//    wprintf(L"  - 迭代间延迟: %d ms\n", g_config.delayBetweenIterations);
//    wprintf(L"  - 启用组查询: %s\n", g_config.enableGroupQuery ? L"是" : L"否");
//    wprintf(L"  - 启用重试: %s\n", g_config.enableRetry ? L"是" : L"否");
//
//    wprintf(L"\n结果统计:\n");
//    wprintf(L"  - 成功线程数: %d/%d (%.1f%%)\n", successfulThreads, g_config.threadCount,
//        (double)successfulThreads / g_config.threadCount * 100);
//    wprintf(L"  - 总请求数: %ld\n", g_totalRequests);
//    wprintf(L"  - 成功请求数: %ld\n", g_successfulRequests);
//    wprintf(L"  - 失败请求数: %ld\n", g_failedRequests);
//    wprintf(L"  - 总体成功率: %.2f%%\n", (double)g_successfulRequests / g_totalRequests * 100);
//    wprintf(L"  - 平均耗时: %.1f ms\n", (double)totalTime / g_config.threadCount);
//    wprintf(L"  - 总查询用户数: %d\n", totalUsers);
//    wprintf(L"  - 总组查询数: %d\n", totalGroupQueries);
//    wprintf(L"  - 总重试次数: %d\n", totalRetries);
//
//    // 计算吞吐量
//    if (totalTime > 0) {
//        double avgTime = (double)totalTime / g_config.threadCount / 1000.0; // 转换为秒
//        double requestsPerSecond = g_totalRequests / avgTime;
//        wprintf(L"  - 平均请求速率: %.2f 请求/秒\n", requestsPerSecond);
//    }
//
//    wprintf(L"=====================================\n");
//}
//
//// 解析命令行参数
//BOOL ParseCommandLine(int argc, wchar_t* argv[]) {
//    // 设置默认值
//    wcscpy_s(g_config.serverName, 256, L"");
//    g_config.threadCount = 5;
//    g_config.iterationsPerThread = 3;
//    g_config.delayBetweenIterations = 1000;
//    g_config.enableGroupQuery = FALSE;
//    g_config.enableRetry = TRUE;
//    g_config.maxRetries = 3;
//    g_config.verboseOutput = FALSE;
//
//    for (int i = 1; i < argc; i++) {
//        if (wcscmp(argv[i], L"-s") == 0 || wcscmp(argv[i], L"--server") == 0) {
//            if (i + 1 < argc) {
//                wcscpy_s(g_config.serverName, 256, argv[++i]);
//            }
//        }
//        else if (wcscmp(argv[i], L"-t") == 0 || wcscmp(argv[i], L"--threads") == 0) {
//            if (i + 1 < argc) {
//                g_config.threadCount = _wtoi(argv[++i]);
//            }
//        }
//        else if (wcscmp(argv[i], L"-i") == 0 || wcscmp(argv[i], L"--iterations") == 0) {
//            if (i + 1 < argc) {
//                g_config.iterationsPerThread = _wtoi(argv[++i]);
//            }
//        }
//        else if (wcscmp(argv[i], L"-d") == 0 || wcscmp(argv[i], L"--delay") == 0) {
//            if (i + 1 < argc) {
//                g_config.delayBetweenIterations = _wtoi(argv[++i]);
//            }
//        }
//        else if (wcscmp(argv[i], L"-g") == 0 || wcscmp(argv[i], L"--groups") == 0) {
//            g_config.enableGroupQuery = TRUE;
//        }
//        else if (wcscmp(argv[i], L"--no-retry") == 0) {
//            g_config.enableRetry = FALSE;
//        }
//        else if (wcscmp(argv[i], L"-r") == 0 || wcscmp(argv[i], L"--retries") == 0) {
//            if (i + 1 < argc) {
//                g_config.maxRetries = _wtoi(argv[++i]);
//            }
//        }
//        else if (wcscmp(argv[i], L"-v") == 0 || wcscmp(argv[i], L"--verbose") == 0) {
//            g_config.verboseOutput = TRUE;
//        }
//        else if (wcscmp(argv[i], L"/?") == 0 || wcscmp(argv[i], L"-h") == 0 || wcscmp(argv[i], L"--help") == 0) {
//            ShowLoadTestUsage();
//            return FALSE;
//        }
//    }
//
//    // 验证参数
//    if (g_config.threadCount <= 0 || g_config.threadCount > 100) {
//        wprintf(L"错误: 线程数必须在1-100之间\n");
//        return FALSE;
//    }
//
//    if (g_config.iterationsPerThread <= 0 || g_config.iterationsPerThread > 50) {
//        wprintf(L"错误: 迭代数必须在1-50之间\n");
//        return FALSE;
//    }
//
//    return TRUE;
//}
//
//// 显示帮助信息
//void ShowLoadTestUsage(void) {
//    wprintf(L"域服务器负载测试程序\n");
//    wprintf(L"用法:\n");
//    wprintf(L"  LoadTest.exe [选项]\n");
//    wprintf(L"\n");
//    wprintf(L"选项:\n");
//    wprintf(L"  -s, --server <名称>     目标服务器名称 (默认: 当前域控制器)\n");
//    wprintf(L"  -t, --threads <数量>    并发线程数 (默认: 5, 范围: 1-100)\n");
//    wprintf(L"  -i, --iterations <数量> 每线程迭代数 (默认: 3, 范围: 1-50)\n");
//    wprintf(L"  -d, --delay <毫秒>      迭代间延迟 (默认: 1000ms)\n");
//    wprintf(L"  -g, --groups            启用组查询 (增加负载)\n");
//    wprintf(L"  --no-retry              禁用重试机制\n");
//    wprintf(L"  -r, --retries <数量>    最大重试次数 (默认: 3)\n");
//    wprintf(L"  -v, --verbose           详细输出\n");
//    wprintf(L"  -h, --help              显示帮助信息\n");
//    wprintf(L"\n");
//    wprintf(L"示例:\n");
//    wprintf(L"  LoadTest.exe                           # 默认测试 (5线程, 3迭代)\n");
//    wprintf(L"  LoadTest.exe -t 10 -i 5                # 10线程, 5迭代\n");
//    wprintf(L"  LoadTest.exe -s dc01.example.com -g    # 指定服务器, 启用组查询\n");
//    wprintf(L"  LoadTest.exe -t 20 -d 500 -v           # 20线程, 500ms延迟, 详细输出\n");
//    wprintf(L"\n");
//    wprintf(L"注意事项:\n");
//    wprintf(L"- 需要管理员权限\n");
//    wprintf(L"- 高并发测试可能对域服务器造成负载\n");
//    wprintf(L"- 建议先进行小规模测试\n");
//}
//
//// 主函数
//int wmain(int argc, wchar_t* argv[]) {
//    setlocale(LC_ALL, "chs");
//    SetConsoleOutputCP(CP_UTF8);
//
//    wprintf(L"========================================\n");
//    wprintf(L"      域服务器负载测试程序\n");
//    wprintf(L"========================================\n");
//
//    if (!ParseCommandLine(argc, argv)) {
//        return 0;
//    }
//
//    // 初始化临界区
//    InitializeCriticalSection(&g_printLock);
//
//    // 分配线程统计结构
//    g_threadStats = (ThreadStats*)calloc(g_config.threadCount, sizeof(ThreadStats));
//    g_threadHandles = (HANDLE*)calloc(g_config.threadCount, sizeof(HANDLE));
//
//    if (!g_threadStats || !g_threadHandles) {
//        wprintf(L"内存分配失败\n");
//        return 1;
//    }
//
//    // 显示测试配置
//    wprintf(L"测试配置:\n");
//    wprintf(L"- 目标服务器: %s\n", wcslen(g_config.serverName) > 0 ? g_config.serverName : L"当前域控制器");
//    wprintf(L"- 并发线程数: %d\n", g_config.threadCount);
//    wprintf(L"- 每线程迭代数: %d\n", g_config.iterationsPerThread);
//    wprintf(L"- 迭代间延迟: %d ms\n", g_config.delayBetweenIterations);
//    wprintf(L"- 启用组查询: %s\n", g_config.enableGroupQuery ? L"是" : L"否");
//    wprintf(L"- 启用重试: %s (最大重试: %d)\n", g_config.enableRetry ? L"是" : L"否", g_config.maxRetries);
//    wprintf(L"- 详细输出: %s\n", g_config.verboseOutput ? L"是" : L"否");
//    wprintf(L"========================================\n");
//
//
//    DWORD testEndTime = GetTickCount();
//    DWORD testStartTime = GetTickCount();
//    int validHandleCount = 0;
//    HANDLE* validHandles = NULL;
//
//    wprintf(L"警告: 此测试可能对域服务器造成负载，确认继续? (Y/N): ");
//    int ch = _getch();
//    wprintf(L"%c\n", ch);
//    if (ch != 'Y' && ch != 'y') {
//        wprintf(L"测试已取消\n");
//        goto cleanup;
//    }
//
//    wprintf(L"\n开始负载测试...\n");
//
//    
//
//    // 创建并启动工作线程
//    for (int i = 0; i < g_config.threadCount; i++) {
//        g_threadStats[i].threadId = i + 1;
//        g_threadStats[i].success = FALSE;
//
//        g_threadHandles[i] = (HANDLE)_beginthreadex(
//            NULL, 0, WorkerThread, &g_threadStats[i], 0, NULL);
//
//        if (!g_threadHandles[i]) {
//            wprintf(L"创建线程 %d 失败\n", i + 1);
//            g_config.threadCount = i; // 调整实际线程数
//            break;
//        }
//
//        PrintThreadMessage(0, L"启动线程 %d", i + 1);
//        Sleep(100); // 避免同时启动造成的冲击
//    }
//
//    // 等待所有线程完成
//    wprintf(L"\n等待所有线程完成...\n");
//
//    // 显示进度
//    validHandles = (HANDLE*)malloc(g_config.threadCount * sizeof(HANDLE));
//    
//
//    for (int i = 0; i < g_config.threadCount; i++) {
//        if (g_threadHandles[i]) {
//            validHandles[validHandleCount++] = g_threadHandles[i];
//        }
//    }
//
//    if (validHandleCount > 0) {
//        while (validHandleCount > 0) {
//            DWORD waitResult = WaitForMultipleObjects(validHandleCount, validHandles, FALSE, 5000);
//
//            if (waitResult == WAIT_TIMEOUT) {
//                DisplayRealTimeStats();
//            }
//            else if (waitResult >= WAIT_OBJECT_0 && waitResult < WAIT_OBJECT_0 + validHandleCount) {
//                // 有线程完成，从等待列表中移除
//                int completedIndex = waitResult - WAIT_OBJECT_0;
//                CloseHandle(validHandles[completedIndex]);
//
//                // 移动数组元素
//                for (int j = completedIndex; j < validHandleCount - 1; j++) {
//                    validHandles[j] = validHandles[j + 1];
//                }
//                validHandleCount--;
//
//                wprintf(L"线程完成，剩余: %d\n", validHandleCount);
//            }
//        }
//    }
//
//    free(validHandles);
//
//    
//    wprintf(L"\n所有线程已完成，总测试时间: %lu 毫秒\n", testEndTime - testStartTime);
//
//    // 显示测试结果
//    DisplayTestResults();
//
//cleanup:
//    // 清理资源
//    if (g_threadStats) {
//        free(g_threadStats);
//    }
//    if (g_threadHandles) {
//        free(g_threadHandles);
//    }
//
//    DeleteCriticalSection(&g_printLock);
//
//    wprintf(L"\n按任意键退出...\n");
//    _getch();
//
//    return 0;
//}

#include <windows.h>
#include <lm.h>
#include <winldap.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <conio.h>
#include <locale.h>
#include <time.h>
#include <process.h>

#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "wldap32.lib")

// 连接测试模式
typedef enum {
    TEST_NETAPI_RAPID,      // NetAPI快速连接
    TEST_NETAPI_PERSISTENT, // NetAPI持久连接
    TEST_LDAP_RAPID,        // LDAP快速连接
    TEST_LDAP_PERSISTENT,   // LDAP持久连接
    TEST_MIXED_MODE         // 混合模式
} TestMode;

// 线程统计信息
typedef struct ThreadStats {
    int threadId;
    TestMode mode;
    int queryCount;
    int connectionCount;
    DWORD startTime;
    DWORD endTime;
    BOOL keepRunning;
    HANDLE* connections;    // 保存连接句柄
    int maxConnections;
} ThreadStats;

// 全局配置
typedef struct ConnectionTestConfig {
    WCHAR serverName[256];
    TestMode testMode;
    int threadCount;
    int connectionsPerThread;
    int queryInterval;      // 查询间隔(毫秒)
    BOOL holdConnections;   // 是否保持连接不关闭
    BOOL enableNetstat;     // 是否显示网络连接状态
    int testDuration;       // 测试持续时间(秒)
    BOOL verboseOutput;
} ConnectionTestConfig;

// 全局变量
ConnectionTestConfig g_config = { 0 };
ThreadStats* g_threadStats = NULL;
HANDLE* g_threadHandles = NULL;
CRITICAL_SECTION g_printLock;
volatile BOOL g_stopTest = FALSE;

// 函数声明
unsigned int __stdcall ConnectionWorkerThread(void* param);
BOOL TestNetAPIConnections(ThreadStats* stats);
BOOL TestLDAPConnections(ThreadStats* stats);
void DisplayNetworkConnections(void);
void MonitorConnections(void);
void ShowConnectionUsage(void);
BOOL ParseConnectionTestArgs(int argc, wchar_t* argv[]);
void PrintWithTimestamp(const WCHAR* message, ...);

// 带时间戳的打印
void PrintWithTimestamp(const WCHAR* message, ...) {
    EnterCriticalSection(&g_printLock);

    SYSTEMTIME st;
    GetLocalTime(&st);
    wprintf(L"[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond);

    va_list args;
    va_start(args, message);
    vwprintf(message, args);
    va_end(args);
    wprintf(L"\n");

    LeaveCriticalSection(&g_printLock);
}

// 显示当前网络连接状态
void DisplayNetworkConnections(void) {
    PrintWithTimestamp(L"=== 当前网络连接状态 ===");

    // 执行netstat命令显示连接到lsass的连接
    WCHAR command[512];
    swprintf_s(command, 512, L"netstat -ano | findstr :389");

    FILE* pipe = _wpopen(command, L"r");
    if (pipe) {
        wchar_t buffer[256];
        int connectionCount = 0;
        while (fgetws(buffer, 256, pipe)) {
            if (wcsstr(buffer, L"ESTABLISHED")) {
                connectionCount++;
                if (g_config.verboseOutput) {
                    wprintf(L"  %s", buffer);
                }
            }
        }
        _pclose(pipe);
        PrintWithTimestamp(L"LDAP连接数 (端口389): %d", connectionCount);
    }

    // 检查RPC连接
    swprintf_s(command, 512, L"netstat -ano | findstr :135");
    pipe = _wpopen(command, L"r");
    if (pipe) {
        wchar_t buffer[256];
        int rpcConnectionCount = 0;
        while (fgetws(buffer, 256, pipe)) {
            if (wcsstr(buffer, L"ESTABLISHED")) {
                rpcConnectionCount++;
            }
        }
        _pclose(pipe);
        PrintWithTimestamp(L"RPC连接数 (端口135): %d", rpcConnectionCount);
    }

    // 检查高端口连接
    swprintf_s(command, 512, L"netstat -ano | findstr ESTABLISHED | findstr 49");
    pipe = _wpopen(command, L"r");
    if (pipe) {
        wchar_t buffer[256];
        int highPortCount = 0;
        while (fgetws(buffer, 256, pipe)) {
            highPortCount++;
            if (g_config.verboseOutput) {
                wprintf(L"  %s", buffer);
            }
        }
        _pclose(pipe);
        PrintWithTimestamp(L"高端口连接数 (49xxx): %d", highPortCount);
    }
}

// NetAPI连接测试
BOOL TestNetAPIConnections(ThreadStats* stats) {
    PUSER_INFO_1 userInfo = NULL;
    DWORD entriesRead = 0;
    DWORD totalEntries = 0;
    DWORD resumeHandle = 0;
    NET_API_STATUS status;

    while (stats->keepRunning && stats->connectionCount < g_config.connectionsPerThread) {
        status = NetUserEnum(
            wcslen(g_config.serverName) > 0 ? g_config.serverName : NULL,
            1,
            FILTER_NORMAL_ACCOUNT,
            (LPBYTE*)&userInfo,
            MAX_PREFERRED_LENGTH,
            &entriesRead,
            &totalEntries,
            &resumeHandle
        );

        if (status == NERR_Success || status == ERROR_MORE_DATA) {
            stats->queryCount++;
            stats->connectionCount++;

            if (g_config.verboseOutput) {
                PrintWithTimestamp(L"线程%d: NetAPI查询成功, 用户数: %lu",
                    stats->threadId, entriesRead);
            }

            // 如果不保持连接，正常释放
            if (!g_config.holdConnections && userInfo) {
                NetApiBufferFree(userInfo);
                userInfo = NULL;
            }
            // 如果保持连接，故意不释放资源（模拟连接泄漏）
        }
        else {
            PrintWithTimestamp(L"线程%d: NetAPI查询失败, 错误: %lu", stats->threadId, status);
            break;
        }

        if (g_config.queryInterval > 0) {
            Sleep(g_config.queryInterval);
        }

        if (status == NERR_Success) break; // 完整查询完成
    }

    return TRUE;
}

// LDAP连接测试
BOOL TestLDAPConnections(ThreadStats* stats) {
    LDAP** ldapConnections = NULL;

    if (g_config.holdConnections) {
        ldapConnections = (LDAP**)malloc(g_config.connectionsPerThread * sizeof(LDAP*));
        if (!ldapConnections) return FALSE;
        memset(ldapConnections, 0, g_config.connectionsPerThread * sizeof(LDAP*));
    }

    while (stats->keepRunning && stats->connectionCount < g_config.connectionsPerThread) {
        LDAP* pLdap = NULL;
        LDAPMessage* pResult = NULL;
        ULONG result;
        WCHAR targetDomain[256];

        // 准备域名
        if (wcslen(g_config.serverName) > 0) {
            wcscpy_s(targetDomain, 256, g_config.serverName);
        }
        else {
            wcscpy_s(targetDomain, 256, L"localhost");
        }

        // 创建LDAP连接
        pLdap = ldap_initW(targetDomain, LDAP_PORT);
        if (!pLdap) {
            PrintWithTimestamp(L"线程%d: LDAP初始化失败", stats->threadId);
            break;
        }

        // 设置LDAP选项
        ULONG version = LDAP_VERSION3;
        ldap_set_optionW(pLdap, LDAP_OPT_PROTOCOL_VERSION, &version);

        // 绑定
        result = ldap_bind_sW(pLdap, NULL, NULL, LDAP_AUTH_NEGOTIATE);
        if (result != LDAP_SUCCESS) {
            PrintWithTimestamp(L"线程%d: LDAP绑定失败, 错误: %lu", stats->threadId, result);
            ldap_unbind(pLdap);
            break;
        }

        // 执行查询
        static WCHAR filter[] = L"(&(objectClass=user)(objectCategory=person))";
        static WCHAR attr_sam[] = L"sAMAccountName";
        PWCHAR attributes[2] = { attr_sam, NULL };

        WCHAR baseDN[256];
        swprintf_s(baseDN, 256, L"DC=%s,DC=local", targetDomain);

        result = ldap_search_sW(pLdap, baseDN, LDAP_SCOPE_SUBTREE,
            filter, attributes, 0, &pResult);

        if (result == LDAP_SUCCESS) {
            ULONG entryCount = ldap_count_entries(pLdap, pResult);
            stats->queryCount++;
            stats->connectionCount++;

            if (g_config.verboseOutput) {
                PrintWithTimestamp(L"线程%d: LDAP查询成功, 条目数: %lu",
                    stats->threadId, entryCount);
            }

            if (pResult) {
                ldap_msgfree(pResult);
            }
        }
        else {
            PrintWithTimestamp(L"线程%d: LDAP查询失败, 错误: %lu", stats->threadId, result);
        }

        // 连接处理策略
        if (g_config.holdConnections) {
            // 保持连接不关闭（模拟连接泄漏）
            ldapConnections[stats->connectionCount - 1] = pLdap;
            PrintWithTimestamp(L"线程%d: 保持LDAP连接 #%d 打开",
                stats->threadId, stats->connectionCount);
        }
        else {
            // 正常关闭连接
            ldap_unbind(pLdap);
        }

        if (g_config.queryInterval > 0) {
            Sleep(g_config.queryInterval);
        }
    }

    // 清理（如果需要）
    if (ldapConnections) {
        stats->connections = (HANDLE*)ldapConnections;
    }

    return TRUE;
}

// 连接工作线程
unsigned int __stdcall ConnectionWorkerThread(void* param) {
    ThreadStats* stats = (ThreadStats*)param;

    PrintWithTimestamp(L"线程%d启动: 模式=%d, 目标连接数=%d",
        stats->threadId, stats->mode, g_config.connectionsPerThread);

    stats->startTime = GetTickCount();
    stats->keepRunning = TRUE;

    switch (stats->mode) {
    case TEST_NETAPI_RAPID:
    case TEST_NETAPI_PERSISTENT:
        TestNetAPIConnections(stats);
        break;

    case TEST_LDAP_RAPID:
    case TEST_LDAP_PERSISTENT:
        TestLDAPConnections(stats);
        break;

    case TEST_MIXED_MODE:
        // 混合模式：一半NetAPI，一半LDAP
        if (stats->threadId % 2 == 0) {
            TestNetAPIConnections(stats);
        }
        else {
            TestLDAPConnections(stats);
        }
        break;
    }

    stats->endTime = GetTickCount();

    PrintWithTimestamp(L"线程%d完成: 查询数=%d, 连接数=%d, 耗时=%lums",
        stats->threadId, stats->queryCount, stats->connectionCount,
        stats->endTime - stats->startTime);

    return 0;
}

// 连接监控线程
void MonitorConnections(void) {
    PrintWithTimestamp(L"开始连接监控...");

    DWORD startTime = GetTickCount();

    while (!g_stopTest) {
        Sleep(5000); // 每5秒检查一次

        if (g_config.enableNetstat) {
            DisplayNetworkConnections();
        }

        // 检查测试持续时间
        if (g_config.testDuration > 0) {
            DWORD elapsed = (GetTickCount() - startTime) / 1000;
            if (elapsed >= g_config.testDuration) {
                PrintWithTimestamp(L"达到测试时间限制 (%d秒), 停止测试", g_config.testDuration);
                g_stopTest = TRUE;

                // 通知所有线程停止
                for (int i = 0; i < g_config.threadCount; i++) {
                    if (g_threadStats) {
                        g_threadStats[i].keepRunning = FALSE;
                    }
                }
                break;
            }
        }
    }
}

// 解析命令行参数
BOOL ParseConnectionTestArgs(int argc, wchar_t* argv[]) {
    // 默认配置
    wcscpy_s(g_config.serverName, 256, L"");
    g_config.testMode = TEST_LDAP_PERSISTENT;
    g_config.threadCount = 10;
    g_config.connectionsPerThread = 5;
    g_config.queryInterval = 1000;
    g_config.holdConnections = TRUE;
    g_config.enableNetstat = TRUE;
    g_config.testDuration = 30;
    g_config.verboseOutput = FALSE;

    for (int i = 1; i < argc; i++) {
        if (wcscmp(argv[i], L"-s") == 0 || wcscmp(argv[i], L"--server") == 0) {
            if (i + 1 < argc) wcscpy_s(g_config.serverName, 256, argv[++i]);
        }
        else if (wcscmp(argv[i], L"-m") == 0 || wcscmp(argv[i], L"--mode") == 0) {
            if (i + 1 < argc) {
                int mode = _wtoi(argv[++i]);
                if (mode >= 0 && mode <= 4) {
                    g_config.testMode = (TestMode)mode;
                }
            }
        }
        else if (wcscmp(argv[i], L"-t") == 0 || wcscmp(argv[i], L"--threads") == 0) {
            if (i + 1 < argc) g_config.threadCount = _wtoi(argv[++i]);
        }
        else if (wcscmp(argv[i], L"-c") == 0 || wcscmp(argv[i], L"--connections") == 0) {
            if (i + 1 < argc) g_config.connectionsPerThread = _wtoi(argv[++i]);
        }
        else if (wcscmp(argv[i], L"-i") == 0 || wcscmp(argv[i], L"--interval") == 0) {
            if (i + 1 < argc) g_config.queryInterval = _wtoi(argv[++i]);
        }
        else if (wcscmp(argv[i], L"-d") == 0 || wcscmp(argv[i], L"--duration") == 0) {
            if (i + 1 < argc) g_config.testDuration = _wtoi(argv[++i]);
        }
        else if (wcscmp(argv[i], L"--close-connections") == 0) {
            g_config.holdConnections = FALSE;
        }
        else if (wcscmp(argv[i], L"--no-netstat") == 0) {
            g_config.enableNetstat = FALSE;
        }
        else if (wcscmp(argv[i], L"-v") == 0 || wcscmp(argv[i], L"--verbose") == 0) {
            g_config.verboseOutput = TRUE;
        }
        else if (wcscmp(argv[i], L"/?") == 0 || wcscmp(argv[i], L"-h") == 0) {
            ShowConnectionUsage();
            return FALSE;
        }
    }

    return TRUE;
}

// 显示帮助
void ShowConnectionUsage(void) {
    wprintf(L"连接泄漏测试程序\n");
    wprintf(L"用法: ConnectionTest.exe [选项]\n\n");
    wprintf(L"选项:\n");
    wprintf(L"  -s, --server <名称>        目标服务器\n");
    wprintf(L"  -m, --mode <0-4>           测试模式:\n");
    wprintf(L"                             0=NetAPI快速 1=NetAPI持久\n");
    wprintf(L"                             2=LDAP快速   3=LDAP持久\n");
    wprintf(L"                             4=混合模式\n");
    wprintf(L"  -t, --threads <数量>       线程数 (默认:10)\n");
    wprintf(L"  -c, --connections <数量>   每线程连接数 (默认:5)\n");
    wprintf(L"  -i, --interval <毫秒>      查询间隔 (默认:1000)\n");
    wprintf(L"  -d, --duration <秒>        测试持续时间 (默认:30)\n");
    wprintf(L"  --close-connections        正常关闭连接\n");
    wprintf(L"  --no-netstat               禁用netstat监控\n");
    wprintf(L"  -v, --verbose              详细输出\n\n");
    wprintf(L"示例:\n");
    wprintf(L"  ConnectionTest.exe                    # 默认LDAP持久连接测试\n");
    wprintf(L"  ConnectionTest.exe -m 3 -t 20         # 20线程LDAP持久连接\n");
    wprintf(L"  ConnectionTest.exe --close-connections # 正常关闭连接模式\n");
}

// 主函数
int wmain(int argc, wchar_t* argv[]) {
    setlocale(LC_ALL, "chs");
    SetConsoleOutputCP(CP_UTF8);

    wprintf(L"========================================\n");
    wprintf(L"        连接泄漏测试程序\n");
    wprintf(L"========================================\n");

    if (!ParseConnectionTestArgs(argc, argv)) {
        return 0;
    }

    InitializeCriticalSection(&g_printLock);

    // 显示测试配置
    const wchar_t* modeNames[] = {
        L"NetAPI快速连接", L"NetAPI持久连接",
        L"LDAP快速连接", L"LDAP持久连接", L"混合模式"
    };

    PrintWithTimestamp(L"测试配置:");
    wprintf(L"  - 服务器: %s\n", wcslen(g_config.serverName) > 0 ? g_config.serverName : L"localhost");
    wprintf(L"  - 测试模式: %s\n", modeNames[g_config.testMode]);
    wprintf(L"  - 线程数: %d\n", g_config.threadCount);
    wprintf(L"  - 每线程连接数: %d\n", g_config.connectionsPerThread);
    wprintf(L"  - 查询间隔: %d ms\n", g_config.queryInterval);
    wprintf(L"  - 保持连接: %s\n", g_config.holdConnections ? L"是" : L"否");
    wprintf(L"  - 测试时长: %d 秒\n", g_config.testDuration);

    // 显示初始连接状态
    PrintWithTimestamp(L"=== 测试前的连接状态 ===");
    DisplayNetworkConnections();

    // 分配资源
    g_threadStats = (ThreadStats*)calloc(g_config.threadCount, sizeof(ThreadStats));
    g_threadHandles = (HANDLE*)calloc(g_config.threadCount, sizeof(HANDLE));

    if (!g_threadStats || !g_threadHandles) {
        PrintWithTimestamp(L"内存分配失败");
        return 1;
    }

    PrintWithTimestamp(L"开始连接测试...");

    // 创建工作线程
    for (int i = 0; i < g_config.threadCount; i++) {
        g_threadStats[i].threadId = i + 1;
        g_threadStats[i].mode = g_config.testMode;
        g_threadStats[i].queryCount = 0;
        g_threadStats[i].connectionCount = 0;
        g_threadStats[i].keepRunning = FALSE;
        g_threadStats[i].connections = NULL;

        g_threadHandles[i] = (HANDLE)_beginthreadex(
            NULL, 0, ConnectionWorkerThread, &g_threadStats[i], 0, NULL);

        if (g_threadHandles[i]) {
            Sleep(50); // 错开启动时间
        }
    }

    // 启动监控
    _beginthreadex(NULL, 0, (unsigned int(__stdcall*)(void*))MonitorConnections, NULL, 0, NULL);

    // 等待测试完成或用户中断
    PrintWithTimestamp(L"测试运行中... 按任意键提前停止");

    while (!g_stopTest) {
        if (_kbhit()) {
            _getch();
            PrintWithTimestamp(L"用户中断测试");
            g_stopTest = TRUE;
            break;
        }
        Sleep(100);
    }

    // 停止所有线程
    for (int i = 0; i < g_config.threadCount; i++) {
        if (g_threadStats) {
            g_threadStats[i].keepRunning = FALSE;
        }
    }

    // 等待线程结束
    if (g_threadHandles) {
        WaitForMultipleObjects(g_config.threadCount, g_threadHandles, TRUE, 5000);
        for (int i = 0; i < g_config.threadCount; i++) {
            if (g_threadHandles[i]) {
                CloseHandle(g_threadHandles[i]);
            }
        }
    }

    // 显示最终连接状态
    PrintWithTimestamp(L"=== 测试后的连接状态 ===");
    DisplayNetworkConnections();

    // 清理
    if (g_threadStats) {
        for (int i = 0; i < g_config.threadCount; i++) {
            if (g_threadStats[i].connections) {
                free(g_threadStats[i].connections);
            }
        }
        free(g_threadStats);
    }
    if (g_threadHandles) {
        free(g_threadHandles);
    }

    DeleteCriticalSection(&g_printLock);

    PrintWithTimestamp(L"测试完成");
    wprintf(L"按任意键退出...\n");
    _getch();

    return 0;
}
