#include <chrono>
#include <string>
#include <iomanip>
#include <sstream>

#include "pypy_utils.hpp"

using namespace std;

#include "pypylog_entry.hpp"


    PyPylogEntry::PyPylogEntry()
    {

    };

    PyPylogEntry::PyPylogEntry(string _time, string _url, string _title )
    {
        this->title = _title;
        this->url = _url;
        this->time = vrchatLogTimeToTimePoint(_time);
    };

    PyPylogEntry::PyPylogEntry( chrono::system_clock::time_point _time, string _url, string _title )
    {
        this->title = _title;
        this->url = _url;
        this->time = _time;
    };

    bool PyPylogEntry::needsTitleLookup()
    {
        return this->title.find("Playing Custom URL:") != string::npos;
    };

    long long PyPylogEntry::getTimeAsUnixTimestamp()
    {
        chrono::milliseconds ms = chrono::duration_cast<chrono::milliseconds>(this->time.time_since_epoch());
        return ms.count() / (long long)1000;
    }

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
