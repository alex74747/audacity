/**********************************************************************

  Audacity: A Digital Audio Editor

  Nyquist.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_EFFECT_NYQUIST__
#define __AUDACITY_EFFECT_NYQUIST__

#include "../Effect.h"
#include "FileNames.h"

#include "nyx.h"

class wxArrayString;
class wxFileName;
class wxCheckBox;
class wxTextCtrl;

#define NYQUISTEFFECTS_VERSION wxT("1.0.0.0")

enum NyqControlType
{
   NYQ_CTRL_INT,
   NYQ_CTRL_FLOAT,
   NYQ_CTRL_STRING,
   NYQ_CTRL_CHOICE,
   NYQ_CTRL_INT_TEXT,
   NYQ_CTRL_FLOAT_TEXT,
   NYQ_CTRL_TEXT,
   NYQ_CTRL_TIME,
   NYQ_CTRL_FILE,
};

struct NyqValue {
   wxString valStr;
   double val = 0.0;
};

class NyqControl
{
public:
   NyqControl() = default;
   NyqControl( const NyqControl& ) = default;
   NyqControl &operator = ( const NyqControl & ) = default;
   //NyqControl( NyqControl && ) = default;
   //NyqControl &operator = ( NyqControl && ) = default;

   int type;
   wxString var;
   wxString name;
   wxString label;
   std::vector<EnumValueSymbol> choices;
   FileNames::FileTypes fileTypes;
   wxString lowStr;
   wxString highStr;
   double low;
   double high;
   int ticks;
};

// Protect Nyquist from selections greater than 2^31 samples (bug 439)
#define NYQ_MAX_LEN (std::numeric_limits<long>::max())

//! Properties of a Nyquist program deduced at parsing time
struct NyqProperties {
   NyqProperties() = default;
   explicit NyqProperties(TranslatableString name)
      : mName{ std::move(name) }
   {}
   explicit NyqProperties(TranslatableString name,
      bool ok, bool tool, EffectType type)
      : mName{ std::move(name) }
      , mOK{ ok }
      , mIsTool{ tool }
      , mType{ type }
   {}

   //! Name of the Effect (untranslated)
   TranslatableString mName;
   TranslatableString mAction = XO("Applying Nyquist Effect...");
   TranslatableString mInfo;
   TranslatableString mAuthor = XO("n/a");
   TranslatableString mCopyright = XO("n/a");

   //! ONLY use if a help page exists in the manual.
   /*! If not wxEmptyString, must be a page in the Audacity manual. */
   wxString          mManPage;
   
   //! If not wxEmptyString, must be a valid HTML help file.
   wxString          mHelpFile;

   //! Version number of the specific plug-in (not to be confused with mVersion)
   /*! For shipped plug-ins this will be the same as the Audacity release version
    when the plug-in was last modified. */
   TranslatableString mReleaseVersion = XO("n/a");

   wxArrayString     mCategories;

   sampleCount       mMaxLen = NYQ_MAX_LEN;

   bool              mOK = false;
   bool              mIsTool = false;

   // Bug 1934.
   // All Nyquist plug-ins should have a ';type' field, but if they don't we default to
   // being an Effect.
   EffectType        mType = EffectTypeProcess;

   bool              mIsSpectral = false;
   bool              mIsSal = false;
   bool              mFoundType = false; // not used after parsing

   //! True when *tracenable* or *sal-traceback* are enabled
   bool              mTrace = false;
   bool              mCompiler = false;

   /*! Syntactic version of Nyquist plug-in
    (not to be confused with mReleaseVersion) */
   int               mVersion = 4;

   //! Preview button enabled by default.
   bool              mEnablePreview = true;

   //! Default (auto):  Merge if length remains unchanged.
   int                mMergeClips = -1;

   // Default: Restore split lines.
   bool              mRestoreSplits = true;

   //! Debug button enabled by default. Set to false to disable Debug button.
   bool              mDebugButton = true;
};

class AUDACITY_DLL_API NyquistEffect final : public Effect
{
public:

   /** @param fName File name of the Nyquist script defining this effect. If
    * an empty string, then prompt the user for the Nyquist code to interpret.
    */
   NyquistEffect(const wxString &fName);
   virtual ~NyquistEffect();

   // ComponentInterface implementation

   PluginPath GetPath() const override;
   ComponentInterfaceSymbol GetSymbol() const override;
   VendorSymbol GetVendor() const override;
   wxString GetVersion() const override;
   TranslatableString GetDescription() const override;
   
   ManualPageID ManualPage() override;
   FilePath HelpPage() override;

   // EffectDefinitionInterface implementation

   EffectType GetType() override;
   EffectType GetClassification() override;
   EffectFamilySymbol GetFamily() override;
   bool IsInteractive() override;
   bool IsDefault() override;
   bool EnablesDebug() override;

   bool GetAutomationParameters(CommandParameters & parms) override;
   bool SetAutomationParameters(CommandParameters & parms) override;

   // EffectProcessor implementation

   bool DefineParams( ShuttleParams & S ) override;
   int SetLispVarsFromParameters(CommandParameters & parms, bool bTestOnly);

   // EffectUIClientInterface implementaton
   bool ValidateUI() override;

   // Effect implementation

   bool Init() override;
   bool CheckWhetherSkipEffect() override;
   bool Process() override;
   int ShowHostInterface( wxWindow &parent,
      const EffectDialogFactory &factory, bool forceModal = false) override;
   void PopulateOrExchange(ShuttleGui & S) override;
   bool TransferDataToWindow() override;
   bool TransferDataFromWindow() override;

   // NyquistEffect implementation
   // For Nyquist Workbench support
   void RedirectOutput();
   void SetCommand(const wxString &cmd);
   void Continue();
   void Break();
   void Stop();

private:
   static int mReentryCount;
   // NyquistEffect implementation

   void ClearBuffers();
   bool BeginTrack( WaveTrack *pTrack );
   bool ProcessOne();
   void EndTrack( WaveTrack *pTrack );

   void BuildPromptWindow(ShuttleGui & S);
   void BuildEffectWindow(ShuttleGui & S);

   bool TransferDataToPromptWindow();
   bool TransferDataToEffectWindow();

   bool TransferDataFromPromptWindow();
   bool TransferDataFromEffectWindow();

   bool IsOk();
   const TranslatableString &InitializationError() const { return mInitError; }

   static FilePaths GetNyquistSearchPath();

   static wxString NyquistToWxString(const char *nyqString);
   wxString EscapeString(const wxString & inStr);
   static std::vector<EnumValueSymbol> ParseChoice(const wxString & text);

   FileExtensions ParseFileExtensions(const wxString & text);
   FileNames::FileType ParseFileType(const wxString & text);
   FileNames::FileTypes ParseFileTypes(const wxString & text);

   static int StaticGetCallback(float *buffer, int channel,
                                int64_t start, int64_t len, int64_t totlen,
                                void *userdata);
   static int StaticPutCallback(float *buffer, int channel,
                                int64_t start, int64_t len, int64_t totlen,
                                void *userdata);
   static void StaticOutputCallback(int c, void *userdata);
   static void StaticOSCallback(void *userdata);

   int GetCallback(float *buffer, int channel,
                   int64_t start, int64_t len, int64_t totlen);
   int PutCallback(float *buffer, int channel,
                   int64_t start, int64_t len, int64_t totlen);
   void OutputCallback(int c);
   void OSCallback();

   void ParseFile();
   bool ParseCommand(const wxString & cmd);
   bool ParseProgram(wxInputStream & stream);
   struct Tokenizer {
      bool sl { false };
      bool q { false };
      int paren{ 0 };
      wxString tok;
      wxArrayStringEx tokens;

      bool Tokenize(
         const wxString &line, bool eof,
         size_t trimStart, size_t trimEnd);
   };
   bool Parse(Tokenizer &tokenizer, const wxString &line, bool eof, bool first);
   void SetProperties();

   static TranslatableString UnQuoteMsgid(const wxString &s, bool allowParens = true,
                           wxString *pExtraString = nullptr);
   static wxString UnQuote(const wxString &s, bool allowParens = true,
                           wxString *pExtraString = nullptr);

   void OnLoad(wxCommandEvent & evt);
   void OnSave(wxCommandEvent & evt);
   void OnDebug(wxCommandEvent & evt);

   void OnText(wxCommandEvent & evt);
   void OnSlider(wxCommandEvent & evt);
   void OnChoice(wxCommandEvent & evt);
   void OnTime(wxCommandEvent & evt);
   void OnFileButton(wxCommandEvent & evt);

   void resolveFilePath(wxString & path, FileExtension extension = {});
   bool validatePath(wxString path);
   wxString ToTimeFormat(double t);

private:

   wxString          mXlispPath;

   wxFileName        mFileName;  ///< Name of the Nyquist script file this effect is loaded from
   wxDateTime        mFileModified; ///< When the script was last modified on disk

   bool              mStop;
   bool              mBreak;
   bool              mCont;

   bool              mExternal;
   bool              mIsSpectral = false;
   bool              mIsTool = false;
   /** True if the code to execute is obtained interactively from the user via
    * the "Nyquist Effect Prompt", or "Nyquist Prompt", false for all other effects (lisp code read from
    * files)
    */
   bool              mIsPrompt;
   TranslatableString mInitError;
   wxString          mInputCmd; // history: exactly what the user typed
   wxString          mParameters; // The parameters of to be fed to a nested prompt
   wxString          mCmd;      // the command to be processed
   TranslatableString mName;   ///< Name of the Effect (untranslated)
   TranslatableString mPromptName; // If a prompt, we need to remember original name.
   bool              mHelpFileExists;
   EffectType        mType = EffectTypeProcess;
   EffectType        mPromptType; // If a prompt, need to remember original type.

   bool              mEnablePreview = true;
   bool              mDebugButton = true;

   bool              mDebug;        // When true, debug window is shown.
   bool              mRedirectOutput;
   bool              mProjectChanged;
   wxString          mDebugOutputStr;
   TranslatableString mDebugOutput;

   int               mVersion;
   std::vector<NyqControl>   mControls;
   std::vector<NyqValue> mBindings; //!< in correspondence with mControls
   NyqProperties mProperties;

   unsigned          mCurNumChannels;
   WaveTrack         *mCurTrack[2];
   sampleCount       mCurStart[2];
   sampleCount       mCurLen;
   int               mTrackIndex;
   bool              mFirstInGroup;
   Track             *mgtLast = nullptr;
   double            mOutputTime;
   unsigned          mCount;
   unsigned          mNumSelectedChannels;
   double            mProgressIn;
   double            mProgressOut;
   double            mProgressTot;
   double            mScale;

   using Buffer = std::unique_ptr<float[]>;
   Buffer            mCurBuffer[2];
   sampleCount       mCurBufferStart[2];
   size_t            mCurBufferLen[2];

   WaveTrack        *mOutputTrack[2];

   wxString          mProps;
   wxString          mPerTrackProps;

   wxTextCtrl *mCommandText;

   std::exception_ptr mpException {};

   DECLARE_EVENT_TABLE()

   friend class NyquistEffectsModule;
};

class NyquistOutputDialog final : public wxDialogWrapper
{
public:
   NyquistOutputDialog(wxWindow * parent, wxWindowID id,
                       const TranslatableString & title,
                       const TranslatableString & prompt,
                       const TranslatableString &message);

private:
   void OnOk(wxCommandEvent & event);

private:
   DECLARE_EVENT_TABLE()
};


#endif
