/**********************************************************************

   Audacity - A Digital Audio Editor
   Copyright 1999-2018 Audacity Team
   License: wxwidgets

   Dan Horgan
   James Crook

******************************************************************//**

\file SetTrackCommand.cpp
\brief Definitions for SetTrackCommand built up from 
SetTrackBase, SetTrackStatusCommand, SetTrackAudioCommand and
SetTrackVisualsCommand

\class SetTrackBase
\brief Base class for the various SetTrackCommand classes.  
Sbclasses provide the settings that are relevant to them.

\class SetTrackStatusCommand
\brief A SetTrackBase that sets name, selected and focus.

\class SetTrackAudioCommand
\brief A SetTrackBase that sets pan, gain, mute and solo.

\class SetTrackVisualsCommand
\brief A SetTrackBase that sets appearance of a track.

\class SetTrackCommand
\brief A SetTrackBase that combines SetTrackStatusCommand,
SetTrackAudioCommand and SetTrackVisualsCommand.

*//*******************************************************************/


#include "SetTrackInfoCommand.h"

#include "LoadCommands.h"
#include "Project.h"
#include "../TrackPanelAx.h"
#include "../TrackPanel.h"
#include "../WaveTrack.h"
#include "../prefs/WaveformSettings.h"
#include "../prefs/SpectrogramSettings.h"
#include "../Shuttle.h"
#include "../ShuttleGui.h"
#include "../tracks/playabletrack/wavetrack/ui/WaveTrackView.h"
#include "../tracks/playabletrack/wavetrack/ui/WaveTrackViewConstants.h"
#include "CommandContext.h"

SetTrackBase::SetTrackBase(){
   mbPromptForTracks = true;
   bIsSecondChannel = false;
}

//Define for the old scheme, where SetTrack defines its own track selection.
//rather than using the current selection.
//#define USE_OWN_TRACK_SELECTION


bool SetTrackBase::ApplyInner( const CommandContext &context, Track *t  )
{
      static_cast<void>(&context);
      static_cast<void>(&t);
      return true;
};


bool SetTrackBase::DefineParams( ShuttleParams & S)
{
   static_cast<void>(S);
#ifdef USE_OWN_TRACK_SELECTION
   S.OptionalY( bHasTrackIndex     ).Define(     mTrackIndex,     L"Track",      0, 0, 100 );
   S.OptionalN( bHasChannelIndex   ).Define(     mChannelIndex,   L"Channel",    0, 0, 100 );
#endif
   return true;
}

void SetTrackBase::PopulateOrExchange(ShuttleGui & S)
{
   static_cast<void>(S);
#ifdef USE_OWN_TRACK_SELECTION
   if( !mbPromptForTracks )
      return;
   S.AddSpace(0, 5);
   S.StartMultiColumn(3, GroupOptions{wxEXPAND}.StretchyColumn(2));
   {
      S
         .Optional( bHasTrackIndex  )
         .Target( mTrackIndex )
         .AddTextBox( XO("Track Index:") );

      S
         .Optional( bHasChannelIndex)
         .Target( mChannelIndex )
         .AddTextBox( XO("Channel Index:") );
   }
   S.EndMultiColumn();
#endif
}

bool SetTrackBase::Apply(const CommandContext & context  )
{
   long i = 0;// track counter
   long j = 0;// channel counter
   auto &tracks = TrackList::Get( context.project );
   for ( auto t : tracks.Leaders() )
   {
      auto channels = TrackList::Channels(t);
      for ( auto channel : channels ) {
         bool bThisTrack =
#ifdef USE_OWN_TRACK_SELECTION
         (bHasTrackIndex && (i==mTrackIndex)) ||
         (bHasChannelIndex && (j==mChannelIndex ) ) ||
         (!bHasTrackIndex && !bHasChannelIndex) ;
#else
         channel->GetSelected();
#endif

         if( bThisTrack ){
            ApplyInner( context, channel );
         }
         ++j; // count all channels
      }
      ++i; // count groups of channels
   }
   return true;
}

const ComponentInterfaceSymbol SetTrackStatusCommand::Symbol
{ XO("Set Track Status") };

namespace{ BuiltinCommandsModule::Registration< SetTrackStatusCommand > reg; }

bool SetTrackStatusCommand::DefineParams( ShuttleParams & S ){
   SetTrackBase::DefineParams( S );
   S.OptionalN( bHasTrackName      ).Define(     mTrackName,      L"Name",       _("Unnamed") );
   // There is also a select command.  This is an alternative.
   S.OptionalN( bHasSelected       ).Define(     bSelected,       L"Selected",   false );
   S.OptionalN( bHasFocused        ).Define(     bFocused,        L"Focused",    false );
   return true;
};

void SetTrackStatusCommand::PopulateOrExchange(ShuttleGui & S)
{
   SetTrackBase::PopulateOrExchange( S );
   S.StartMultiColumn(3, GroupOptions{}.Position(wxEXPAND).StretchyColumn(2));
   {
      S
         .Optional( bHasTrackName   )
         .Target( mTrackName )
         .AddTextBox( XXO("Name:") );
   }
   S.EndMultiColumn();
   S.StartMultiColumn(2, GroupOptions{}.Position(wxEXPAND).StretchyColumn(1));
   {
      S
         .Optional( bHasSelected       )
         .Target(bSelected)
         .AddCheckBox( XXO("Selected") );

      S
         .Optional( bHasFocused        )
         .Target(bFocused)
         .AddCheckBox( XXO("Focused") );
   }
   S.EndMultiColumn();
}

bool SetTrackStatusCommand::ApplyInner(const CommandContext & context, Track * t )
{
   //auto wt = dynamic_cast<WaveTrack *>(t);
   //auto pt = dynamic_cast<PlayableTrack *>(t);

   // You can get some intriguing effects by setting R and L channels to 
   // different values.
   if( bHasTrackName )
      t->SetName(mTrackName);

   // In stereo tracks, both channels need selecting/deselecting.
   if( bHasSelected )
      t->SetSelected(bSelected);

   // These ones don't make sense on the second channel of a stereo track.
   if( !bIsSecondChannel ){
      if( bHasFocused )
      {
         auto &trackFocus = TrackFocus::Get( context.project );
         if( bFocused)
            trackFocus.Set( t );
         else if( t == trackFocus.Get() )
            trackFocus.Set( nullptr );
      }
   }
   return true;
}



const ComponentInterfaceSymbol SetTrackAudioCommand::Symbol
{ XO("Set Track Audio") };

namespace{ BuiltinCommandsModule::Registration< SetTrackAudioCommand > reg2; }

bool SetTrackAudioCommand::DefineParams( ShuttleParams & S ){ 
   SetTrackBase::DefineParams( S );
   S.OptionalN( bHasMute           ).Define(     bMute,           L"Mute",       false );
   S.OptionalN( bHasSolo           ).Define(     bSolo,           L"Solo",       false );

   S.OptionalN( bHasGain           ).Define(     mGain,           L"Gain",       0.0,  -36.0, 36.0);
   S.OptionalN( bHasPan            ).Define(     mPan,            L"Pan",        0.0, -100.0, 100.0);
   return true;
};

void SetTrackAudioCommand::PopulateOrExchange(ShuttleGui & S)
{
   SetTrackBase::PopulateOrExchange( S );
   S.StartMultiColumn(2, GroupOptions{}.Position(wxEXPAND).StretchyColumn(1));
   {
      S
         .Optional( bHasMute           )
         .Target(bMute)
         .AddCheckBox( XXO("Mute") );

      S
         .Optional( bHasSolo           )
         .Target(bSolo)
         .AddCheckBox( XXO("Solo") );
   }
   S.EndMultiColumn();
   S.StartMultiColumn(3, GroupOptions{}.Position(wxEXPAND).StretchyColumn(2));
   {
      S
         .Optional( bHasGain        )
         .Target( mGain )
         .AddSlider(          XXO("Gain:"),          0, 36.0,-36.0);

      S
         .Optional( bHasPan         )
         .Target( mPan )
         .AddSlider(          XXO("Pan:"),           0,  100.0, -100.0);
   }
   S.EndMultiColumn();
}

bool SetTrackAudioCommand::ApplyInner(const CommandContext & context, Track * t )
{
   static_cast<void>(context);
   auto wt = dynamic_cast<WaveTrack *>(t);
   auto pt = dynamic_cast<PlayableTrack *>(t);

   if( wt && bHasGain )
      wt->SetGain(DB_TO_LINEAR(mGain));
   if( wt && bHasPan )
      wt->SetPan(mPan/100.0);

   // These ones don't make sense on the second channel of a stereo track.
   if( !bIsSecondChannel ){
      if( pt && bHasSolo )
         pt->SetSolo(bSolo);
      if( pt && bHasMute )
         pt->SetMute(bMute);
   }
   return true;
}



const ComponentInterfaceSymbol SetTrackVisualsCommand::Symbol
{ XO("Set Track Visuals") };

namespace{ BuiltinCommandsModule::Registration< SetTrackVisualsCommand > reg3; }

enum kColours
{
   kColour0,
   kColour1,
   kColour2,
   kColour3,
   nColours
};

static const EnumValueSymbol kColourStrings[nColours] =
{
   { L"Color0", XO("Color 0") },
   { L"Color1", XO("Color 1") },
   { L"Color2", XO("Color 2") },
   { L"Color3", XO("Color 3") },
};


enum kScaleTypes
{
   kLinear,
   kDb,
   nScaleTypes
};

static const EnumValueSymbol kScaleTypeStrings[nScaleTypes] =
{
   // These are acceptable dual purpose internal/visible names
   { XO("Linear") },
   /* i18n-hint: abbreviates decibels */
   { XO("dB") },
};

enum kZoomTypes
{
   kReset,
   kTimes2,
   kHalfWave,
   nZoomTypes
};

static const EnumValueSymbol kZoomTypeStrings[nZoomTypes] =
{
   { XO("Reset") },
   { L"Times2", XO("Times 2") },
   { XO("HalfWave") },
};

static std::vector<EnumValueSymbol> DiscoverSubViewTypes()
{
   std::vector<EnumValueSymbol> result;
   const auto &types = WaveTrackSubViewType::All();
   for ( const auto &type : types )
      result.emplace_back( type.name.Internal(), type.name.Stripped() );
   result.push_back( WaveTrackViewConstants::MultiViewSymbol );
   return result;
}

bool SetTrackVisualsCommand::DefineParams( ShuttleParams & S ){ 
   SetTrackBase::DefineParams( S );
   S.OptionalN( bHasHeight         ).Define(     mHeight,         L"Height",     120, 44, 2000 );

   {
      auto symbols = DiscoverSubViewTypes();
      S.OptionalN( bHasDisplayType    ).DefineEnum( mDisplayType,    L"Display",    0, symbols.data(), symbols.size() );
   }

   S.OptionalN( bHasScaleType      ).DefineEnum( mScaleType,      L"Scale",      kLinear,   kScaleTypeStrings, nScaleTypes );
   S.OptionalN( bHasColour         ).DefineEnum( mColour,         L"Color",      kColour0,  kColourStrings, nColours );
   S.OptionalN( bHasVZoom          ).DefineEnum( mVZoom,          L"VZoom",      kReset,    kZoomTypeStrings, nZoomTypes );
   S.OptionalN( bHasVZoomTop       ).Define(     mVZoomTop,       L"VZoomHigh",  1.0,  -2.0,  2.0 );
   S.OptionalN( bHasVZoomBottom    ).Define(     mVZoomBottom,    L"VZoomLow",   -1.0, -2.0,  2.0 );

   S.OptionalN( bHasUseSpecPrefs   ).Define(     bUseSpecPrefs,   L"SpecPrefs",  false );
   S.OptionalN( bHasSpectralSelect ).Define(     bSpectralSelect, L"SpectralSel",true );

   auto schemes = SpectrogramSettings::GetColorSchemeNames();
   S.OptionalN( bHasSpecColorScheme).DefineEnum( mSpecColorScheme,L"SpecColor",  SpectrogramSettings::csColorNew, schemes.data(), schemes.size());

   return true;
};

void SetTrackVisualsCommand::PopulateOrExchange(ShuttleGui & S)
{
   SetTrackBase::PopulateOrExchange( S );
   S.StartMultiColumn(3, GroupOptions{}.Position(wxEXPAND).StretchyColumn(2));
   {
      S
         .Optional( bHasHeight      )
         .Target( mHeight )
         .AddTextBox( XXO("Height:") );

      S.Optional( bHasColour      )
         .Target( mColour )
         .AddChoice( XXO("Color:"), Msgids(  kColourStrings, nColours ) );
      
      {
         auto symbols = DiscoverSubViewTypes();
         auto typeNames = transform_container<TranslatableStrings>(
             symbols, std::mem_fn( &EnumValueSymbol::Msgid ) );
         S
            .Optional( bHasDisplayType )
            .Target( mDisplayType )
            .AddChoice( XXO("Display:"), typeNames );
      }

      S
         .Optional( bHasScaleType   )
         .Target( mScaleType )
         .AddChoice( XXO("Scale:"), Msgids( kScaleTypeStrings, nScaleTypes ) );

      S
         .Optional( bHasVZoom       )
         .Target( mVZoom )
         .AddChoice( XXO("VZoom:"), Msgids( kZoomTypeStrings, nZoomTypes ) );

      S
         .Optional( bHasVZoomTop    )
         .Target( mVZoomTop )
         .AddTextBox( XXO("VZoom Top:")  );

      S
         .Optional( bHasVZoomBottom )
         .Target( mVZoomBottom )
         .AddTextBox( XXO("VZoom Bottom:") );
   }
   S.EndMultiColumn();
   S.StartMultiColumn(2, GroupOptions{}.Position(wxEXPAND).StretchyColumn(1));
   {
      S.Optional( bHasUseSpecPrefs   )
         .Target(bUseSpecPrefs)
         .AddCheckBox( XXO("Use Spectral Prefs") );

      S
         .Optional( bHasSpectralSelect )
         .Target( bSpectralSelect )
         .AddCheckBox( XXO("Spectral Select") );
   }
   S.EndMultiColumn();
   S.StartMultiColumn(3, GroupOptions{}.Position(wxEXPAND).StretchyColumn(2));
   {
      auto schemes = SpectrogramSettings::GetColorSchemeNames();
      S
         .Optional( bHasSpecColorScheme)
         .Target( mSpecColorScheme )
         .AddChoice( XXC("Sche&me", "spectrum prefs"),
            Msgids( schemes.data(), schemes.size() ) );
   }
   S.EndMultiColumn();
}

bool SetTrackVisualsCommand::ApplyInner(const CommandContext & context, Track * t )
{
   static_cast<void>(context);
   auto wt = dynamic_cast<WaveTrack *>(t);
   //auto pt = dynamic_cast<PlayableTrack *>(t);
   static const double ZOOMLIMIT = 0.001f;

   // You can get some intriguing effects by setting R and L channels to 
   // different values.
   if( wt && bHasColour )
      wt->SetWaveColorIndex( mColour );

   if( t && bHasHeight )
      TrackView::Get( *t ).SetExpandedHeight( mHeight );

   if( wt && bHasDisplayType  ) {
      auto &view = WaveTrackView::Get( *wt );
      auto &all = WaveTrackSubViewType::All();
      if (mDisplayType < all.size())
         view.SetDisplay( all[ mDisplayType ].id );
      else {
         view.SetMultiView( true );
         view.SetDisplay( WaveTrackSubViewType::Default(), false );
      }
   }
   if( wt && bHasScaleType )
      wt->GetWaveformSettings().scaleType = 
         (mScaleType==kLinear) ? 
            WaveformSettings::stLinear
            : WaveformSettings::stLogarithmic;

   if( wt && bHasVZoom ){
      switch( mVZoom ){
         default:
         case kReset: wt->SetDisplayBounds(-1,1); break;
         case kTimes2: wt->SetDisplayBounds(-2,2); break;
         case kHalfWave: wt->SetDisplayBounds(0,1); break;
      }
   }

   if ( wt && (bHasVZoomTop || bHasVZoomBottom) && !bHasVZoom){
      float vzmin, vzmax;
      wt->GetDisplayBounds(&vzmin, &vzmax);

      if ( !bHasVZoomTop ){
         mVZoomTop = vzmax;
      }
      if ( !bHasVZoomBottom ){
         mVZoomBottom = vzmin;
      }

      // Can't use std::clamp until C++17
      mVZoomTop = std::max(-2.0, std::min(mVZoomTop, 2.0));
      mVZoomBottom = std::max(-2.0, std::min(mVZoomBottom, 2.0));

      if (mVZoomBottom > mVZoomTop){
         std::swap(mVZoomTop, mVZoomBottom);
      }
      if ( mVZoomTop - mVZoomBottom < ZOOMLIMIT ){
         double c = (mVZoomBottom + mVZoomTop) / 2;
         mVZoomBottom = c - ZOOMLIMIT / 2.0;
         mVZoomTop = c + ZOOMLIMIT / 2.0;
      }
      wt->SetDisplayBounds(mVZoomBottom, mVZoomTop);
      auto &tp = TrackPanel::Get( context.project );
      tp.UpdateVRulers();
   }

   if( wt && bHasUseSpecPrefs   ){
      wt->UseSpectralPrefs( bUseSpecPrefs );
   }
   if( wt && bHasSpectralSelect ){
      wt->GetSpectrogramSettings().spectralSelection = bSpectralSelect;
   }
   if (wt && bHasSpecColorScheme) {
      wt->GetSpectrogramSettings().colorScheme = (SpectrogramSettings::ColorScheme)mSpecColorScheme;
   }

   return true;
}


const ComponentInterfaceSymbol SetTrackCommand::Symbol
{ XO("Set Track") };

namespace{ BuiltinCommandsModule::Registration< SetTrackCommand > reg4; }

SetTrackCommand::SetTrackCommand()
{
   mSetStatus.mbPromptForTracks = false;
   mSetAudio.mbPromptForTracks = false;
   mSetVisuals.mbPromptForTracks = false;
}

