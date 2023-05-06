#pragma once

#include <chrono>
#include <string>

std::chrono::system_clock::time_point vrchatLogTimeToTimePoint( std::string _time );
std::chrono::system_clock::time_point vrchatLogFilenameTimeToTimePoint( std::string _time );

long long durationToSeconds( std::chrono::system_clock::duration dur  );
std::string formatDuration( std::chrono::system_clock::duration dur  );