/*
 * $Id: ReadMe.txt,v 1.2 2002-01-01 23:11:49 craigs Exp $
 *
 * T38FAX Pseudo Modem
 *
 * Original author: Vyacheslav Frolov
 *
 * $Log: ReadMe.txt,v $
 * Revision 1.2  2002-01-01 23:11:49  craigs
 * New version from Vyacheslav Frolov
 * Removed references to unneeded OpenH323 patches
 * Removed reference to -k and -m options in usage
 * Change to use -route option
 *
 * Revision 1.2  2002/01/01 23:11:49  craigs
 * New version from Vyacheslav Frolov
 * Removed references to unneeded OpenH323 patches
 * Removed reference to -k and -m options in usage
 * Change to use -route option
 *
 */

1. Building
-----------

1.1. Compiling
-------------

$ make both

2. Examples
-----------

2.1. Starting
-------------

$ ./obj_linux_x86_d/pmodem -n -o trace.log -f -p ttyx0,ttyx1 --route 0@127.0.0.1 --route all@172.16.33.21

Creates two modems /dev/ttyx0 and /dev/ttyx1
If dialed number begins with '0' then it will be routed to local host ('0' will be discarded).
If not then it will be routed to 172.16.33.21.

2.2. Testing (you need two consoles)
------------------------------------

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
                            --> NDID=12345
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


2.3. Example of Cisco config (loopback)
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

2.4. Example of HylaFAX modem config filas

Copy HylaFAX/etc/config.ttyx to HylaFAX's etc directory

Create simbolic links:

config.ttyx0 -> config.ttyx
config.ttyx1 -> config.ttyx

Start HylaFAX with new modems:

$ .../faxgetty -D ttyx0
$ .../faxgetty -D ttyx1

