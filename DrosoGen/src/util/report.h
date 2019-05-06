#ifndef REPORT_H
#define REPORT_H

#include <iostream>

/** helper macros to unify reports: */
#define REPORT(x)        std::cout << __FUNCTION__ << "(): " << x << std::endl;
#define REPORT_NOENDL(x) std::cout << __FUNCTION__ << "(): " << x;

#ifdef DEBUG
	#define DEBUG_REPORT(x)         REPORT(x)
	#define DEBUG_REPORT_NOENDL(x)  REPORT_NOENDL(x)
#else
	#define DEBUG_REPORT(x)
	#define DEBUG_REPORT_NOENDL(x)
#endif

/** to be used in constructs such as:  throw new std::runtime_error( EREPORT("refuse to deal with NULL agent.") ); */
#define EREPORT(x) std::string(__FUNCTION__).append("(): ").append(x)

/** shortcut for the often-used std::runtime_error exception:  throw ERROR_REPORT("refuse to deal with NULL agent."); */
#define ERROR_REPORT(x)  new std::runtime_error( EREPORT(x) )

/** any AbstractAgent-derived agent can use this to label its own report messages,
    e.g. DEBUG_REPORT(IDSIGN << "reporting signed message"); */
#define SIGN    agentType << ": "
#define IDSIGN  ID << "-" << agentType << ": "

#endif
