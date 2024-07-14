
#include "led-matrix.h"
#include "graphics.h"

#include "configuration.h"

#include <string.h>
#include <fstream>
#include <time.h>
#include <unistd.h>
#include <bits/stdc++.h>

using namespace rgb_matrix;

const TZDataWanted tz_wanted_built_in[] = {
//	{ "Africa/Johannesburg", "JNB"},
	{ "America/Denver",      "DEN"},
//	{ "America/Los_Angeles", "SEA"},
	{ "America/New_York",    "NYC"},
//	{ "America/Sao_Paulo",   "BRA"},
//	{ "Pacific/Auckland",    "AKL"},
//	{ "Asia/Katmandu",       "KTM"},
	{ "Asia/Kolkata",        "BLR"},
//	{ "Asia/Hong_Kong",      "HKG"},
//	{ "Asia/Singapore",      "SIN"},	// Singapore does not have a timezone name (but it should be SGT)
//	{ "Asia/Tel_Aviv",       "TLV"},
//	{ "Europe/London",       "LHR"},
	{ "Australia/Sydney",    "SYD"},
	{ "Europe/Paris",        "EUR"},	// "EUR" for Europe vs the specific city
	{ "UTC",                 "UTC"}
};

const size_t tz_wanted_len_built_in = sizeof(tz_wanted_built_in)/sizeof(tz_wanted_built_in[0]);

TZDataWanted *tz_wanted = (TZDataWanted *)tz_wanted_built_in;
size_t tz_wanted_len = sizeof(tz_wanted_built_in)/sizeof(tz_wanted_built_in[0]);

#define	MAX_ENTRIES	100

size_t read_city_list(const char *filename)
{
	std::fstream f;

	// if no file passed - we are still ok with built in list
	if (filename == NULL || strlen(filename) == 0) {
		return tz_wanted_len;
	}

	// hopefully read the cities.txt file - if not, we have a list above to fall back on
	f.open(filename);
	if (!f.is_open()) {
		return 0;
	}

	TZDataWanted *p = (TZDataWanted *)malloc(sizeof(TZDataWanted)*(MAX_ENTRIES+1));
	size_t n = 0;
	std::string my_line;

	while (!f.eof()) {
		if (n >= MAX_ENTRIES)
			break;
		std::getline(f, my_line);
		if (my_line.length() == 0 || my_line[0] == '#')
			continue;

		char *line_timezone = NULL;
		char *line_cityname = NULL;
		char *token = strtok((char *)my_line.c_str(), " \t");
		if (token != NULL) {
			line_cityname = token;
			token = strtok(NULL, " \t");
			if (token != NULL) {
				line_timezone = token;
			}
		}

		// Store away
		p[n].tzString = strdup(line_cityname);
		p[n].tzDisplay = strdup(line_timezone);

		n++;
	}
	f.close();
        p[n].tzString = NULL;
        p[n].tzDisplay = NULL;

	tz_wanted = p;
	tz_wanted_len = n;

	return n;
}
