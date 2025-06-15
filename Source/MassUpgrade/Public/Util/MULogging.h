#pragma once

#include "Logging/LogMacros.h"

#include <sstream>

#define FUNCTIONSTR2(x) #x
#define FUNCTIONSTR TEXT(FUNCTIONSTR2(__FUNCTION__))

class CommaLog
{
public:
	inline CommaLog&
	operator,(const FString& value)
	{
		for (TCHAR ch : value)
		{
			wos << static_cast<wchar_t>(ch);
		}

		return *this;
	}

	inline CommaLog&
	operator,(const FText& value)
	{
		for (TCHAR ch : value.ToString())
		{
			wos << static_cast<wchar_t>(ch);
		}

		return *this;
	}

	inline CommaLog&
	operator,(const TCHAR* value)
	{
		for (size_t x = 0; value[x]; x++)
		{
			wos << static_cast<wchar_t>(value[x]);
		}

		return *this;
	}

	template <typename T>
	inline CommaLog&
	operator,(const T& value)
	{
		wos << value;

		return *this;
	}
	
	std::wostringstream wos;
};

DECLARE_LOG_CATEGORY_EXTERN(LogMassUpgrade, Log, All)

#define MU_LOG_Verbosity(verbosity, first, ...) \
	{ \
		CommaLog l; \
		l, first, ##__VA_ARGS__; \
		UE_LOG(LogMassUpgrade, verbosity, TEXT("%s"), l.wos.str().c_str()) \
	}

#define MU_LOG_Log(first, ...) MU_LOG_Verbosity(Log, first, ##__VA_ARGS__)
#define MU_LOG_Display(first, ...) MU_LOG_Verbosity(Display, first, ##__VA_ARGS__)
#define MU_LOG_Warning(first, ...) MU_LOG_Verbosity(Warning, first, ##__VA_ARGS__)
#define MU_LOG_Error(first, ...) MU_LOG_Verbosity(Error, first, ##__VA_ARGS__)

#define IS_MU_LOG_LEVEL(level) (UMassUpgradeConfiguration::configuration.logLevel > 0 && UMassUpgradeConfiguration::configuration.logLevel >= static_cast<uint8>(level))

#define MU_LOG_Log_Condition(first, ...) if(IS_MU_LOG_LEVEL(ELogVerbosity::Log)) MU_LOG_Log(first, ##__VA_ARGS__)
#define MU_LOG_Display_Condition(first, ...) if(IS_MU_LOG_LEVEL(ELogVerbosity::Display)) MU_LOG_Display(first, ##__VA_ARGS__)
#define MU_LOG_Warning_Condition(first, ...) if(IS_MU_LOG_LEVEL(ELogVerbosity::Warning)) MU_LOG_Warning(first, ##__VA_ARGS__)
#define MU_LOG_Error_Condition(first, ...) if(IS_MU_LOG_LEVEL(ELogVerbosity::Error)) MU_LOG_Error(first, ##__VA_ARGS__)
