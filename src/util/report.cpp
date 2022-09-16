#include "report.hpp"
#include <filesystem>
#include <fmt/core.h>
#include <iostream>

namespace {
#ifndef NDEBUG
constexpr bool macro_debug = true;
#else
constexpr bool macro_debug = false;
#endif
} // namespace

namespace report {

std::string getIdent(const std::source_location&
                         location /* = std::source_location::current() */) {
	std::filesystem::path path = location.file_name();
	return fmt::format("{0}::{1}(...) at {2}: ", path.filename().string(),
	                   location.function_name(), location.line());
}

std::runtime_error
rtError(const std::string& msg,
        const std::source_location&
            location /* = std::source_location::current() */) {
	return std::runtime_error(getIdent(location) + msg);
}

void toStream(const std::string& msg,
              std::ostream& stream,
              ReportAttrs attrs /* = ReportAttrs{} */,
              const std::source_location&
                  location /* = std::source_location::current() */) {

	if (attrs.ident)
		stream << getIdent(location);

	stream << msg;

	if (attrs.new_line)
		stream << '\n';

	if (attrs.flush)
		stream.flush();
}

void message(const std::string& msg,
             ReportAttrs attrs /* = ReportAttrs{} */,
             const std::source_location&
                 location /* = std::source_location::current() */) {
	toStream(msg, std::cout, attrs, location);
}

void error(const std::string& msg,
           ReportAttrs attrs /* = {true, true, true} */,
           const std::source_location&
               location /* = std::source_location::current() */) {
	toStream(msg, std::cerr, attrs, location);
}

void debugToStream(const std::string& msg,
                   std::ostream& stream,
                   ReportAttrs attrs /* = ReportAttrs{} */,
                   const std::source_location&
                       location /* = std::source_location::current() */) {
	if constexpr (!macro_debug)
		toStream(msg, stream, attrs, location);
}

void debugMessage(const std::string& msg,
                  ReportAttrs attrs /* = ReportAttrs{} */,
                  const std::source_location&
                      location /* = std::source_location::current() */) {

	if constexpr (!macro_debug)
		message(msg, attrs, location);
}

void debugError(const std::string& msg,
                ReportAttrs attrs /* = {true, true, true} */,
                const std::source_location&
                    location /* = std::source_location::current() */) {
	if constexpr (!macro_debug)
		error(msg, attrs, location);
}

} // namespace report
