DATABOX24 - Polling SG700MD
=========

databox24 is a simple 'c' application to read a communication port that is attached to
the newenergytek.com DATA BOX that is connected via USB port to your linux machine
it will communicate to the SG Series of micro inverters. 
200,250,300,350,400,450,500,600,700,1000,1200 and 1400

I have only tested on model SG700MD - http://www.newenergytek.com/index.php/content-48

  - requires [databox24g] datacollector http://www.newenergytek.com/index.php/content-57
  - application run as root

This is a very simple server gateway app to poll and send the data to a web server to be processed. 
 


Device
--
[DATA BOX 24G] - New Engery Tek (no version number or model on the unit

{note:] I did receive a response from the company, and they gave me a quick layout of the data.  I have updated
my code to reflect their definition of the data.

CMD - Get Status (15 bytes total)
	0x43		- Header (1 byte)
	0xC0		- CMD - 0xC0=Device status (1 byte)	
	0x1122 	- Databox ID (2 bytes)
	0x0000		- Area ID (2 bytes) What is passed in is returned
	0x11223344	- Inverter ID (4 bytes)
	0x112233	- Total Power Generations (3 Bytes) Set to 0x000000 on command
	0x00		- Value Parameter (1 byte) 0x00=Status, 0x01=Turn ON, 0x02=Turn OFF, 0x03=Reboot
	0xCD		- Check Digit

RCV - Receive Status (27 bytes total (15 CMD header + 12 bytes Status))
	0x43		- Header (1 byte)
	0xC0		- CMD - 0xC0=Device status (1 byte)
	0x1122 	- Databox ID (2 bytes)
	0x0000		- Area ID (2 bytes) Whatever is passed in is returned back
	0x11223344	- Inverter ID (4 bytes)
	0x112233	- Total Power Generations (3 Bytes) Set to 0x000000 on command
	0x00		- Value Parameter (1 byte) 0x00=Status, 0x01=Turn ON, 0x02=Turn OFF, 0x03=Reboot
	0xCD		- Check Digit (1 byte)
	0x1122		- DC Voltage divide by 100 for 2 places after decimal point (2 bytes)
	0x1122		- DC Current divide by 100 for 2 places after decimal point (2 bytes)
	0x1122		- AC Voltage divide by 100 for 2 places after decimal point (2 bytes)
	0x1122		- AC Current divide by 100 for 2 places after decimal point (2 bytes)
	0x0000		- Reserved (2 bytes)
	0x00		- xx UNKNOWN (1 byte)
	0x11		- Temperature C° (1 byte)



Computer
--
I tested and run this application on an Intel NUC machine.

STEP #1 - Check and find the port that your linux machine assigned when plugging in the DATABOX wireless modem
-
run ->  lsusb
Look for bus and device

Bus 001 Device 012: ID 067b:2303 Prolific Technology, Inc. PL2303 Serial Port


also run -> sudo dmesg

To list the port number it assigns

 stty -F /dev/ttyUSB0 9600 cs8 -cstopb -parenb   

 
STEP #2 - Compile application 
-
    
    
Pull the files to a directory and run make to compile the application.  

    make

Also supported is a clean make command

	make clean


STEP #4 - Install/RUN
-

There is no install script, so I just run it where its compiled.
You will want to edit and modify the ** databox24.ini ** file to match your setup

	[DataBox24]
	  Debug=Yes
	  DebugLog=./databox24.log
	  USBDev=/dev/ttyUSB0
	  Inverters=41000001,41000002,41000003
	  PollTimeMin=1

	  PostWrite=Yes
	  PostHost=https://yourwebserver/databox24.php
	  
	  CSVWrite=No
	  CSVFile=./databox24.csv



License
-

Open Software License v. 3.0 ([OSL-3.0])


  [OSL-3.0]:http://opensource.org/licenses/OSL-3.0
  

    
