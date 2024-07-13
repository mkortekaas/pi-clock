//
// Config info is stored as follows:
//

using namespace rgb_matrix;

// internal structure to make code easier below
struct TZData {
	char *textBuffer;
	const char *tzString;
	const char *tzDisplay;
	struct tm tm;
	Color tzColor;
	bool isMyTimezone;
}; 

struct TZDataWanted {
	const char *tzString;
	const char *tzDisplay;
}; 

extern TZDataWanted *tz_wanted;
extern size_t tz_wanted_len;

extern size_t read_city_list(const char *filename);

