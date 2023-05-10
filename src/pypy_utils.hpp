#pragma once

#include <chrono>
#include <string>


std::string getSimpleDate( std::chrono::system_clock::time_point inputTime);
std::wstring ConvertUtf8ToWide(const std::string& str);
std::string cleanFilename(std::string & filename); 
std::chrono::system_clock::time_point vrchatLogTimeToTimePoint( std::string _time );
std::chrono::system_clock::time_point vrchatLogFilenameTimeToTimePoint( std::string _time );

long long timestampToUnixTime( std::chrono::system_clock::time_point time  );
long long durationToSeconds( std::chrono::system_clock::duration dur  );
std::string formatDuration( std::chrono::system_clock::duration dur  );