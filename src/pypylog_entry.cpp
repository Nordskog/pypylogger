#include <chrono>
#include <string>
#include <iomanip>
#include <sstream>
#include <regex>

#include "pypy_utils.hpp"

using namespace std;

#include "pypylog_entry.hpp"

const std::regex YOUTUBE_REGEX_PATTERN(R"((?:http?s:\/\/)(?:www.)?(?:(?:youtube.com\/watch\?v=)|(?:youtu.be\/))([\w\d_-]+))");

    PyPylogEntry::PyPylogEntry()
    {

    };

    PyPylogEntry::PyPylogEntry(string _time, string _url, string _title )
    {
        this->title = _title;
        this->url = _url;
        this->time = vrchatLogTimeToTimePoint(_time);
        populateYoutubeId();
    };

    PyPylogEntry::PyPylogEntry( chrono::system_clock::time_point _time, string _url, string _title )
    {
        this->title = _title;
        this->url = _url;
        this->time = _time;
        populateYoutubeId();
    };

    void PyPylogEntry::populateYoutubeId()
    {
	    std::smatch match;
        if (regex_search(this->url, match, YOUTUBE_REGEX_PATTERN))
        {
            this->youtubeId = match[1].str();
        }
    }

    bool PyPylogEntry::needsTitleLookup()
    {
        return this->title.find("Playing Custom URL:") != string::npos;
    };

    bool PyPylogEntry::isBeforeStart()
    {
        return this->videoTime < chrono::system_clock::duration::zero();
    }

    string PyPylogEntry::toString()
    {
        stringstream ss;
        ss << formatDuration(videoTime) << ", " << this->url << ", " << this->title; 
        return ss.str();
    }

    void PyPylogEntry::setVideoStartTime( chrono::system_clock::time_point startTime )
    {
        this->videoTime = this->time - startTime;
    }
