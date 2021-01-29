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
  

    
