#pragma once

#include <chrono>
#include <string>

using namespace std;

class PyPylogEntry
{
    public:
    chrono::system_clock::time_point time;
    chrono::system_clock::duration videoTime;
    string url;
    string title;

    PyPylogEntry();
    PyPylogEntry(string _time, string _url, string _title );
    PyPylogEntry(chrono::system_clock::time_point _time, string _url, string _title );
    
    std::string toString();
    bool needsTitleLookup();

    chrono::system_clock::duration PyPylogEntry::getTimeFrom( chrono::system_clock::time_point fromTime );
    long long PyPylogEntry::getTimeAsUnixTimestamp();
    bool isBeforeStart();

    void setVideoStartTime( chrono::system_clock::time_point startTime );

    private:

};