#include "BreakpadConfigurer.h"

#if defined(WIN32)
#include "internal/win32/CrashReportContext.h"
#else
#include "internal/unix/CrashReportContext.h"
#endif

BreakpadConfigurer& BreakpadConfigurer::setDatabasePathUTF8(const std::string& pathUTF8)
{
	mDatabasePathUTF8 = pathUTF8;
	return *this;
}

BreakpadConfigurer& BreakpadConfigurer::setReportURL(const std::string& reportURL)
{
	mReportURL = reportURL;
	return *this;
}

BreakpadConfigurer& BreakpadConfigurer::setParameters(const std::map<std::string, std::string>& parameters)
{
	mParameters = parameters;
	return *this;
}

BreakpadConfigurer& BreakpadConfigurer::setSenderPathUTF8(const std::string& pathUTF8)
{
	mSenderPathUTF8 = pathUTF8;
	return *this;
}

void BreakpadConfigurer::start()
{
	static CrashReportContext context{};
	bool ok = context.setSenderPathUTF8(mSenderPathUTF8);
	ok &= context.setReportURL(mReportURL);
	ok &= context.setParameters(mParameters);
	if (ok)
		context.startHandler(mDatabasePathUTF8);
}
