#include <wx/wx.h>
#include <map>
#include <string>

class CrashReportApp : public wxApp
{
    std::string mURL;
    wxString mMinidumpPath;
    std::map<std::string, std::string> mArguments;

    bool mSilent{ false };
public:
    bool OnInit() override;
    void OnInitCmdLine(wxCmdLineParser& parser) override;
    bool OnCmdLineParsed(wxCmdLineParser& parser) override;
};

DECLARE_APP(CrashReportApp);
