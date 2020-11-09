/*
 * manager.h
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
 * $Log: manager.h,v $
 * Revision 1.6  2010-03-15 13:40:27  vfrolov
 * Removed unused code
 *
 * Revision 1.6  2010/03/15 13:40:27  vfrolov
 * Removed unused code
 *
 * Revision 1.5  2009/11/10 11:30:57  vfrolov
 * Implemented G.711 fallback to fax pass-through mode
 *
 * Revision 1.4  2009/07/15 13:23:20  vfrolov
 * Added Descriptions(args)
 *
 * Revision 1.3  2009/01/15 08:46:34  vfrolov
 * Fixed OnRouteConnection() be able to compile with OPAL trunk since 21925
 *
 * Revision 1.2  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.1  2007/05/28 12:47:52  vfrolov
 * Initial revision
 *
 */

#ifndef _PM_MANAGER_H
#define _PM_MANAGER_H

#include <opal/manager.h>
#include "../version.h"

/////////////////////////////////////////////////////////////////////////////
/**Create a process for OpalManagerConsole based applications.
  */
template <class Manager,                          ///< Class of OpalManagerConsole derived class
          const char Manuf[],                     ///< Name of manufacturer
          const char Name[],                      ///< Name of product
          unsigned MajorVersion = MAJOR_VERSION,  ///< Major version number of the product
          unsigned MinorVersion = MINOR_VERSION,  ///< Minor version number of the product
          PProcess::CodeStatus Status = BUILD_TYPE, ///< Development status of the product
          unsigned BuildNumber = BUILD_NUMBER,    ///< Build number of the product
          bool Verbose = true>

class MyManagerProcess : public PProcess
{
    PCLASSINFO(MyManagerProcess, PProcess)
  public:
    MyManagerProcess()
      : PProcess(Manuf, Name, MajorVersion, MinorVersion, Status, BuildNumber)
      , m_manager(NULL)
    {
    }

    ~MyManagerProcess()
    {
      delete this->m_manager;
    }

    virtual void Main()
    {
      this->SetTerminationValue(1);
      this->m_manager = new Manager;
      if (this->m_manager->Initialise(this->GetArguments(), Verbose)) {
        this->SetTerminationValue(0);
        this->m_manager->Run();
      }
    }

    virtual bool OnInterrupt(bool)
    {
      if (this->m_manager == NULL)
        return false;

      this->m_manager->EndRun(true);
      return false;
    }

  private:
    Manager * m_manager;
};


/////////////////////////////////////////////////////////////////////////////

class MyManagerEndPoint;

class MyManager : public OpalManager
{
  PCLASSINFO(MyManager, OpalManager);

  public:
    MyManager()
    //  : m_endpointPrefixes(PConstString(endpointPrefixes).Tokenise(" \t\n"))
      : m_interrupted(false)
      , m_verbose(false)
      , m_outputStream(&cout)
    {
    }

    virtual PString GetArgumentSpec();
    virtual void Usage(ostream & strm, const PArgList & args);
    virtual bool PreInitialise(PArgList & args, bool verbose = false);
    virtual bool Initialise(PArgList & args, bool verbose, const PString & defaultRoute = PString::Empty());
    virtual void Run();
    virtual void EndRun(bool interrupt = false);
    virtual void Broadcast(const PString & msg);

    virtual bool OnRouteConnection(
      PStringSet & routesTried,     ///< Set of routes already tried
      const PString & a_party,      ///< Source local address
      const PString & b_party,      ///< Destination indicated by source
      OpalCall & call,              ///< Call for new connection
      unsigned options,             ///< Options for new connection (can't use default as overrides will fail)
      OpalConnection::StringOptions * stringOptions
    );
    virtual void OnClearedCall(OpalCall & call);

    virtual PBoolean OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream);
    virtual void OnClosedMediaStream(const OpalMediaStream & stream);

    virtual PString ApplyRouteTable(const PString & proto, const PString & addr, PINDEX & entry);

    class LockedStream : PWaitAndSignal
    {
      protected:
        ostream & m_stream;
      public:
        LockedStream(const MyManager & mgr)
          : PWaitAndSignal(mgr.m_outputMutex)
          , m_stream(*mgr.m_outputStream)
        {
        }

        ostream & operator *() const { return m_stream; }
        operator  ostream & () const { return m_stream; }
    };
    friend class LockedStream;
    __inline LockedStream LockedOutput() const { return *this; }

  protected:
    PMutex              m_outputMutex;
    PSimpleTimer        m_competionTimeout;
    bool                m_showProgress;
    PSyncPoint          m_endRun;
    PStringArray        m_endpointPrefixes;
    bool                m_interrupted;
    bool                m_verbose;
    ostream           * m_outputStream;
    MyManagerEndPoint * epSIP;
    MyManagerEndPoint * epFAX;
};
/////////////////////////////////////////////////////////////////////////////

/**This class allows for each end point class, e.g. SIPEndPoint, to add it's
   set of parameters/commands to to the console application.
  */
class MyManagerEndPoint
{
protected:
  MyManagerEndPoint(MyManager & manager) : m_mgr(manager) { }

  void AddRoutesFor(const OpalEndPoint * endpoint, const PString & defaultRoute)
    {
      if (defaultRoute.IsEmpty())
        return;
    
      PStringList prefixes = m_mgr.GetPrefixNames(endpoint);
    
      for (PINDEX i = 0; i < prefixes.GetSize(); ++i)
        m_mgr.AddRouteEntry(prefixes[i] + ":.* = " + defaultRoute);
    }


public:
  virtual ~MyManagerEndPoint() { }
  enum InitResult {
    InitFailed,
    InitDisabled,
    InitSuccess
  };
  virtual bool Initialise(PArgList & args, bool verbose, const PString & defaultRoute) = 0;

protected:
  MyManager & m_mgr;
};

#endif  // _PM_MANAGER_H

