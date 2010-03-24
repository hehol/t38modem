/*
 * fake_codecs.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2010 Vyacheslav Frolov
 *
 * t38modem Project
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is t38modem.
 *
 * The Initial Developer of the Original Code is Vyacheslav Frolov
 *
 * Contributor(s):
 *
 * $Log: fake_codecs.cxx,v $
 * Revision 1.3  2010-03-24 10:48:29  vfrolov
 * Fixed incompatibility with OPAL trunk
 *
 * Revision 1.3  2010/03/24 10:48:29  vfrolov
 * Fixed incompatibility with OPAL trunk
 *
 * Revision 1.2  2010/03/19 08:27:52  vfrolov
 * Added "search_for_fake_transcoder" command
 *
 * Revision 1.1  2010/02/12 08:20:10  vfrolov
 * Initial revision
 *
 */

#include <ptlib.h>

#include <opal/buildopts.h>

/////////////////////////////////////////////////////////////////////////////
#define PACK_VERSION(major, minor, build) (((((major) << 8) + (minor)) << 8) + (build))

#if !(PACK_VERSION(OPAL_MAJOR, OPAL_MINOR, OPAL_BUILD) >= PACK_VERSION(3, 8, 1))
  #error *** Uncompatible OPAL version (required >= 3.8.1) ***
#endif

#undef PACK_VERSION
/////////////////////////////////////////////////////////////////////////////

#include <opal/transcoders.h>

#include "fake_codecs.h"

#define new PNEW

/////////////////////////////////////////////////////////////////////////////
#define FAKE_AUDIO_FORMATS(pattern) \
  pattern(G722)                     \
  pattern(G7221)                    \
  pattern(G7222)                    \
  pattern(G726_40K)                 \
  pattern(G726_32K)                 \
  pattern(G726_24K)                 \
  pattern(G726_16K)                 \
  pattern(G728)                     \
  pattern(G729)                     \
  pattern(G729A)                    \
  pattern(G729B)                    \
  pattern(G729AB)                   \
  pattern(G7231_6k3)                \
  pattern(G7231_5k3)                \
  pattern(G7231A_6k3)               \
  pattern(G7231A_5k3)               \
  pattern(GSM0610)                  \
  pattern(GSMAMR)                   \
  pattern(iLBC)                     \
/////////////////////////////////////////////////////////////////////////////
namespace FakeCodecs {
/////////////////////////////////////////////////////////////////////////////
class FakeFramedAudioTranscoder : public OpalFramedTranscoder
{
  PCLASSINFO(FakeFramedAudioTranscoder, OpalFramedTranscoder);
  public:
    FakeFramedAudioTranscoder(const char * inFormat, const char * outFormat)
      : OpalFramedTranscoder(inFormat, outFormat)
    {}

    PBoolean ExecuteCommand(const OpalMediaCommand & command) {
      if (command.GetName() == "search_for_fake_transcoder") {
        PTRACE(4, "FakeFramedAudioTranscoder::ExecuteCommand: found fake transcoder " << *this);
        return true;
      }

      return OpalFramedTranscoder::ExecuteCommand(command);
    }

    PBoolean ConvertFrame(const BYTE *, PINDEX &, BYTE *, PINDEX &outLen) {
      outLen = 0;
      return PTrue;
    }
};
/////////////////////////////////////////////////////////////////////////////
#define DECLARE_FAKE_AUDIO_TRANSCODER(inFormat, outFormat) \
  class Fake_##inFormat##_##outFormat##_Transcoder : public FakeFramedAudioTranscoder \
  { \
    public: \
      Fake_##inFormat##_##outFormat##_Transcoder() \
      : FakeFramedAudioTranscoder(OPAL_##inFormat, OPAL_##outFormat) {} \
  } \

#define DECLARE_FAKE_AUDIO_CODEC(format) \
  DECLARE_FAKE_AUDIO_TRANSCODER(format, PCM16); \
  DECLARE_FAKE_AUDIO_TRANSCODER(PCM16, format); \
/////////////////////////////////////////////////////////////////////////////
#define REGISTER_FAKE_TRANSCODER(inFormat, outFormat) \
  new OpalTranscoderFactory::Worker<Fake_##inFormat##_##outFormat##_Transcoder>( \
    MakeOpalTranscoderKey(OPAL_##inFormat, OPAL_##outFormat), false) \

#define REGISTER_FAKE_AUDIO_CODEC(format) \
  { \
  REGISTER_FAKE_TRANSCODER(format, PCM16); \
  REGISTER_FAKE_TRANSCODER(PCM16, format); \
  } \
/////////////////////////////////////////////////////////////////////////////
FAKE_AUDIO_FORMATS(DECLARE_FAKE_AUDIO_CODEC);
/////////////////////////////////////////////////////////////////////////////
static bool IsMatch(const PString &str, const PStringArray &wildcards)
{
  for (PINDEX i = 0 ; i < wildcards.GetSize() ; i++) {
    PString pattern = wildcards[i];

    if (pattern.IsEmpty())
      continue;

    bool negative;

    if (pattern[0] == '!') {
      negative = true;
      pattern = pattern.Mid(1);

      if (pattern.IsEmpty())
        return true;
    } else {
      negative = false;
    }

    pattern = PRegularExpression::EscapeString(pattern);
    pattern.Replace("\\*", ".*", true);

    PRegularExpression regexp;

    if (!regexp.Compile(pattern, PRegularExpression::IgnoreCase|PRegularExpression::Extended)) {
      PTRACE(1, "FakeCodecs::IsMatch: "
                "Wildcard \"" << wildcards[i] << "\" ignored"
                ", \"" << pattern << "\" - " << regexp.GetErrorText());
      cerr << "Wildcard \"" << wildcards[i] << "\" ignored" << endl;
      continue;
    }

    if (str.MatchesRegEx(regexp) != negative)
      return true;
  }

  return false;
}
/////////////////////////////////////////////////////////////////////////////
static OpalMediaFormatList GetNormalMediaFormats()
{
  static const OpalMediaFormatList *pFormats = NULL;

  if (pFormats == NULL)
    pFormats = new OpalMediaFormatList(OpalMediaFormat::GetAllRegisteredMediaFormats());

  return *pFormats;
}
/////////////////////////////////////////////////////////////////////////////
static OpalMediaFormatList GetFakeAudioFormats(const PStringArray &wildcards = PStringArray())
{
  GetNormalMediaFormats(); // init list of normal formats

  static OpalMediaFormatList formats;
  const OpalMediaFormatList registered = OpalMediaFormat::GetAllRegisteredMediaFormats();

  #define ADD_FORMAT(format) \
    if (!registered.HasFormat(OPAL_##format) && IsMatch(OPAL_##format, wildcards)) { \
      REGISTER_FAKE_AUDIO_CODEC(format); \
      formats += Opal##format; \
      PTRACE(3, "FakeCodecs::GetFakeAudioFormats: Registered fake audio format " << OPAL_##format); \
    } \

  FAKE_AUDIO_FORMATS(ADD_FORMAT);

  #undef ADD_FORMAT

  return formats;
}
/////////////////////////////////////////////////////////////////////////////
static PStringArray GetNotRegisteredFakeAudioFormatNames()
{
  PStringArray names;
  const OpalMediaFormatList registered = OpalMediaFormat::GetAllRegisteredMediaFormats();

  #define ADD_FORMAT(format) \
    if (!registered.HasFormat(OPAL_##format)) { \
      names += OPAL_##format; \
    } \

  FAKE_AUDIO_FORMATS(ADD_FORMAT);

  #undef ADD_FORMAT

  return names;
}
/////////////////////////////////////////////////////////////////////////////
PStringArray GetAvailableAudioFormatsDescription(const char *name, const char *protocol)
{
  PStringArray descriptions;

  descriptions.Append(new PString(PString("Available audio formats for ") + name + ":"));

  OpalMediaFormatList formats;

  formats = GetNormalMediaFormats();

  for (OpalMediaFormatList::iterator f = formats.begin(); f != formats.end(); ++f) {
    if (f->GetMediaType() == OpalMediaType::Audio() && f->IsTransportable() && f->IsValidForProtocol(protocol))
      descriptions.Append(new PString(PString("  ") + f->GetName()));
  }

  formats = GetFakeAudioFormats();

  for (OpalMediaFormatList::iterator f = formats.begin(); f != formats.end(); ++f) {
    if (f->GetMediaType() == OpalMediaType::Audio() && f->IsTransportable() && f->IsValidForProtocol(protocol))
      descriptions.Append(new PString(PString("  fake ") + f->GetName()));
  }

  PStringArray names = GetNotRegisteredFakeAudioFormatNames();

  for (PINDEX i = 0 ; i < names.GetSize() ; i++) {
      descriptions.Append(new PString(PString("  not registered fake ") + names[i]));
  }

  return descriptions;
}
/////////////////////////////////////////////////////////////////////////////
void RegisterFakeAudioFormats(const PStringArray &wildcards)
{
  GetFakeAudioFormats(wildcards);
}
/////////////////////////////////////////////////////////////////////////////
} // namespace FakeCodecs
/////////////////////////////////////////////////////////////////////////////

