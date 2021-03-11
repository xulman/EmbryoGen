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

template <typename T>
std::string humanFriendlyNumber(const T number)
{
	double origSize = (double)number;
	std::string msg;

	if (origSize <= 1024.0)
	{
		msg = std::to_string(origSize) + " ";
	}
	else
	{
		char unit;
		if (origSize > 1024.0*1024.0*1024.0)
		{
			origSize /= 1024.0*1024.0*1024.0;
			unit = 'G';
		}
		else
		if (origSize > 1024.0*1024.0)
		{
			origSize /= 1024.0*1024.0;
			unit = 'M';
		}
		else
		{
			origSize /= 1024.0;
			unit = 'K';
		}

		int intDigits  = (int)origSize;
		int fracDigits = (int)(origSize*10.0) %10;
		msg = std::to_string(intDigits) + "." + std::to_string(fracDigits) + " " + unit;
	}

	return msg;
}

template std::string humanFriendlyNumber(const int number);
template std::string humanFriendlyNumber(const long number);
template std::string humanFriendlyNumber(const size_t number);
template std::string humanFriendlyNumber(const float number);
template std::string humanFriendlyNumber(const double number);
