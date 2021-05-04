
#include <errno.h>
#include <map>
#include <sstream>

#if defined(__APPLE__)
#include "client/mac/handler/exception_handler.h"
#else
#include "client/linux/handler/exception_handler.h"
#endif

#include "CrashReportContext.h"
namespace
{

std::string stringifyParameters(const std::map<std::string, std::string>& parameters)
{
    std::stringstream stream;

    std::size_t parameterIndex = 0;
    std::size_t parametersCount = parameters.size();
    for (auto& pair : parameters)
    {
        stream << pair.first.c_str() << "=\"" << pair.second.c_str() << "\"";
        ++parameterIndex;
        if (parameterIndex < parametersCount)
            stream << ",";
    }
    return stream.str();
}

bool strcpyChecked(char* dest, size_t destsz, const std::string& src)
{
    if(src.length() < destsz)
    {
        memcpy(dest, src.c_str(), src.length());
        dest[src.length()] = '\0';
        return true;
    }
    return false;
}

#if defined(__APPLE__)

static constexpr int MaxDumpPathLength{ 4096 };
static char DumpPath[MaxDumpPathLength];

bool dumpCallback(const char* dump_dir, const char* minidump_id, void* context, bool succeeded)
{
    if(succeeded)
    {
        const int PathDumpLength = strlen(dump_dir) + strlen("/") + strlen(minidump_id) + strlen(".dmp");
        if(PathDumpLength < MaxDumpPathLength)
        {
            strcpy(DumpPath, dump_dir);
            strcat(DumpPath, "/");
            strcat(DumpPath, minidump_id);
            strcat(DumpPath, ".dmp");
            auto crashReportContext = reinterpret_cast<CrashReportContext*>(context);
            return crashReportContext->send(DumpPath);
        }
        return false;
    }
    return false;
}
#else
bool dumpCallback(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool succeeded) 
{
    if(succeeded)
    {
        auto crashReportContext = reinterpret_cast<CrashReportContext*>(context);
        return crashReportContext->send(descriptor.path());
    }
    return succeeded;
}
#endif

}

bool CrashReportContext::setSenderPathUTF8(const std::string& path)
{
    return strcpyChecked(senderPath, MaxBufferLength, path);
}

bool CrashReportContext::setReportURL(const std::string& url)
{
    return strcpyChecked(reportURL, MaxBufferLength, url);
}

bool CrashReportContext::setParameters(const std::map<std::string, std::string>& p)
{
    auto str = stringifyParameters(p);
    return strcpyChecked(parameters, MaxBufferLength, str);
}

void CrashReportContext::startHandler(const std::string& databasePath)
{
#if(__APPLE__)
    static std::unique_ptr<google_breakpad::ExceptionHandler> handler(new google_breakpad::ExceptionHandler(
        databasePath,
        nullptr,
        dumpCallback,
        this,
        true,
        nullptr
    ));
#else 
    google_breakpad::MinidumpDescriptor descriptor(databasePath);
	static std::unique_ptr<google_breakpad::ExceptionHandler> handler(new google_breakpad::ExceptionHandler(
		descriptor,
		nullptr,
		dumpCallback,
		this,
		true,
		-1
	));
#endif
}

bool CrashReportContext::send(const char* minidumpPath)
{
    auto pid = fork();
    if(pid == 0)
    {
        static const char* procName = "crashreporter";
        if(parameters[0] != 0)
        {
            execl(senderPath, procName, "-a", parameters, "-u", reportURL, minidumpPath, NULL);
        }
        else
        {
            execl(senderPath, procName, "-u", reportURL, minidumpPath, NULL);
        }
        fprintf(stderr, "Failed to start handler: %s\n", strerror(errno));
        abort();
    }
    return pid != -1;
}
