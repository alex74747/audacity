#include <locale>
#include <codecvt>
#include <sstream>
#include "client/windows/handler/exception_handler.h"
#include "CrashReportContext.h"

namespace {
	bool uploadReport(
		const wchar_t* dump_path,
		const wchar_t* minidump_id,
		void* context,
		EXCEPTION_POINTERS* exinfo,
		MDRawAssertionInfo* assertion,
		bool succeeded)
	{
		CrashReportContext* crashReportContext = reinterpret_cast<CrashReportContext*>(context);
		if (!crashReportContext->send(dump_path, minidump_id))
			return false;
		return succeeded;
	}

	std::string stringifyParameters(const std::map<std::string, std::string>& parameters)
	{
		std::stringstream stream;

		std::size_t parameterIndex = 0;
		std::size_t parametersCount = parameters.size();
		for (auto& pair : parameters)
		{
			stream << pair.first.c_str() << "=\\\"" << pair.second.c_str() << "\\\"";
			++parameterIndex;
			if (parameterIndex < parametersCount)
				stream << ",";
		}
		return stream.str();
	}
}

bool CrashReportContext::setSenderPathUTF8(const std::string& path)
{
    return wcscpy_s(senderPath, MaxBufferLength, std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(path).c_str()) == 0;
}

bool CrashReportContext::setReportURL(const std::string& url)
{
    return wcscpy_s(reportURL, MaxBufferLength, std::wstring(url.begin(), url.end()).c_str()) == 0;
}

bool CrashReportContext::setParameters(const std::map<std::string, std::string>& p)
{
	auto str = stringifyParameters(p);
	return wcscpy_s(parameters, MaxBufferLength, std::wstring(str.begin(), str.end()).c_str()) == 0;
}

void CrashReportContext::startHandler(const std::string& databasePath)
{
	static std::unique_ptr<google_breakpad::ExceptionHandler> exceptionHandler(
		new google_breakpad::ExceptionHandler(
			std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(databasePath),
			NULL,
			uploadReport,
			this,
			google_breakpad::ExceptionHandler::HANDLER_ALL));
}

bool CrashReportContext::send(const wchar_t* path, const wchar_t* id)
{
	if (!makeCommand(path, id))
		return false;

	STARTUPINFOW si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOW;

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));

	if (CreateProcessW(NULL, command, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return true;
	}
	else
		return false;
}

bool CrashReportContext::makeCommand(const wchar_t* path, const wchar_t* id)
{
	//utility path
	auto err = wcscpy_s(command, CrashReportContext::MaxCommandLength, L"\"");
	err = wcscat_s(command, CrashReportContext::MaxCommandLength, senderPath);
	err = wcscat_s(command, CrashReportContext::MaxCommandLength, L"\"");
	
	//parameters: /p "..."
	if (parameters[0] != 0)
	{
		err |= wcscat_s(command, CrashReportContext::MaxCommandLength, L" /a \"");
		err |= wcscat_s(command, CrashReportContext::MaxCommandLength, parameters);
		err |= wcscat_s(command, CrashReportContext::MaxCommandLength, L"\"");
	}
	//crash report URL: /u https://...
	err |= wcscat_s(command, CrashReportContext::MaxBufferLength, L" /u \"");
	err |= wcscat_s(command, CrashReportContext::MaxBufferLength, reportURL);
	err |= wcscat_s(command, CrashReportContext::MaxBufferLength, L"\" ");
	//minidump path: path/to/minidump.dmp
	err |= wcscat_s(command, CrashReportContext::MaxCommandLength, L" \"");
	err |= wcscat_s(command, CrashReportContext::MaxCommandLength, path);
	err |= wcscat_s(command, CrashReportContext::MaxCommandLength, L"\\");
	err |= wcscat_s(command, CrashReportContext::MaxCommandLength, id);
	err |= wcscat_s(command, CrashReportContext::MaxCommandLength, L".dmp\"");
	return err == 0;
}