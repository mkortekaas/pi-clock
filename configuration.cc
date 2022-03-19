
#include "led-matrix.h"
#include "graphics.h"

#include "configuration.h"

#include <string.h>
#include <time.h>
#include <unistd.h>
#include <bits/stdc++.h>

using namespace rgb_matrix;

const TZDataWanted tz_wanted[] = {
//	{ "Africa/Johannesburg", "JNB"},
	{ "America/Denver",      "DEN"},
	{ "America/Los_Angeles", "SEA"},
	{ "America/New_York",    "NYC"},
//	{ "Pacific/Auckland",    "AKL"},
	{ "Asia/Kolkata",        "BLR"},
//	{ "Asia/Hong_Kong",      "HKG"},
//	{ "Asia/Singapore",      "SIN"},	// Singapore does not have a timezone name (but it should be SGT)
//	{ "Asia/Tel_Aviv",       "TLV"},
	{ "Europe/London",       "LHR"},
	{ "Europe/Paris",        "EUR"}		// "EUR" for Europe vs the specific city
};

const size_t tz_wanted_len = sizeof(tz_wanted)/sizeof(tz_wanted[0]);
