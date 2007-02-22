/*
 * $Id: ReadMe.txt,v 1.13 2007-02-22 16:00:33 vfrolov Exp $
 *
 * T38FAX Pseudo Modem
 *
 * Original author: Vyacheslav Frolov
 *
 */

1. Introduction
---------------

What is t38modem?

From your fax application view point it's a fax modem pool.
From IP network view point it's a H.323 endpoint with T.38 fax support.
From your view point it's a gateway between a fax application and IP network.

2. Building
-----------

2.1. Compiling for Unix
-----------------------

$ make opt

2.2. Compiling for Windows
--------------------------

Start MSVC (v5 or v6) with t38modem.dsp file.
Set Active Configuration to "t38modem - Win32 Release".
Build t38modem.exe.

3. Examples
-----------

3.1. Starting
-------------

$ ./obj_linux_x86_r/t38modem -n -o trace.log -p ttyx0,ttyx1 --route 0@127.0.0.1 --route all@172.16.33.21

Creates two modems /dev/ttyx0 and /dev/ttyx1

Q. I try to use T38modem, but after run "t38modem -p ttyx0" I get a message
   "Could not open /dev/ptyx0: No such file or directory".
A. Looks like you don't have legacy PTY devices compiled in your kernel.
   You need to re-compile the kernel with 'Legacy PTY Support'.

FreeBSD Users: You need to use  -p ttypa,ttypb instead of -p ttyx0,ttyx1.
               Remember to replace ttyx0 with ttypa and ttyx1 with ttypb
               when following the rest of these instructions.
               This will create two modems /dev/ttypa and /dev/ttypb.

Windows Users: You need two COM ports connected via Null-modem cable to create one modem.
               If your COM1 connected to COM2 and COM3 connected to COM4 you need to use
               -p \\.\COM2,\\.\COM4 instead of  -p ttyx0,ttyx1.
               This will create two modems COM1 and COM3.
               Q. How to use t38modem without additional COM port hardware on Windows?
               A. Replace a pair of physical COM ports with a pair of virtual COM ports.
                  See http://com0com.sourceforge.net/ project for details.
               Q. What model of modem to select in Add Hardware Wizard?
               A. Select "Standard 1440 bps Modem".

Cisco Users:   You additionaly need to use --old-asn and --h245tunneldisable options.

If dialed number begins with '0' then it will be routed to local host ('0' will be discarded).
If not then it will be routed to 172.16.33.21.

3.2. Testing (you need two consoles)
------------------------------------
(FreeBSD users - remeber to use /dev/ttypa and /dev/ttypb with 'cu -l')
(Windows users - use COM1 and COM3 with HyperTerminal)

$ cu -l /dev/ttyx0	    $ cu -l /dev/ttyx1
Connected.		    Connected.
<-- at		            <-- at
--> OK                      --> OK
                            (wait at least 10 secs)
<-- atdt012345

                            --> 
                            --> RING
                            --> 
                            --> RING
                            <-- ati9
                            --> NDID = 12345
                            --> OK
                            --> 
                            --> RING
                            --> 
                            --> RING
                            <-- ata
--> CONNECT                 --> CONNECT
<-- x
--> OK
<-- ath
--> OK
                            --> 
                            --> ERROR
<-- at
--> OK
                            <-- at
                            --> OK
...                         ...


3.3. Example of Cisco config (loopback)
---------------------------------------

10.0.2.12 --> Cisco port 2:D --E1-cable--> Cisco port 3:D --> 10.0.2.12

dial-peer voice 3340 voip
 incoming called-number 3334....
 codec g711alaw
 fax rate 14400
 fax protocol t38 ls-redundancy 0 hs-redundancy 0

dial-peer voice 3341 pots
 destination-pattern 3334....
 port 2:D
 forward-digits 7

dial-peer voice 3342 pots
 incoming called-number 334....
 direct-inward-dial
 port 3:D
exit

dial-peer voice 3343 voip
 destination-pattern 334....
 session target ipv4:10.0.2.12
 codec g711alaw
 fax rate 14400
 fax protocol t38 ls-redundancy 0 hs-redundancy 0
exit

3.4. Example of HylaFAX modem config files
------------------------------------------

Copy HylaFAX/etc/config.ttyx to HylaFAX's etc directory

Create simbolic links:

config.ttyx0 -> config.ttyx
config.ttyx1 -> config.ttyx

Start HylaFAX with new modems:

$ .../faxgetty -D ttyx0
$ .../faxgetty -D ttyx1

(FreeBSD users - don't forget we are using ttypa and ttypb)

4. AT commands specific to t38modem
-----------------------------------

4.1. AT#CID command
-------------------

4.1.1 calling/called number reporting
-------------------------------------

#CID=0	- disables calling/called number reporting (default).
#CID=10	- Enables calling/called number reporting after the first RING.

Example:

--> RING
--> NMBR = <calling number>
--> NDID = <called number>
--> RING
--> RING

4.2. ATI command
----------------

4.2.1 calling/called number reporting
-------------------------------------

I8	- reports calling number for last incoming call.
I9	- reports called number for last incoming call.

Example:

<-- ATI8I9
--> NMBR = <calling number>
--> NDID = <called number>
--> OK

4.3. ATD command
----------------

4.3.1 T.38 mode modifiers
-------------------------

F - enable T.38 mode request after dialing.
V - disable T.38 mode request after dialing (default).

Examples:

<-- ATDF<user's number>

    enables T.38 mode request after dialing but user can override it by
    inserting V into <user's number>.

<-- ATD<user's number>V

    disables T.38 mode request after dialing and user can't override it
    by inserting F into <user's number>.

4.3.2 calling/called number modifiers
-------------------------------------

L - reset and begin of calling number.
D - continue of called number.

If calling number is empty after processing ATD command then t38modem's
local party number used.

Examples:

<-- ATD<user's number>

    calling number is t38modem's local party number but user can override
    it by inserting L<user's calling number> into <user's number>

<-- ATD<user's number>L

    calling number is t38modem's local party number and user can't override
    it by inserting L<user's calling number> into <user's number>

<-- ATDL<calling number>D<user's number>

    calling number is <calling number> but user can override
    it by inserting L<user's calling number> into <user's number>

<-- ATD<user's number>L<calling number>

    calling number is <calling number> and user can't override
    it by inserting L<user's calling number> into <user's number>

4.4. AT#DFRMC command
---------------------

4.4.1 Set delay for CONNECT result code for AT+FRM command
----------------------------------------------------------

#DFRMC=0	- disable delay (default).
#DFRMC=25	- set delay to 250 ms.

4.5. AT#HCLR command
--------------------

4.5.1 Set clear call mode for ATH command
-----------------------------------------

#HCLR=0	- ATH command will not clear not answered incoming call (default).
#HCLR=1	- ATH command will clear not answered incoming call.


                         -----------------------------

/*
 * $Log: ReadMe.txt,v $
 * Revision 1.13  2007-02-22 16:00:33  vfrolov
 * Implemented AT#HCLR command
 *
 * Revision 1.13  2007/02/22 16:00:33  vfrolov
 * Implemented AT#HCLR command
 *
 * Revision 1.12  2007/02/21 08:21:47  vfrolov
 * Added question about 'Legacy PTY Support'
 *
 * Revision 1.11  2005/03/04 16:41:01  vfrolov
 * Implemented AT#DFRMC command
 *
 * Revision 1.10  2005/02/10 15:07:15  vfrolov
 * Added more comments for Windows users
 *
 * Revision 1.9  2005/02/07 10:07:38  vfrolov
 * Fixed com0com link
 * Moved Log to the bottom
 *
 * Revision 1.8  2005/02/03 13:20:01  vfrolov
 * Added comments for Windows users
 *
 * Revision 1.7  2002/12/19 10:41:03  vfrolov
 * Added "Introduction" and "AT commands" sections and made some fixes
 *
 * Revision 1.6  2002/11/18 22:57:53  craigs
 * Added patches from Vyacheslav Frolov for CORRIGENDUM
 *
 * Revision 1.5  2002/03/22 09:40:57  vfrolov
 * Removed obsoleted option -f
 *
 * Revision 1.4  2002/01/09 16:14:58  rogerh
 * FreeBSD uses /dev/ttypa and /dev/ttypb
 *
 * Revision 1.3  2002/01/09 16:01:03  rogerh
 * Executable is called t38modem
 *
 * Revision 1.2  2002/01/01 23:11:49  craigs
 * New version from Vyacheslav Frolov
 * Removed references to unneeded OpenH323 patches
 * Removed reference to -k and -m options in usage
 * Change to use -route option
 *
 */

