#pragma once

#include <string>
#include <map>

class BreakpadConfigurer final
{
	std::string mDatabasePathUTF8;
	std::string mSenderPathUTF8;
	std::string mReportURL;
	std::map<std::string, std::string> mParameters;
public:
	BreakpadConfigurer& setDatabasePathUTF8(const std::string& pathUTF8);
	//URL encoded
	BreakpadConfigurer& setReportURL(const std::string& reportURL);
	//ASCII encoded
	BreakpadConfigurer& setParameters(const std::map<std::string, std::string>& parameters);
	BreakpadConfigurer& setSenderPathUTF8(const std::string& pathUTF8);

	void start();
};
