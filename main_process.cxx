/*
 * main_process.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2007-2008 Vyacheslav Frolov
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
 * Revision 1.2  2008-09-10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.2  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.1  2007/05/17 08:32:44  vfrolov
 * Moved class T38Modem from main.h and main.cxx to main_process.cxx
 *
 */

#include <ptlib.h>

#include "version.h"

#ifdef USE_OPAL
  #include "opal/manager.h"
#else
  #include "main.h"
#endif

#define new PNEW

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
  : PProcess("OpenH323 Project", "T38Modem",
             MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER)
{
}

void T38Modem::Main()
{
  cout << GetName()
       << " Version " << GetVersion(TRUE) << "\n"
       << " by " << GetManufacturer()
       << " on " << GetOSClass() << ' ' << GetOSName()
       << " (" << GetOSVersion() << '-' << GetOSHardware() << ")\n\n";

  if (!Initialise())
    return;

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

  PTRACE(1, GetName()
      << " Version " << GetVersion(TRUE)
      << " on " << GetOSClass() << " " << GetOSName()
      << " (" << GetOSVersion() << '-' << GetOSHardware() << ")");
#endif

  if (args.HasOption('h')) {
    cout <<
        "Usage:\n"
        "  " << GetName() << " [options]\n"
        "\n"
        "Options:\n"
#if PTRACING
        "  -t --trace                : Enable trace, use multiple times for more detail.\n"
        "  -o --output               : File for trace output, default is stderr.\n"
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
      cout << descriptions[i] << "\n";

    return FALSE;
  }

  if (args.HasOption('v'))
    return FALSE;

  if (args.HasOption("save")) {
    args.Save("save");
    cout << "Arguments were saved in configuration file\n";
    return FALSE;
  }

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

