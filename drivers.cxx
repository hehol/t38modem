/*
 * drivers.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2004-2008 Vyacheslav Frolov
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
 * $Log: drivers.cxx,v $
 * Revision 1.4  2008-09-10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.4  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.3  2007/01/29 12:44:41  vfrolov
 * Added ability to put args to drivers
 *
 * Revision 1.2  2004/07/07 13:45:17  vfrolov
 * Added C0C driver for Windows
 *
 * Revision 1.1  2004/07/07 12:38:32  vfrolov
 * The code for pseudo-tty (pty) devices that communicates with fax application formed to PTY driver.
 * 
 */

#include <ptlib.h>
#include "pmodemi.h"
#include "drivers.h"
#include "drv_pty.h"
#include "drv_c0c.h"

///////////////////////////////////////////////////////////////

#define new PNEW

///////////////////////////////////////////////////////////////

static int numDrivers = 0;

///////////////////////////////////////////////////////////////
#define MAX_DRIVERS 10

static struct {
  const char *name;
  PBoolean (*CheckTty)(
    const PString &_tty
  );
  PString (*ArgSpec)();
  PStringArray (*Description)();
  PseudoModem *(*CreatePseudoModem)(
    const PString &tty,
    const PString &route,
    const PConfigArgs &args,
    const PNotifier &callbackEndPoint
  );
} drivers[MAX_DRIVERS];

static int addDriver(
  const char *name,
  PBoolean (*CheckTty)(
    const PString &_tty
  ),
  PString (*ArgSpec)(),
  PStringArray (*Description)(),
  PseudoModem *(*CreatePseudoModem)(
    const PString &tty,
    const PString &route,
    const PConfigArgs &args,
    const PNotifier &callbackEndPoint
  )
)
{
  if (numDrivers >= MAX_DRIVERS)
    return -1;

  drivers[numDrivers].name = name;
  drivers[numDrivers].CheckTty = CheckTty;
  drivers[numDrivers].ArgSpec = ArgSpec;
  drivers[numDrivers].Description = Description;
  drivers[numDrivers].CreatePseudoModem = CreatePseudoModem;
  return numDrivers++;
}

///////////////////////////////////////////////////////////////
#define DECLARE_MODEM_DRIVER(name, suffix)                    \
  static PseudoModem *CreatePseudoModem##suffix(              \
      const PString &tty,                                     \
      const PString &route,                                   \
      const PConfigArgs &args,                                \
      const PNotifier &callbackEndPoint)                      \
  {                                                           \
    return new                                                \
      PseudoModem##suffix(tty, route, args, callbackEndPoint);\
  }                                                           \
  static const int ___addDriver##suffix = addDriver(          \
    name,                                                     \
    &PseudoModem##suffix::CheckTty,                           \
    &PseudoModem##suffix::ArgSpec,                            \
    &PseudoModem##suffix::Description,                        \
    &CreatePseudoModem##suffix                                \
  );                                                          \

///////////////////////////////////////////////////////////////
#ifdef MODEM_DRIVER_Pty
  DECLARE_MODEM_DRIVER("PTY", Pty)
#endif
#ifdef MODEM_DRIVER_C0C
  DECLARE_MODEM_DRIVER("C0C", C0C)
#endif
///////////////////////////////////////////////////////////////
PseudoModem *PseudoModemDrivers::CreateModem(
    const PString &tty,
    const PString &route,
    const PConfigArgs &args,
    const PNotifier &callbackEndPoint
)
{
  PseudoModem *modem = NULL;

  for (int i = 0 ; i < numDrivers ; i++) {
    if (drivers[i].CheckTty(tty)) {
      modem = drivers[i].CreatePseudoModem(tty, route, args, callbackEndPoint);

      if (!modem) {
        myPTRACE(1, "PseudoModemDrivers::CreateModem " << tty << " was not created");
        return NULL;
      }
      break;
    }
  }

  if (!modem) {
    myPTRACE(1, "PseudoModemDrivers::CreateModem " << tty << " is not a valid value");
    return NULL;
  }

  if (!modem->IsValid()) {
    myPTRACE(1, "PseudoModemDrivers::CreateModem " << tty << " is not valid");
    delete modem;
    return NULL;
  }

  return modem;
}

PString PseudoModemDrivers::ArgSpec()
{
  PString argSpec;

  for (int i = 0 ; i < numDrivers ; i++)
    argSpec += drivers[i].ArgSpec();

  return argSpec;
}

PStringArray PseudoModemDrivers::Descriptions()
{
  PStringArray descriptions;

  for (int i = 0 ; i < numDrivers ; i++) {
    descriptions.Append(new PString(drivers[i].name));

    PStringArray description = drivers[i].Description();

    for (PINDEX j = 0 ; j < description.GetSize() ; j++)
      descriptions.Append(new PString(PString("  ") + description[j]));
  }

  return descriptions;
}
///////////////////////////////////////////////////////////////

