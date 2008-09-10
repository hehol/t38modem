/*
 * pmutils.h
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2001-2008 Vyacheslav Frolov
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
 * Contributor(s): Equivalence Pty ltd
 *
 * $Log: pmutils.h,v $
 * Revision 1.18  2008-09-10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.18  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.17  2006/12/11 10:27:35  vfrolov
 * Disabled renaming thread if no PTRACING
 *
 * Revision 1.16  2005/03/05 15:39:12  vfrolov
 * Ignore COUT_TRACE if not PTRACING
 *
 * Revision 1.15  2005/02/04 10:18:49  vfrolov
 * Fixed warnings for No Trace build
 *
 * Revision 1.14  2005/02/03 11:32:12  vfrolov
 * Fixed MSVC compile warnings
 *
 * Revision 1.13  2004/10/20 14:15:09  vfrolov
 * Added reset of signal counter to WaitDataReady()
 *
 * Revision 1.12  2004/07/07 07:53:58  vfrolov
 * Moved ptlib.h including to *.cxx for precompiling
 * Fixed compiler warning
 *
 * Revision 1.11  2004/03/09 17:23:19  vfrolov
 * Added PROCESS_PER_THREAD ifdef
 *
 * Revision 1.10  2003/12/04 13:22:35  vfrolov
 * Removed ambiguous isEof()
 * Improved memory usage in DataStream
 * Fixed myPTRACE
 *
 * Revision 1.9  2003/01/08 16:37:29  vfrolov
 * Changed class DataStream:
 *   members moved to private section and added isEof()
 *   added threshold and isFull()
 *
 * Revision 1.8  2002/12/30 12:49:39  vfrolov
 * Added tracing thread's CPU usage (Linux only)
 *
 * Revision 1.7  2002/12/20 10:13:01  vfrolov
 * Implemented tracing with PID of thread (for LinuxThreads)
 *   or ID of thread (for other POSIX Threads)
 *
 * Revision 1.6  2002/04/27 10:12:21  vfrolov
 * If defined MYPTRACE_LEVEL=N then myPTRACE() will output the trace with level N
 *
 * Revision 1.5  2002/03/07 07:30:44  vfrolov
 * Fixed endless recursive call SignalChildStop(). Possible there is
 * a bug in gcc version 2.95.4 20010902 (Debian prerelease).
 * Markus Storm reported the promlem.
 *
 * Revision 1.4  2002/03/01 08:17:28  vfrolov
 * Added Copyright header
 * Removed virtual modifiers
 *
 * Revision 1.3  2002/02/11 08:35:12  vfrolov
 * myPTRACE() outputs trace to cout only if defined COUT_TRACE
 *
 * Revision 1.2  2002/01/10 06:10:03  craigs
 * Added MPL header
 *
 * Revision 1.1  2002/01/01 23:06:54  craigs
 * Initial version
 *
 */

#ifndef _PMUTILS_H
#define _PMUTILS_H

///////////////////////////////////////////////////////////////
class ModemThread : public PThread
{
    PCLASSINFO(ModemThread, PThread);
  public:
  /**@name Construction */
  //@{
    ModemThread();
  //@}

  /**@name Operations */
  //@{
    void SignalDataReady() { dataReadySyncPoint.Signal(); }
    void SignalChildStop();
    virtual void SignalStop();
  //@}

  protected:
    virtual void Main() = 0;
    void WaitDataReady();

    volatile PBoolean stop;		// *this was requested to stop
    volatile PBoolean childstop;	// there is a child that was requested to stop
    PSyncPoint dataReadySyncPoint;
};
///////////////////////////////////////////////////////////////
class ModemThreadChild : public ModemThread
{
    PCLASSINFO(ModemThreadChild, ModemThread);
  public:
  /**@name Construction */
  //@{
    ModemThreadChild(ModemThread &_parent);
  //@}

  /**@name Operations */
  //@{
    virtual void SignalStop();
  //@}

  protected:
    ModemThread &parent;
};
///////////////////////////////////////////////////////////////
PQUEUE(_PBYTEArrayQ, PBYTEArray);

class PBYTEArrayQ : public _PBYTEArrayQ
{
    PCLASSINFO(PBYTEArrayQ, _PBYTEArrayQ);
  public:
    PBYTEArrayQ() : count(0) {}
    ~PBYTEArrayQ() { Clean(); }

    virtual void Enqueue(PBYTEArray *buf) {
      PWaitAndSignal mutexWait(Mutex);
      count += buf->GetSize();
      _PBYTEArrayQ::Enqueue(buf);
    }

    virtual PBYTEArray *Dequeue() {
      PWaitAndSignal mutexWait(Mutex);
      PBYTEArray *buf = _PBYTEArrayQ::Dequeue();
      if( buf ) count -= buf->GetSize();
      return buf;
    }

    PINDEX GetCount() const { return count; }

    void Clean() {
      PBYTEArray *buf;
      while( (buf = Dequeue()) != NULL ) {
        delete buf;
      }
    }
  protected:
    PINDEX count;
    PMutex Mutex;
};
///////////////////////////////////////////////////////////////
class ChunkStream : public PObject
{
    PCLASSINFO(ChunkStream, PObject);
  public:
    ChunkStream() : first(0), last(0) {}

    int write(const void *pBuf, PINDEX count);
    int read(void *pBuf, PINDEX count);

  private:
    BYTE data[256];
    PINDEX first;
    PINDEX last;
};

PQUEUE(ChunkStreamQ, ChunkStream);
///////////////////////////////////////////////////////////////
class DataStream : public PObject
{
    PCLASSINFO(DataStream, PObject);
  public:
    DataStream(PINDEX _threshold = 0)
      : firstBuf(NULL), lastBuf(NULL), busy(0),
        threshold(_threshold), eof(FALSE), diag(0) {}
    ~DataStream() { DataStream::Clean(); }

    int PutData(const void *pBuf, PINDEX count);
    int GetData(void *pBuf, PINDEX count);
    void PutEof() { eof = TRUE; }
    int GetDiag() const { return diag; }
    DataStream &SetDiag(int _diag) { diag = _diag; return *this; }
    PBoolean isFull() const { return threshold && threshold < busy; }
    virtual void Clean();

  private:
    ChunkStream *firstBuf;
    ChunkStreamQ bufQ;
    ChunkStream *lastBuf;	// if not NULL then it should be in bufQ or firstBuf
    PINDEX busy;

    PINDEX threshold;
    PBoolean eof;
    int diag;
};
///////////////////////////////////////////////////////////////
PQUEUE(_DataStreamQ, DataStream);

class DataStreamQ : public _DataStreamQ
{
    PCLASSINFO(DataStreamQ, _DataStreamQ);
  public:
    DataStreamQ() {}
    ~DataStreamQ() { Clean(); }

    virtual void Enqueue(DataStream *buf) {
      PWaitAndSignal mutexWait(Mutex);
      _DataStreamQ::Enqueue(buf);
    }

    virtual DataStream *Dequeue() {
      PWaitAndSignal mutexWait(Mutex);
      return _DataStreamQ::Dequeue();
    }

    void Clean() {
      DataStream *buf;
      while( (buf = Dequeue()) != NULL ) {
        delete buf;
      }
    }
  protected:
    PMutex Mutex;
};
///////////////////////////////////////////////////////////////
#ifdef _MSC_VER
// warning C4127: conditional expression is constant
#pragma warning(disable:4127)
#endif // _MSC_VER

#if !PTRACING && defined COUT_TRACE
#undef COUT_TRACE
#endif

#ifdef COUT_TRACE
#define _myPTRACE(level, args) do { \
  PTRACE(level, args); \
  cout << PThread::Current()->GetThreadName() << ": " << args << endl; \
} while(0)
#define myCanTrace(level) TRUE
#define myPTRACE_PARAM(param) param
#else
#define _myPTRACE(level, args) do { PTRACE(level, args); } while(0)
#define myCanTrace(level) PTrace::CanTrace(level)
#define myPTRACE_PARAM(param) PTRACE_PARAM(param)
#endif // COUT_TRACE

#ifdef MYPTRACE_LEVEL
#define myPTRACE(level, args) _myPTRACE(MYPTRACE_LEVEL, args)
#else
#define myPTRACE(level, args) _myPTRACE(level, args)
#endif // MYPTRACE_LEVEL

#define PRTHEX(data) " {\n" << setprecision(2) << hex << setfill('0') << data << dec << setfill(' ') << " }"
///////////////////////////////////////////////////////////////
#if PTRACING
extern void RenameCurrentThread(const PString &newname);
#else
#define RenameCurrentThread(newname)
#endif /* PTRACING */

#ifdef PROCESS_PER_THREAD
extern const PString GetThreadTimes(const char *head = "", const char *tail = "");
#else
inline const PString GetThreadTimes(const char *head = "", const char *tail = "");

inline const PString GetThreadTimes(const char *, const char *)
{
  return "";
}
#endif
///////////////////////////////////////////////////////////////

#endif  // _PMUTILS_H

