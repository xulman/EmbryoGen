#ifndef REPORT_H
#define REPORT_H

#include <iostream>

/// helper macro to unify reports:
#define REPORT(x)        std::cout << __FUNCTION__ << "(): " << x << std::endl;
#define REPORT_NOENDL(x) std::cout << __FUNCTION__ << "(): " << x;

#ifdef DEBUG
	#define DEBUG_REPORT(x)         REPORT(x)
	#define DEBUG_REPORT_NOENDL(x)  REPORT_NOENDL(x)
#else
	#define DEBUG_REPORT(x)
	#define DEBUG_REPORT_NOENDL(x)
#endif

#endif
