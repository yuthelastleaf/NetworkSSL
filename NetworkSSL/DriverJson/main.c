#include "../../include/CJSON/CDriverJson.h"

#include <stdio.h>

int main() {

	while (1)
	{
		cDriverJSON* root = NULL;
		InitDriverJSON(&root, NULL, NULL);

		cDriverJSON* obj = root->get(root, "/heihei/bushiba");
		obj->setstring(obj, "test");

		obj = root->get(root, "bushi/ceshi/");
		obj->setint(obj, 12);

		char* str = root->getjsonstring(root);
		printf("%s\n", str);

		cJSON_free(str);
		cJSON_Delete(root->json_obj);

		// 6. ½âÎöJSON×Ö·û´®
		const char* json_data = "{\"name\":\"OpenAI\",\"type\":\"Research\",\"employees\":1000}";
		cJSON* parsed_json = cJSON_Parse(json_data);

		cDriverJSON* parse_root = NULL;
		InitDriverJSON(&parse_root, parsed_json, NULL);

		cDriverJSON* obj_new = parse_root->get(parse_root, "test/newname");
		obj_new->setstring(obj_new, "new");

		obj_new = parse_root->get(parse_root, "type");
		obj_new->setint(obj_new, 26);

		char* parse_str = parse_root->getjsonstring(parse_root);
		printf("%s\n", parse_str);

		cJSON_free(parse_str);
		cJSON_Delete(parse_root->json_obj);

	}

	getchar();

}
