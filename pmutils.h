/*
 * $Id: pmutils.h,v 1.1 2002-01-01 23:06:54 craigs Exp $
 *
 * T38FAX Pseudo Modem
 *
 * Original author: Vyacheslav Frolov
 *
 * $Log: pmutils.h,v $
 * Revision 1.1  2002-01-01 23:06:54  craigs
 * Initial version
 *
 * Revision 1.1  2002/01/01 23:06:54  craigs
 * Initial version
 *
 */

#ifndef _PMUTILS_H
#define _PMUTILS_H

#include <ptlib.h>

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
    virtual void SignalDataReady() { dataReadySyncPoint.Signal(); }
    virtual void SignalChildStop() {
      childstop = TRUE;
      SignalDataReady();
    }
    virtual void SignalStop() {
      stop = TRUE;
      SignalDataReady();
    }
  //@}

  protected:
    virtual void Main() = 0;
    virtual void WaitDataReady() { dataReadySyncPoint.Wait(); }
    
    volatile BOOL stop;		// *this was requested to stop
    volatile BOOL childstop;	// there is a child that was requested to stop
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
class DataStream : public PObject
{
    PCLASSINFO(DataStream, PObject);
  public:
    
    DataStream() : done(0), eof(FALSE), diag(0) {}
    
    int PutData(const void *pBuf, PINDEX count);
    int GetData(void *pBuf, PINDEX count);
    void PutEof() { eof = TRUE; }
    int GetDiag() const { return diag; }
    DataStream &SetDiag(int _diag) { diag = _diag; return *this; }
    
    virtual void Clean() {
      CleanData();
      eof = FALSE;
      diag = 0;
    }
  protected:
    void CleanData() {
      data = PBYTEArray();
      done = 0;
    }
    
    PBYTEArray data;
    PINDEX done;
    BOOL eof;
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
#define myPTRACE(level, args) {	\
  PTRACE(level, args);		\
  cout << PThread::Current()->GetThreadName() << ": " << args << endl;		\
}

#define PRTHEX(data) " {\n" << setprecision(2) << hex << setfill('0') << data << dec << setfill(' ') << " }"
///////////////////////////////////////////////////////////////

#endif  // _PMUTILS_H

