/*
 * pty.cxx
 *
 * T38FAX Pseudo Modem
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
 * $Log: pty.cxx,v $
 * Revision 1.3  2002-01-10 06:10:03  craigs
 * Added MPL header
 *
 * Revision 1.3  2002/01/10 06:10:03  craigs
 * Added MPL header
 *
 * Revision 1.2  2002/01/01 23:43:57  craigs
 * Added fix from Vyacheslav Frolov
 *
 * Revision 1.1  2002/01/01 23:06:54  craigs
 * Initial version
 *
 */

#include <sys/time.h>
#include "pmodemi.h"
#include "pty.h"

#define new PNEW

#define PrepareSelect(n, fdset, tv)	\
  int n = hPty + 1;			\
  fd_set fdset;				\
  struct timeval tv;			\
  FD_ZERO(&fdset);			\
  FD_SET(hPty, &fdset);			\
  tv.tv_sec = 5;			\
  tv.tv_usec = 0;
///////////////////////////////////////////////////////////////
UniPty::UniPty(PseudoModemBody &_parent)
  : ModemThreadChild(_parent),
    hPty(_parent.handlePty())
{
}
///////////////////////////////////////////////////////////////
InPty::InPty(PseudoModemBody &_parent)
  : UniPty(_parent)
{
  SetThreadName(Parent().ptyName() + "(i):%0x");
}

void InPty::Main()
{
    myPTRACE(1, "--> Started");
    
    for(;;) {
      PrepareSelect(n, fdset, tv);
      
      if( stop ) break;
      ::select(n, &fdset, NULL, NULL, &tv);

      if(FD_ISSET(hPty, &fdset)) {
        char buf[1024];
        int len;
        
        {
          PWaitAndSignal mutexWait(Parent().ptyMutex);	// ???
          if( stop ) break;
          len = ::read(hPty, buf, 1024);
          
          if( len < 0 ) {
            int err = errno;
            myPTRACE(1, "--> read ERROR " << len << " " << strerror(err));
            SignalStop();
            break;
            //continue;
          }
        }
        
        if( len > 0 ) {
          Parent().ToPtyQ(buf, len, FALSE);
          if( stop ) break;
        }
      }
    }
    myPTRACE(1, "--> Stoped");
}
///////////////////////////////////////////////////////////////
OutPty::OutPty(PseudoModemBody &_parent)
  : UniPty(_parent)
{
  SetThreadName(Parent().ptyName() + "(o):%0x");
}
    
void OutPty::Main()
{
    myPTRACE(1, "<-- Started");

    PBYTEArray *buf = NULL;
    PINDEX done = 0;

    for(;;) {
      PrepareSelect(n, fdset, tv);
      
      while( buf == NULL ) {
        if( stop ) break;
        WaitDataReady();
        if( stop ) break;
        buf = Parent().FromOutPtyQ();
        done = 0;
      }
      
      if( stop ) break;
      ::select(n, NULL, &fdset, NULL, &tv);

      if(FD_ISSET(hPty, &fdset)) {
        int len;
        
        {
          PWaitAndSignal mutexWait(Parent().ptyMutex);	// ???
          if( stop ) break;
          len = ::write(hPty, (const BYTE *)*buf + done, buf->GetSize() - done);
          
          if( len < 0 ) {
            int err = errno;
            myPTRACE(1, "<-- write ERROR " << len << " " << strerror(err));
            SignalStop();
            break;
            /*
            while( buf != NULL ) {
              if( buf->GetSize() != done ) {
                PTRACE(1, "<-- Not sent " << PRTHEX(PBYTEArray((const BYTE *)*buf + done, buf->GetSize() - done)));
              }
              delete buf;
              buf = Parent().FromOutPtyQ();
              done = 0;
            }
            continue;
            */
          }
        }

        done += len;
        if( buf->GetSize() <= done ) {
          if( buf->GetSize() < done ) {
            myPTRACE(1, "<-- " << buf->GetSize() << "(size) < (done)" << done << " " << len);
          }
          delete buf;
          buf = NULL;
        }
      }
    }
    
    if( buf ) {
      if( buf->GetSize() != done ) {
        PTRACE(1, "<-- Not sent " << PRTHEX(PBYTEArray((const BYTE *)*buf + done, buf->GetSize() - done)));
      }
      delete buf;
    }

    myPTRACE(1, "<-- Stoped");
}
///////////////////////////////////////////////////////////////

