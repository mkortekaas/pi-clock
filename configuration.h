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
}; 

struct TZDataWanted {
	const char *tzString;
	const char *tzDisplay;
}; 

extern const TZDataWanted tz_wanted[];
extern const size_t tz_wanted_len;
