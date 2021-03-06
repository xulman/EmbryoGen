#ifndef REPORT_H
#define REPORT_H

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <chrono>

/** this macro is essentially what `basename __FILE__` would do on UNIX prompt/console */
#define __SHORTFILE__ (std::strrchr(__FILE__,'/') ? std::strrchr(__FILE__,'/')+1 : __FILE__)

/** helper macros to unify reports: */
#define REPORT(x)        std::cout << __SHORTFILE__ << "::" << __FUNCTION__ << "(): " << x << std::endl;
#define REPORT_NOENDL(x) std::cout << __SHORTFILE__ << "::" << __FUNCTION__ << "(): " << x;

#define REPORT_NOHEADER(x)         std::cout << x << std::endl;
#define REPORT_NOHEADER_NOENDL(x)  std::cout << x;
#define REPORT_NOHEADER_JUSTENDL() std::cout << std::endl;

#ifdef DEBUG
	#define DEBUG_REPORT(x)         REPORT(x)
	#define DEBUG_REPORT_NOENDL(x)  REPORT_NOENDL(x)

	#define DEBUG_REPORT_NOHEADER(x)          REPORT_NOHEADER(x)
	#define DEBUG_REPORT_NOHEADER_NOENDL(x)   REPORT_NOHEADER_NOENDL(x)
	#define DEBUG_REPORT_NOHEADER_JUSTENDL()  REPORT_NOHEADER_JUSTENDL()
#else
	#define DEBUG_REPORT(x)
	#define DEBUG_REPORT_NOENDL(x)

	#define DEBUG_REPORT_NOHEADER(x)
	#define DEBUG_REPORT_NOHEADER_NOENDL(x)
	#define DEBUG_REPORT_NOHEADER_JUSTENDL()
#endif

/** calls a no-parameter, string-returning lambda that is defined on demand, inprinting the given
    stream x into its definition, and the execution of it returns the desired string */
#define buildStringFromStream(x) [&](){ std::ostringstream qqq; qqq << x; return qqq.str(); }()

/** to be used in constructs such as:  throw new std::runtime_error( EREPORT("refuse to deal with NULL agent.") ); */
#define EREPORT(x) std::string(__SHORTFILE__).append("::").append(__FUNCTION__).append("(): ").append( buildStringFromStream(x) )

/** shortcut for the often-used std::runtime_error exception:  throw ERROR_REPORT("refuse to deal with NULL agent."); */
#define ERROR_REPORT(x)  new std::runtime_error( EREPORT(x) )

/** any AbstractAgent-derived agent can use this to label its own report messages,
    e.g. DEBUG_REPORT(IDSIGN << "reporting signed message"); */
#define SIGN                 "\"" << this->agentType << "\": "
#define IDSIGN  this->ID << " \"" << this->agentType << "\": "

#define SIGNHIM(ag)                     "\"" << (ag).getAgentType() << "\": "
#define IDSIGNHIM(ag)  (ag).getID() << " \"" << (ag).getAgentType() << "\": "


/** profiling aiders: starts stopwatch and returns "session handler",
    which is actually the current system time */
const std::chrono::time_point<std::chrono::system_clock> tic(void);

/** profiling aiders: stops stopwatch and reports the elapsed time,
    requires some counter-part earlier time (e.g. the "session handler") */
std::string toc(const std::chrono::time_point<std::chrono::system_clock>& ticTime);
#endif
