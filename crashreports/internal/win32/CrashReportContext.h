#pragma once

#include <string>
#include <map>

class CrashReportContext
{
	static constexpr int MaxBufferLength{ 2048 };
	static constexpr int MaxCommandLength{ 8192 };

	wchar_t senderPath[MaxBufferLength]{};
	wchar_t reportURL[MaxBufferLength]{};
	wchar_t parameters[MaxBufferLength]{};
	wchar_t command[MaxCommandLength]{};

public:
	bool setSenderPathUTF8(const std::string& path);
	bool setReportURL(const std::string& path);
	bool setParameters(const std::map<std::string, std::string>& p);

	void startHandler(const std::string& databasePath);

	bool send(const wchar_t* path, const wchar_t* id);
private:
	bool makeCommand(const wchar_t* path, const wchar_t* id);
};
