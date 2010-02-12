/*
 * main_process.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2007-2010 Vyacheslav Frolov
 *
 * Open H323 Project
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Vyacheslav Frolov
 *
 * Contributor(s):
 *
 * $Log: main_process.cxx,v $
 * Revision 1.9  2010-02-12 08:33:44  vfrolov
 * Moved PTrace::Initialise() to beginning
 *
 * Revision 1.9  2010/02/12 08:33:44  vfrolov
 * Moved PTrace::Initialise() to beginning
 *
 * Revision 1.8  2009/07/29 10:39:04  vfrolov
 * Moved h323lib specific code to h323lib directory
 *
 * Revision 1.7  2009/07/15 13:23:19  vfrolov
 * Added Descriptions(args)
 *
 * Revision 1.6  2009/07/03 09:18:11  vfrolov
 * Included opal/buildopts.h
 * Added workarounds for race condition on exit
 * Suppressed version tracing on help output
 *
 * Revision 1.5  2009/04/03 12:04:36  vfrolov
 * Added versions of used libs to output
 *
 * Revision 1.4  2009/01/21 12:25:12  vfrolov
 * Added tracing of options
 *
 * Revision 1.3  2008/09/11 16:04:47  frolov
 * Added list of libs to output
 *
 * Revision 1.2  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.1  2007/05/17 08:32:44  vfrolov
 * Moved class T38Modem from main.h and main.cxx to main_process.cxx
 *
 */

#include <ptlib.h>

#ifdef USE_OPAL
  #include <opal/buildopts.h>
#endif

#include "version.h"

#ifdef USE_OPAL
  #include "opal/manager.h"
#else
  #include "h323lib/h323ep.h"
#endif

#define new PNEW

/////////////////////////////////////////////////////////////////////////////
static PString GetListOfLibs()
{
  return
#ifdef USE_OPAL
    PString("OPAL-")
    + PString(OPAL_VERSION)
    + PString("/")
    + OpalGetVersion()
#else
  #if OPENH323_MAJOR < 1 || (OPENH323_MAJOR == 1 && OPENH323_MINOR <= 19)
    PString("OpenH323-") + PString(OPENH323_VERSION)
  #else
    PString("H323plus-") + PString(OPENH323_VERSION)
  #endif
#endif
#ifdef PTLIB_VERSION
    + PString(", PTLIB-")
    + PString(PTLIB_VERSION)
  #if PTLIB_MAJOR > 2 || (PTLIB_MAJOR == 2 && PTLIB_MINOR >= 6)
    + PString("/")
    + PProcess::GetLibVersion()
  #endif
#endif
#ifdef PWLIB_VERSION
    + PString(", PWLIB-") + PString(PWLIB_VERSION)
#endif
  ;
}
/////////////////////////////////////////////////////////////////////////////
class T38Modem : public PProcess
{
  PCLASSINFO(T38Modem, PProcess)

  public:
    T38Modem();

    void Main();

  protected:
    PBoolean Initialise();
};

PCREATE_PROCESS(T38Modem);
///////////////////////////////////////////////////////////////
T38Modem::T38Modem()
  : PProcess("Vyacheslav Frolov", "T38Modem",
             MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER)
{
}

void T38Modem::Main()
{
  cout << GetName()
       << " Version " << GetVersion(TRUE) << "\n"
       << " (" << GetListOfLibs() << ")"
       << " by " << GetManufacturer()
       << " on " << GetOSClass() << ' ' << GetOSName()
       << " (" << GetOSVersion() << '-' << GetOSHardware() << ")\n"
       << endl;

  if (!Initialise()) {
    PThread::Sleep(100);  // workaround for race condition
    return;
  }

  for (;;)
    PThread::Sleep(5000);
}

PBoolean T38Modem::Initialise()
{
  PConfigArgs args(GetArguments());

  args.Parse(
#ifdef USE_OPAL
             MyManager::ArgSpec() +
#else
             MyH323EndPoint::ArgSpec() +
#endif
             "h-help."
             "v-version."
#if PMEMORY_CHECK
             "-setallocationbreakpoint:"
#endif
#if PTRACING
             "t-trace."
             "o-output:"
#endif
             "-save."
          , FALSE);

#if PMEMORY_CHECK
  if (args.HasOption("setallocationbreakpoint"))
    PMemoryHeap::SetAllocationBreakpoint(args.GetOptionString("setallocationbreakpoint").AsInteger());
#endif

#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
                     PTrace::DateAndTime | PTrace::Thread | PTrace::Blocks);
#endif

  if (args.HasOption('h')) {
    cout <<
        "Usage:\n"
        "  " << GetName() << " [options]\n"
        "\n"
        "Options:\n"
#if PTRACING
        "  -t --trace                : Enable trace, use multiple times for more detail.\n"
        "  -o --output file          : File for trace output, default is stderr.\n"
#endif
        "     --save                 : Save arguments in configuration file and exit.\n"
        "  -v --version              : Display version.\n"
        "  -h --help                 : Display this help message.\n"
        "\n";

    PStringArray descriptions =
#ifdef USE_OPAL
        MyManager::Descriptions();
#else
        MyH323EndPoint::Descriptions();
#endif

    for (PINDEX i = 0 ; i < descriptions.GetSize() ; i++)
      cout << descriptions[i] << endl;

    return FALSE;
  }

  PStringArray info =
#ifdef USE_OPAL
      MyManager::Descriptions(args);
#else
      MyH323EndPoint::Descriptions(args);
#endif

  if (info.GetSize() > 0) {
    for (PINDEX i = 0 ; i < info.GetSize() ; i++)
      cout << info[i] << endl;

    return FALSE;
  }

  if (args.HasOption('v'))
    return FALSE;

  if (args.HasOption("save")) {
    args.Save("save");
    cout << "Arguments were saved in configuration file" << endl;
    return FALSE;
  }

  PTRACE(1, GetName()
      << " Version " << GetVersion(TRUE)
      << " (" << GetListOfLibs() << ")"
      << " on " << GetOSClass() << " " << GetOSName()
      << " (" << GetOSVersion() << '-' << GetOSHardware() << ")");

#if PTRACING
  if (PTrace::CanTrace(3)) {
    PTRACE(3, "Options: " << args);

    const PConfig config;
    const PStringArray keys = config.GetKeys();

    if (!keys.IsEmpty()) {
      PTRACE(3, "Config:");
      for (PINDEX iK = 0 ; iK < keys.GetSize() ; iK++) {
        const PStringArray values = config.GetString(keys[iK]).Lines();

        for (PINDEX iV = 0 ; iV < values.GetSize() ; iV++) {
          PTRACE(3, "  --" << keys[iK] << "=" << values[iV]);
        }
      }
    }
  }
#endif

#ifdef USE_OPAL
  MyManager *manager = new MyManager();

  if (!manager->Initialise(args))
    return FALSE;
#else
  if (!MyH323EndPoint::Create(args))
    return FALSE;
#endif

  return TRUE;
}
/////////////////////////////////////////////////////////////////////////////

