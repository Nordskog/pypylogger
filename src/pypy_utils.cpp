#include <sstream>
#include <iomanip>
#include <cmath>

#include "pypy_utils.hpp"

using namespace std;

chrono::system_clock::time_point vrchatLogTimeToTimePoint( string _time )
{
    // 2023.04.12 23:20:00
    tm tm = {};
    stringstream(_time) >> get_time(&tm, "%Y.%m.%d %H:%M:%S");
    return chrono::system_clock::from_time_t(mktime(&tm));
};

chrono::system_clock::time_point vrchatLogFilenameTimeToTimePoint( string _time )
{
    // 2023.04.12 23:20:00
    tm tm = {};
    stringstream(_time) >> get_time(&tm, "%Y-%m-%d_%H-%M-%S");
    return chrono::system_clock::from_time_t(mktime(&tm));
};

long long timestampToUnixTime( chrono::system_clock::time_point time)
{
    chrono::milliseconds ms = chrono::duration_cast<chrono::milliseconds>(time.time_since_epoch());
    return ms.count() / (long long)1000;
}

long long durationToSeconds( chrono::system_clock::duration dur  )
{
    return chrono::duration_cast<chrono::seconds>(dur).count();
};


string formatDuration( chrono::system_clock::duration dur  )
{
    bool isNegative = dur < chrono::system_clock::duration::zero();

    stringstream ss;
    ss.fill('0');

    if (isNegative)
        ss << "-";

    auto h = chrono::duration_cast<chrono::hours>(dur);
    dur -= h;
    auto m = chrono::duration_cast<chrono::minutes>(dur);
    dur -= m;
    auto s = chrono::duration_cast<chrono::seconds>(dur);
    ss  << setw(2) << abs(h.count()) << ":"
        << setw(2) << abs(m.count()) << ":"
        << setw(2) << abs(s.count());
    ss.fill('0');

    return ss.str();
};