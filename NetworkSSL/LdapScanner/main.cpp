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
//// �û���Ϣ�ṹ - �������Ա��Ϣ
//typedef struct UserInfo {
//    WCHAR samAccountName[256];
//    WCHAR displayName[256];
//    WCHAR userPrincipalName[256];
//    WCHAR** memberOfGroups;  // �û��������DN����
//    int groupCount;          // �û������������
//    int groupCapacity;       // ����������
//} UserInfo;
//
//// ����Ϣ�ṹ - ���ӳ�Ա��Ϣ
//typedef struct GroupInfo {
//    WCHAR groupName[256];
//    WCHAR description[256];
//    WCHAR groupType[64];
//    WCHAR distinguishedName[512];
//    WCHAR** members;         // ���Ա��DN����
//    int memberCount;         // ���Ա����
//    int memberCapacity;      // ��Ա��������
//} GroupInfo;
//
//// ȫ�ֱ���
//UserInfo* g_users = NULL;
//int g_userCount = 0;
//int g_userCapacity = 0;
//
//GroupInfo* g_groups = NULL;
//int g_groupCount = 0;
//int g_groupCapacity = 0;
//
//// ��������
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
//// ��DN����ȡCN
//WCHAR* ExtractCNFromDN(const WCHAR* dn) {
//    if (!dn) return NULL;
//
//    static WCHAR cn[256];
//    const WCHAR* start = wcsstr(dn, L"CN=");
//    if (!start) return NULL;
//
//    start += 3; // ���� "CN="
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
//// �ͷ��û���Ϣ�ڴ�
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
//// �ͷ�����Ϣ�ڴ�
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
//// ����û����б� - �������Ա��Ϣ
//void AddUser(const WCHAR* sam, const WCHAR* display, const WCHAR* upn, PWCHAR* memberOf, int memberCount) {
//    if (g_userCount >= g_userCapacity) {
//        g_userCapacity = g_userCapacity == 0 ? 1000 : g_userCapacity * 2;
//        UserInfo* newUsers = (UserInfo*)realloc(g_users, g_userCapacity * sizeof(UserInfo));
//        if (!newUsers) {
//            wprintf(L"�ڴ����ʧ��\n");
//            return;
//        }
//        g_users = newUsers;
//    }
//
//    UserInfo* user = &g_users[g_userCount];
//
//    // ��ʼ���û���Ϣ
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
//    // ��ʼ������Ϣ
//    user->memberOfGroups = NULL;
//    user->groupCount = 0;
//    user->groupCapacity = 0;
//
//    // ������Ա��Ϣ
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
//// ����鵽�б� - ������Ա��Ϣ
//void AddGroup(const WCHAR* name, const WCHAR* desc, const WCHAR* type, const WCHAR* dn, PWCHAR* members, int memberCount) {
//    if (g_groupCount >= g_groupCapacity) {
//        g_groupCapacity = g_groupCapacity == 0 ? 500 : g_groupCapacity * 2;
//        GroupInfo* newGroups = (GroupInfo*)realloc(g_groups, g_groupCapacity * sizeof(GroupInfo));
//        if (!newGroups) {
//            wprintf(L"���б��ڴ����ʧ��\n");
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
//    // ��ʼ����Ա��Ϣ
//    group->members = NULL;
//    group->memberCount = 0;
//    group->memberCapacity = 0;
//
//    // ��ӳ�Ա��Ϣ
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
//// ��ȡ��ǰ����
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
//// ����DN�ַ���
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
//// ����������
//void ParseGroupType(DWORD groupType, WCHAR* typeStr, DWORD size) {
//    if (groupType & 0x00000002) {
//        wcscpy_s(typeStr, size, L"ȫ��");
//    }
//    else if (groupType & 0x00000004) {
//        wcscpy_s(typeStr, size, L"�򱾵�");
//    }
//    else if (groupType & 0x00000008) {
//        wcscpy_s(typeStr, size, L"ͨ��");
//    }
//    else {
//        wcscpy_s(typeStr, size, L"δ֪");
//    }
//
//    if (groupType & 0x80000000) {
//        wcscat_s(typeStr, size, L"��ȫ��");
//    }
//    else {
//        wcscat_s(typeStr, size, L"ͨѶ��");
//    }
//}
//
//// ִ��LDAP�û���ѯ - ����memberOf����
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
//            wprintf(L"����: �޷���ȡ��ǰ������ʹ��localhost\n");
//        }
//    }
//
//    wprintf(L"�������ӵ���: %s\n", targetDomain);
//
//    pLdap = ldap_initW(targetDomain, LDAP_PORT);
//    if (!pLdap) {
//        wprintf(L"����: LDAP��ʼ��ʧ��\n");
//        return FALSE;
//    }
//
//    ULONG version = LDAP_VERSION3;
//    result = ldap_set_optionW(pLdap, LDAP_OPT_PROTOCOL_VERSION, &version);
//
//    LDAP_TIMEVAL timeout = { 30, 0 };
//    ldap_set_optionW(pLdap, LDAP_OPT_TIMELIMIT, &timeout);
//
//    wprintf(L"���ڽ��������֤...\n");
//
//    result = ldap_bind_sW(pLdap, NULL, NULL, LDAP_AUTH_NEGOTIATE);
//    if (result != LDAP_SUCCESS) {
//        PWCHAR errorMsg = ldap_err2stringW(result);
//        wprintf(L"����: LDAP��ʧ�� - %s (����: %lu)\n", errorMsg, result);
//        ldap_unbind(pLdap);
//        return FALSE;
//    }
//
//    wprintf(L"�����֤�ɹ���\n");
//
//    BuildBaseDN(targetDomain, baseDN, 512);
//    wprintf(L"��������DN: %s\n", baseDN);
//
//    static WCHAR filter[] = L"(&(objectClass=user)(objectCategory=person)(!(userAccountControl:1.2.840.113556.1.4.803:=2)))";
//
//    wprintf(L"��ʼ�����û�(�������Ա��Ϣ)...\n");
//
//    result = ldap_search_sW(pLdap, baseDN, LDAP_SCOPE_SUBTREE, filter, attributes, 0, &pResult);
//
//    if (result != LDAP_SUCCESS) {
//        PWCHAR errorMsg = ldap_err2stringW(result);
//        wprintf(L"����: LDAP����ʧ�� - %s (����: %lu)\n", errorMsg, result);
//        ldap_unbind(pLdap);
//        return FALSE;
//    }
//
//    ULONG entryCount = ldap_count_entries(pLdap, pResult);
//    wprintf(L"�ҵ� %lu ���û���Ŀ\n", entryCount);
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
//        // ��ȡmemberOf����
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
//    wprintf(L"LDAP�û���ѯ��ɣ��ܹ���ȡ�� %d ���û�\n", g_userCount);
//    return TRUE;
//}
//
//// ִ��LDAP���ѯ - ����member����
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
//            wprintf(L"����: �޷���ȡ��ǰ������ʹ��localhost\n");
//        }
//    }
//
//    wprintf(L"�������ӵ����ѯ��: %s\n", targetDomain);
//
//    pLdap = ldap_initW(targetDomain, LDAP_PORT);
//    if (!pLdap) {
//        wprintf(L"����: LDAP��ʼ��ʧ��(���ѯ)\n");
//        return FALSE;
//    }
//
//    ULONG version = LDAP_VERSION3;
//    result = ldap_set_optionW(pLdap, LDAP_OPT_PROTOCOL_VERSION, &version);
//
//    LDAP_TIMEVAL timeout = { 30, 0 };
//    ldap_set_optionW(pLdap, LDAP_OPT_TIMELIMIT, &timeout);
//
//    wprintf(L"���ڽ������ѯ�����֤...\n");
//
//    result = ldap_bind_sW(pLdap, NULL, NULL, LDAP_AUTH_NEGOTIATE);
//    if (result != LDAP_SUCCESS) {
//        PWCHAR errorMsg = ldap_err2stringW(result);
//        wprintf(L"����: LDAP��ʧ��(���ѯ) - %s (����: %lu)\n", errorMsg, result);
//        ldap_unbind(pLdap);
//        return FALSE;
//    }
//
//    wprintf(L"���ѯ�����֤�ɹ���\n");
//
//    BuildBaseDN(targetDomain, baseDN, 512);
//    wprintf(L"����������DN: %s\n", baseDN);
//
//    static WCHAR filter[] = L"(objectClass=group)";
//
//    wprintf(L"��ʼ������(������Ա��Ϣ)...\n");
//
//    result = ldap_search_sW(pLdap, baseDN, LDAP_SCOPE_SUBTREE, filter, attributes, 0, &pResult);
//
//    if (result != LDAP_SUCCESS) {
//        PWCHAR errorMsg = ldap_err2stringW(result);
//        wprintf(L"����: LDAP������ʧ�� - %s (����: %lu)\n", errorMsg, result);
//        ldap_unbind(pLdap);
//        return FALSE;
//    }
//
//    ULONG entryCount = ldap_count_entries(pLdap, pResult);
//    wprintf(L"�ҵ� %lu ������Ŀ\n", entryCount);
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
//        // ��ȡmember����
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
//    wprintf(L"LDAP���ѯ��ɣ��ܹ���ȡ�� %d ����\n", g_groupCount);
//    return TRUE;
//}
//
//// ��ʾ�û��б� - �������Ա��Ϣ
//void DisplayUsers(int maxCount) {
//    wprintf(L"\n============ �û��б�(��������Ϣ) ============\n");
//    wprintf(L"���û���: %d\n", g_userCount);
//
//    int displayCount = (maxCount > 0 && maxCount < g_userCount) ? maxCount : g_userCount;
//    wprintf(L"��ʾǰ %d ���û�:\n", displayCount);
//    wprintf(L"--------------------------------------------\n");
//
//    for (int i = 0; i < displayCount; i++) {
//        wprintf(L"[%d] �û���: %s", i + 1, g_users[i].samAccountName);
//
//        if (wcslen(g_users[i].displayName) > 0) {
//            wprintf(L", ��ʾ��: %s", g_users[i].displayName);
//        }
//
//        if (wcslen(g_users[i].userPrincipalName) > 0) {
//            wprintf(L", UPN: %s", g_users[i].userPrincipalName);
//        }
//
//        wprintf(L"\n");
//
//        // ��ʾ�û���������
//        if (g_users[i].groupCount > 0) {
//            wprintf(L"    ������ (%d��):\n", g_users[i].groupCount);
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
//            wprintf(L"    ������: (��)\n");
//        }
//        wprintf(L"\n");
//    }
//
//    if (g_userCount > displayCount) {
//        wprintf(L"... ���� %d ���û�δ��ʾ\n", g_userCount - displayCount);
//    }
//}
//
//// ��ʾ���б� - ������Ա��Ϣ
//void DisplayGroups(int maxCount) {
//    wprintf(L"\n============ ���б�(������Ա��Ϣ) ============\n");
//    wprintf(L"������: %d\n", g_groupCount);
//
//    int displayCount = (maxCount > 0 && maxCount < g_groupCount) ? maxCount : g_groupCount;
//    wprintf(L"��ʾǰ %d ����:\n", displayCount);
//    wprintf(L"--------------------------------------------\n");
//
//    for (int i = 0; i < displayCount; i++) {
//        wprintf(L"[%d] ����: %s", i + 1, g_groups[i].groupName);
//
//        if (wcslen(g_groups[i].groupType) > 0) {
//            wprintf(L", ����: %s", g_groups[i].groupType);
//        }
//
//        if (wcslen(g_groups[i].description) > 0) {
//            wprintf(L", ����: %s", g_groups[i].description);
//        }
//
//        wprintf(L"\n");
//
//        // ��ʾ���Ա
//        if (g_groups[i].memberCount > 0) {
//            wprintf(L"    ���Ա (%d��):\n", g_groups[i].memberCount);
//            for (int j = 0; j < g_groups[i].memberCount && j < 10; j++) { // �����ʾ10����Ա
//                WCHAR* memberCN = ExtractCNFromDN(g_groups[i].members[j]);
//                if (memberCN) {
//                    wprintf(L"      - %s\n", memberCN);
//                }
//                else {
//                    wprintf(L"      - %s\n", g_groups[i].members[j]);
//                }
//            }
//            if (g_groups[i].memberCount > 10) {
//                wprintf(L"      ... ���� %d ����Ա\n", g_groups[i].memberCount - 10);
//            }
//        }
//        else {
//            wprintf(L"    ���Ա: (��)\n");
//        }
//        wprintf(L"\n");
//    }
//
//    if (g_groupCount > displayCount) {
//        wprintf(L"... ���� %d ����δ��ʾ\n", g_groupCount - displayCount);
//    }
//}
//
//// ��ʾ�û�-���ϵӳ��
//void DisplayUserGroupMapping(int maxCount) {
//    wprintf(L"\n============ �û�-���ϵӳ�� ============\n");
//
//    int displayCount = (maxCount > 0 && maxCount < g_userCount) ? maxCount : g_userCount;
//    wprintf(L"��ʾǰ %d ���û������ϵ:\n", displayCount);
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
//                    wprintf(L"  ���� %s\n", groupCN);
//                }
//            }
//        }
//        else {
//            wprintf(L"  ���� (�����Ա��ϵ)\n");
//        }
//        wprintf(L"\n");
//    }
//}
//
//// ��ʾ������Ϣ
//void ShowUsage(void) {
//    wprintf(L"LDAP���û����������ѯ����\n");
//    wprintf(L"�÷�:\n");
//    wprintf(L"  LDAPTest.exe [����] [��ʾ����] [��ѯ����]\n");
//    wprintf(L"\n");
//    wprintf(L"����:\n");
//    wprintf(L"  ����       - Ҫ��ѯ������ (��ѡ��Ĭ��Ϊ��ǰ��)\n");
//    wprintf(L"  ��ʾ����   - ��ʾ����Ŀ���� (��ѡ��Ĭ��50)\n");
//    wprintf(L"  ��ѯ����   - users|groups|both|mapping (��ѡ��Ĭ��both)\n");
//    wprintf(L"\n");
//    wprintf(L"��ѯ����˵��:\n");
//    wprintf(L"  users    - ����ѯ�û�(������������Ϣ)\n");
//}
//
//// ������
//int wmain(int argc, wchar_t* argv[]) {
//    setlocale(LC_ALL, "chs");
//
//    SetConsoleOutputCP(CP_UTF8);
//
//    wprintf(L"========================================\n");
//    wprintf(L"   LDAP���û������ѯ���Գ���\n");
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
//    wprintf(L"������Ϣ:\n");
//    wprintf(L"- Ŀ����: %s\n", domainName);
//    wprintf(L"- ��ʾ����: %d\n", displayCount);
//    wprintf(L"- ��ѯ����: %s\n", queryType);
//    wprintf(L"- ʹ�ö˿�: 389 (LDAP)\n");
//    wprintf(L"========================================\n");
//
//    WCHAR userName[256];
//    DWORD userNameSize = 256;
//    if (GetUserNameW(userName, &userNameSize)) {
//        wprintf(L"��ǰ�û�: %s\n", userName);
//    }
//
//    wprintf(L"��ʼLDAP��ѯ...\n");
//
//    DWORD startTime = GetTickCount();
//    BOOL userSuccess = TRUE;
//    BOOL groupSuccess = TRUE;
//
//    if (wcscmp(queryType, L"users") == 0) {
//        wprintf(L"=== ����ѯ�û� ===\n");
//        userSuccess = QueryUsersViaLDAP(domainName);
//        groupSuccess = TRUE;
//    }
//    else if (wcscmp(queryType, L"groups") == 0) {
//        wprintf(L"=== ����ѯ�� ===\n");
//        groupSuccess = QueryGroupsViaLDAP(domainName);
//        userSuccess = TRUE;
//    }
//    else {
//        wprintf(L"=== ��ѯ�û����� ===\n");
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
//        wprintf(L"\n��ѯ�ɹ���ɣ�\n");
//        wprintf(L"����ʱ: %lu ����\n", endTime - startTime);
//
//        if (wcscmp(queryType, L"groups") != 0 && g_userCount > 0) {
//            DisplayUsers(displayCount);
//        }
//
//        if (wcscmp(queryType, L"users") != 0 && g_groupCount > 0) {
//            DisplayGroups(displayCount);
//        }
//
//        wprintf(L"\n============ ͳ����Ϣ ============\n");
//        if (wcscmp(queryType, L"groups") != 0) {
//            wprintf(L"�û�����: %d\n", g_userCount);
//        }
//        if (wcscmp(queryType, L"users") != 0) {
//            wprintf(L"������: %d\n", g_groupCount);
//        }
//        wprintf(L"��ѯ��ʱ: %lu ����\n", endTime - startTime);
//
//        wprintf(L"\n========================================\n");
//        wprintf(L"������ɣ���������˳�...\n");
//
//    }
//    else {
//        wprintf(L"\n��ѯʧ�ܣ�\n");
//        wprintf(L"���ܵ�ԭ��:\n");
//        wprintf(L"1. ������������\n");
//        wprintf(L"2. ����������ɴ�\n");
//        wprintf(L"3. �����֤ʧ��\n");
//        wprintf(L"4. Ȩ�޲���\n");
//        wprintf(L"5. ����ǽ��ֹ�˿�389\n");
//        wprintf(L"6. DNS����ʧ��\n");
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
//// �û���Ϣ�ṹ - �������Ա��Ϣ
//typedef struct UserInfo {
//    WCHAR samAccountName[256];
//    WCHAR domainName[256];
//    BOOL isDisabled;         // �û��Ƿ񱻽���
//    WCHAR** memberOfGroups;  // �û��������DN����
//    int groupCount;          // �û������������
//    int groupCapacity;       // ����������
//} UserInfo;
//
//// ȫ�ֱ���
//UserInfo* g_users = NULL;
//int g_userCount = 0;
//int g_userCapacity = 0;
//
//// ��������
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
//// ��DN����ȡCN
//WCHAR* ExtractCNFromDN(const WCHAR* dn) {
//    if (!dn) return NULL;
//
//    static WCHAR cn[256];
//    const WCHAR* start = wcsstr(dn, L"CN=");
//    if (!start) return NULL;
//
//    start += 3; // ���� "CN="
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
//// �ͷ��û���Ϣ�ڴ�
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
//// ����û����б� - �������Ա��Ϣ
//void AddUser(const WCHAR* sam, const WCHAR* domain, BOOL disabled, PWCHAR* memberOf, int memberCount) {
//    if (g_userCount >= g_userCapacity) {
//        g_userCapacity = g_userCapacity == 0 ? 1000 : g_userCapacity * 2;
//        UserInfo* newUsers = (UserInfo*)realloc(g_users, g_userCapacity * sizeof(UserInfo));
//        if (!newUsers) {
//            wprintf(L"�ڴ����ʧ��\n");
//            return;
//        }
//        g_users = newUsers;
//    }
//
//    UserInfo* user = &g_users[g_userCount];
//
//    // ��ʼ���û���Ϣ
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
//    // ��ʼ������Ϣ
//    user->memberOfGroups = NULL;
//    user->groupCount = 0;
//    user->groupCapacity = 0;
//
//    // ������Ա��Ϣ
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
//// ��ȡ��ǰ����
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
//// ����DN�ַ���
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
//// ִ��LDAP�û���ѯ - ����memberOf���Ժ�userAccountControl
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
//            wprintf(L"����: �޷���ȡ��ǰ������ʹ��localhost\n");
//        }
//    }
//
//    wprintf(L"�������ӵ���: %s\n", targetDomain);
//
//    pLdap = ldap_initW(targetDomain, LDAP_PORT);
//    if (!pLdap) {
//        wprintf(L"����: LDAP��ʼ��ʧ��\n");
//        return FALSE;
//    }
//
//    ULONG version = LDAP_VERSION3;
//    result = ldap_set_optionW(pLdap, LDAP_OPT_PROTOCOL_VERSION, &version);
//
//    LDAP_TIMEVAL timeout = { 30, 0 };
//    ldap_set_optionW(pLdap, LDAP_OPT_TIMELIMIT, &timeout);
//
//    wprintf(L"���ڽ��������֤...\n");
//
//    result = ldap_bind_sW(pLdap, NULL, NULL, LDAP_AUTH_NEGOTIATE);
//    if (result != LDAP_SUCCESS) {
//        PWCHAR errorMsg = ldap_err2stringW(result);
//        wprintf(L"����: LDAP��ʧ�� - %s (����: %lu)\n", errorMsg, result);
//        ldap_unbind(pLdap);
//        return FALSE;
//    }
//
//    wprintf(L"�����֤�ɹ���\n");
//
//    BuildBaseDN(targetDomain, baseDN, 512);
//    wprintf(L"��������DN: %s\n", baseDN);
//
//    static WCHAR filter[] = L"(&(objectClass=user)(objectCategory=person))";
//
//    wprintf(L"��ʼ�����û�(��������״̬�����Ա��Ϣ)...\n");
//
//    result = ldap_search_sW(pLdap, baseDN, LDAP_SCOPE_SUBTREE, filter, attributes, 0, &pResult);
//
//    if (result != LDAP_SUCCESS) {
//        PWCHAR errorMsg = ldap_err2stringW(result);
//        wprintf(L"����: LDAP����ʧ�� - %s (����: %lu)\n", errorMsg, result);
//        ldap_unbind(pLdap);
//        return FALSE;
//    }
//
//    ULONG entryCount = ldap_count_entries(pLdap, pResult);
//    wprintf(L"�ҵ� %lu ���û���Ŀ\n", entryCount);
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
//        // ��ȡuserAccountControl�������ж��û��Ƿ񱻽���
//        PWCHAR* uacValues = ldap_get_valuesW(pLdap, pEntry, attr_uac);
//        if (uacValues && uacValues[0]) {
//            DWORD userAccountControl = (DWORD)_wtol(uacValues[0]);
//            // ADS_UF_ACCOUNTDISABLE = 0x2
//            isDisabled = (userAccountControl & 0x2) != 0;
//        }
//
//        // ��ȡmemberOf����
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
//    wprintf(L"LDAP�û���ѯ��ɣ��ܹ���ȡ�� %d ���û�\n", g_userCount);
//    return TRUE;
//}
//
//// ��ʾ�û��б� - �����û���������������״̬������Ϣ
//void DisplayUsers(int maxCount) {
//    wprintf(L"\n============ �û��б� ============\n");
//    wprintf(L"���û���: %d\n", g_userCount);
//
//    int displayCount = (maxCount > 0 && maxCount < g_userCount) ? maxCount : g_userCount;
//    wprintf(L"��ʾǰ %d ���û�:\n", displayCount);
//    wprintf(L"--------------------------------------------\n");
//
//    for (int i = 0; i < displayCount; i++) {
//        wprintf(L"[%d] �û���: %s", i + 1, g_users[i].samAccountName);
//        wprintf(L", ����: %s", g_users[i].domainName);
//        wprintf(L", ״̬: %s", g_users[i].isDisabled ? L"�ѽ���" : L"����");
//        wprintf(L"\n");
//
//        // ��ʾ�û���������
//        if (g_users[i].groupCount > 0) {
//            wprintf(L"    ������ (%d��):\n", g_users[i].groupCount);
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
//            wprintf(L"    ������: (��)\n");
//        }
//        wprintf(L"\n");
//    }
//
//    if (g_userCount > displayCount) {
//        wprintf(L"... ���� %d ���û�δ��ʾ\n", g_userCount - displayCount);
//    }
//
//    // ��ʾͳ����Ϣ
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
//    wprintf(L"\n========== �û�״̬ͳ�� ==========\n");
//    wprintf(L"�����û�: %d\n", enabledCount);
//    wprintf(L"�����û�: %d\n", disabledCount);
//    wprintf(L"�ܼ��û�: %d\n", g_userCount);
//}
//
//// ��ʾ�û�-���ϵӳ��
//void DisplayUserGroupMapping(int maxCount) {
//    wprintf(L"\n============ �û�-���ϵӳ�� ============\n");
//
//    int displayCount = (maxCount > 0 && maxCount < g_userCount) ? maxCount : g_userCount;
//    wprintf(L"��ʾǰ %d ���û������ϵ:\n", displayCount);
//    wprintf(L"----------------------------------------\n");
//
//    for (int i = 0; i < displayCount; i++) {
//        wprintf(L"%s@%s", g_users[i].samAccountName, g_users[i].domainName);
//        wprintf(L" [%s]", g_users[i].isDisabled ? L"����" : L"����");
//        wprintf(L":\n");
//
//        if (g_users[i].groupCount > 0) {
//            for (int j = 0; j < g_users[i].groupCount; j++) {
//                WCHAR* groupCN = ExtractCNFromDN(g_users[i].memberOfGroups[j]);
//                if (groupCN) {
//                    wprintf(L"  ���� %s\n", groupCN);
//                }
//            }
//        }
//        else {
//            wprintf(L"  ���� (�����Ա��ϵ)\n");
//        }
//        wprintf(L"\n");
//    }
//}
//
//// ��ʾ������Ϣ
//void ShowUsage(void) {
//    wprintf(L"LDAP���û���ѯ����\n");
//    wprintf(L"�÷�:\n");
//    wprintf(L"  LDAPUserQuery.exe [����] [��ʾ����] [��ʾ����]\n");
//    wprintf(L"\n");
//    wprintf(L"����:\n");
//    wprintf(L"  ����       - Ҫ��ѯ������ (��ѡ��Ĭ��Ϊ��ǰ��)\n");
//    wprintf(L"  ��ʾ����   - ��ʾ���û����� (��ѡ��Ĭ��50)\n");
//    wprintf(L"  ��ʾ����   - users|mapping (��ѡ��Ĭ��users)\n");
//    wprintf(L"\n");
//    wprintf(L"��ʾ����˵��:\n");
//    wprintf(L"  users    - ��ʾ�û���ϸ��Ϣ(����������)\n");
//    wprintf(L"  mapping  - ��ʾ�û�-���ϵӳ��\n");
//    wprintf(L"\n");
//    wprintf(L"ʾ��:\n");
//    wprintf(L"  LDAPUserQuery.exe                    # ��ѯ��ǰ���ǰ50���û�\n");
//    wprintf(L"  LDAPUserQuery.exe example.com        # ��ѯָ�����ǰ50���û�\n");
//    wprintf(L"  LDAPUserQuery.exe example.com 100    # ��ѯָ�����ǰ100���û�\n");
//    wprintf(L"  LDAPUserQuery.exe localhost 20 mapping  # ��ʾǰ20���û�����ӳ��\n");
//}
//
//// ������
//int wmain(int argc, wchar_t* argv[]) {
//    setlocale(LC_ALL, "chs");
//    SetConsoleOutputCP(CP_UTF8);
//
//    wprintf(L"========================================\n");
//    wprintf(L"      LDAP���û���ѯ����\n");
//    wprintf(L"========================================\n");
//
//    WCHAR domainName[256] = { 0 };
//    int displayCount = 50;
//    WCHAR displayType[32] = L"users";
//
//    // ���������в���
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
//    // ��ȡ����
//    if (wcslen(domainName) == 0) {
//        GetCurrentDomain(domainName, 256);
//    }
//
//    // ��ʾ������Ϣ
//    wprintf(L"������Ϣ:\n");
//    wprintf(L"- Ŀ����: %s\n", domainName);
//    wprintf(L"- ��ʾ����: %d\n", displayCount);
//    wprintf(L"- ��ʾ����: %s\n", displayType);
//    wprintf(L"- ʹ�ö˿�: 389 (LDAP)\n");
//    wprintf(L"========================================\n");
//
//    // ��ʾ��ǰ�û�
//    WCHAR userName[256];
//    DWORD userNameSize = 256;
//    if (GetUserNameW(userName, &userNameSize)) {
//        wprintf(L"��ǰ�û�: %s\n", userName);
//    }
//
//    wprintf(L"��ʼLDAP�û���ѯ...\n");
//
//    DWORD startTime = GetTickCount();
//    BOOL success = QueryUsersViaLDAP(domainName);
//    DWORD endTime = GetTickCount();
//
//    if (success) {
//        wprintf(L"\n��ѯ�ɹ���ɣ�\n");
//        wprintf(L"����ʱ: %lu ����\n", endTime - startTime);
//
//        // ������ʾ������ʾ���
//        if (wcscmp(displayType, L"mapping") == 0) {
//            DisplayUserGroupMapping(displayCount);
//        }
//        else {
//            DisplayUsers(displayCount);
//        }
//
//        wprintf(L"\n============ ͳ����Ϣ ============\n");
//        wprintf(L"�û�����: %d\n", g_userCount);
//        wprintf(L"��ѯ��ʱ: %lu ����\n", endTime - startTime);
//
//        wprintf(L"\n========================================\n");
//        wprintf(L"��ѯ��ɣ���������˳�...\n");
//    }
//    else {
//        wprintf(L"\n��ѯʧ�ܣ�\n");
//        wprintf(L"���ܵ�ԭ��:\n");
//        wprintf(L"1. ������������\n");
//        wprintf(L"2. ����������ɴ�\n");
//        wprintf(L"3. �����֤ʧ��\n");
//        wprintf(L"4. Ȩ�޲���\n");
//        wprintf(L"5. ����ǽ��ֹ�˿�389\n");
//        wprintf(L"6. DNS����ʧ��\n");
//
//        // �����ڴ�
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
//    // �����ڴ�
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
//// �߳�ͳ����Ϣ�ṹ
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
//// �������ýṹ
//typedef struct LoadTestConfig {
//    WCHAR serverName[256];
//    int threadCount;
//    int iterationsPerThread;
//    int delayBetweenIterations;  // ����
//    BOOL enableGroupQuery;
//    BOOL enableRetry;
//    int maxRetries;
//    BOOL verboseOutput;
//} LoadTestConfig;
//
//// ȫ�ֱ���
//LoadTestConfig g_config = { 0 };
//ThreadStats* g_threadStats = NULL;
//HANDLE* g_threadHandles = NULL;
//CRITICAL_SECTION g_printLock;
//volatile LONG g_totalRequests = 0;
//volatile LONG g_successfulRequests = 0;
//volatile LONG g_failedRequests = 0;
//
//// ��������
//unsigned int __stdcall WorkerThread(void* param);
//BOOL QueryUsersWithRetry(const WCHAR* serverName, ThreadStats* stats);
//BOOL GetUserGroupsCount(const WCHAR* serverName, const WCHAR* userName, int* groupCount);
//void PrintThreadMessage(int threadId, const WCHAR* message, ...);
//void DisplayTestResults(void);
//void DisplayRealTimeStats(void);
//void ShowLoadTestUsage(void);
//BOOL ParseCommandLine(int argc, wchar_t* argv[]);
//
//// �̰߳�ȫ�Ĵ�ӡ����
//void PrintThreadMessage(int threadId, const WCHAR* message, ...) {
//    if (!g_config.verboseOutput) return;
//
//    EnterCriticalSection(&g_printLock);
//
//    SYSTEMTIME st;
//    GetLocalTime(&st);
//
//    wprintf(L"[%02d:%02d:%02d.%03d][�߳�%02d] ",
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
//// ��ȡ�û����������򻯰汾�����ٲ�ѯ������
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
//// �����Ի��Ƶ��û���ѯ
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
//            1,  // ʹ��USER_INFO_1�������ݴ���
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
//            // ����������ѯ��Ϊ�����û���ѯ����Ϣ
//            if (g_config.enableGroupQuery && entriesRead > 0) {
//                int groupQueryLimit = min(entriesRead, 10); // �������ѯ�����Լ��ٸ���
//                for (DWORD i = 0; i < groupQueryLimit; i++) {
//                    int groupCount = 0;
//                    if (GetUserGroupsCount(wcslen(serverName) > 0 ? serverName : NULL,
//                        userInfo[i].usri1_name, &groupCount)) {
//                        stats->groupQueryCount++;
//                    }
//                }
//            }
//
//            PrintThreadMessage(stats->threadId, L"��ѯ�ɹ�: %lu �û�, �ܼ�: %d",
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
//            PrintThreadMessage(stats->threadId, L"��ѯʧ�� (���� %d): ���� %lu", attempt, status);
//
//            if (g_config.enableRetry && attempt < g_config.maxRetries) {
//                stats->retryCount++;
//                Sleep(100 * attempt); // ָ���˱�
//                PrintThreadMessage(stats->threadId, L"�ȴ� %d ���������...", 100 * attempt);
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
//// �����̺߳���
//unsigned int __stdcall WorkerThread(void* param) {
//    ThreadStats* stats = (ThreadStats*)param;
//
//    PrintThreadMessage(stats->threadId, L"��ʼִ�У��ƻ� %d �ε���", g_config.iterationsPerThread);
//
//    stats->startTime = GetTickCount();
//
//    for (int i = 0; i < g_config.iterationsPerThread; i++) {
//        PrintThreadMessage(stats->threadId, L"��ʼ�� %d/%d �β�ѯ", i + 1, g_config.iterationsPerThread);
//
//        DWORD queryStart = GetTickCount();
//
//        if (QueryUsersWithRetry(g_config.serverName, stats)) {
//            DWORD queryTime = GetTickCount() - queryStart;
//            PrintThreadMessage(stats->threadId, L"�� %d �β�ѯ��ɣ���ʱ: %lu ���룬�û���: %d",
//                i + 1, queryTime, stats->userCount);
//            stats->success = TRUE;
//        }
//        else {
//            PrintThreadMessage(stats->threadId, L"�� %d �β�ѯʧ�ܣ�����: %lu", i + 1, stats->lastError);
//        }
//
//        // �������ӳ�
//        if (i < g_config.iterationsPerThread - 1 && g_config.delayBetweenIterations > 0) {
//            PrintThreadMessage(stats->threadId, L"�ȴ� %d ����...", g_config.delayBetweenIterations);
//            Sleep(g_config.delayBetweenIterations);
//        }
//    }
//
//    stats->endTime = GetTickCount();
//    stats->totalTime = stats->endTime - stats->startTime;
//
//    PrintThreadMessage(stats->threadId, L"������в�ѯ���ܺ�ʱ: %lu ����", stats->totalTime);
//
//    return 0;
//}
//
//// ��ʾʵʱͳ��
//void DisplayRealTimeStats(void) {
//    wprintf(L"\n========== ʵʱͳ�� ==========\n");
//    wprintf(L"��������: %ld\n", g_totalRequests);
//    wprintf(L"�ɹ�����: %ld\n", g_successfulRequests);
//    wprintf(L"ʧ������: %ld\n", g_failedRequests);
//    if (g_totalRequests > 0) {
//        wprintf(L"�ɹ���: %.2f%%\n", (double)g_successfulRequests / g_totalRequests * 100);
//    }
//    wprintf(L"=============================\n");
//}
//
//// ��ʾ���Խ��
//void DisplayTestResults(void) {
//    wprintf(L"\n============ ���ز��Խ�� ============\n");
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
//        wprintf(L"�߳� %02d: ", stats->threadId);
//        if (stats->success) {
//            wprintf(L"�ɹ�");
//            successfulThreads++;
//        }
//        else {
//            wprintf(L"ʧ�� (����: %lu)", stats->lastError);
//        }
//
//        wprintf(L", ��ʱ: %5lu ms, �û�: %4d, ���ѯ: %3d, ����: %d\n",
//            stats->totalTime, stats->userCount, stats->groupQueryCount, stats->retryCount);
//
//        totalTime += stats->totalTime;
//        totalUsers += stats->userCount;
//        totalGroupQueries += stats->groupQueryCount;
//        totalRetries += stats->retryCount;
//    }
//
//    wprintf(L"\n========== ����ͳ�� ==========\n");
//    wprintf(L"��������:\n");
//    wprintf(L"  - Ŀ�������: %s\n", wcslen(g_config.serverName) > 0 ? g_config.serverName : L"�����������");
//    wprintf(L"  - �����߳���: %d\n", g_config.threadCount);
//    wprintf(L"  - ÿ�̵߳�����: %d\n", g_config.iterationsPerThread);
//    wprintf(L"  - �������ӳ�: %d ms\n", g_config.delayBetweenIterations);
//    wprintf(L"  - �������ѯ: %s\n", g_config.enableGroupQuery ? L"��" : L"��");
//    wprintf(L"  - ��������: %s\n", g_config.enableRetry ? L"��" : L"��");
//
//    wprintf(L"\n���ͳ��:\n");
//    wprintf(L"  - �ɹ��߳���: %d/%d (%.1f%%)\n", successfulThreads, g_config.threadCount,
//        (double)successfulThreads / g_config.threadCount * 100);
//    wprintf(L"  - ��������: %ld\n", g_totalRequests);
//    wprintf(L"  - �ɹ�������: %ld\n", g_successfulRequests);
//    wprintf(L"  - ʧ��������: %ld\n", g_failedRequests);
//    wprintf(L"  - ����ɹ���: %.2f%%\n", (double)g_successfulRequests / g_totalRequests * 100);
//    wprintf(L"  - ƽ����ʱ: %.1f ms\n", (double)totalTime / g_config.threadCount);
//    wprintf(L"  - �ܲ�ѯ�û���: %d\n", totalUsers);
//    wprintf(L"  - �����ѯ��: %d\n", totalGroupQueries);
//    wprintf(L"  - �����Դ���: %d\n", totalRetries);
//
//    // ����������
//    if (totalTime > 0) {
//        double avgTime = (double)totalTime / g_config.threadCount / 1000.0; // ת��Ϊ��
//        double requestsPerSecond = g_totalRequests / avgTime;
//        wprintf(L"  - ƽ����������: %.2f ����/��\n", requestsPerSecond);
//    }
//
//    wprintf(L"=====================================\n");
//}
//
//// ���������в���
//BOOL ParseCommandLine(int argc, wchar_t* argv[]) {
//    // ����Ĭ��ֵ
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
//    // ��֤����
//    if (g_config.threadCount <= 0 || g_config.threadCount > 100) {
//        wprintf(L"����: �߳���������1-100֮��\n");
//        return FALSE;
//    }
//
//    if (g_config.iterationsPerThread <= 0 || g_config.iterationsPerThread > 50) {
//        wprintf(L"����: ������������1-50֮��\n");
//        return FALSE;
//    }
//
//    return TRUE;
//}
//
//// ��ʾ������Ϣ
//void ShowLoadTestUsage(void) {
//    wprintf(L"����������ز��Գ���\n");
//    wprintf(L"�÷�:\n");
//    wprintf(L"  LoadTest.exe [ѡ��]\n");
//    wprintf(L"\n");
//    wprintf(L"ѡ��:\n");
//    wprintf(L"  -s, --server <����>     Ŀ����������� (Ĭ��: ��ǰ�������)\n");
//    wprintf(L"  -t, --threads <����>    �����߳��� (Ĭ��: 5, ��Χ: 1-100)\n");
//    wprintf(L"  -i, --iterations <����> ÿ�̵߳����� (Ĭ��: 3, ��Χ: 1-50)\n");
//    wprintf(L"  -d, --delay <����>      �������ӳ� (Ĭ��: 1000ms)\n");
//    wprintf(L"  -g, --groups            �������ѯ (���Ӹ���)\n");
//    wprintf(L"  --no-retry              �������Ի���\n");
//    wprintf(L"  -r, --retries <����>    ������Դ��� (Ĭ��: 3)\n");
//    wprintf(L"  -v, --verbose           ��ϸ���\n");
//    wprintf(L"  -h, --help              ��ʾ������Ϣ\n");
//    wprintf(L"\n");
//    wprintf(L"ʾ��:\n");
//    wprintf(L"  LoadTest.exe                           # Ĭ�ϲ��� (5�߳�, 3����)\n");
//    wprintf(L"  LoadTest.exe -t 10 -i 5                # 10�߳�, 5����\n");
//    wprintf(L"  LoadTest.exe -s dc01.example.com -g    # ָ��������, �������ѯ\n");
//    wprintf(L"  LoadTest.exe -t 20 -d 500 -v           # 20�߳�, 500ms�ӳ�, ��ϸ���\n");
//    wprintf(L"\n");
//    wprintf(L"ע������:\n");
//    wprintf(L"- ��Ҫ����ԱȨ��\n");
//    wprintf(L"- �߲������Կ��ܶ����������ɸ���\n");
//    wprintf(L"- �����Ƚ���С��ģ����\n");
//}
//
//// ������
//int wmain(int argc, wchar_t* argv[]) {
//    setlocale(LC_ALL, "chs");
//    SetConsoleOutputCP(CP_UTF8);
//
//    wprintf(L"========================================\n");
//    wprintf(L"      ����������ز��Գ���\n");
//    wprintf(L"========================================\n");
//
//    if (!ParseCommandLine(argc, argv)) {
//        return 0;
//    }
//
//    // ��ʼ���ٽ���
//    InitializeCriticalSection(&g_printLock);
//
//    // �����߳�ͳ�ƽṹ
//    g_threadStats = (ThreadStats*)calloc(g_config.threadCount, sizeof(ThreadStats));
//    g_threadHandles = (HANDLE*)calloc(g_config.threadCount, sizeof(HANDLE));
//
//    if (!g_threadStats || !g_threadHandles) {
//        wprintf(L"�ڴ����ʧ��\n");
//        return 1;
//    }
//
//    // ��ʾ��������
//    wprintf(L"��������:\n");
//    wprintf(L"- Ŀ�������: %s\n", wcslen(g_config.serverName) > 0 ? g_config.serverName : L"��ǰ�������");
//    wprintf(L"- �����߳���: %d\n", g_config.threadCount);
//    wprintf(L"- ÿ�̵߳�����: %d\n", g_config.iterationsPerThread);
//    wprintf(L"- �������ӳ�: %d ms\n", g_config.delayBetweenIterations);
//    wprintf(L"- �������ѯ: %s\n", g_config.enableGroupQuery ? L"��" : L"��");
//    wprintf(L"- ��������: %s (�������: %d)\n", g_config.enableRetry ? L"��" : L"��", g_config.maxRetries);
//    wprintf(L"- ��ϸ���: %s\n", g_config.verboseOutput ? L"��" : L"��");
//    wprintf(L"========================================\n");
//
//
//    DWORD testEndTime = GetTickCount();
//    DWORD testStartTime = GetTickCount();
//    int validHandleCount = 0;
//    HANDLE* validHandles = NULL;
//
//    wprintf(L"����: �˲��Կ��ܶ����������ɸ��أ�ȷ�ϼ���? (Y/N): ");
//    int ch = _getch();
//    wprintf(L"%c\n", ch);
//    if (ch != 'Y' && ch != 'y') {
//        wprintf(L"������ȡ��\n");
//        goto cleanup;
//    }
//
//    wprintf(L"\n��ʼ���ز���...\n");
//
//    
//
//    // ���������������߳�
//    for (int i = 0; i < g_config.threadCount; i++) {
//        g_threadStats[i].threadId = i + 1;
//        g_threadStats[i].success = FALSE;
//
//        g_threadHandles[i] = (HANDLE)_beginthreadex(
//            NULL, 0, WorkerThread, &g_threadStats[i], 0, NULL);
//
//        if (!g_threadHandles[i]) {
//            wprintf(L"�����߳� %d ʧ��\n", i + 1);
//            g_config.threadCount = i; // ����ʵ���߳���
//            break;
//        }
//
//        PrintThreadMessage(0, L"�����߳� %d", i + 1);
//        Sleep(100); // ����ͬʱ������ɵĳ��
//    }
//
//    // �ȴ������߳����
//    wprintf(L"\n�ȴ������߳����...\n");
//
//    // ��ʾ����
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
//                // ���߳���ɣ��ӵȴ��б����Ƴ�
//                int completedIndex = waitResult - WAIT_OBJECT_0;
//                CloseHandle(validHandles[completedIndex]);
//
//                // �ƶ�����Ԫ��
//                for (int j = completedIndex; j < validHandleCount - 1; j++) {
//                    validHandles[j] = validHandles[j + 1];
//                }
//                validHandleCount--;
//
//                wprintf(L"�߳���ɣ�ʣ��: %d\n", validHandleCount);
//            }
//        }
//    }
//
//    free(validHandles);
//
//    
//    wprintf(L"\n�����߳�����ɣ��ܲ���ʱ��: %lu ����\n", testEndTime - testStartTime);
//
//    // ��ʾ���Խ��
//    DisplayTestResults();
//
//cleanup:
//    // ������Դ
//    if (g_threadStats) {
//        free(g_threadStats);
//    }
//    if (g_threadHandles) {
//        free(g_threadHandles);
//    }
//
//    DeleteCriticalSection(&g_printLock);
//
//    wprintf(L"\n��������˳�...\n");
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

// ���Ӳ���ģʽ
typedef enum {
    TEST_NETAPI_RAPID,      // NetAPI��������
    TEST_NETAPI_PERSISTENT, // NetAPI�־�����
    TEST_LDAP_RAPID,        // LDAP��������
    TEST_LDAP_PERSISTENT,   // LDAP�־�����
    TEST_MIXED_MODE         // ���ģʽ
} TestMode;

// �߳�ͳ����Ϣ
typedef struct ThreadStats {
    int threadId;
    TestMode mode;
    int queryCount;
    int connectionCount;
    DWORD startTime;
    DWORD endTime;
    BOOL keepRunning;
    HANDLE* connections;    // �������Ӿ��
    int maxConnections;
} ThreadStats;

// ȫ������
typedef struct ConnectionTestConfig {
    WCHAR serverName[256];
    TestMode testMode;
    int threadCount;
    int connectionsPerThread;
    int queryInterval;      // ��ѯ���(����)
    BOOL holdConnections;   // �Ƿ񱣳����Ӳ��ر�
    BOOL enableNetstat;     // �Ƿ���ʾ��������״̬
    int testDuration;       // ���Գ���ʱ��(��)
    BOOL verboseOutput;
} ConnectionTestConfig;

// ȫ�ֱ���
ConnectionTestConfig g_config = { 0 };
ThreadStats* g_threadStats = NULL;
HANDLE* g_threadHandles = NULL;
CRITICAL_SECTION g_printLock;
volatile BOOL g_stopTest = FALSE;

// ��������
unsigned int __stdcall ConnectionWorkerThread(void* param);
BOOL TestNetAPIConnections(ThreadStats* stats);
BOOL TestLDAPConnections(ThreadStats* stats);
void DisplayNetworkConnections(void);
void MonitorConnections(void);
void ShowConnectionUsage(void);
BOOL ParseConnectionTestArgs(int argc, wchar_t* argv[]);
void PrintWithTimestamp(const WCHAR* message, ...);

// ��ʱ����Ĵ�ӡ
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

// ��ʾ��ǰ��������״̬
void DisplayNetworkConnections(void) {
    PrintWithTimestamp(L"=== ��ǰ��������״̬ ===");

    // ִ��netstat������ʾ���ӵ�lsass������
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
        PrintWithTimestamp(L"LDAP������ (�˿�389): %d", connectionCount);
    }

    // ���RPC����
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
        PrintWithTimestamp(L"RPC������ (�˿�135): %d", rpcConnectionCount);
    }

    // ���߶˿�����
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
        PrintWithTimestamp(L"�߶˿������� (49xxx): %d", highPortCount);
    }
}

// NetAPI���Ӳ���
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
                PrintWithTimestamp(L"�߳�%d: NetAPI��ѯ�ɹ�, �û���: %lu",
                    stats->threadId, entriesRead);
            }

            // ������������ӣ������ͷ�
            if (!g_config.holdConnections && userInfo) {
                NetApiBufferFree(userInfo);
                userInfo = NULL;
            }
            // ����������ӣ����ⲻ�ͷ���Դ��ģ������й©��
        }
        else {
            PrintWithTimestamp(L"�߳�%d: NetAPI��ѯʧ��, ����: %lu", stats->threadId, status);
            break;
        }

        if (g_config.queryInterval > 0) {
            Sleep(g_config.queryInterval);
        }

        if (status == NERR_Success) break; // ������ѯ���
    }

    return TRUE;
}

// LDAP���Ӳ���
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

        // ׼������
        if (wcslen(g_config.serverName) > 0) {
            wcscpy_s(targetDomain, 256, g_config.serverName);
        }
        else {
            wcscpy_s(targetDomain, 256, L"localhost");
        }

        // ����LDAP����
        pLdap = ldap_initW(targetDomain, LDAP_PORT);
        if (!pLdap) {
            PrintWithTimestamp(L"�߳�%d: LDAP��ʼ��ʧ��", stats->threadId);
            break;
        }

        // ����LDAPѡ��
        ULONG version = LDAP_VERSION3;
        ldap_set_optionW(pLdap, LDAP_OPT_PROTOCOL_VERSION, &version);

        // ��
        result = ldap_bind_sW(pLdap, NULL, NULL, LDAP_AUTH_NEGOTIATE);
        if (result != LDAP_SUCCESS) {
            PrintWithTimestamp(L"�߳�%d: LDAP��ʧ��, ����: %lu", stats->threadId, result);
            ldap_unbind(pLdap);
            break;
        }

        // ִ�в�ѯ
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
                PrintWithTimestamp(L"�߳�%d: LDAP��ѯ�ɹ�, ��Ŀ��: %lu",
                    stats->threadId, entryCount);
            }

            if (pResult) {
                ldap_msgfree(pResult);
            }
        }
        else {
            PrintWithTimestamp(L"�߳�%d: LDAP��ѯʧ��, ����: %lu", stats->threadId, result);
        }

        // ���Ӵ������
        if (g_config.holdConnections) {
            // �������Ӳ��رգ�ģ������й©��
            ldapConnections[stats->connectionCount - 1] = pLdap;
            PrintWithTimestamp(L"�߳�%d: ����LDAP���� #%d ��",
                stats->threadId, stats->connectionCount);
        }
        else {
            // �����ر�����
            ldap_unbind(pLdap);
        }

        if (g_config.queryInterval > 0) {
            Sleep(g_config.queryInterval);
        }
    }

    // ���������Ҫ��
    if (ldapConnections) {
        stats->connections = (HANDLE*)ldapConnections;
    }

    return TRUE;
}

// ���ӹ����߳�
unsigned int __stdcall ConnectionWorkerThread(void* param) {
    ThreadStats* stats = (ThreadStats*)param;

    PrintWithTimestamp(L"�߳�%d����: ģʽ=%d, Ŀ��������=%d",
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
        // ���ģʽ��һ��NetAPI��һ��LDAP
        if (stats->threadId % 2 == 0) {
            TestNetAPIConnections(stats);
        }
        else {
            TestLDAPConnections(stats);
        }
        break;
    }

    stats->endTime = GetTickCount();

    PrintWithTimestamp(L"�߳�%d���: ��ѯ��=%d, ������=%d, ��ʱ=%lums",
        stats->threadId, stats->queryCount, stats->connectionCount,
        stats->endTime - stats->startTime);

    return 0;
}

// ���Ӽ���߳�
void MonitorConnections(void) {
    PrintWithTimestamp(L"��ʼ���Ӽ��...");

    DWORD startTime = GetTickCount();

    while (!g_stopTest) {
        Sleep(5000); // ÿ5����һ��

        if (g_config.enableNetstat) {
            DisplayNetworkConnections();
        }

        // �����Գ���ʱ��
        if (g_config.testDuration > 0) {
            DWORD elapsed = (GetTickCount() - startTime) / 1000;
            if (elapsed >= g_config.testDuration) {
                PrintWithTimestamp(L"�ﵽ����ʱ������ (%d��), ֹͣ����", g_config.testDuration);
                g_stopTest = TRUE;

                // ֪ͨ�����߳�ֹͣ
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

// ���������в���
BOOL ParseConnectionTestArgs(int argc, wchar_t* argv[]) {
    // Ĭ������
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

// ��ʾ����
void ShowConnectionUsage(void) {
    wprintf(L"����й©���Գ���\n");
    wprintf(L"�÷�: ConnectionTest.exe [ѡ��]\n\n");
    wprintf(L"ѡ��:\n");
    wprintf(L"  -s, --server <����>        Ŀ�������\n");
    wprintf(L"  -m, --mode <0-4>           ����ģʽ:\n");
    wprintf(L"                             0=NetAPI���� 1=NetAPI�־�\n");
    wprintf(L"                             2=LDAP����   3=LDAP�־�\n");
    wprintf(L"                             4=���ģʽ\n");
    wprintf(L"  -t, --threads <����>       �߳��� (Ĭ��:10)\n");
    wprintf(L"  -c, --connections <����>   ÿ�߳������� (Ĭ��:5)\n");
    wprintf(L"  -i, --interval <����>      ��ѯ��� (Ĭ��:1000)\n");
    wprintf(L"  -d, --duration <��>        ���Գ���ʱ�� (Ĭ��:30)\n");
    wprintf(L"  --close-connections        �����ر�����\n");
    wprintf(L"  --no-netstat               ����netstat���\n");
    wprintf(L"  -v, --verbose              ��ϸ���\n\n");
    wprintf(L"ʾ��:\n");
    wprintf(L"  ConnectionTest.exe                    # Ĭ��LDAP�־����Ӳ���\n");
    wprintf(L"  ConnectionTest.exe -m 3 -t 20         # 20�߳�LDAP�־�����\n");
    wprintf(L"  ConnectionTest.exe --close-connections # �����ر�����ģʽ\n");
}

// ������
int wmain(int argc, wchar_t* argv[]) {
    setlocale(LC_ALL, "chs");
    SetConsoleOutputCP(CP_UTF8);

    wprintf(L"========================================\n");
    wprintf(L"        ����й©���Գ���\n");
    wprintf(L"========================================\n");

    if (!ParseConnectionTestArgs(argc, argv)) {
        return 0;
    }

    InitializeCriticalSection(&g_printLock);

    // ��ʾ��������
    const wchar_t* modeNames[] = {
        L"NetAPI��������", L"NetAPI�־�����",
        L"LDAP��������", L"LDAP�־�����", L"���ģʽ"
    };

    PrintWithTimestamp(L"��������:");
    wprintf(L"  - ������: %s\n", wcslen(g_config.serverName) > 0 ? g_config.serverName : L"localhost");
    wprintf(L"  - ����ģʽ: %s\n", modeNames[g_config.testMode]);
    wprintf(L"  - �߳���: %d\n", g_config.threadCount);
    wprintf(L"  - ÿ�߳�������: %d\n", g_config.connectionsPerThread);
    wprintf(L"  - ��ѯ���: %d ms\n", g_config.queryInterval);
    wprintf(L"  - ��������: %s\n", g_config.holdConnections ? L"��" : L"��");
    wprintf(L"  - ����ʱ��: %d ��\n", g_config.testDuration);

    // ��ʾ��ʼ����״̬
    PrintWithTimestamp(L"=== ����ǰ������״̬ ===");
    DisplayNetworkConnections();

    // ������Դ
    g_threadStats = (ThreadStats*)calloc(g_config.threadCount, sizeof(ThreadStats));
    g_threadHandles = (HANDLE*)calloc(g_config.threadCount, sizeof(HANDLE));

    if (!g_threadStats || !g_threadHandles) {
        PrintWithTimestamp(L"�ڴ����ʧ��");
        return 1;
    }

    PrintWithTimestamp(L"��ʼ���Ӳ���...");

    // ���������߳�
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
            Sleep(50); // ������ʱ��
        }
    }

    // �������
    _beginthreadex(NULL, 0, (unsigned int(__stdcall*)(void*))MonitorConnections, NULL, 0, NULL);

    // �ȴ�������ɻ��û��ж�
    PrintWithTimestamp(L"����������... ���������ǰֹͣ");

    while (!g_stopTest) {
        if (_kbhit()) {
            _getch();
            PrintWithTimestamp(L"�û��жϲ���");
            g_stopTest = TRUE;
            break;
        }
        Sleep(100);
    }

    // ֹͣ�����߳�
    for (int i = 0; i < g_config.threadCount; i++) {
        if (g_threadStats) {
            g_threadStats[i].keepRunning = FALSE;
        }
    }

    // �ȴ��߳̽���
    if (g_threadHandles) {
        WaitForMultipleObjects(g_config.threadCount, g_threadHandles, TRUE, 5000);
        for (int i = 0; i < g_config.threadCount; i++) {
            if (g_threadHandles[i]) {
                CloseHandle(g_threadHandles[i]);
            }
        }
    }

    // ��ʾ��������״̬
    PrintWithTimestamp(L"=== ���Ժ������״̬ ===");
    DisplayNetworkConnections();

    // ����
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

    PrintWithTimestamp(L"�������");
    wprintf(L"��������˳�...\n");
    _getch();

    return 0;
}
