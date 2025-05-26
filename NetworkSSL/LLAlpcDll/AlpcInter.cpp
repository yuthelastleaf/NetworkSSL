#include "AlpcInter.h"
#include "../../include/Alpc/alpc_util.h"

ALPC_API bool ALPC_CALL start_server(const char* port_name)
{
	return AlpcConn::getInstance().start_server(port_name);
}

ALPC_API bool ALPC_CALL notify_msg(const char* port_name, const char* json, bool post)
{
	CJSONHandler json_post(json);
	return AlpcConn::getInstance().notify_msg(port_name, json_post, post);
}
