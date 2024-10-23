#include "../../include/CJSON/CDriverJson.h"

#include <stdio.h>

int main() {

	cDriverJSON* root = NULL;
	InitDriverJSON(&root, NULL, NULL);

	cDriverJSON* obj = root->get(root, "/heihei/bushiba");
	obj->setstring(obj, "test");

	obj = root->get(root, "bushi/ceshi/");
	obj->setint(obj, 12);

	printf("%s\n", root->getjsonstring(root));
	getchar();

}
