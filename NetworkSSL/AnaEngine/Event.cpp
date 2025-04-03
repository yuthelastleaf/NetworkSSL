#include "Event.h"
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <ctime>

namespace malware_analysis {
	
	APTEvent::APTEvent(std::string json)
	{
		event_prop_.assign(EventProp::COUNT, "");


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
			if (type != EVENT_NULL && type != event_type_) {
				break;
			}

			if (epindex >= event_prop_.size()) {
				break;
			}

			if (event_prop_[epindex].length() < match_value.length()) {
				break;
			}

			switch (match) {
			case MATCH_CONTAINS:
				flag = (event_prop_[epindex].find(match_value) != std::string::npos);
				break;
			case MATCH_START:
				flag = (event_prop_[epindex].find(match_value) == 0);
				break;
			case MATCH_END:
				flag = (event_prop_[epindex].substr(event_prop_[epindex].length() - match_value.length(), match_value.length()) == match_value);
				break;
			}
		} while (0);

		return flag;
	}
} // namespace malware_analysis