#include <sstream>
#include <iomanip>
#include <cmath>
#include <regex>
#include <locale>
#include <codecvt>
#include <Windows.h>

#include "pypy_utils.hpp"

using namespace std;

std::string getSimpleDate( chrono::system_clock::time_point inputTime)
{
    // Convert to a local time_t
    std::time_t time = std::chrono::system_clock::to_time_t(inputTime);

    // Convert to a tm struct for local time
    std::tm* tm = std::localtime(&time);

    // Format the date as DD/MM
    char buffer[6];
    std::strftime(buffer, sizeof(buffer), "%d-%m", tm);

    return string(buffer);
}


// Convert a string to a wide string in UTF-8 encoding
std::wstring ConvertUtf8ToWide(const std::string& str)
{
    int length =  (int)str.length();
    int count = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), length, NULL, 0);
    std::wstring wstr(count, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), length, &wstr[0], count);
    return wstr;
}

string cleanFilename(string & filename) 
{ 
    std::string illegalChars = "<>:\"/\\|?*";
    std::string cleanName = filename;

    for (auto c : illegalChars) {
        std::replace(cleanName.begin(), cleanName.end(), c, '_');
    }

    return cleanName;
}

chrono::system_clock::time_point vrchatLogTimeToTimePoint( string _time )
{
    // 2023.04.12 23:20:00
    tm tm = {};
    tm.tm_isdst = -1; // Let the system figure out DST
    stringstream(_time) >> get_time(&tm, "%Y.%m.%d %H:%M:%S");

    std::time_t t = mktime(&tm);
    std::chrono::system_clock::time_point tp =std::chrono::system_clock::from_time_t(t);
    return tp;
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

string timepointToString( chrono::system_clock::time_point inputTime )
{
    std::time_t now_tt = std::chrono::system_clock::to_time_t(inputTime);
    std::tm tm = *std::localtime(&now_tt);

    return (ostringstream() << std::put_time(&tm, "%c %Z")).str();
}

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