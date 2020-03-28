#include "strings.h"

/** The buffer length into which any MPI-communicated string must be imprint, The original
    string is padded with zero-value characters if it is too short, or trimmed if too long. */
const size_t StringsImprintSize = 256;


std::ostream& operator<<(std::ostream& s, const hashedString& hs)
{
	s << hs.getString();
	return s;
}
