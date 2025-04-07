#include "yaml-cpp/yaml.h"

#include <iostream>

#pragma comment(lib, "yaml-cppd.lib")

const char yaml_content[] =
"title: Access To Potentially Sensitive Sysvol Files By Uncommon Applications\n"
"id: d51694fe-484a-46ac-92d6-969e76d60d10\n"
"related:\n"
"    - id: 8344c19f-a023-45ff-ad63-a01c5396aea0\n"
"      type: derived\n"
"status: experimental\n"
"description: Detects file access requests to potentially sensitive files hosted on the Windows Sysvol share.\n"
"references:\n"
"    - https://github.com/vletoux/pingcastle\n"
"author: frack113\n"
"date: 2023-12-21\n"
"modified: 2024-07-29\n"
"tags:\n"
"    - attack.credential-access\n"
"    - attack.t1552.006\n"
"logsource:\n"
"    category: file_access\n"
"    product: windows\n"
"    definition: 'Requirements: Microsoft-Windows-Kernel-File ETW provider'\n"
"detection:\n"
"    selection:\n"
"        FileName|startswith: '\\\\'\n"
"        FileName|contains|all:\n"
"            - '\\\\sysvol\\\\'\n"
"            - '\\\\Policies\\\\'\n"
"        FileName|endswith:\n"
"            - 'audit.csv'\n"
"            - 'Files.xml'\n"
"            - 'GptTmpl.inf'\n"
"            - 'groups.xml'\n"
"            - 'Registry.pol'\n"
"            - 'Registry.xml'\n"
"            - 'scheduledtasks.xml'\n"
"            - 'scripts.ini'\n"
"            - 'services.xml'\n"
"    filter_main_generic:\n"
"        Image|startswith:\n"
"            - 'C:\\\\Program Files (x86)\\\\'\n"
"            - 'C:\\\\Program Files\\\\'\n"
"            - 'C:\\\\Windows\\\\system32\\\\'\n"
"            - 'C:\\\\Windows\\\\SysWOW64\\\\'\n"
"    filter_main_explorer:\n"
"        Image: 'C:\\\\Windows\\\\explorer.exe'\n"
"    condition: selection and not 1 of filter_main_*\n"
"falsepositives:\n"
"    - Unknown\n"
"level: medium\n";

int main() {

	YAML::Node config = YAML::Load(yaml_content);

	std::cout << "Node type " << config["title"].Type() << std::endl;
	std::cout << config["title"].as<std::string>() << std::endl;

	int tetp = config["detection"].Type();
	// 迭代器遍历
	for (YAML::const_iterator it = config["detection"].begin(); it != config["detection"].end(); ++it)
	{
		std::cout << it->first.as<std::string>();

		YAML::Node detail = config["detection"][it->first.as<std::string>()];

		if (it->second.Type() != YAML::NodeType::Map) {
			std::cout << " :" << it->second.as<std::string>();
		}
		int tt = it->first.Type();

		std::cout << std::endl;

		for (YAML::const_iterator sit = it->second.begin();
			sit != it->second.end(); ++sit) {
			std::cout << "  " << sit->first.as<std::string>();

			if (sit->second.Type() == YAML::NodeType::Scalar || sit->second.Type() == YAML::NodeType::Map) {
				std::cout << " : " << sit->second.as<std::string>() << std::endl;
			}
			else if (sit->second.Type() == YAML::NodeType::Sequence) {
				for (int i = 0; i < sit->second.size(); i++)
				{
					//注意：这里如果确定Sequence中每个元素都是Scalar类型的，可以使用as()进行访问，否则需要使用IsXXX()先判断类型
					std::string tmp_meta = sit->second[i].as<std::string>();
					std::cout << "  -" << tmp_meta << std::endl;
				}
			}

		}
	}

	return 0;
}
