#pragma once

#include <chrono>
#include <string>
#include <regex>


extern const std::regex YOUTUBE_REGEX_PATTERN;


class PyPylogEntry
{
    public:
    std::chrono::system_clock::time_point time;
    std::chrono::system_clock::duration videoTime;
    std::string youtubeId;
    std::string url;
    std::string title;

    PyPylogEntry();
    PyPylogEntry(std::string _time, std::string _url, std::string _title );
    PyPylogEntry(std::chrono::system_clock::time_point _time, std::string _url, std::string _title );
    
    std::string toString();
    std::string toLogString();
    bool needsTitleLookup();

    std::string getYoutubeChapterString();
    std::chrono::system_clock::duration getTimeFrom( std::chrono::system_clock::time_point fromTime );
    long long getTimeAsUnixTimestamp();
    bool isBeforeStart();

    void setVideoStartTime( std::chrono::system_clock::time_point startTime );
    void populateYoutubeId();

    private:


};