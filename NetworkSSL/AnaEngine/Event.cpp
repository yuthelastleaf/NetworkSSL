#include "Event.h"
#include <sstream>
#include <iomanip>
#include <ctime>

#include "../../include/CJSON/CJSONHanler.h"

namespace malware_analysis {
	
	APTEvent::APTEvent(std::string json)
	{
		event_prop_.assign(static_cast<int>(EventProp::COUNT), "");

		CJSONHandler event_json(json.c_str());
		
		auto getcontent = [&](std::string str_name) {
			const char* str = event_json[str_name.c_str()].GetString();
			std::string content;
			if (str) {
				content = str;
			}
			return content;
		};

		event_type_ = static_cast<unsigned int>(String2EventType[getcontent("eventtype").c_str()]);

		for (auto it : String2EventProp) {
			event_prop_[static_cast<int>(it.second)] = getcontent(it.first.data());
		}
	}

	APTEvent::~APTEvent()
	{
	}


	bool APTEvent::Match(EventType type, MatchType match,
		EventProp epindex, std::string match_value)
	{
		bool flag = false;

		do
		{
			if (type != EventType::EVENT_NULL && type != static_cast<EventType>(event_type_)) {
				break;
			}

			if (static_cast<int>(epindex) >= event_prop_.size()) {
				break;
			}

			if (event_prop_[static_cast<int>(epindex)].length() < match_value.length()) {
				break;
			}

			switch (match) {
			case MatchType::MATCH_CONTAINS:
				flag = (event_prop_[static_cast<int>(epindex)].find(match_value) != std::string::npos);
				break;
			case MatchType::MATCH_START:
				flag = (event_prop_[static_cast<int>(epindex)].find(match_value) == 0);
				break;
			case MatchType::MATCH_END:
				flag = (event_prop_[static_cast<int>(epindex)].substr(event_prop_[static_cast<int>(epindex)].length() - match_value.length(), match_value.length()) == match_value);
				break;
			}
		} while (0);

		return flag;
	}
} // namespace malware_analysis