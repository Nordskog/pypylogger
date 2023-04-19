#pragma once

#include <chrono>
#include <string>

using namespace std;

chrono::system_clock::time_point vrchatLogTimeToTimePoint( string _time );
chrono::system_clock::time_point vrchatLogFilenameTimeToTimePoint( string _time );

long long durationToSeconds( chrono::system_clock::duration dur  );
string formatDuration( chrono::system_clock::duration dur  );