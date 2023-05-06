#pragma once

//#define DEBUG
// For AllocConsole
#ifdef DEBUG
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <Windows.h>
#endif
#endif

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <iterator>
#include <filesystem>
#include <chrono>
#include <thread>
#include <functional> 