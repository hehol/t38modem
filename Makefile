# 
# Makefile
# 
# T38FAX Pseudo Modem
# 
# Open H323 Project
# 
# The contents of this file are subject to the Mozilla Public License
# Version 1.0 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
# 
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
# the License for the specific language governing rights and limitations
# under the License.
# 
# The Original Code is Open H323 Library.
# 
# The Initial Developer of the Original Code is Vyacheslav Frolov
# 
# Contributor(s): Equivalence Pty ltd
#
# $Log: Makefile,v $
# Revision 1.10  2004-03-09 17:22:58  vfrolov
# Added PROCESS_PER_THREAD ifdef
#
# Revision 1.10  2004/03/09 17:22:58  vfrolov
# Added PROCESS_PER_THREAD ifdef
#
# Revision 1.9  2003/12/04 15:56:45  vfrolov
# Added hdlc.cxx t30.cxx fcs.cxx
#
# Revision 1.8  2002/08/05 10:10:29  robertj
# Normalised Makefile usage of openh323u.mak include file, fixing odd messages.
#
# Revision 1.7  2002/04/30 11:05:17  vfrolov
# Implemented T.30 Calling Tone (CNG) generation
#
# Revision 1.6  2002/04/30 03:52:28  craigs
# Added option for G.723.1 codec
#
# Revision 1.5  2002/04/27 10:17:20  vfrolov
# Added checking if COUT_TRACE or MYPTRACE_LEVEL defined
# Do not add -DCOUT_TRACE by default
#
# Revision 1.4  2002/02/11 08:35:08  vfrolov
# myPTRACE() outputs trace to cout only if defined COUT_TRACE
#
# Revision 1.3  2002/01/10 06:10:02  craigs
# Added MPL header
#
# 

PROG		= t38modem
SOURCES		:= pmutils.cxx dle.cxx pmodem.cxx pty.cxx pmodemi.cxx \
		   g7231_fake.cxx t30tone.cxx hdlc.cxx t30.cxx fcs.cxx \
		   pmodeme.cxx t38engine.cxx main.cxx

ifndef OPENH323DIR
OPENH323DIR=$(HOME)/openh323
endif

include $(OPENH323DIR)/openh323u.mak

#
# If defined COUT_TRACE then enable duplicate the
# output of myPTRACE() to cout
#
ifdef COUT_TRACE
STDCCFLAGS += -DCOUT_TRACE
endif

#
# If defined MYPTRACE_LEVEL=N then myPTRACE() will
# output the trace with level N
#
ifdef MYPTRACE_LEVEL
STDCCFLAGS += -DMYPTRACE_LEVEL=$(MYPTRACE_LEVEL)
endif

#
# If defined PROCESS_PER_THREAD then
#  - PID will be used in thread name rather then TID
#  - CPU usage will be traced
#
ifdef PROCESS_PER_THREAD
STDCCFLAGS += -DPROCESS_PER_THREAD
endif

