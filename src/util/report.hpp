#ifndef REPORT_H
#define REPORT_H

#include <source_location>
#include <string>
#include <cstring>
#include <iostream>
#include <sstream>
#include <chrono>

/** profiling aiders: starts stopwatch and returns "session handler",
    which is actually the current system time */
inline const std::chrono::time_point<std::chrono::system_clock> tic(void)
{
	return ( std::chrono::system_clock::now() );
}

/** profiling aiders: stops stopwatch and reports the elapsed time,
    requires some counter-part earlier time (e.g. the "session handler") */
inline std::string toc(const std::chrono::time_point<std::chrono::system_clock>& ticTime)
{
	const std::chrono::duration<double>&& deltaT = std::chrono::system_clock::now() - ticTime;
	return ( std::to_string(deltaT.count()).append(" seconds") );
}

/** Translates everything stream-able to string */
std::string toString(auto arg)
{
    std::ostringstream _tmp;
    _tmp << arg;
    return _tmp.str();
}

namespace report {

/** Parameters for reporting message and error */
struct ReportAttrs {
	bool ident = true;
	bool new_line = true;
	bool flush = false;
};

/** Get runtime error object with inserted message and identification*/
std::runtime_error rtError(const std::string& msg, 
    const std::source_location& location  = std::source_location::current());

/** Get location identifier */
std::string getIdent(
    const std::source_location& location = std::source_location::current());

/** Send message to stream */
void toStream(
    const std::string& msg,
    std::ostream& stream,
    ReportAttrs attrs = ReportAttrs{},
    const std::source_location& location = std::source_location::current());

/** Send message to standard output */
void message(
    const std::string& msg,
    ReportAttrs attrs = ReportAttrs{},
    const std::source_location& location = std::source_location::current());

/** Send message to standard error output */
void error(
    const std::string& msg,
    ReportAttrs attrs = {true, true, true},
    const std::source_location& location = std::source_location::current());

/** Send message to stream, disabled when not debug */
void debugToStream(
    const std::string& msg,
    std::ostream& stream,
    ReportAttrs attrs = ReportAttrs{},
    const std::source_location& location = std::source_location::current());

/** Send message to standard output, disabled when not debug */
void debugMessage(
    const std::string& msg,
    ReportAttrs attrs = ReportAttrs{},
    const std::source_location& location = std::source_location::current());

/** Send message to standard error output, disabled when not debug */
void debugError(
    const std::string& msg,
    ReportAttrs attrs = {true, true, true},
    const std::source_location& location = std::source_location::current());

} // namespace report
