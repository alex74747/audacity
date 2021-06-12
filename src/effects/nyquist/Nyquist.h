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

struct NyquistNameAndType {
};

struct Tokenizer;

class NyquistProgram {
public:
   explicit NyquistProgram(const FilePath &fname);

   EffectType GetType() const { return mType; }
   bool IsPrompt() const { return mIsPrompt; }
   bool IsUpToDate() const{
      return mFileName.GetModificationTime().IsLaterThan(mFileModified); }
   const std::vector<NyqControl> &GetControls() const { return mControls; }
   bool DefineParams( ShuttleParams &S, std::vector<NyqValue> &bindings );
   bool GetAutomationParameters(
      CommandParameters & parms, std::vector<NyqValue> &bindings);
   bool SetAutomationParameters(
      CommandParameters & parms, std::vector<NyqValue> &bindings,
      bool isBatchProcessing);
   int SetLispVarsFromParameters(
      CommandParameters & parms, std::vector<NyqValue> &bindings,
      bool bTestOnly);
   const TranslatableString &InitializationError() const { return mInitError; }
   PluginPath GetPath() const;

private:
   void Parse(const FilePath &fname);
   bool ParseCommand(const wxString & cmd);
   bool ParseProgram(wxInputStream & stream);
   static std::vector<EnumValueSymbol> ParseChoice(const wxString & text);
   FileExtensions ParseFileExtensions(const wxString & text);
   FileNames::FileType ParseFileType(const wxString & text);
   FileNames::FileTypes ParseFileTypes(const wxString & text);
   bool Parse(Tokenizer &tokenizer, const wxString &line, bool eof, bool first);

   EffectType        mType = EffectTypeProcess;
   std::vector<NyqControl>   mControls;
   wxString          mInputCmd; // history: exactly what the user typed
   wxString          mParameters; // The parameters of to be fed to a nested prompt

   wxFileName        mFileName;  ///< Name of the Nyquist script file this effect is loaded from
   wxDateTime        mFileModified; ///< When the script was last modified on disk

   /** True if the code to execute is obtained interactively from the user via
    * the "Nyquist Effect Prompt", or "Nyquist Prompt", false for all other effects (lisp code read from
    * files)
    */
   bool              mIsPrompt{ false };
   bool              mOK{ false };
   TranslatableString mInitError;
};

//! Holds parameters and state for one processing of a Nyquist effect
class AUDACITY_DLL_API NyquistContext {
public:
   NyquistContext(const TranslatableString &debugOutput,
      EffectType effectType, int version, double t0, double t1,
      const wxString &props, const wxString &perTrackProps);
   ~NyquistContext();

   //! @name interactive control of the progress of the Lisp interpreter
   //! @{
   void Continue();
   void Break();
   void Stop();
   //! @}

   void RedirectOutput();

   void ClearBuffers();
   bool BeginTrack( WaveTrack *pTrack );
   bool ProcessOne();
   void EndTrack( WaveTrack *pTrack );
   bool ProcessLoop();

   EffectType GetType() const { return mEffectType; }

   const TranslatableString &DebugOutput() const { return mDebugOutput; } 
private:
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

   std::exception_ptr mpException {};
   const wxString mProps;
   const wxString mPerTrackProps;
   unsigned          mCurNumChannels;
   unsigned          mCount{ 0 };

   bool              mFirstInGroup{ true };
   Track             *mgtLast = nullptr;

   WaveTrack         *mCurTrack[2];
   sampleCount       mCurStart[2];
   sampleCount       mCurLen;

   const EffectType mEffectType;
   const int mVersion;

   const double mT0, mT1;

   wxString          mDebugOutputStr;
   TranslatableString mDebugOutput;

   double            mProgressIn{ 0 };
   double            mProgressOut{ 0 };
   double            mProgressTot{ 0 };

   int               mTrackIndex{ 0 };
   bool              mStop{ false };
   bool              mBreak{ false };
   bool              mCont{ false };
   bool              mRedirectOutput{ false };


   using Buffer = std::unique_ptr<float[]>;
   Buffer            mCurBuffer[2];
   sampleCount       mCurBufferStart[2];
   size_t            mCurBufferLen[2];

   double            mScale;
   Effect *const mpEffect;
};

class AUDACITY_DLL_API NyquistEffect final : public Effect
{
public:

   /** @param fName File name of the Nyquist script defining this effect. If
    * an empty string, then prompt the user for the Nyquist code to interpret.
    */
   NyquistEffect(const FilePath &fName);
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
   void SetCommand(const wxString &cmd);

   // non-null only while processing
   NyquistContext *GetContext() const { return mpContext.get(); };

private:
   static int mReentryCount;
   // NyquistEffect implementation

   void BuildPromptWindow(ShuttleGui & S);
   void BuildEffectWindow(ShuttleGui & S);

   bool TransferDataToPromptWindow();
   bool TransferDataToEffectWindow();

   bool TransferDataFromPromptWindow();
   bool TransferDataFromEffectWindow();

   bool IsOk();
   const TranslatableString &InitializationError() const
   { return mpProgram->InitializationError(); }

   static FilePaths GetNyquistSearchPath();

   static wxString NyquistToWxString(const char *nyqString);

   void SetProperties();

   void OnLoad(wxCommandEvent & evt);
   void OnSave(wxCommandEvent & evt);
   void OnDebug(wxCommandEvent & evt);

   void OnText(wxCommandEvent & evt);
   void OnSlider(wxCommandEvent & evt);
   void OnChoice(wxCommandEvent & evt);
   void OnTime(wxCommandEvent & evt);
   void OnFileButton(wxCommandEvent & evt);

   bool validatePath(wxString path);
   wxString ToTimeFormat(double t);

private:

   wxString          mXlispPath;


   std::unique_ptr<NyquistContext> mpContext;

   bool              mExternal;
   bool              mIsSpectral = false;
   bool              mIsTool = false;
   wxString          mCmd;      // the command to be processed
   TranslatableString mName;   ///< Name of the Effect (untranslated)
   TranslatableString mPromptName; // If a prompt, we need to remember original name.
   bool              mHelpFileExists;
   EffectType        mPromptType; // If a prompt, need to remember original type.

   bool              mEnablePreview = true;
   bool              mDebugButton = true;

   bool              mDebug;        // When true, debug window is shown.
   bool              mProjectChanged;

   int               mVersion;
   std::vector<NyqValue> mBindings; //!< in correspondence with mControls
   NyqProperties mProperties;

   //! @invariant `mpProgram != nullptr`
   std::unique_ptr <NyquistProgram> mpProgram;

   double            mOutputTime;
   unsigned          mNumSelectedChannels;

   WaveTrack        *mOutputTrack[2];

   wxString          mProps;
   wxString          mPerTrackProps;

   wxTextCtrl *mCommandText;

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
