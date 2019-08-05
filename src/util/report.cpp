#include <string>
#include "report.h"

const std::chrono::time_point<std::chrono::system_clock> tic(void)
{
	return ( std::chrono::system_clock::now() );
}

std::string toc(const std::chrono::time_point<std::chrono::system_clock>& ticTime)
{
	const std::chrono::duration<double>&& deltaT = std::chrono::system_clock::now() - ticTime;
	return ( std::to_string(deltaT.count()).append(" seconds") );
}
