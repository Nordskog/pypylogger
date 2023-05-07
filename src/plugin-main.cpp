/*
Plugin Name
Copyright (C) <Year> <Developer> <Email Address>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs.h>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <unordered_map>

#include "pypylog_entry.hpp"
#include "pypy_utils.hpp"
#include "pypy_web.hpp"

#include "plugin-macros.generated.h"
#include "plugin-main.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

bool finishReading = false;

using namespace std;

std::chrono::time_point recordStartTime = std::chrono::system_clock::now();
std::chrono::time_point recordEndTime = std::chrono::system_clock::now();

// 2023.04.12 23:11:37 Log        -  [VRCX] VideoPlay(VRDancing) "https://www.youtube.com/watch?v=28_GkwWU7-o",0,0,3654,"Roughy~","[ Michael Jackson: The Experience ] Smooth Criminal - Michael Jackson"
// /(\S+\s\S+)\s+\w+\s+-\s+\[VRCX\]\s+VideoPlay\(VRDancing\)\s+"([^"]+)"[\d\.\+\-,]+"([^"]+)","([^"]+)"/
std::regex log_entry_regex(R"!((\S+\s\S+)\s+\w+\s+-\s+\[VRCX\]\s+VideoPlay\(VRDancing\)\s+"([^"]+)"[\d\.\+\-,]+"([^"]+)","([^"]+)")!");

// output_log_2023-04-12_23-08-58.txt
// /output_log_(\d{4}-\d{2}-\d{2}_\d{2}-\d{2}-\d{2})\.txt/
std::regex log_file_regex(R"!(output_log_(\d{4}-\d{2}-\d{2}_\d{2}-\d{2}-\d{2})\.txt)!" );

auto PREV_SESSION_CUTOFF = chrono::minutes(15);

vector< pair< string,chrono::system_clock::time_point> > get_files()
{
	auto VRCHAT_LOGS_DIR = "C:\\Users\\Roughy\\workspace\\pypylogger\\data\\logs";

	vector< pair< string,chrono::system_clock::time_point> > logFiles;

	std::cmatch match;
	for ( const auto& entry : filesystem::directory_iterator(VRCHAT_LOGS_DIR))
	{
		if (entry.is_directory())
			continue;

		//blog( LOG_INFO, "Checking file: %s", entry.path().string().c_str());

		bool matched = std::regex_match( entry.path().filename().string().c_str(), match, log_file_regex, std::regex_constants::match_default);
		if ( matched )
		{
			if ( match.size() == 2 )
			{
				// #0 matched string
				// #1 2023-04-12_23-08-58
				std::string logfileTimeRaw = match[1].str();
				auto logFileTime = vrchatLogFilenameTimeToTimePoint(logfileTimeRaw);

				logFiles.push_back(std::pair(entry.path().string(), logFileTime ) );

				//blog( LOG_INFO, "LogfileTime 1: %s", logfileTimeRaw.c_str() );
			}
			else 
			{
				blog( LOG_INFO, "Log file named matched but unexpected match count was: %i", match.size() );	
			}
		}
		else 
		{
			//blog( LOG_INFO, "Not a match");
		}
	}

	// Sort them so older goes first
	sort(logFiles.begin(), logFiles.end(), 
		[](const auto& a, const auto& b) 
		{ return a.first < b.first; } 
	);

	blog( LOG_INFO, "Found %i logfiles", logFiles.size());

	return logFiles;
}

void read_files( vector< pair< string,chrono::system_clock::time_point> > logFiles, std::vector<PyPylogEntry>& logEntries )
{
	for (const auto& logFile : logFiles)
	{
		blog( LOG_INFO, "Parsing logfile %s", logFile.first.c_str());

		std::ifstream file(logFile.first);
		if (file.is_open())
		{
			std::string line;
			std::cmatch match;
			PyPylogEntry* prevEntry = nullptr;
			while (std::getline(file, line)) 
			{
				if (line.find( "[VRCX] VideoPlay(VRDancing)" ) != std::string::npos)
				{
					//blog( LOG_INFO, "Line: %s", line.c_str() );
					bool matched = std::regex_match(line.c_str(), match, log_entry_regex, std::regex_constants::match_default);
					if ( matched )
					{
						if ( match.size() == 5 )
						{
							// #0 matched string
							// #1 time 2023.04.12 23:20:00
							// #2 youtube url
							// #3 requester name
							// #4 title

							std::string time = 	match[1].str();
							std::string url = 	match[2].str();
							std::string title = match[4].str();

							//blog( LOG_INFO, "Time is: %s", time.c_str() );
							//blog( LOG_INFO, "Url 2: %s", url.c_str() );
							//blog( LOG_INFO, "Title 3: %s", title.c_str() );

							PyPylogEntry newEntry = PyPylogEntry(time, url, title);
							newEntry.setVideoStartTime(recordStartTime);

							//blog( LOG_INFO, "Epoch time is: %i", newEntry.getTimeAsUnixTimestamp() );

							if (prevEntry != nullptr)
							{
								// same url, same title
								if ( prevEntry->url == newEntry.url && prevEntry->title == newEntry.title )
								{
									// Skip item if same url and title as previous
									// There will be duplicates in the log, and we don't want to cut if it's the same song anyway
									continue;
								}

								// same title, different url
								if ( prevEntry->url != newEntry.url && prevEntry->title == newEntry.title )
								{
									// After a period of inactivity, pypy will start playing a filler video with music.
									// The log entry for this will have the correct url ( youtube ), but the title is not updated.
									// This seems to occur after roughly 8 minutes, which is way longer than most songs.
									// If entry is same title as previous entry, but has a different url, and there is more than 6 minutes 
									// since the previous video started playing, give it a dummy custom url title so the title will be grabbed from youtube later.
									if ( (newEntry.time - prevEntry->time) > chrono::minutes(6) )
									{
										blog( LOG_INFO, "Same title but different url, treating as custom url: %s", newEntry.toString().c_str());

										ostringstream oss;
										oss << "Playing Custom URL: " << newEntry.url;
										newEntry.title = oss.str();
									}
								}
							}

							if ( newEntry.time > recordEndTime)
							{
								blog( LOG_INFO, "Found entry after recording end time. Breaking.");	
								break;
							}

							logEntries.push_back( newEntry );
							prevEntry = &(logEntries.back());
						}
						else 
						{
							blog( LOG_INFO, "Unexpected match count was: %i", match.size() );	
						}
					}
					else 
					{
							blog( LOG_INFO, "No match");	
					}
				}

			}
			file.close();
		} 
		else 
		{
			blog( LOG_INFO, "Could not open VRChat log at path: %s", logFile.first.c_str() );	
		}
	}
}

/**
 * videoIds may be a single id, or multiple comma-delimited ids
*/
bool queryYoutubeTitlesSingle(string videoIds, unordered_map<string,string>& idTitleMap )
{
	blog( LOG_INFO, "getting title of IDs: %s", videoIds.c_str() );

	string api_key = YOUTUBE_API_KEY;

	std::ostringstream oss;
	oss 
	<< R"(https://youtube.googleapis.com/youtube/v3/videos?part=snippet%2CcontentDetails%2Cstatistics&id=)" 
	<< videoIds 
	<< "&key="
	<< api_key;

	string lookupUrl = oss.str();
	blog( LOG_INFO, "Will query url: %s", lookupUrl.c_str() );

	
	nlohmann::json res;
	if ( pypyweb::httpGetJson( lookupUrl, res))
	{
		if (!res.contains("items"))
		{
			blog( LOG_INFO, "Json missing [items]" );
			return false;
		}
		
		nlohmann::json items = res["items"];
		for(int i = 0; i < items.size(); i++)
		{
			nlohmann::json item = items[i];

			if (!item.contains("id"))
			{
				blog( LOG_INFO, "Json missing [id]" );
				continue;
			}

			string id = item["id"];

			if (!item.contains("snippet"))
			{
				blog( LOG_INFO, "Json missing [snippet]" );
				continue;
			}

			nlohmann::json snippet = item["snippet"];

			if (!snippet.contains("title") )
			{
				blog( LOG_INFO, "Json missing [title]" );
				continue;
			}

			string title = snippet["title"];

			idTitleMap[id] = title;

		}

		return true;
	}
	
	return false;


}

void queryYoutubeTitlesAll(unordered_map<string,string>& idTitleMap)
{
	ostringstream oss;
	int iterator = 0;
	for (auto& entry : idTitleMap)
	{
		oss << entry.first << ",";
		iterator++;

		// looks like youtube may allow up to 50 results, so split into chunks
		if (iterator >= 49)
		{
			// Lookup however many we have
			queryYoutubeTitlesSingle(oss.str(), idTitleMap);

			// clear and repeat
			oss.clear();
			iterator = 0;
		}
	}

	// Handle any remaining entries below the 49 entry count
	if (oss.tellp() > 0)
	{
		queryYoutubeTitlesSingle(oss.str(), idTitleMap);
	}
}

void populateCustomUrlTitles(std::vector<PyPylogEntry>& logEntries )
{
	blog( LOG_INFO, "Entry count is: %i", logEntries.size() );	

	// Chuck all the video ids we want to look up into a map
	unordered_map<string,string> idTitleMap;

	for( auto& entry : logEntries)
	{
		if (entry.needsTitleLookup() && !entry.youtubeId.empty() )
		{
			// Leave url as default "title"
			idTitleMap[entry.youtubeId] = entry.url;
		}
	}

	// Query youtube for titles
	queryYoutubeTitlesAll(idTitleMap);

	// Populate entries from results
	for( auto& entry : logEntries)
	{
		if (entry.needsTitleLookup() && !entry.youtubeId.empty() )
		{
			// Can assume that key will exist, since we populated it this way too
			entry.title = idTitleMap.at(entry.youtubeId);
		}
	}
}

void filterZeroDurationEntries( std::vector<PyPylogEntry>& entries )
{
	// Occasionally there will be an extra log entry at the same time as a new video plays.
	// This entry will use a song that was played previously with a mismatched requester.
	// Since they occur at the same moment as a new video actually plays, the entry will be 0 seconds.
	vector<PyPylogEntry>::iterator it = entries.begin();

	while (it != entries.end())
	{
		vector<PyPylogEntry>::iterator itnext = it + 1;
		if (itnext != entries.end())
		{
			chrono::system_clock::duration timeDiff = itnext->time - it->time;
			
			// Nuke entry if less than 2 seconds to next entry
			if ( timeDiff < chrono::seconds(2))
			{
				blog(LOG_INFO, "Found very short entry. Deleting: %s", it->toString().c_str());
				it = entries.erase(it);
				continue;
			}
		}

		++it;
	}
}

void do_stuff()
{
	auto logFiles = get_files();

	if (logFiles.empty())
	{
		blog(LOG_INFO, "No VRChat logs, returning.");
		return;
	}
	
	std::vector<PyPylogEntry> logEntries;
	read_files(logFiles, logEntries);

	if (logEntries.empty())
	{
		blog(LOG_INFO, "No PyPy log entries, returning.");
		return;
	}

	// Find the first first entr before recordStartTime and set its time to be exactly the same as the start time.
	// If it is greater than 15 minutes before recordStartTime, instead create a new entry of UNKNOWN.
	// Nuke everything else before the start.

	// First, find the first entry that is within the recording window
	int firstValid = -1;
	int iterator = 0;
	for ( auto& entry : logEntries)
	{
		if (!entry.isBeforeStart())
		{
			firstValid = iterator;
			break;
		}
		iterator++;
	}

	if ( firstValid == -1)
	{
		// No valid entries, return
		blog(LOG_INFO, "No PyPy log entries during recording window");
		return;
	}

	// Create an entry for the very beginning
	PyPylogEntry startEntry = PyPylogEntry( recordStartTime, "", "START" );
	startEntry.setVideoStartTime( recordStartTime );

	if ( firstValid > 0 )
	{
		// At least 1 entry before recording window
		blog(LOG_INFO, "First entry during session at index %i", firstValid);
		PyPylogEntry preStartEntry = logEntries[firstValid-1];
		if ( chrono::abs(preStartEntry.videoTime) < PREV_SESSION_CUTOFF )
		{
			// Within 15 min of recording start, probably part of same session
			// copy its values over to our start entry
			startEntry.title = preStartEntry.title;
			startEntry.url = preStartEntry.url;	
			startEntry.populateYoutubeId();
		}

		// Erase everything up to the first in-session entry. exclusive index I guess?
		logEntries.erase( logEntries.begin(), logEntries.begin() + (firstValid));
	}

	// Add our start entry to the vector
	// Ensure that there isn't already an entry that starts at the eaxct moment recording started.
	// Extremely unlikely to ever happen, but theoretically could.
	if ( logEntries[0].videoTime != chrono::system_clock::duration::zero() )
	{
		logEntries.insert(logEntries.begin(), startEntry);	// Sue me
	}

	filterZeroDurationEntries(logEntries);

	blog( LOG_INFO, "Looking up youtube urls." );	

	populateCustomUrlTitles(logEntries);

	blog( LOG_INFO, "Printing parsed." );	
	for (auto entry : logEntries)
	{
		blog( LOG_INFO, "%s", entry.toString().c_str() );	
	}

}

// Callback for when a recording stops
obs_frontend_event_cb EventHandler = [](enum obs_frontend_event event, void*) 
{
	switch (event) 
	{
		case OBS_FRONTEND_EVENT_RECORDING_STARTED: 
		{
			blog(LOG_INFO, "Record start!");

			recordStartTime = std::chrono::system_clock::now();

			// TODO remove testing: 
			recordStartTime = vrchatLogTimeToTimePoint("2023.04.12 23:15:10");

			break;
		}

		case OBS_FRONTEND_EVENT_RECORDING_PAUSED: 
		{
			break;
		}

		case OBS_FRONTEND_EVENT_RECORDING_UNPAUSED: 
		{
			break;
		}

		case OBS_FRONTEND_EVENT_RECORDING_STOPPED: 
		{
			blog(LOG_INFO, "Record stopped!");

			recordEndTime = std::chrono::system_clock::now();

			// TODO remove testing: 
			recordEndTime = vrchatLogTimeToTimePoint("2023.04.13 01:40:51");

			thread t(do_stuff);
			t.detach();

			//do_stuff();
			//get_files();
			//read_file();
	
			break;
		}

		case OBS_FRONTEND_EVENT_EXIT:
		{
			break;
		}

		default:
			break;
		
	}
};

bool obs_module_load(void)
{
	blog(LOG_INFO, "plugin loaded successfully (version %s)",
	     PLUGIN_VERSION);

	obs_frontend_add_event_callback(EventHandler, NULL);

	return true;
}

void obs_module_unload()
{
	blog(LOG_INFO, "plugin unloaded");
}