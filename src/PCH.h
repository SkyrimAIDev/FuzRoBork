#pragma once

// CommonLibSSE-NG includes
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

// Windows headers
#include <windows.h>
#include <sapi.h>
#include <sphelper.h>
#include <ShlObj.h>
#include <shlwapi.h>

// STL
#include <string>
#include <vector>
#include <map>
#include <unordered_set>
#include <thread>
#include <chrono>
#include <regex>
#include <codecvt>
#include <locale>
#include <fstream>
#include <filesystem>

// External dependencies
#include <tinyxml2.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

// Project includes
#include "include/SME_Prefix.h"
#include "include/INIManager.h"
#include "include/StringHelpers.h"
#include "include/MiscGunk.h"
#include "include/MemoryHandler.h"
#include "include/Functors.h"

using namespace std::literals;
