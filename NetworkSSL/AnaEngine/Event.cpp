#include "Event.h"
#include <sstream>
#include <iomanip>
#include <ctime>

#include "../../include/CJSON/CJSONHanler.h"

namespace malware_analysis {
	
	EventType ToEventType(std::string_view str) {
		if (auto it = String2EventType.find(str); it != String2EventType.end()) {
			return it->second;
		}
		return EventType::COUNT;
	}

	EventProp ToEventProp(std::string_view str) {
		if (auto it = String2EventProp.find(str); it != String2EventProp.end()) {
			return it->second;
		}
		return EventProp::COUNT;
	}

	MatchType ToMatchType(std::string_view str) {
		if (auto it = String2MatchType.find(str); it != String2MatchType.end()) {
			return it->second;
		}
		return MatchType::COUNT;
	}

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

		event_type_ = static_cast<unsigned int>(ToEventType(getcontent("eventtype")));

		for (auto& it : String2EventProp) {
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
			case MatchType::MATCH_COMPLETE:
				flag = (event_prop_[static_cast<int>(epindex)] == match_value);
			}
		} while (0);

		return flag;
	}
} // namespace malware_analysis