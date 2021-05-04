#pragma once

#include <string>

class CrashReportContext
{
    static constexpr int MaxBufferLength{ 2048 };

    char senderPath[MaxBufferLength]{};
    char reportURL[MaxBufferLength]{};
    char parameters[MaxBufferLength]{};

public:

    bool setSenderPathUTF8(const std::string& path);
    bool setReportURL(const std::string& url);
    bool setParameters(const std::map<std::string, std::string>& p);

    void startHandler(const std::string& databasePath);

    bool send(const char* minidumpPath);
};
