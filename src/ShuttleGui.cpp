/**********************************************************************

  Audacity: A Digital Audio Editor

  ShuttleGui.cpp

  James Crook

  Audacity is free software.
  This file is licensed under the wxWidgets license, see License.txt

**********************************************************************//**

\file ShuttleGui.cpp
\brief Implements ShuttleGui, ShuttleGuiBase and InvisiblePanel.

*//***************************************************************//**

\class ShuttleGui
\brief
  Derived from ShuttleGuiBase, an Audacity specific class for shuttling
  data to and from GUI.

  ShuttleGui extends the idea of the data Shuttle class to include creation
  of dialog controls.  As part of this it provides an interface to sizers
  that leads to shorter more readable code.

  It also allows the code that is used to create dialogs to be reused
  to shuttle information in and out.

  Most of the ShuttleGui functions are actually defined in
  ShuttleGuiBase.
     - wxWidgets widgets are dealt with by ShuttleGuiBase.
     - Audacity specific widgets are dealt with by ShuttleGui

  There is documentation on how to use this class in \ref ShuttleSystem

*//***************************************************************//**

\class ShuttleGuiBase
\brief Base class for shuttling data to and from a GUI.

see also ShuttleGui

Use the:
  - \p Start / \p End methods for containers, like two-column-layout.
  - \p Add methods if you are only interested in creating the controls.
  - \p Tie methods if you also want to exchange data using ShuttleGui.

The code in this file is fairly repetitive.  We are dealing with
  - Many different types of Widget.
  - Creation / Reading / Writing / Exporting / Importing
  - int, float, string variants (for example of TextCtrl contents).

A technique used to reduce the size of the \p Tie functions is to
have one generic \p Tie function that uses WrappedType for its
data type.  Type specific \p Tie functions themselves call the generic
variant.

A second technique used to reduce the size of \p Tie functions
only comes into play for two-step \p Tie functions.  (A two step
\p Tie function is one that transfers data between the registry
and the GUI via an intermediate temporary variable). In the two
step style, a function ShuttleGuiBase::DoStep() determines which
transfers in the function are to be done, reducing repetitive
if-then-else's.

Although unusual, these two techniques make the code easier to
add to and much easier to check for correctness.  The alternative
'more obvious' code that just repeats code as needed is
considerably longer.

You would rarely use ShuttleGuiBase directly, instead you'd use
ShuttleGui.

There is DOxygen documentation on how to use the ShuttleGui
class in \ref ShuttleSystem .

*//***************************************************************//**

\class InvisiblePanel
\brief An InvisiblePanel is a panel which does not repaint its
own background.

It is used (a) To group together widgets which need to be refreshed
together.  A single refresh of the panel causes all the subwindows to
refresh.  (b) as a base class for some flicker-free classes for which
the background is never repainted.

JKC: InvisiblePanel will probably be replaced in time by a mechanism
for registering for changes.

*//******************************************************************/



#include "ShuttleGui.h"



#include "MemoryX.h"
#include "Prefs.h"
#include "ShuttlePrefs.h"
#include "Theme.h"

#include <wx/setup.h> // for wxUSE_* macros
#include <wx/grid.h>
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include <wx/simplebook.h>
#include <wx/treectrl.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/bmpbuttn.h>
#include <wx/wrapsizer.h>

#include "ComponentInterface.h"
#include "widgets/ReadOnlyText.h"
#include "widgets/wxPanelWrapper.h"
#include "widgets/wxTextCtrlWrapper.h"
#include "AllThemeResources.h"

#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/dcclient.h>
#include <wx/frame.h>
#include <wx/listbox.h>
#include <wx/menu.h>
#include <wx/radiobut.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/valtext.h>

#if wxUSE_ACCESSIBILITY
#include "widgets/WindowAccessible.h"
#endif

PreferenceVisitor::~PreferenceVisitor() = default;

ShuttleGuiState::ShuttleGuiState(
   wxWindow *pDlg, teShuttleMode ShuttleMode, bool vertical, wxSize minSize,
   const std::shared_ptr< PreferenceVisitor > &pVisitor )
   : mpDlg{ pDlg }
   , mShuttleMode{ ShuttleMode }
   , mpVisitor{ pVisitor }
{
   wxASSERT( (pDlg != NULL ) || ( ShuttleMode != eIsCreating));

   if( mShuttleMode != eIsCreating )
      return;

   mpSizer = mpParent->GetSizer();

#if 0
   if( mpSizer == NULL )
   {
      wxWindow * pGrandParent = mpParent->GetParent();
      if( pGrandParent )
      {
         mpSizer = pGrandParent->GetSizer();
      }
   }
#endif

   if( !mpSizer )
   {
      mpParent->SetSizer(
         mpSizer = safenew wxBoxSizer(vertical ? wxVERTICAL : wxHORIZONTAL));
   }
   PushSizer();
   mpSizer->SetMinSize(minSize);
}

ShuttleGuiBase::ShuttleGuiBase(
   wxWindow * pParent, teShuttleMode ShuttleMode, bool vertical, wxSize minSize,
   const std::shared_ptr< PreferenceVisitor > &pVisitor )
{
   mpState = std::make_shared< ShuttleGuiState >(
      pParent, ShuttleMode, vertical, minSize, pVisitor );
}

ShuttleGuiBase::~ShuttleGuiBase()
{
}

void ShuttleGuiBase::ResetId()
{
   miIdSetByUser = -1;
   miId = -1;
   mpState -> miIdNext = 3000;
}


int ShuttleGuiBase::GetBorder() const noexcept
{
   return mpState -> miBorder;
}

/// Used to modify an already placed FlexGridSizer to make a column stretchy.
void ShuttleGuiBase::SetStretchyCol( int i )
{
   if( mpState -> mShuttleMode != eIsCreating )
      return;
   wxFlexGridSizer *pSizer = wxDynamicCast(mpState -> mpSizer, wxFlexGridSizer);
   wxASSERT( pSizer );
   pSizer->AddGrowableCol( i, 1 );
}

/// Used to modify an already placed FlexGridSizer to make a row stretchy.
void ShuttleGuiBase::SetStretchyRow( int i )
{
   if( mpState -> mShuttleMode != eIsCreating )
      return;
   wxFlexGridSizer *pSizer = wxDynamicCast(mpState -> mpSizer, wxFlexGridSizer);
   wxASSERT( pSizer );
   pSizer->AddGrowableRow( i, 1 );
}


//---- Add Functions.

void ShuttleGuiBase::HandleOptionality(const TranslatableLabel &Prompt)
{
   // If creating, will be handled by an AddPrompt.
   if( mpState -> mShuttleMode == eIsCreating )
      return;
   //wxLogDebug( "Optionality: [%s] Id:%i (%i)", Prompt.c_str(), miId, miIdSetByUser ) ;
   if( mpbOptionalFlag ){
      bool * pVar = mpbOptionalFlag;
      mpbOptionalFlag = nullptr;
      TieCheckBox( Prompt, *pVar);
   }
}

/// Right aligned text string.
void ShuttleGuiBase::AddPrompt(const TranslatableLabel &Prompt, int wrapWidth)
{
   if( mpState -> mShuttleMode != eIsCreating )
      return;
   //wxLogDebug( "Prompt: [%s] Id:%i (%i)", Prompt.c_str(), miId, miIdSetByUser ) ;
   if( mpbOptionalFlag ){
      bool * pVar = mpbOptionalFlag;
      mpbOptionalFlag = nullptr;
      TieCheckBox( {}, *pVar);
      //return;
   }
   if( Prompt.empty() )
      return;
   miProp=1;
   const auto translated = Prompt.Translation();
   auto text = safenew wxStaticText(GetParent(), -1, translated, wxDefaultPosition, wxDefaultSize,
      GetStyle( wxALIGN_RIGHT ));
   mpWind = text;
   if (wrapWidth > 0)
      text->Wrap(wrapWidth);
   mpWind->SetName(wxStripMenuCodes(translated)); // fix for bug 577 (NVDA/Narrator screen readers do not read static text in dialogs)
   UpdateSizersCore( false, wxALL | wxALIGN_CENTRE_VERTICAL, true );
}

/// Left aligned text string.
void ShuttleGuiBase::AddUnits(const TranslatableString &Units, int wrapWidth)
{
   if( Units.empty() )
      return;
   if( mpState -> mShuttleMode != eIsCreating )
      return;
   miProp = 1;
   const auto translated = Units.Translation();
   auto text = safenew wxStaticText(GetParent(), -1, translated, wxDefaultPosition, mItem.mWindowSize,
      GetStyle( wxALIGN_LEFT ));
   mpWind = text;
   if (wrapWidth > 0)
      text->Wrap(wrapWidth);
   mpWind->SetName(translated); // fix for bug 577 (NVDA/Narrator screen readers do not read static text in dialogs)
   UpdateSizersCore( false, wxALL | wxALIGN_CENTRE_VERTICAL );
}

/// Centred text string.
void ShuttleGuiBase::AddTitle(const TranslatableString &Title, int wrapWidth)
{
   if( Title.empty() )
      return;
   if( mpState -> mShuttleMode != eIsCreating )
      return;
   const auto translated = Title.Translation();
   auto text = safenew wxStaticText(GetParent(), -1, translated, wxDefaultPosition, mItem.mWindowSize,
      GetStyle( wxALIGN_CENTRE ));
   mpWind = text;
   if (wrapWidth > 0)
      text->Wrap(wrapWidth);
   mpWind->SetName(translated); // fix for bug 577 (NVDA/Narrator screen readers do not read static text in dialogs)
   UpdateSizers();
}

/// Very generic 'Add' function.  We can add anything we like.
/// Useful for unique controls
wxWindow* ShuttleGuiBase::AddWindow(wxWindow* pWindow, int PositionFlags)
{
   if( mpState -> mShuttleMode != eIsCreating )
      return pWindow;
   mpWind = pWindow;
   SetProportions( 0 );
   UpdateSizersCore(false, PositionFlags | wxALL);
   return pWindow;
}

NumericTextCtrl * ShuttleGuiBase::AddNumericTextCtrl(NumericConverter::Type type,
      const NumericFormatSymbol &formatName,
      double value,
      double sampleRate,
      const NumericTextCtrl::Options &options,
      const wxPoint &pos,
      const wxSize &size )
{
   UseUpId();
   NumericTextCtrl * pCtrl = nullptr;
   mpWind = pCtrl = safenew NumericTextCtrl( GetParent(), miId,
      type, formatName, value, sampleRate, options, pos, size );
   CheckEventType( mItem, {wxEVT_TEXT} );
   UpdateSizers();
   return pCtrl;
}

wxCheckBox * ShuttleGuiBase::AddCheckBox( const TranslatableLabel &Prompt, bool Selected)
{
   HandleOptionality( Prompt );
   auto realPrompt = Prompt.Translation();
   if( mpbOptionalFlag )
   {
      AddPrompt( {} );
      //realPrompt = L"";
   }

   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return wxDynamicCast(wxWindow::FindWindowById( miId, mpState -> mpDlg), wxCheckBox);
   wxCheckBox * pCheckBox;
   miProp=0;
   mpWind = pCheckBox = safenew wxCheckBox(GetParent(), miId, realPrompt, wxDefaultPosition, mItem.mWindowSize,
      GetStyle( 0 ));
   CheckEventType( mItem, {wxEVT_CHECKBOX} );
   pCheckBox->SetValue(Selected);
   if (realPrompt.empty()) {
      // NVDA 2018.3 does not read controls which are buttons, check boxes or radio buttons which have
      // an accessibility name which is empty. Bug 1980.
#if wxUSE_ACCESSIBILITY
      // so that name can be set on a standard control
      pCheckBox->SetAccessible(safenew WindowAccessible(pCheckBox));
#endif
      pCheckBox->SetName(L"\a");      // non-empty string which screen readers do not read
   }
   UpdateSizers();
   return pCheckBox;
}

/// For a consistent two-column layout we want labels on the left and
/// controls on the right.  CheckBoxes break that rule, so we fake it by
/// placing a static text label and then a tick box with an empty label.
wxCheckBox * ShuttleGuiBase::AddCheckBoxOnRight( const TranslatableLabel &Prompt, bool Selected)
{
   HandleOptionality( Prompt );
   AddPrompt( Prompt );
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return wxDynamicCast(wxWindow::FindWindowById( miId, mpState -> mpDlg), wxCheckBox);
   wxCheckBox * pCheckBox;
   miProp=0;
   mpWind = pCheckBox = safenew wxCheckBox(GetParent(), miId, L"", wxDefaultPosition, mItem.mWindowSize,
      GetStyle( 0 ));
   pCheckBox->SetValue(Selected);
   pCheckBox->SetName(Prompt.Stripped().Translation());
   UpdateSizers();
   return pCheckBox;
}

wxButton * ShuttleGuiBase::AddButton(
   const TranslatableLabel &Label, int PositionFlags, bool setDefault)
{
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return wxDynamicCast(wxWindow::FindWindowById( miId, mpState -> mpDlg), wxButton);
   wxButton * pBtn;
   const auto translated = Label.Translation();
   mpWind = pBtn = safenew wxButton(GetParent(), miId,
      translated, wxDefaultPosition, mItem.mWindowSize,
      GetStyle( 0 ) );
   mpWind->SetName(wxStripMenuCodes(translated));
   CheckEventType( mItem, {wxEVT_BUTTON} );
   miProp=0;
   UpdateSizersCore(false, PositionFlags | wxALL);
   if (setDefault)
      pBtn->SetDefault();
   return pBtn;
}

wxBitmapButton * ShuttleGuiBase::AddBitmapButton(
   const wxBitmap &Bitmap, int PositionFlags, bool setDefault)
{
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return wxDynamicCast(wxWindow::FindWindowById( miId, mpState -> mpDlg), wxBitmapButton);
   wxBitmapButton * pBtn;
   mpWind = pBtn = safenew wxBitmapButton(GetParent(), miId, Bitmap,
      wxDefaultPosition, mItem.mWindowSize, GetStyle( wxBU_AUTODRAW ) );
   pBtn->SetBackgroundColour(
      wxColour( 246,246,243));
//      wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
   miProp=0;
   UpdateSizersCore(false, PositionFlags | wxALL);
   if (setDefault)
      pBtn->SetDefault();
   return pBtn;
}

wxChoice * ShuttleGuiBase::AddChoice( const TranslatableLabel &Prompt,
   const TranslatableStrings &choices, int Selected )
{
   HandleOptionality( Prompt );
   AddPrompt( Prompt );
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return wxDynamicCast(wxWindow::FindWindowById( miId, mpState -> mpDlg), wxChoice);
   wxChoice * pChoice;
   miProp=0;

   mpWind = pChoice = safenew wxChoice(
      GetParent(),
      miId,
      wxDefaultPosition,
      mItem.mWindowSize,
      transform_container<wxArrayString>(
         choices, std::mem_fn( &TranslatableString::Translation ) ),
      GetStyle( 0 ) );
   CheckEventType( mItem, {wxEVT_CHOICE} );

   pChoice->SetMinSize( { 180, -1 } );// Use -1 for 'default size' - Platform specific.
#ifdef __WXMAC__
#if wxUSE_ACCESSIBILITY
   // so that name can be set on a standard control
   mpWind->SetAccessible(safenew WindowAccessible(mpWind));
#endif
#endif
   pChoice->SetName(Prompt.Stripped().Translation());
   if ( Selected >= 0 && Selected < (int)choices.size() )
      pChoice->SetSelection( Selected );

   UpdateSizers();
   return pChoice;
}

wxChoice * ShuttleGuiBase::AddChoice( const TranslatableLabel &Prompt,
   const TranslatableStrings &choices, const TranslatableString &Selected )
{
   return AddChoice(
      Prompt, choices, make_iterator_range( choices ).index( Selected ) );
}

void ShuttleGuiBase::AddFixedText(
   const TranslatableString &Str, bool bCenter, int wrapWidth)
{
   const auto translated = Str.Translation();
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return;
   auto text = safenew wxStaticText(GetParent(),
      miId, translated, wxDefaultPosition, mItem.mWindowSize,
      GetStyle( wxALIGN_LEFT ));
   mpWind = text;
   if ( wrapWidth > 0 )
      text->Wrap( wrapWidth );
   mpWind->SetName(wxStripMenuCodes(translated)); // fix for bug 577 (NVDA/Narrator screen readers do not read static text in dialogs)
   if( bCenter )
   {
      miProp=1;
      UpdateSizersC();
   }
   else
      UpdateSizers();
}

wxStaticText * ShuttleGuiBase::AddVariableText(
   const TranslatableString &Str,
   bool bCenter, int PositionFlags, int wrapWidth )
{
   const auto translated = Str.Translation();
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return wxDynamicCast(wxWindow::FindWindowById( miId, mpState -> mpDlg), wxStaticText);

   wxStaticText *pStatic;
   auto text = pStatic = safenew wxStaticText(GetParent(), miId, translated,
      wxDefaultPosition, mItem.mWindowSize,
      GetStyle( wxALIGN_LEFT ));
   mpWind = text;
   if ( wrapWidth > 0 )
      text->Wrap( wrapWidth );
   mpWind->SetName(wxStripMenuCodes(translated)); // fix for bug 577 (NVDA/Narrator screen readers do not read static text in dialogs)
   if( bCenter )
   {
      miProp=1;
      if( PositionFlags )
         UpdateSizersCore( false, PositionFlags );
      else
         UpdateSizersC();
   }
   else
      if( PositionFlags )
         UpdateSizersCore( false, PositionFlags );
      else
         UpdateSizers();
   return pStatic;
}

ReadOnlyText * ShuttleGuiBase::AddReadOnlyText(
   const TranslatableLabel &Caption, const wxString &Value)
{
   const auto translated = Caption.Translation();
   auto style = GetStyle( wxBORDER_NONE );
   HandleOptionality( Caption );
   mItem.miStyle = wxALIGN_CENTER_VERTICAL;
   AddPrompt( Caption );
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return wxDynamicCast(wxWindow::FindWindowById( miId, mpState -> mpDlg), ReadOnlyText);
   ReadOnlyText * pReadOnlyText;
   miProp=0;

   mpWind = pReadOnlyText = safenew ReadOnlyText(GetParent(), miId, Value,
      wxDefaultPosition, wxDefaultSize, GetStyle( style ));
   mpWind->SetName(wxStripMenuCodes(translated));
   UpdateSizers();
   return pReadOnlyText;
}

wxComboBox * ShuttleGuiBase::AddCombo(
   const TranslatableLabel &Prompt,
   const wxString &Selected, const wxArrayStringEx & choices )
{
   const auto translated = Prompt.Translation();
   HandleOptionality( Prompt );
   AddPrompt( Prompt );
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return wxDynamicCast(wxWindow::FindWindowById( miId, mpState -> mpDlg), wxComboBox);
   wxComboBox * pCombo;
   miProp=0;

   int n = choices.size();
   if( n>50 ) n=50;
   int i;
   wxString Choices[50];
   for(i=0;i<n;i++)
   {
      Choices[i] = choices[i];
   }

   mpWind = pCombo = safenew wxComboBox(GetParent(), miId, Selected, wxDefaultPosition, mItem.mWindowSize,
      n, Choices, GetStyle( 0 ));
   mpWind->SetName(wxStripMenuCodes(translated));

   UpdateSizers();
   return pCombo;
}


wxRadioButton * ShuttleGuiBase::DoAddRadioButton(
   const TranslatableLabel &Prompt, int style)
{
   const auto translated = Prompt.Translation();
   /// \todo This function and the next two, suitably adapted, could be
   /// used by TieRadioButton.
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return wxDynamicCast(wxWindow::FindWindowById( miId, mpState -> mpDlg), wxRadioButton);
   wxRadioButton * pRad;
   mpWind = pRad = safenew wxRadioButton(GetParent(), miId, translated,
      wxDefaultPosition, mItem.mWindowSize, GetStyle( style ) );
   mpWind->SetName(wxStripMenuCodes(translated));
   if ( style )
      pRad->SetValue( true );
   UpdateSizers();
   pRad->SetValue( style != 0 );

   if ( auto pList = mRadioButtons.get() )
      pList->push_back( pRad );

   return pRad;
}

wxRadioButton * ShuttleGuiBase::AddRadioButton(
   const TranslatableLabel &Prompt )
{
   wxASSERT( mRadioCount >= 0); // Did you remember to use StartRadioButtonGroup() ?
   return DoAddRadioButton( Prompt, (mRadioCount++ == 0) ? wxRB_GROUP : 0 );
}

#ifdef __WXMAC__
void wxSliderWrapper::SetFocus()
{
   // bypassing the override in wxCompositeWindow<wxSliderBase> which ends up
   // doing nothing
   return wxSliderBase::SetFocus();
}
#endif

wxSlider * ShuttleGuiBase::AddSlider(
   const TranslatableLabel &Prompt, int pos, int Max, int Min,
   int lineSize, int pageSize )
{
   HandleOptionality( Prompt );
   AddPrompt( Prompt );
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return wxDynamicCast(wxWindow::FindWindowById( miId, mpState -> mpDlg), wxSlider);
   wxSlider * pSlider;
   mpWind = pSlider = safenew wxSliderWrapper(GetParent(), miId,
      pos, Min, Max,
      wxDefaultPosition,
      // Bug2289:  On Linux at least, sliders like to be constructed with the
      // proper size, not reassigned size later
      mItem.mWindowSize,
      GetStyle( wxSL_HORIZONTAL | wxSL_LABELS | wxSL_AUTOTICKS )
      );
   CheckEventType( mItem, {wxEVT_SLIDER} );
#if wxUSE_ACCESSIBILITY
   // so that name can be set on a standard control
   mpWind->SetAccessible(safenew WindowAccessible(mpWind));
#endif
   mpWind->SetName(wxStripMenuCodes(Prompt.Translation()));
   miProp=1;

   if ( lineSize > 0 )
      pSlider->SetLineSize( lineSize );
   if ( pageSize > 0 )
      pSlider->SetPageSize( pageSize );
   
   UpdateSizers();
   return pSlider;
}

wxSpinCtrl * ShuttleGuiBase::AddSpinCtrl(
   const TranslatableLabel &Prompt, int Value, int Max, int Min)
{
   const auto translated = Prompt.Translation();
   HandleOptionality( Prompt );
   AddPrompt( Prompt );
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return wxDynamicCast(wxWindow::FindWindowById( miId, mpState -> mpDlg), wxSpinCtrl);
   wxSpinCtrl * pSpinCtrl;
   mpWind = pSpinCtrl = safenew wxSpinCtrl(GetParent(), miId,
      wxEmptyString,
      wxDefaultPosition, mItem.mWindowSize,
      GetStyle( wxSP_VERTICAL | wxSP_ARROW_KEYS ),
      Min, Max, Value
      );
   mpWind->SetName(wxStripMenuCodes(translated));
   miProp=1;
   UpdateSizers();
   return pSpinCtrl;
}

wxTextCtrl * ShuttleGuiBase::AddTextBox(
   const TranslatableLabel &Prompt, const wxString &Value, const int nChars)
{
   const auto translated = Prompt.Translation();
   HandleOptionality( Prompt );
   AddPrompt( Prompt );
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return wxDynamicCast(wxWindow::FindWindowById( miId, mpState -> mpDlg), wxTextCtrl);
   wxTextCtrl * pTextCtrl;
   wxSize Size(mItem.mWindowSize);
   if( nChars > 0 && Size.GetX() == -1 )
   {
      int width;
      mpState -> mpDlg->GetTextExtent( L"9", &width, nullptr );
      Size.SetWidth( nChars * width );
   }
   miProp=0;

#ifdef EXPERIMENTAL_RIGHT_ALIGNED_TEXTBOXES
   long flags = wxTE_RIGHT;
#else
   long flags = wxTE_LEFT;
#endif

   mpWind = pTextCtrl = safenew wxTextCtrlWrapper(GetParent(), miId, Value,
      wxDefaultPosition, Size, GetStyle( flags ));
   CheckEventType( mItem, {wxEVT_TEXT} );
#if wxUSE_ACCESSIBILITY
   // so that name can be set on a standard control
   mpWind->SetAccessible(safenew WindowAccessible(mpWind));
#endif
   mpWind->SetName(wxStripMenuCodes(translated));
   UpdateSizers();
   return pTextCtrl;
}

wxTextCtrl * ShuttleGuiBase::AddNumericTextBox(
   const TranslatableLabel &Prompt, const wxString &Value, const int nChars)
{
   const auto translated = Prompt.Translation();
   HandleOptionality( Prompt );
   AddPrompt( Prompt );
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return wxDynamicCast(wxWindow::FindWindowById( miId, mpState -> mpDlg), wxTextCtrl);
   wxTextCtrl * pTextCtrl;
   wxSize Size(mItem.mWindowSize);
   if( nChars > 0 && Size.GetX() == -1 )
   {
      Size.SetWidth( nChars *5 );
   }
   miProp=0;

#ifdef EXPERIMENTAL_RIGHT_ALIGNED_TEXTBOXES
   long flags = wxTE_RIGHT;
#else
   long flags = wxTE_LEFT;
#endif

   wxTextValidator Validator(wxFILTER_NUMERIC);
   mpWind = pTextCtrl = safenew wxTextCtrl(GetParent(), miId, Value,
      wxDefaultPosition, Size, GetStyle( flags ),
      Validator // It's OK to pass this.  It will be cloned.
      );
#if wxUSE_ACCESSIBILITY
   // so that name can be set on a standard control
   mpWind->SetAccessible(safenew WindowAccessible(mpWind));
#endif
   mpWind->SetName(wxStripMenuCodes(translated));
   UpdateSizers();
   return pTextCtrl;
}

/// Multiline text box that grows.
wxTextCtrl * ShuttleGuiBase::AddTextWindow(const wxString &Value)
{
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return wxDynamicCast(wxWindow::FindWindowById( miId, mpState -> mpDlg), wxTextCtrl);
   wxTextCtrl * pTextCtrl;
   SetProportions( 1 );
   mpWind = pTextCtrl = safenew wxTextCtrl(GetParent(), miId, Value,
      wxDefaultPosition, mItem.mWindowSize, GetStyle( wxTE_MULTILINE ));
#if wxUSE_ACCESSIBILITY
   // so that name can be set on a standard control
   mpWind->SetAccessible(safenew WindowAccessible(mpWind));
#endif
   UpdateSizers();
   // Start off at start of window...
   pTextCtrl->SetInsertionPoint( 0 );
   pTextCtrl->ShowPosition( 0 );
   return pTextCtrl;
}

/// Single line text box of fixed size.
void ShuttleGuiBase::AddConstTextBox(
   const TranslatableString &Prompt, const TranslatableString &Value)
{
   TranslatableLabel label{ Prompt };
   HandleOptionality( label );
   AddPrompt( label );
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return;
//      return wxDynamicCast(wxWindow::FindWindowById( miId, mpDlg), wx);
   miProp=0;
   UpdateSizers();
   miProp=0;
   const auto translatedValue = Value.Translation();
   mpWind = safenew wxStaticText(GetParent(), miId,
      translatedValue, wxDefaultPosition, mItem.mWindowSize,
      GetStyle( 0 ));
   mpWind->SetName(translatedValue); // fix for bug 577 (NVDA/Narrator screen readers do not read static text in dialogs)
   UpdateSizers();
}

wxListBox * ShuttleGuiBase::AddListBox(const wxArrayStringEx &choices)
{
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return wxDynamicCast(wxWindow::FindWindowById( miId, mpState -> mpDlg), wxListBox);
   wxListBox * pListBox;
   SetProportions( 1 );
   mpWind = pListBox = safenew wxListBox(GetParent(), miId,
      wxDefaultPosition, mItem.mWindowSize, choices, GetStyle(0));
   pListBox->SetMinSize( wxSize( 120,150 ));
   CheckEventType( mItem, {wxEVT_LISTBOX, wxEVT_LISTBOX_DCLICK} );
   UpdateSizers();
   return pListBox;
}


wxGrid * ShuttleGuiBase::AddGrid()
{
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return wxDynamicCast(wxWindow::FindWindowById( miId, mpState -> mpDlg), wxGrid);
   wxGrid * pGrid;
   SetProportions( 1 );
   mpWind = pGrid = safenew wxGrid(GetParent(), miId, wxDefaultPosition,
      mItem.mWindowSize, GetStyle( wxWANTS_CHARS ));
   pGrid->SetMinSize( wxSize( 120, 150 ));
   UpdateSizers();
   return pGrid;
}

wxListCtrl * ShuttleGuiBase::AddListControl(
   std::initializer_list<const ListControlColumn> columns,
   long listControlStyles
)
{
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return wxDynamicCast(wxWindow::FindWindowById( miId, mpState -> mpDlg), wxListCtrl);
   wxListCtrl * pListCtrl;
   SetProportions( 1 );
   mpWind = pListCtrl = safenew wxListCtrl(GetParent(), miId,
      wxDefaultPosition, mItem.mWindowSize, GetStyle( wxLC_ICON ));
   pListCtrl->SetMinSize( wxSize( 120,150 ));
   UpdateSizers();

   DoInsertListColumns( pListCtrl, listControlStyles, columns );

   return pListCtrl;
}

wxListCtrl * ShuttleGuiBase::AddListControlReportMode(
   std::initializer_list<const ListControlColumn> columns,
   long listControlStyles
)
{
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return wxDynamicCast(wxWindow::FindWindowById( miId, mpState -> mpDlg), wxListCtrl);
   wxListCtrl * pListCtrl;
   SetProportions( 1 );
   auto Size = mItem.mWindowSize;
   if ( Size == wxDefaultSize )
      Size = wxSize(230,120);//mItem.mWindowSize;
   mpWind = pListCtrl = safenew wxListCtrl(GetParent(), miId,
      wxDefaultPosition,
      Size,
      GetStyle( wxLC_REPORT | wxLC_HRULES | wxLC_VRULES | wxSUNKEN_BORDER ));
//   pListCtrl->SetMinSize( wxSize( 120,150 ));
   UpdateSizers();

   DoInsertListColumns( pListCtrl, listControlStyles, columns );

   return pListCtrl;
}

void ShuttleGuiBase::DoInsertListColumns(
   wxListCtrl *pListCtrl,
   long listControlStyles,
   std::initializer_list<const ListControlColumn> columns )
{
   // Old comment from HistoryWindow.cpp follows
   // -- is it still correct for wxWidgets 3?

   // Do this BEFORE inserting the columns.  On the Mac at least, the
   // columns are deleted and later InsertItem()s will cause Audacity to crash.
   for ( auto style = 1l; style <= listControlStyles; style <<= 1 )
      if ( (style & listControlStyles) )
         pListCtrl->SetSingleStyle(style, true);

   long iCol = 0;
   bool dummyColumn =
      columns.size() > 0 && begin(columns)->format == wxLIST_FORMAT_RIGHT;

   //A dummy first column, which is then deleted, is a workaround -
   // under Windows the first column can't be right aligned.
   if (dummyColumn)
      pListCtrl->InsertColumn( iCol++, wxString{} );

   for (auto &column : columns)
      pListCtrl->InsertColumn(
         iCol++, column.heading.Translation(), column.format, column.width );

   if (dummyColumn)
      pListCtrl->DeleteColumn( 0 );
}

wxTreeCtrl * ShuttleGuiBase::AddTree()
{
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return wxDynamicCast(wxWindow::FindWindowById( miId, mpState -> mpDlg), wxTreeCtrl);
   wxTreeCtrl * pTreeCtrl;
   SetProportions( 1 );
   mpWind = pTreeCtrl = safenew wxTreeCtrl(GetParent(), miId, wxDefaultPosition, mItem.mWindowSize,
      GetStyle( wxTR_HAS_BUTTONS ));
   pTreeCtrl->SetMinSize( wxSize( 120,650 ));
   UpdateSizers();
   return pTreeCtrl;
}

void ShuttleGuiBase::AddIcon(wxBitmap *pBmp)
{
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
//      return wxDynamicCast(wxWindow::FindWindowById( miId, mpDlg), wx);
      return;
   wxBitmapButton * pBtn;
   mpWind = pBtn = safenew wxBitmapButton(GetParent(), miId, *pBmp,
      wxDefaultPosition, mItem.mWindowSize, GetStyle( wxBU_AUTODRAW ) );
   pBtn->SetWindowStyle( wxBORDER_NONE  );
   pBtn->SetCanFocus(false);
   UpdateSizersC();
}

ShuttleGuiBase & ShuttleGuiBase::Prop( int iProp )
{
   miPropSetByUser = iProp;
   return *this;
}

/// Starts a static box around a number of controls.
///  @param Str   The text of the title for the box.
///  @param iProp The resizing proportion value.
/// Use iProp == 0 for a minimum sized static box.
/// Use iProp == 1 for a box that grows if there is space to spare.
wxStaticBox * ShuttleGuiBase::StartStatic(const TranslatableString &Str, int iProp)
{
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return NULL;
   auto translated = Str.Translation();
   wxStaticBox * pBox = safenew wxStaticBoxWrapper(
      GetParent(), miId, translated );
   pBox->SetLabel( translated );
   if (Str.empty()) {
      // NVDA 2018.3 or later does not read the controls in a group box which has
      // an accessibility name which is empty. Bug 2169.
#if wxUSE_ACCESSIBILITY
      // so that name can be set on a standard control
      pBox->SetAccessible(safenew WindowAccessible(pBox));
#endif
      pBox->SetName(L"\a");      // non-empty string which screen readers do not read
   }
   else
      pBox->SetName( wxStripMenuCodes( translated ) );
   mpSubSizer = std::make_unique<wxStaticBoxSizer>(
      pBox,
      wxVERTICAL );
   miSizerProp = iProp;
   UpdateSizers();
   mpState -> mpParent = pBox;
   return pBox;
}

void ShuttleGuiBase::EndStatic()
{
   if( mpState -> mShuttleMode != eIsCreating )
      return;
   mpState -> PopSizer();
   mpState -> mpParent = mpState -> mpParent->GetParent();
}

/// This allows subsequent controls and static boxes to be in
/// a scrolled panel.  Very handy if you are running out of space
/// on a dialog.
///
/// The iStyle parameter is used in some very hacky code that
/// dynamically repopulates a dialog.  It also controls the
/// background colour.  Look at the code for details.
///  @param istyle deprecated parameter, but has been used for hacking.
wxScrolledWindow * ShuttleGuiBase::StartScroller(int iStyle)
{
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return wxDynamicCast(wxWindow::FindWindowById( miId, mpState -> mpDlg), wxScrolledWindow);

   wxScrolledWindow * pScroller;
   mpWind = pScroller = safenew wxScrolledWindow(GetParent(), miId, wxDefaultPosition, mItem.mWindowSize,
      GetStyle( wxSUNKEN_BORDER ) );
   pScroller->SetScrollRate( 20,20 );

   // This fools NVDA into not saying "Panel" when the dialog gets focus
   pScroller->SetName(L"\a");
   pScroller->SetLabel(L"\a");

   SetProportions( 1 );
   if( iStyle==2 )
   {
      UpdateSizersAtStart();
   }
   else
   {
     // mpWind->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_MENUBAR));
      UpdateSizers();  // adds window in to current sizer.
   }

   // create a sizer within the window...
   mpState -> mpParent = pScroller;
   pScroller->SetSizer(mpState -> mpSizer = safenew wxBoxSizer(wxVERTICAL));
   mpState -> PushSizer();
   return pScroller;
}

void ShuttleGuiBase::EndScroller()
{
   if( mpState -> mShuttleMode != eIsCreating )
      return;
   wxSize ScrollSize = mpState -> mpSizer->GetMinSize();
   int yMin = ScrollSize.y+4;
   int xMin = ScrollSize.x+4;
   if( yMin > 400)
   {
      yMin = 400;
      xMin+=50;// extra space for vertical scrollbar.
   }

   mpState -> mpParent->SetMinSize( wxSize(xMin, yMin) );

   mpState -> PopSizer();
   mpState -> mpParent = mpState -> mpParent->GetParent();
}

wxPanel * ShuttleGuiBase::StartPanel(int iStyle)
{
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return wxDynamicCast(wxWindow::FindWindowById( miId, mpState -> mpDlg), wxPanel);
   wxPanel * pPanel;
   mpWind = pPanel = safenew wxPanelWrapper( GetParent(), miId, wxDefaultPosition, mItem.mWindowSize,
      GetStyle( wxNO_BORDER ));

   if( iStyle != 0 )
   {
      mpWind->SetBackgroundColour(
         iStyle==1
         ? wxColour( 190,200,230) :
         wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW)
         );
   }
   SetProportions(0);
   mpState -> miBorder=2;
   UpdateSizers();  // adds window in to current sizer.

   // create a sizer within the window...
   mpState -> mpParent = pPanel;
   pPanel->SetSizer(mpState -> mpSizer = safenew wxBoxSizer(wxVERTICAL));
   mpState -> PushSizer();
   return pPanel;
}

void ShuttleGuiBase::EndPanel()
{
   if( mpState -> mShuttleMode != eIsCreating )
      return;
   mpState -> PopSizer();
   mpState -> mpParent = mpState -> mpParent->GetParent();
}

wxNotebook * ShuttleGuiBase::StartNotebook()
{
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return wxDynamicCast(wxWindow::FindWindowById( miId, mpState -> mpDlg), wxNotebook);
   wxNotebook * pNotebook;
   mpWind = pNotebook = safenew wxNotebook(GetParent(),
      miId, wxDefaultPosition, mItem.mWindowSize, GetStyle( 0 ));
   SetProportions( 1 );
   UpdateSizers();
   mpState -> mpParent = pNotebook;
   return pNotebook;
}

void ShuttleGuiBase::EndNotebook()
{
   //PopSizer();
   mpState -> mpParent = mpState -> mpParent->GetParent();
}


wxSimplebook * ShuttleGuiBase::StartSimplebook()
{
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return wxDynamicCast(wxWindow::FindWindowById( miId, mpState -> mpDlg), wxSimplebook);
   wxSimplebook * pNotebook;
   mpWind = pNotebook = safenew wxSimplebook(GetParent(),
      miId, wxDefaultPosition, mItem.mWindowSize, GetStyle( 0 ));
   SetProportions( 1 );
   UpdateSizers();
   mpState -> mpParent = pNotebook;
   return pNotebook;
}

void ShuttleGuiBase::EndSimplebook()
{
   //PopSizer();
   mpState -> mpParent = mpState -> mpParent->GetParent();
}


wxNotebookPage * ShuttleGuiBase::StartNotebookPage(
   const TranslatableString & Name )
{
   if( mpState -> mShuttleMode != eIsCreating )
      return NULL;
//      return wxDynamicCast(wxWindow::FindWindowById( miId, mpDlg), wx);
   auto pNotebook = static_cast< wxBookCtrlBase* >( mpState -> mpParent );
   wxNotebookPage * pPage = safenew wxPanelWrapper(GetParent());
   const auto translated = Name.Translation();
   pPage->SetName(translated);

   pNotebook->AddPage(
      pPage,
      translated);

   SetProportions( 1 );
   mpState -> mpParent = pPage;
   pPage->SetSizer(mpState -> mpSizer = safenew wxBoxSizer(wxVERTICAL));
   mpState -> PushSizer();
   //   UpdateSizers();
   return pPage;
}

void ShuttleGuiBase::EndNotebookPage()
{
   if( mpState -> mShuttleMode != eIsCreating )
      return;
   mpState -> PopSizer();
   mpState -> mpParent = mpState -> mpParent->GetParent();
}

// Doxygen description is at the start of the file
// this is a wxPanel with erase background disabled.
class InvisiblePanel final : public wxPanelWrapper
{
public:
   InvisiblePanel(
      wxWindow* parent,
      wxWindowID id = -1,
      const wxPoint& pos = wxDefaultPosition,
      const wxSize& size = wxDefaultSize,
      long style = wxTAB_TRAVERSAL ) :
      wxPanelWrapper( parent, id, pos, size, style )
   {
   };
   ~InvisiblePanel(){;};
   void OnPaint( wxPaintEvent &event );
   void OnErase(wxEraseEvent &/*evt*/){;};
   DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(InvisiblePanel, wxPanelWrapper)
//   EVT_PAINT(InvisiblePanel::OnPaint)
     EVT_ERASE_BACKGROUND( InvisiblePanel::OnErase)
END_EVENT_TABLE()

void InvisiblePanel::OnPaint( wxPaintEvent & WXUNUSED(event))
{
   // Don't repaint my background.
   wxPaintDC dc(this);
   // event.Skip(); // swallow the paint event.
}

wxPanel * ShuttleGuiBase::StartInvisiblePanel()
{
   UseUpId();
   if( mpState -> mShuttleMode != eIsCreating )
      return wxDynamicCast(wxWindow::FindWindowById( miId, mpState -> mpDlg), wxPanel);
   wxPanel * pPanel;
   mpWind = pPanel = safenew wxPanelWrapper(GetParent(), miId, wxDefaultPosition, mItem.mWindowSize,
      wxNO_BORDER);

   mpWind->SetBackgroundColour(
      wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE)
      );
   SetProportions( 1 );
   mpState -> miBorder=0;
   UpdateSizers();  // adds window in to current sizer.

   // create a sizer within the window...
   mpState -> mpParent = pPanel;
   pPanel->SetSizer(mpState -> mpSizer = safenew wxBoxSizer(wxVERTICAL));
   mpState -> PushSizer();
   return pPanel;
}

void ShuttleGuiBase::EndInvisiblePanel()
{
   EndPanel();
}


/// Starts a Horizontal Layout.
///  - Use wxEXPAND and 0 to expand horizontally but not vertically.
///  - Use wxEXPAND and 1 to expand horizontally and vertically.
///  - Use wxCENTRE and 1 for no expansion.
/// @param PositionFlag  Typically wxEXPAND or wxALIGN_CENTER.
/// @param iProp         Proportionality for resizing.
void ShuttleGuiBase::StartHorizontalLay( int PositionFlags, int iProp)
{
   if( mpState -> mShuttleMode != eIsCreating )
      return;
   miSizerProp=iProp;
   mpSubSizer = std::make_unique<wxBoxSizer>( wxHORIZONTAL );
   // PRL:  wxALL has no effect because UpdateSizersCore ignores border
   UpdateSizersCore( false, PositionFlags | wxALL );
}

void ShuttleGuiBase::EndHorizontalLay()
{
   if( mpState -> mShuttleMode != eIsCreating )
      return;
   mpState -> PopSizer();
}

void ShuttleGuiBase::StartVerticalLay(int iProp)
{
   if( mpState -> mShuttleMode != eIsCreating )
      return;
   miSizerProp=iProp;
   mpSubSizer = std::make_unique<wxBoxSizer>( wxVERTICAL );
   UpdateSizers();
}

void ShuttleGuiBase::StartVerticalLay(int PositionFlags, int iProp)
{
   if( mpState -> mShuttleMode != eIsCreating )
      return;
   miSizerProp=iProp;
   mpSubSizer = std::make_unique<wxBoxSizer>( wxVERTICAL );
   // PRL:  wxALL has no effect because UpdateSizersCore ignores border
   UpdateSizersCore( false, PositionFlags | wxALL );
}

void ShuttleGuiBase::EndVerticalLay()
{
   if( mpState -> mShuttleMode != eIsCreating )
      return;
   mpState -> PopSizer();
}

void ShuttleGuiBase::StartWrapLay(int PositionFlags, int iProp)
{
   if (mpState -> mShuttleMode != eIsCreating)
      return;

   miSizerProp = iProp;
   mpSubSizer = std::make_unique<wxWrapSizer>(wxHORIZONTAL, 0);

   UpdateSizersCore(false, PositionFlags | wxALL);
}

void ShuttleGuiBase::EndWrapLay()
{
   if (mpState -> mShuttleMode != eIsCreating)
      return;

   mpState -> PopSizer();
}

void ShuttleGuiBase::StartMultiColumn(int nCols, int PositionFlags)
{
   if( mpState -> mShuttleMode != eIsCreating )
      return;
   mpSubSizer = std::make_unique<wxFlexGridSizer>( nCols );
   // PRL:  wxALL has no effect because UpdateSizersCore ignores border
   UpdateSizersCore( false, PositionFlags | wxALL );
}

void ShuttleGuiBase::EndMultiColumn()
{
   if( mpState -> mShuttleMode != eIsCreating )
      return;
   mpState -> PopSizer();
}

/// When we're exchanging with the configured shuttle rather than with the GUI
/// We use this function.
void ShuttleGuiBase::DoDataShuttle( const wxString &Name, WrappedType & WrappedRef )
{
    wxASSERT( mpState -> mpShuttle );
    mpState -> mpShuttle->TransferWrappedType( Name, WrappedRef );
}

//-----------------------------------------------------------------------//

// We now have a group of tie functions which are generic in the type
// they bind to (i.e. WrappedType).
// The type specific versions are much shorter and are later
// in this file.
wxCheckBox * ShuttleGuiBase::DoTieCheckBox(
   const TranslatableLabel &Prompt, WrappedType & WrappedRef)
{
   HandleOptionality( Prompt );
   // The Add function does a UseUpId(), so don't do it here in that case.
   if( mpState -> mShuttleMode == eIsCreating )
      return AddCheckBox( Prompt, WrappedRef.ReadAsString() == L"true");

   UseUpId();

   wxWindow * pWnd      = wxWindow::FindWindowById( miId, mpState -> mpDlg);
   wxCheckBox * pCheckBox = wxDynamicCast(pWnd, wxCheckBox);

   switch( mpState -> mShuttleMode )
   {
   // IF setting internal storage from the controls.
   case eIsGettingFromDialog:
      {
         wxASSERT( pCheckBox );
         WrappedRef.WriteToAsBool( pCheckBox->GetValue() );
      }
      break;
   case eIsSettingToDialog:
      {
         wxASSERT( pCheckBox );
         pCheckBox->SetValue( WrappedRef.ReadAsBool() );
      }
      break;
   default:
      wxASSERT( false );
      break;
   }
   return pCheckBox;
}

wxCheckBox * ShuttleGuiBase::DoTieCheckBoxOnRight(
   const TranslatableLabel &Prompt, WrappedType & WrappedRef)
{
   HandleOptionality( Prompt );
   // The Add function does a UseUpId(), so don't do it here in that case.
   if( mpState -> mShuttleMode == eIsCreating )
      return AddCheckBoxOnRight( Prompt, WrappedRef.ReadAsString() == L"true");

   UseUpId();

   wxWindow * pWnd      = wxWindow::FindWindowById( miId, mpState -> mpDlg);
   wxCheckBox * pCheckBox = wxDynamicCast(pWnd, wxCheckBox);

   switch( mpState -> mShuttleMode )
   {
   // IF setting internal storage from the controls.
   case eIsGettingFromDialog:
      {
         wxASSERT( pCheckBox );
         WrappedRef.WriteToAsBool( pCheckBox->GetValue() );
      }
      break;
   case eIsSettingToDialog:
      {
         wxASSERT( pCheckBox );
         pCheckBox->SetValue( WrappedRef.ReadAsBool() );
      }
      break;
   default:
      wxASSERT( false );
      break;
   }
   return pCheckBox;
}

wxSpinCtrl * ShuttleGuiBase::DoTieSpinCtrl(
   const TranslatableLabel &Prompt,
   WrappedType & WrappedRef, const int max, const int min )
{
   HandleOptionality( Prompt );
   // The Add function does a UseUpId(), so don't do it here in that case.
   if( mpState -> mShuttleMode == eIsCreating )
      return AddSpinCtrl( Prompt, WrappedRef.ReadAsInt(), max, min );

   UseUpId();
   wxSpinCtrl * pSpinCtrl=NULL;

   wxWindow * pWnd  = wxWindow::FindWindowById( miId, mpState -> mpDlg);
   pSpinCtrl = wxDynamicCast(pWnd, wxSpinCtrl);

   switch( mpState -> mShuttleMode )
   {
      // IF setting internal storage from the controls.
   case eIsGettingFromDialog:
      {
         wxASSERT( pSpinCtrl );
         WrappedRef.WriteToAsInt( pSpinCtrl->GetValue() );
      }
      break;
   case eIsSettingToDialog:
      {
         wxASSERT( pSpinCtrl );
         pSpinCtrl->SetValue( WrappedRef.ReadAsInt() );
      }
      break;
   default:
      wxASSERT( false );
      break;
   }
   return pSpinCtrl;
}

wxTextCtrl * ShuttleGuiBase::DoTieTextBox(
   const TranslatableLabel &Prompt, WrappedType & WrappedRef, const int nChars)
{
   HandleOptionality( Prompt );
   // The Add function does a UseUpId(), so don't do it here in that case.
   if( mpState -> mShuttleMode == eIsCreating )
      return AddTextBox( Prompt, WrappedRef.ReadAsString(), nChars );

   UseUpId();
   wxTextCtrl * pTextBox=NULL;

   wxWindow * pWnd  = wxWindow::FindWindowById( miId, mpState -> mpDlg);
   pTextBox = wxDynamicCast(pWnd, wxTextCtrl);

   switch( mpState -> mShuttleMode )
   {
   // IF setting internal storage from the controls.
   case eIsGettingFromDialog:
      {
         wxASSERT( pTextBox );
         WrappedRef.WriteToAsString( pTextBox->GetValue() );
      }
      break;
   case eIsSettingToDialog:
      {
         wxASSERT( pTextBox );
         pTextBox->SetValue( WrappedRef.ReadAsString() );
      }
      break;
   default:
      wxASSERT( false );
      break;
   }
   return pTextBox;
}

wxTextCtrl * ShuttleGuiBase::DoTieNumericTextBox(
   const TranslatableLabel &Prompt, WrappedType & WrappedRef, const int nChars)
{
   HandleOptionality( Prompt );
   // The Add function does a UseUpId(), so don't do it here in that case.
   if( mpState -> mShuttleMode == eIsCreating )
      return AddNumericTextBox( Prompt, WrappedRef.ReadAsString(), nChars );

   UseUpId();
   wxTextCtrl * pTextBox=NULL;

   wxWindow * pWnd  = wxWindow::FindWindowById( miId, mpState -> mpDlg);
   pTextBox = wxDynamicCast(pWnd, wxTextCtrl);

   switch( mpState -> mShuttleMode )
   {
   // IF setting internal storage from the controls.
   case eIsGettingFromDialog:
      {
         wxASSERT( pTextBox );
         WrappedRef.WriteToAsString( pTextBox->GetValue() );
      }
      break;
   case eIsSettingToDialog:
      {
         wxASSERT( pTextBox );
         pTextBox->SetValue( WrappedRef.ReadAsString() );
      }
      break;
   default:
      wxASSERT( false );
      break;
   }
   return pTextBox;
}

wxSlider * ShuttleGuiBase::DoTieSlider(
   const TranslatableLabel &Prompt,
   WrappedType & WrappedRef, const int max, int min )
{
   HandleOptionality( Prompt );
   // The Add function does a UseUpId(), so don't do it here in that case.
   if( mpState -> mShuttleMode != eIsCreating )
      UseUpId();
   wxSlider * pSlider=NULL;
   switch( mpState -> mShuttleMode )
   {
      case eIsCreating:
         {
            pSlider = AddSlider( Prompt, WrappedRef.ReadAsInt(), max, min );
         }
         break;
      // IF setting internal storage from the controls.
      case eIsGettingFromDialog:
         {
            wxWindow * pWnd  = wxWindow::FindWindowById( miId, mpState -> mpDlg);
            pSlider = wxDynamicCast(pWnd, wxSlider);
            wxASSERT( pSlider );
            WrappedRef.WriteToAsInt( pSlider->GetValue() );
         }
         break;
      case eIsSettingToDialog:
         {
            wxWindow * pWnd  = wxWindow::FindWindowById( miId, mpState -> mpDlg);
            pSlider = wxDynamicCast(pWnd, wxSlider);
            wxASSERT( pSlider );
            pSlider->SetValue( WrappedRef.ReadAsInt() );
         }
         break;
      default:
         wxASSERT( false );
         break;
   }
   return pSlider;
}


wxChoice * ShuttleGuiBase::TieChoice(
   const TranslatableLabel &Prompt,
   int &Selected,
   const TranslatableStrings &choices )
{
   HandleOptionality( Prompt );

   // The Add function does a UseUpId(), so don't do it here in that case.
   if( mpState -> mShuttleMode != eIsCreating )
      UseUpId();

   wxChoice * pChoice=NULL;
   switch( mpState -> mShuttleMode )
   {
   case eIsCreating:
      {
         pChoice = AddChoice( Prompt, choices, Selected );
         SetMinSize(pChoice, choices);
      }
      break;
   // IF setting internal storage from the controls.
   case eIsGettingFromDialog:
      {
         wxWindow * pWnd  = wxWindow::FindWindowById( miId, mpState -> mpDlg);
         pChoice = wxDynamicCast(pWnd, wxChoice);
         wxASSERT( pChoice );
         Selected = pChoice->GetSelection();
      }
      break;
   case eIsSettingToDialog:
      {
         wxWindow * pWnd  = wxWindow::FindWindowById( miId, mpState -> mpDlg);
         pChoice = wxDynamicCast(pWnd, wxChoice);
         wxASSERT( pChoice );
         pChoice->SetSelection( Selected );
      }
      break;
   default:
      wxASSERT( false );
      break;
   }
   return pChoice;
}

/// This function must be within a StartRadioButtonGroup - EndRadioButtonGroup pair.
wxRadioButton * ShuttleGuiBase::TieRadioButton()
{
   wxASSERT( mRadioCount >= 0); // Did you remember to use StartRadioButtonGroup() ?

   TranslatableLabel label;
   wxString Temp;
   if (mRadioCount >= 0 && mRadioCount < (int)mRadioLabels.size() ) {
      label = mRadioLabels[ mRadioCount ];
      Temp = mRadioValues[ mRadioCount ].GET();
   }

   // In what follows, WrappedRef is used in read only mode, but we
   // don't have a 'read-only' version, so we copy to deal with the constness.
   wxASSERT( !Temp.empty() ); // More buttons than values?

   WrappedType WrappedRef( Temp );

   mRadioCount++;

   UseUpId();
   wxRadioButton * pRadioButton = NULL;

   switch( mpState -> mShuttleMode )
   {
   case eIsCreating:
      {
         const auto &Prompt = label.Translation();

         mpWind = pRadioButton = safenew wxRadioButton(GetParent(), miId, Prompt,
            wxDefaultPosition, mItem.mWindowSize,
            (mRadioCount==1)?wxRB_GROUP:0);

         wxASSERT( WrappedRef.IsString() );
         wxASSERT( mRadioValue->IsString() );
         const bool value =
            (WrappedRef.ReadAsString() == mRadioValue->ReadAsString() );
         pRadioButton->SetValue( value );

         pRadioButton->SetName(wxStripMenuCodes(Prompt));
         UpdateSizers();
      }
      break;
   case eIsGettingFromDialog:
      {
         wxWindow * pWnd  = wxWindow::FindWindowById( miId, mpState -> mpDlg);
         pRadioButton = wxDynamicCast(pWnd, wxRadioButton);
         wxASSERT( pRadioButton );
         if( pRadioButton->GetValue() )
            mRadioValue->WriteToAsString( WrappedRef.ReadAsString() );
      }
      break;
   default:
      wxASSERT( false );
      break;
   }
   return pRadioButton;
}

/// Call this before AddRadioButton calls.
void ShuttleGuiBase::StartRadioButtonGroup()
{
   mRadioButtons =
      std::make_shared< RadioButtonList >();
   mRadioCount = 0;
}

/// Call this before any TieRadioButton calls.
void ShuttleGuiBase::StartRadioButtonGroup( const LabelSetting &Setting )
{
   mRadioLabels = Setting.GetLabels();
   mRadioValues = Setting.GetValues();

   // Configure the generic type mechanism to use OUR string.
   mRadioValueString = Setting.GetDefault().GET();
   mRadioValue.emplace( mRadioValueString );

   // Now actually start the radio button group.
   mRadioSettingName = Setting.GetPath();
   mRadioCount = 0;
   if ( mpState -> mShuttleMode == eIsCreating )
      DoDataShuttle( Setting.GetPath(), *mRadioValue );
}

/// Call this after any TieRadioButton calls.
/// It's generic too.  We don't need type-specific ones.
void ShuttleGuiBase::EndRadioButtonGroup()
{
   // too few buttons?
   wxASSERT( !mRadioButtons ||
      mRadioCount == mRadioButtons->size() );

   if( mpState -> mShuttleMode == eIsGettingFromDialog )
      DoDataShuttle( mRadioSettingName, *mRadioValue );
   mRadioValue.reset();// Clear it out...
   mRadioSettingName = L"";
   mRadioCount = -1; // So we detect a problem.
   mRadioLabels.clear();
   mRadioValues.clear();
   mRadioValueString.clear();
   mRadioButtons.reset();
}

//-----------------------------------------------------------------------//
//-- Now we are into type specific Tie() functions.
//-- These are all 'one-step' tie functions.

wxCheckBox * ShuttleGuiBase::TieCheckBox(
   const TranslatableLabel &Prompt, bool &Var)
{
   WrappedType WrappedRef( Var );
   return DoTieCheckBox( Prompt, WrappedRef );
}

// See comment in AddCheckBoxOnRight() for why we have this variant.
wxCheckBox * ShuttleGuiBase::TieCheckBoxOnRight(
   const TranslatableLabel &Prompt, bool &Var)
{
   // Only does anything different if it's creating.
   WrappedType WrappedRef( Var );
   if( mpState -> mShuttleMode == eIsCreating )
      return AddCheckBoxOnRight( Prompt, WrappedRef.ReadAsString() == L"true" );
   return DoTieCheckBox( Prompt, WrappedRef );
}

wxSpinCtrl * ShuttleGuiBase::TieSpinCtrl(
   const TranslatableLabel &Prompt, int &Value, const int max, const int min )
{
   WrappedType WrappedRef(Value);
   return DoTieSpinCtrl( Prompt, WrappedRef, max, min );
}

wxTextCtrl * ShuttleGuiBase::TieTextBox(
   const TranslatableLabel &Prompt, wxString &Selected, const int nChars)
{
   WrappedType WrappedRef(Selected);
   return DoTieTextBox( Prompt, WrappedRef, nChars );
}

wxTextCtrl * ShuttleGuiBase::TieTextBox(
   const TranslatableLabel &Prompt, int &Selected, const int nChars)
{
   WrappedType WrappedRef( Selected );
   return DoTieTextBox( Prompt, WrappedRef, nChars );
}

wxTextCtrl * ShuttleGuiBase::TieTextBox(
   const TranslatableLabel &Prompt, double &Value, const int nChars)
{
   WrappedType WrappedRef( Value );
   return DoTieTextBox( Prompt, WrappedRef, nChars );
}

wxTextCtrl * ShuttleGuiBase::TieNumericTextBox(
   const TranslatableLabel &Prompt, int &Value, const int nChars)
{
   WrappedType WrappedRef( Value );
   return DoTieNumericTextBox( Prompt, WrappedRef, nChars );
}

wxTextCtrl * ShuttleGuiBase::TieNumericTextBox(
   const TranslatableLabel &Prompt, double &Value, const int nChars)
{
   WrappedType WrappedRef( Value );
   return DoTieNumericTextBox( Prompt, WrappedRef, nChars );
}

wxSlider * ShuttleGuiBase::TieSlider(
   const TranslatableLabel &Prompt, int &pos, const int max, const int min )
{
   WrappedType WrappedRef( pos );
   return DoTieSlider( Prompt, WrappedRef, max, min );
}

wxSlider * ShuttleGuiBase::TieSlider(
   const TranslatableLabel &Prompt,
   double &pos, const double max, const double min )
{
   WrappedType WrappedRef( pos );
   return DoTieSlider( Prompt, WrappedRef, max, min );
}

wxSlider * ShuttleGuiBase::TieSlider(
   const TranslatableLabel &Prompt,
   float &pos, const float fMin, const float fMax)
{
   const float RoundFix=0.0000001f;
   int iVal=(pos-fMin+RoundFix)*100.0/(fMax-fMin);
   wxSlider * pWnd = TieSlider( Prompt, iVal, 100 );
   pos = iVal*(fMax-fMin)*0.01+fMin;
   return pWnd;
}

wxSlider * ShuttleGuiBase::TieVSlider(
   const TranslatableLabel &Prompt,
   float &pos, const float fMin, const float fMax)
{
   int iVal=(pos-fMin)*100.0/(fMax-fMin);
//   if( mShuttleMode == eIsCreating )
//   {
//      return AddVSlider( Prompt, iVal, 100 );
//   }
   wxSlider * pWnd = TieSlider( Prompt, iVal, 100 );
   pos = iVal*(fMax-fMin)*0.01+fMin;
   return pWnd;
}

wxChoice * ShuttleGuiBase::TieChoice(
   const TranslatableLabel &Prompt,
   TranslatableString &Selected,
   const TranslatableStrings &choices )
{
   int Index = make_iterator_range( choices ).index( Selected );
   auto result = TieChoice( Prompt, Index, choices );
   if ( Index >= 0 && Index < choices.size() )
      Selected = choices[ Index ];
   else
      Selected = {};
   return result;
}

//-----------------------------------------------------------------------//

// ShuttleGui utility functions to look things up in a list.
// If not present, we use the configured default index value.

//-----------------------------------------------------------------------//

/// String-to-Index
int ShuttleGuiBase::TranslateToIndex(
   const wxString &Value, const Identifiers &Choices )
{
   int n = make_iterator_range( Choices ).index( Value );
   if( n == wxNOT_FOUND  )
      n=miNoMatchSelector;
   miNoMatchSelector = 0;
   return n;
}

/// Index-to-String
Identifier ShuttleGuiBase::TranslateFromIndex(
   const int nIn, const Identifiers &Choices )
{
   int n = nIn;
   if( n== wxNOT_FOUND )
      n=miNoMatchSelector;
   miNoMatchSelector = 0;
   if( n < (int)Choices.size() )
   {
      return Choices[n];
   }
   return L"";
}

//-----------------------------------------------------------------------//


// ShuttleGui code uses the model that you read into program variables
// and write out from program variables.

// In programs like Audacity which don't use internal program variables
// you have to do both steps in one go, using variants of the standard
// 'Tie' functions which call the underlying Tie functions twice.

//----------------------------------------------------------------------//


/**
 Code-Condenser function.

We have functions which need to do:

\code
  // Either: Values are coming in:
  DoDataShuttle( SettingName, WrappedRef );
  TieMyControl( Prompt, WrappedRef );

  // Or: Values are going out:
  TieMyControl( Prompt, WrappedRef );
  DoDataShuttle( SettingName, WrappedRef );
\endcode

So we make a list of all the possible steps,
and have DoStep choose which ones are actually done,
like this:

\code
  if( DoStep(1) ) DoFirstThing();
  if( DoStep(2) ) DoSecondThing();
  if( DoStep(3) ) DoThirdThing();
\endcode

The repeated choice logic can then be taken out of those
functions.

JKC: This paves the way for doing data validation too,
though when we add that we will need to renumber the
steps.
*/
bool ShuttleGuiBase::DoStep( int iStep )
{
   // Get value and create
   if( mpState -> mShuttleMode == eIsCreating )
   {
      return (iStep==1) || (iStep==2);
   }
   // Like creating, get the value and set.
   if( mpState -> mShuttleMode == eIsSettingToDialog )
   {
      return (iStep==1) || (iStep==2);
   }
   if( mpState -> mShuttleMode == eIsGettingFromDialog )
   {
      return (iStep==2) || (iStep==3);
   }
   wxASSERT( false );
   return false;
}


/// Variant of the standard TieCheckBox which does the two step exchange
/// between gui and stack variable and stack variable and shuttle.
wxCheckBox * ShuttleGuiBase::TieCheckBox(
   const TranslatableLabel &Prompt,
   const BoolSetting &Setting)
{
   if ( mpVisitor )
      mpVisitor->Visit( Prompt.Translation(), Setting );

   wxCheckBox * pCheck=NULL;

   auto Value = Setting.GetDefault();
   WrappedType WrappedRef( Value );
   if( DoStep(1) ) DoDataShuttle( Setting.GetPath(), WrappedRef );
   if( DoStep(2) ) pCheck = DoTieCheckBox( Prompt, WrappedRef );
   if( DoStep(3) ) DoDataShuttle( Setting.GetPath(), WrappedRef );

   return pCheck;
}

/// Variant of the standard TieCheckBox which does the two step exchange
/// between gui and stack variable and stack variable and shuttle.
wxCheckBox * ShuttleGuiBase::TieCheckBoxOnRight(
   const TranslatableLabel &Prompt,
   const BoolSetting & Setting)
{
   if ( mpVisitor )
      mpVisitor->Visit( Prompt.Translation(), Setting );

   wxCheckBox * pCheck=NULL;

   auto Value = Setting.GetDefault();
   WrappedType WrappedRef( Value );
   if( DoStep(1) ) DoDataShuttle( Setting.GetPath(), WrappedRef );
   if( DoStep(2) ) pCheck = DoTieCheckBoxOnRight( Prompt, WrappedRef );
   if( DoStep(3) ) DoDataShuttle( Setting.GetPath(), WrappedRef );

   return pCheck;
}

/// Variant of the standard TieSlider which does the two step exchange
/// between gui and stack variable and stack variable and shuttle.
wxSlider * ShuttleGuiBase::TieSlider(
   const TranslatableLabel &Prompt,
   const IntSetting & Setting,
   const int max,
   const int min)
{
   if ( mpVisitor )
      mpVisitor->Visit( Prompt.Translation(), Setting );

   wxSlider * pSlider=NULL;

   auto Value = Setting.GetDefault();
   WrappedType WrappedRef( Value );
   if( DoStep(1) ) DoDataShuttle( Setting.GetPath(), WrappedRef );
   if( DoStep(2) ) pSlider = DoTieSlider( Prompt, WrappedRef, max, min );
   if( DoStep(3) ) DoDataShuttle( Setting.GetPath(), WrappedRef );

   return pSlider;
}

/// Variant of the standard TieSpinCtrl which does the two step exchange
/// between gui and stack variable and stack variable and shuttle.
wxSpinCtrl * ShuttleGuiBase::TieSpinCtrl(
   const TranslatableLabel &Prompt,
   const IntSetting &Setting,
   const int max,
   const int min)
{
   if ( mpVisitor )
      mpVisitor->Visit( Prompt.Translation(), Setting );

   wxSpinCtrl * pSpinCtrl=NULL;

   auto Value = Setting.GetDefault();
   WrappedType WrappedRef( Value );
   if( DoStep(1) ) DoDataShuttle( Setting.GetPath(), WrappedRef );
   if( DoStep(2) ) pSpinCtrl = DoTieSpinCtrl( Prompt, WrappedRef, max, min );
   if( DoStep(3) ) DoDataShuttle( Setting.GetPath(), WrappedRef );

   return pSpinCtrl;
}

/// Variant of the standard TieTextBox which does the two step exchange
/// between gui and stack variable and stack variable and shuttle.
wxTextCtrl * ShuttleGuiBase::TieTextBox(
   const TranslatableLabel & Prompt,
   const StringSetting & Setting,
   const int nChars)
{
   if ( mpVisitor )
      mpVisitor->Visit( Prompt.Translation(), Setting );

   wxTextCtrl * pText=(wxTextCtrl*)NULL;

   auto Value = Setting.GetDefault();
   WrappedType WrappedRef( Value );
   if( DoStep(1) ) DoDataShuttle( Setting.GetPath(), WrappedRef );
   if( DoStep(2) ) pText = DoTieTextBox( Prompt, WrappedRef, nChars );
   if( DoStep(3) ) DoDataShuttle( Setting.GetPath(), WrappedRef );
   return pText;
}

/// Variant of the standard TieTextBox which does the two step exchange
/// between gui and stack variable and stack variable and shuttle.
/// This one does it for double values...
wxTextCtrl * ShuttleGuiBase::TieIntegerTextBox(
   const TranslatableLabel & Prompt,
   const IntSetting &Setting,
   const int nChars)
{
   if ( mpVisitor )
      mpVisitor->Visit( Prompt.Translation(), Setting );

   wxTextCtrl * pText=(wxTextCtrl*)NULL;

   auto Value = Setting.GetDefault();
   WrappedType WrappedRef( Value );
   if( DoStep(1) ) DoDataShuttle( Setting.GetPath(), WrappedRef );
   if( DoStep(2) ) pText = DoTieNumericTextBox( Prompt, WrappedRef, nChars );
   if( DoStep(3) ) DoDataShuttle( Setting.GetPath(), WrappedRef );
   return pText;
}

/// Variant of the standard TieTextBox which does the two step exchange
/// between gui and stack variable and stack variable and shuttle.
/// This one does it for double values...
wxTextCtrl * ShuttleGuiBase::TieNumericTextBox(
   const TranslatableLabel & Prompt,
   const DoubleSetting & Setting,
   const int nChars)
{
   if ( mpVisitor )
      mpVisitor->Visit( Prompt.Translation(), Setting );

   wxTextCtrl * pText=(wxTextCtrl*)NULL;

   auto Value = Setting.GetDefault();
   WrappedType WrappedRef( Value );
   if( DoStep(1) ) DoDataShuttle( Setting.GetPath(), WrappedRef );
   if( DoStep(2) ) pText = DoTieNumericTextBox( Prompt, WrappedRef, nChars );
   if( DoStep(3) ) DoDataShuttle( Setting.GetPath(), WrappedRef );
   return pText;
}

/// Variant of the standard TieChoice which does the two step exchange
/// between gui and stack variable and stack variable and shuttle.
///   @param Prompt             The prompt shown beside the control.
///   @param Setting            Encapsulates setting name, internal and visible
///                             choice strings, and a designation of one of
///                             those as default.
wxChoice *ShuttleGuiBase::TieChoice(
   const TranslatableLabel &Prompt,
   const ChoiceSetting &choiceSetting )
{
   if ( mpVisitor )
      mpVisitor->Visit( Prompt.Translation(), choiceSetting );

   return DoTieChoice( Prompt, choiceSetting );
}

wxChoice *ShuttleGuiBase::DoTieChoice(
   const TranslatableLabel &Prompt,
   const ChoiceSetting &choiceSetting )
{
   // Do this to force any needed migrations first
   choiceSetting.Read();

   const auto &SettingName = choiceSetting.GetPath();
   const auto &Default = choiceSetting.GetDefault();

   // uh oh
   TranslatableStrings Choices;
   for ( const auto &msgid : choiceSetting.GetLabels() )
      Choices.push_back( msgid );
   const auto &InternalChoices = choiceSetting.GetValues();

   wxChoice * pChoice=(wxChoice*)NULL;

   int TempIndex=0;
//   int TempIndex = TranslateToIndex( Default, InternalChoices );
   wxString TempStr = Default.GET();
   WrappedType WrappedRef( TempStr );
   // Get from prefs does 1 and 2.
   // Put to prefs does 2 and 3.
   if( DoStep(1) ) DoDataShuttle( SettingName, WrappedRef ); // Get Index from Prefs.
   if( DoStep(1) ) TempIndex = TranslateToIndex( TempStr, InternalChoices ); // To an index
   if( DoStep(2) ) pChoice = TieChoice( Prompt, TempIndex, Choices );
   if( DoStep(3) ) TempStr = TranslateFromIndex( TempIndex, InternalChoices ).GET(); // To a string
   if( DoStep(3) ) DoDataShuttle( SettingName, WrappedRef ); // Put into Prefs.
   return pChoice;
}

/// Variant of the standard TieChoice which does the two step exchange
/// between gui and stack variable and stack variable and shuttle.
/// The Translated choices and default are integers, not Strings.
/// Behaves identically to the previous, but is meant for use when the choices
/// are non-exhaustive and there is a companion control for arbitrary entry.
///   @param Prompt             The prompt shown beside the control.
///   @param SettingName        The setting name as stored in gPrefs
///   @param Default            The default integer value for this control
///   @param Choices            An array of choices that appear on screen.
///   @param pInternalChoices   The corresponding values (as an integer array)
///                             if null, then use 0, 1, 2, ...
wxChoice * ShuttleGuiBase::TieNumberAsChoice(
   const TranslatableLabel &Prompt,
   const IntSetting & Setting,
   const TranslatableStrings & Choices,
   const std::vector< int > * pInternalChoices,
   int iNoMatchSelector)
{
   if ( mpVisitor )
      mpVisitor->Visit( Prompt.Translation(), Setting );

   auto fn = [](int arg){ return wxString::Format( "%d", arg ); };

   Identifiers InternalChoices;
   if ( pInternalChoices )
      InternalChoices =
         transform_container<Identifiers>(*pInternalChoices, fn);
   else
      for ( int ii = 0; ii < (int)Choices.size(); ++ii )
         InternalChoices.emplace_back( fn( ii ) );


   const auto Default = Setting.GetDefault();

   miNoMatchSelector = iNoMatchSelector;

   long defaultIndex;
   if ( pInternalChoices )
      defaultIndex =  make_iterator_range( *pInternalChoices ).index( Default );
   else
      defaultIndex = Default;
   if ( defaultIndex < 0 || defaultIndex >= (int)Choices.size() )
      defaultIndex = -1;

   ChoiceSetting choiceSetting{
      Setting.GetPath(),
      {
         ByColumns,
         Choices,
         InternalChoices,
      },
      defaultIndex
   };

   return ShuttleGuiBase::DoTieChoice( Prompt, choiceSetting );
}

//------------------------------------------------------------------//

// We're now into ShuttleGuiBase sizer and misc functions.

/// The Ids increment as we add NEW controls.
/// However, the user can force the id manually, for example
/// if they need a specific Id for a button, and then let it
/// resume normal numbering later.
/// UseUpId() sets miId to the next Id, either using the
/// user specicfied one, or resuming the sequence.
void ShuttleGuiBase::UseUpId()
{
   if( miIdSetByUser > 0)
   {
      miId = miIdSetByUser;
      miIdSetByUser = -1;
      return;
   }
   miId = mpState -> miIdNext++;
}

void ShuttleGuiBase::SetProportions( int Default )
{
   if( miPropSetByUser >=0 )
   {
      miProp = miPropSetByUser;
      miPropSetByUser =-1;
      return;
   }
   miProp = Default;
}


void ShuttleGuiBase::CheckEventType(
   const DialogDefinition::Item &item,
   std::initializer_list<wxEventType> types )
{
   if ( item.mAction || item.mValidatorSetter ) {
      if ( item.mEventType ) {
         // Require the explicitly given event type to be one of the preferred
         // kinds
         auto begin = types.begin(), end = types.end(),
            iter = std::find( begin, end, item.mEventType );
         if ( iter == end ) {
            wxASSERT( false );
            item.mEventType = 0;
            //item.mAction = nullptr;
            //item.mpSink = nullptr;
         }
      }
      else if ( types.size() > 0 )
         // Supply the preferred event type
         item.mEventType = *types.begin();
      else
         wxASSERT( false );
   }
}

void ShuttleGuiBase::ApplyItem( int step, const DialogDefinition::Item &item,
   wxWindow *pWind, wxWindow *pDlg )
{  
   if ( step == 0 ) {
      // Do these steps before adding the window to the sizer
      if( item.mUseBestSize )
         pWind->SetMinSize( pWind->GetBestSize() );
      else if( item.mHasMinSize )
         pWind->SetMinSize( item.mMinSize );

      if ( item.mWindowSize != wxDefaultSize )
         pWind->SetSize( item.mWindowSize );
   }
   else if ( step == 1) {
      // Apply certain other optional window attributes here

      if ( item.mAction || item.mValidatorSetter ) {
         if ( !item.mAction ) {
            auto action = item.mAction;
            pDlg->Bind(
               item.mEventType,
               [pWind, action, pDlg]( wxCommandEvent& ){
                  if ( pWind->GetValidator() ) {
                     if ( !pWind->GetValidator()->Validate( pWind ) )
                        return;
                     if ( !pWind->GetValidator()->TransferFromWindow() )
                        return;
                  }
                  if ( action )
                     action();
                  if ( pWind->GetValidator() || action )
                     pDlg->TransferDataToWindow();
               },
               pWind->GetId() );
         }
         else
            wxASSERT( false );
      }

      if ( item.mValidatorSetter )
         item.mValidatorSetter( pWind );

      if ( !item.mText.mToolTip.empty() )
         pWind->SetToolTip( item.mText.mToolTip.Translation() );

      if ( !item.mText.mName.empty() ) {
         // This affects the audible screen-reader name
         pWind->SetName( item.mText.mName.Translation() );
#ifndef __WXMAC__
         if (auto pButton = dynamic_cast< wxBitmapButton* >( pWind ))
            pButton->SetLabel(  item.mName.Translation() );
#endif
      }

      if ( !item.mText.mLabel.empty() ) {
         // Takes precedence over any name specification,
         // for the (visible) label
         pWind->SetLabel( item.mText.mLabel.Translation() );
      }

      if ( !item.mText.mSuffix.empty() )
         pWind->SetName(
            pWind->GetName() + " " + item.mText.mSuffix.Translation() );

      if (item.mFocused)
         pWind->SetFocus();

      if (item.mDefault) {
         if ( auto pButton = dynamic_cast< wxButton *>( pWind ) )
            pButton->SetDefault();
         else
            wxASSERT( false );
      }

      if (item.mDisabled)
         pWind->Enable( false );

      for (auto &pair : item.mRootConnections)
         pWind->Connect( pair.first, pair.second, nullptr, pDlg );
   }
}


void ShuttleGuiBase::UpdateSizersCore(bool bPrepend, int Flags, bool prompt)
{
   if( mpWind && mpState -> mpParent )
   {
      int useFlags = Flags;

      if ( !prompt && mItem.mWindowPositionFlags )
         // override the given Flags
         useFlags = mItem.mWindowPositionFlags;

      if (!prompt)
         ApplyItem( 0, mItem, mpWind, mpState -> mpDlg );

      if( mpState -> mpSizer){
         if( bPrepend )
         {
            mpState -> mpSizer->Prepend(mpWind, miProp, useFlags, mpState -> miBorder);
         }
         else
         {
            mpState -> mpSizer->Add(mpWind, miProp, useFlags, mpState -> miBorder);
         }
      }

      if (!prompt) {
         ApplyItem( 1, mItem, mpWind, mpState -> mpDlg );
         // Reset to defaults
         mItem = {};
      }
   }

   if( mpSubSizer && mpState -> mpSizer )
   {
      // When adding sizers into sizers, don't add a border.
      // unless it's a static box sizer.
      wxSizer *const pSubSizer = mpSubSizer.get();
      if (wxDynamicCast(pSubSizer, wxStaticBoxSizer))
      {
         mpState -> mpSizer->Add( mpSubSizer.release(), miSizerProp, Flags , mpState -> miBorder);
      }
      else
      {
         mpState -> mpSizer->Add( mpSubSizer.release(), miSizerProp, Flags ,0);//miBorder);
      }
      mpState -> mpSizer = pSubSizer;
      mpState -> PushSizer();
   }

   mpWind = NULL;
   miProp = 0;
   miSizerProp =0;
}

// Sizer is added into parent sizer, and will expand/shrink.
void ShuttleGuiBase::UpdateSizers()
{  UpdateSizersCore( false, wxEXPAND | wxALL );}

// Sizer is added into parent sizer, centred
void ShuttleGuiBase::UpdateSizersC()
{  UpdateSizersCore( false, wxALIGN_CENTRE | wxALL );}

// Sizer is added into parent sizer, and will expand/shrink.
// added to start of sizer list.
void ShuttleGuiBase::UpdateSizersAtStart()
{  UpdateSizersCore( true, wxEXPAND | wxALL );}

void ShuttleGuiState::PopSizer()
{
   mSizerDepth--;
   wxASSERT( mSizerDepth >=0 );
   mpSizer = pSizerStack[ mSizerDepth ];
}

void ShuttleGuiState::PushSizer()
{
   mSizerDepth++;
   wxASSERT( mSizerDepth < nMaxNestedSizers );
   pSizerStack[ mSizerDepth ] = mpSizer;
}

long ShuttleGuiBase::GetStyle( long style )
{
   if( mItem.miStyle )
      style = mItem.miStyle;
   mItem.miStyle = 0;
   return style;
}

// A rarely used helper function that sets a pointer
// ONLY if the value it is to be set to is non NULL.
void SetIfCreated( wxChoice * &Var, wxChoice * Val )
{
   if( Val != NULL )
      Var = Val;
};
void SetIfCreated( wxTextCtrl * &Var, wxTextCtrl * Val )
{
   if( Val != NULL )
      Var = Val;
};
void SetIfCreated( wxStaticText *&Var, wxStaticText * Val )
{
   if( Val != NULL )
      Var = Val;
};

#ifdef EXPERIMENTAL_TRACK_PANEL
// Additional includes down here, to make it easier to split this into
// two files at some later date.
#include "../extnpanel-src/GuiWaveTrack.h"
#endif

static
std::unique_ptr<wxSizer> CreateStdButtonSizer(
   wxWindow *pDlg, wxWindow *parent,
   long buttons, DialogDefinition::Items items, wxWindow *extra,
   DialogDefinition::Item extraItem )
{
   wxASSERT(parent != NULL); // To justify safenew

   for ( const auto &item : items )
      buttons |= item.mStandardButton;

   bool givenDefault = std::any_of( items.begin(), items.end(),
      std::mem_fn( &DialogDefinition::Item::mDefault ) );

   int margin;
   {
#if defined(__WXMAC__)
      margin = 12;
#elif defined(__WXGTK20__)
      margin = 12;
#elif defined(__WXMSW__)
      wxButton b(parent, 0, wxEmptyString);
      margin = b.ConvertDialogToPixels(wxSize(2, 0)).x;
#else
      wxButton b(parent, 0, wxEmptyString);
      margin = b->ConvertDialogToPixels(wxSize(4, 0)).x;
#endif
   }

   wxButton *b = NULL;
   auto bs = std::make_unique<wxStdDialogButtonSizer>();
   
   const auto makeButton =
   [parent]( wxWindowID id, const wxString label = {} ) {
      auto result = safenew wxButton( parent, id, label );
      result->SetName( result->GetLabel() );
      return result;
   };

   auto addButton =
   [&]( StandardButtonID id, wxButton *pButton, int insertAt = 0 ) {
      // Find any matching item
      auto pred = [id]( decltype( *items.begin() ) &item ){
         return item.mStandardButton == id; };
      auto end = items.end(),
         iter = std::find_if( items.begin(), end, pred );
      // If found, it should be unique
      wxASSERT( iter == end ||
         end == std::find_if( iter + 1, end, pred ) );

      if (iter != end)
         ShuttleGuiBase::ApplyItem( 0, *iter, pButton, pDlg );
      if ( insertAt == 0 )
         bs->AddButton( pButton );
      else if ( insertAt == -1 )
         bs->Add( pButton, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, margin );
      else
         bs->Insert( insertAt,
            pButton, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, margin );
      if (iter != end) {
         ShuttleGuiBase::CheckEventType( *iter, { wxEVT_BUTTON } );
         ShuttleGuiBase::ApplyItem( 1, *iter, pButton, pDlg );
      }
   };

   if( buttons & eOkButton )
   {
      b = makeButton( wxID_OK );
      if( !givenDefault )
         b->SetDefault();
      addButton( eOkButton, b );
   }

   if( buttons & eCancelButton )
   {
      addButton( eCancelButton, makeButton( wxID_CANCEL ) );
   }

   if( buttons & eYesButton )
   {
      b = makeButton( wxID_YES );
      if( !givenDefault )
         b->SetDefault();
      addButton( eYesButton, b );
   }

   if( buttons & eNoButton )
   {
      addButton( eNoButton, makeButton( wxID_NO ) );
   }

   if( buttons & eApplyButton )
   {
      b = makeButton( wxID_APPLY );
      b->SetDefault();
      if( !givenDefault )
         b->SetDefault();
      addButton( eApplyButton, b );
   }

   if( buttons & eCloseButton )
   {
      addButton( eCloseButton,
         makeButton( wxID_CANCEL, XO("&Close").Translation() ) );
   }

#if defined(__WXMSW__)
   // See below for explanation
   if( buttons & eHelpButton )
   {
      // Replace standard Help button with smaller icon button.
      // bs->AddButton(safenew wxButton(parent, wxID_HELP));
      b = safenew wxBitmapButton(parent, wxID_HELP, theTheme.Bitmap( bmpHelpIcon ));
      b->SetToolTip( XO("Help").Translation() );
      b->SetLabel(XO("Help").Translation());       // for screen readers
      b->SetName( b->GetLabel() );
      addButton( eHelpButton, b );
   }
#endif

   if (buttons & ePreviewButton)
   {
      addButton( ePreviewButton,
         makeButton( ePreviewID, XO("&Preview").Translation() ),
         -1 );
   }
   if (buttons & ePreviewDryButton) {
      addButton( ePreviewDryButton,
         makeButton( ePreviewDryID, XO("Dry Previe&w").Translation() ),
         -1 );
      bs->Add( 20, 0 );
   }

   if( buttons & eSettingsButton )
   {
      addButton( eSettingsButton,
         makeButton( eSettingsID, XO("&Settings").Translation() ),
         -1 );
      bs->Add( 20, 0 );
   }

   if( extra )
   {
      ShuttleGuiBase::ApplyItem( 0, extraItem, extra, pDlg );
      bs->Add( extra, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, margin );
      bs->Add( 40, 0 );
      ShuttleGuiBase::ApplyItem( 1, extraItem, extra, pDlg );
   }

   bs->AddStretchSpacer();
   bs->Realize();

   size_t lastLastSpacer = 0;
   size_t lastSpacer = 0;
   wxSizerItemList & list = bs->GetChildren();
   for( size_t i = 0, cnt = list.size(); i < cnt; i++ )
   {
      if( list[i]->IsSpacer() )
      {
         lastSpacer = i;
      }
      else
      {
         lastLastSpacer = lastSpacer;
      }
   }

   // Add any buttons that need to cuddle up to the right hand cluster
   // Add any buttons that need to cuddle up to the right hand cluster
   if( buttons & eDebugButton )
   {
      addButton( eDebugButton,
         makeButton( eDebugID, XO("Debu&g").Translation() ),
         ++lastLastSpacer );
   }

#if !defined(__WXMSW__)
   // Bug #2432: Couldn't find GTK guidelines, but Mac HIGs state:
   //
   //    View style 	                                          Help button position
   //    Dialog with dismissal buttons (like OK and Cancel). 	Lower-left corner, vertically aligned with the dismissal buttons.
   //    Dialog without dismissal buttons. 	                  Lower-left or lower-right corner.
   //    Preference window or pane. 	                        Lower-left or lower-right corner.
   //
   // So, we're gonna cheat a little and use the lower-right corner.
   if( buttons & eHelpButton )
   {
      // Replace standard Help button with smaller icon button.
      // bs->AddButton(safenew wxButton(parent, wxID_HELP));
      b = safenew wxBitmapButton(parent, wxID_HELP, theTheme.Bitmap( bmpHelpIcon ));
      b->SetToolTip( XO("Help").Translation() );
      b->SetLabel(XO("Help").Translation());       // for screen readers
      b->SetName( b->GetLabel() );
      bs->Add( b, 0, wxALIGN_CENTER );
   }
#endif

   auto s = std::make_unique<wxBoxSizer>( wxVERTICAL );
   s->Add( bs.release(), 1, wxEXPAND | wxALL, 7 );
   s->Add( 0, 3 );   // a little extra space

   return std::unique_ptr<wxSizer>{ s.release() };
}

void ShuttleGuiBase::AddStandardButtons(
   long buttons, DialogDefinition::Items items, wxWindow *extra,
   DialogDefinition::Item extraItem )
{
   if( mpState -> mShuttleMode != eIsCreating )
      return;

   StartVerticalLay( false );

   miSizerProp = false;
   mpSubSizer = CreateStdButtonSizer(
      mpState -> mpDlg, mpState -> mpParent,
      buttons, items, extra, extraItem );
   UpdateSizers();
   mpState -> PopSizer();

   EndVerticalLay();
}

wxSizerItem * ShuttleGuiBase::AddSpace( int width, int height, int prop )
{
   if( mpState -> mShuttleMode != eIsCreating )
      return NULL;

//   SetProportions(0);
  // return mpSizer->Add( width, height, miProp);

   return mpState -> mpSizer->Add( width, height, prop );
}

void ShuttleGuiBase::SetMinSize( wxWindow *window, const TranslatableStrings & items )
{
   SetMinSize( window,
      transform_container<wxArrayStringEx>(
         items, std::mem_fn( &TranslatableString::Translation ) ) );
}

void ShuttleGuiBase::SetMinSize( wxWindow *window, const wxArrayStringEx & items )
{
   int maxw = 0;

   for( size_t i = 0; i < items.size(); i++ )
   {
      int x;
      int y;

      window->GetTextExtent(items[i], &x, &y );
      if( x > maxw )
      {
         maxw = x;
      }
   }

   // Would be nice to know the sizes of the button and borders, but this is
   // the best we can do for now.
#if defined(__WXMAC__)
   maxw += 50;
#elif defined(__WXMSW__)
   maxw += 50;
#elif defined(__WXGTK__)
   maxw += 50;
#else
   maxw += 50;
#endif

   window->SetMinSize( { maxw, -1 } );
}

/*
void ShuttleGuiBase::SetMinSize( wxWindow *window, const std::vector<int> & items )
{
   wxArrayStringEx strs;

   for( size_t i = 0; i < items.size(); i++ )
   {
      strs.Add( wxString::Format( L"%d", items[i] ) );
   }

   SetMinSize( window, strs );
}
*/

TranslatableStrings Msgids(
   const EnumValueSymbol strings[], size_t nStrings)
{
   return transform_range<TranslatableStrings>(
      strings, strings + nStrings,
      std::mem_fn( &EnumValueSymbol::Msgid )
   );
}

TranslatableStrings Msgids( const std::vector<EnumValueSymbol> &strings )
{
   return Msgids( strings.data(), strings.size() );
}
