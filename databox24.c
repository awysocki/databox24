
#include <stdarg.h>
#include <stdlib.h>
#include <getopt.h>
#include "databox24.h"
#include "parse.h"
#include "dbglog.h"

#include <stdio.h>
#include <errno.h>
//#include <termios.h> // POSIX terminal control definitionss
#include <time.h>
#include <utime.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <dirent.h>
#include <string.h>
#include <sys/ioctl.h> //ioctl() call stuff
#include <termios.h>

#include <curl/curl.h>

#define 	BYTETIME		1042		// Microseconds time by byte there are 10 bits xmited per byte (10 bits / 9600 bps) 
#define		MAX_INVERTERS	10
#define		MAX_INVERTER_TRIES	3

enum {
	SUCCESSFUL = 0,
	ERR_BADFDOPEN = -1,
	ERR_BADFHOPEN= -2,
	ERR_BADPARM = -3,
	ERR_DBLOGON = -4,
};


/* GLOBAL Vars */

char	*gAgent = "DataBox24_Jan-28-2021/v0.02";		// This is used on the HTTP POST call specifing the agent that is calling

char	*gStartUp = "\n\ndatabox24 - AB Software LLC\nVersion 0.02 Jan 11, 2021\nRead COM port for Data Box micro inverter converter\r\n\r\n";

char    *gApp = "databox24";
char    *gINIFile="databox24.ini";


char	gDBGVerbose = 1;  // Default to VERBOSE during debug
char    gDebugLog[255];
char    gDebug;
char	gDbgBuf[2048];
char	gHexBuf[2048];
char	gRunLogBuf[1024*5];

char	gReadBuf[1024];

char	gInvertersStr[10*1024];
int		gPollTimeMin = 5;

int		gInverters[MAX_INVERTERS];			// Local list of inverters
int		gICnt = 0;

char	gUSBDev[128];

int		gFD;


char	gCmdBuffer[1024*5];
int		gCmdBufferLen;

char	gRecvBuffer[ 1024 ];

char	gPostVars[1024*4];
char	gCurlVars[1024*5];

char	gPostHost[1024*2];
int		gPostWrite=0;

char	gCSVFile[1024*2];
int		gCSVWrite=0;


short	gDataBoxID=0;


//----------------------------------------
//----------------------------------------
void LoadINIParms( );
void PollInverters ( void );
void ProcessStatus( char *_status, int _inverterid );
int	OpenPort( void );
int ConfigurePort( int );
int	FormatCmdBuffer( unsigned char *_cmd );
int SendCmd( unsigned char _cmd, int _databoxid, int _areaid, int _inverterid, unsigned char _val );
int RecvCmd( unsigned char *_buf, int _len, int _tries );
void SleepBytes( int _len );
unsigned char CheckSum( unsigned char *_sumstr, int _len );
unsigned short Swap2Endian(unsigned short _val);
unsigned int Swap3Endian(unsigned char *_val);
unsigned int Swap4Endian(unsigned int _val);
void HexString( char *_buffer, int _len, char *_rstr, int _split );
int PostData(char *_vars );
void WriteCSV( char *_Str );

//----------------------------------------
//----------------------------------------
int
main (argc, argv, envp)
int argc;
char *argv[];
char *envp;
{
	int		c;
	int		rc;
	char	buf[1024];
	int		wl;
	struct	CU_RecvHello 	*RH;


	while( ( c = getopt( argc, argv, "v?h" ) ) != -1 )
	{
		switch( c )
		{

		case 'v':
			gDBGVerbose = 1;
			break;

		case '?':
		case 'h':
			printf( gStartUp );
			printf( "\r\n-v - VERBOSE prints everything that would go to DEBUG LOG if debug was turned on\r\n" );
			exit(SUCCESSFUL);
			break;

		default:
			printf("? Unrecognizable switch [%s] - program aborted\n", optarg );
			exit( ERR_BADPARM );
		}
	}

	LoadINIParms();               // Get my custom parms

	InitDBGLog( "DataBox24", gDebugLog, gDebug, gDBGVerbose);

	WriteDBGLog( gStartUp );

	for( c=0; c < gICnt; c++ )
	{
		sprintf( gDbgBuf, "Inverter ID=%04X", gInverters[c] );
		WriteDBGLog( gDbgBuf );
		
	}

	WriteDBGLog( "Calling OpenPort" );

	rc = OpenPort();
	
	if ( !rc )
	{

		WriteDBGLog( "Sending down the first command" );
		
		wl = SendCmd( CMD_HELLO, 0x00, 0x00, 0x00, 0x00 );
		sprintf( gDbgBuf, "Wrote INITALIZE Hello Command=%d", wl );
		WriteDBGLog( gDbgBuf );
		
		SleepBytes( RCV_HELLO_LEN + SND_MSG_LEN ); 	// Sleep to send & receive the data for the far end to reply
		
		wl = RecvCmd( gRecvBuffer, RCV_HELLO_LEN, 2 );
		sprintf( gDbgBuf, "Read Hello Structure=%d", wl );
		WriteDBGLog( gDbgBuf );
		
		if ( wl == RCV_HELLO_LEN )
		{
			RH = (struct CU_RecvHello *)gRecvBuffer;
			
			gDataBoxID = RH->head.databoxid;
			
			sprintf( gDbgBuf, "Rcv databox ID=%04x", Swap2Endian(gDataBoxID) );
			WriteDBGLog( gDbgBuf );

			memset( gPostVars, 0, sizeof( gPostVars ) );
			memset( buf, 0, sizeof( buf ) );


			gHexBuf[0] = 0;
			HexString ( (char *)gRecvBuffer, wl, gHexBuf , 0 );

	
			/* POSTVAR */
			sprintf( gPostVars, "Cmd=%02X&DataBoxID=%04X&AreaID=%04X&InverterID=%08X&TotalWatts=%u&Value=%02X&CheckDigit=%02X&RawData=%s", 
				RH->head.cmd, 
				Swap2Endian(RH->head.databoxid), 
				Swap2Endian(RH->head.areaid), 
				Swap4Endian(RH->head.inverterid),
				Swap3Endian(RH->head.tw),
				RH->head.value, 
				RH->check,
				gHexBuf
				);


	
			sprintf( gDbgBuf, "DataBox: %04X | Inverter# %04X | TotalWatts=%u, value=%x", 
				Swap2Endian(RH->head.databoxid), 
				Swap4Endian(RH->head.inverterid), 
				Swap3Endian(RH->head.tw), 
				RH->head.value );
				
			WriteDBGLog( gDbgBuf );
			
			if ( gPostWrite )
				PostData( gPostVars );

			PollInverters();
		}
	}


	return rc;

}


/*===========================================================================*/
/* POLL INVERTERS - Process the usb com port and HTTP data to server         */
/*===========================================================================*/
void
PollInverters(  )
{
	int forever = 1;

	int slen, rlen;
	
	int	idx, try, finished;
	
	time_t time_s, time_e; 
	struct tm *tm;
	double	timetill;
	int	i;
	
	
	WriteDBGLog( "Starting POLL" );

	while( forever )  		// Stay in loop forever or till something really bad happens and we can't continue
	{
		for (idx = 0; idx < gICnt; idx++ )
		{

			finished = 0;
			
			sprintf( gDbgBuf, "POLLING INVERTER =========> %04X", gInverters[idx] );
			WriteDBGLog( gDbgBuf );

			for (try = 0; try < MAX_INVERTER_TRIES && !finished; try++ )
			{
	
				// DataBoxID is NOT swapped when we recive it. We only swap it when printing
				slen = SendCmd( CMD_DEVICE, gDataBoxID, Swap4Endian(0x00), Swap4Endian(gInverters[idx]), VALUE_STATUS );
		
				sprintf( gDbgBuf, "Sent Status Command : %d", slen );
				WriteDBGLog( gDbgBuf );
			
				SleepBytes( RCV_STATUS_LEN + 1  );			// Sleep to  so we can send and receive the data for the far end to reply
			
				rlen = RecvCmd( gRecvBuffer, RCV_STATUS_LEN, MAX_INVERTER_TRIES);
				sprintf( gDbgBuf, "Read STATUS Structure Length=%d", rlen );
				WriteDBGLog( gDbgBuf );
				
				if ( rlen == RCV_STATUS_LEN )
				{
					ProcessStatus( gRecvBuffer, gInverters[idx] );
					finished = 1;
				}
				else if ( try >= MAX_INVERTER_TRIES )		// Tried the number of times and no response
				{
					sprintf( gDbgBuf, "FAILED ====>%04x tried %d times", gInverters[idx],  try );
					WriteDBGLog( gDbgBuf );
					
				}
			}
						
		}
		
		time(&time_s);
		tm = localtime(&time_s);
		tm->tm_sec = 0;
		time_e = mktime(tm);
		
		timetill=difftime( time_s, time_e );
		i = (int) timetill;
		
		sprintf( gDbgBuf, "DONE for now Sleeping %d seconds PollMin=%d, timeworked=%d", ((gPollTimeMin*60) - i), gPollTimeMin, i ); 
		WriteDBGLog( gDbgBuf );

		
		sleep((gPollTimeMin * 60)  - timetill );			// Sleep between polls
		
	}	
	
	return ;
	

}

/*===========================================================================*/
/* Process Satus - We received data from the micro inverter now parse it out */
/*	Log to CSV and or POST servers.  Or neither								 */
/*===========================================================================*/
void
ProcessStatus( char *_status, int _inverterid )
{
	
	struct CU_RecvInverterStatus *CRIS;

	CRIS = (struct CU_RecvInverterStatus *)_status;
	
	unsigned int	uf;
	unsigned int	dcv, dcva, dcvb;
	unsigned int	dcc, dcca, dccb;
	unsigned int	acv, acva, acvb;
	unsigned int	acc, acca, accb;
	unsigned int	t, ta, tb, fa, fb;
		
	char	locbuf[5*1024];
		
	memset( gPostVars, 0, sizeof( gPostVars ) );
	memset( locbuf, 0, sizeof( locbuf ) );
	
	/* -------------------------------------------------------------------- */
	/* POSTVAR - FORMAT all the variables we will send to the HTTP logger 	*/
	/*			The TS variable is added just before sending				*/
	/* -------------------------------------------------------------------- */
	sprintf( locbuf, "Cmd=%02X&DataBoxID=%04X&AreaID=%04X&InverterID=%08X&TotalWatts=%u&Value=%02X&CheckDigit=%02X&Unknown=%02X&Status=%02X", 
		CRIS->head.cmd, 
		Swap2Endian(CRIS->head.databoxid), 
		Swap2Endian(CRIS->head.areaid), 
		Swap4Endian(CRIS->head.inverterid),
		Swap3Endian(CRIS->head.tw),
		CRIS->head.value, 
		CRIS->check,
		CRIS->unknown,
		CRIS->status
		);
		
	strcat( gPostVars, locbuf);
	
	sprintf( gDbgBuf, "DataBox: %04X | Inverter# %08X | TotalWatts=%u, value=%x", 
		Swap2Endian(CRIS->head.databoxid), 
		Swap4Endian(CRIS->head.inverterid), 
		Swap3Endian(CRIS->head.tw), 
		CRIS->head.value );
		
	WriteDBGLog( gDbgBuf );
	
	/* DC Voltage */
	dcv = (unsigned int)Swap2Endian(CRIS->DCVoltage);
	
	sprintf( locbuf, "&DCVoltage=%u", dcv );
	strcat( gPostVars, locbuf);
	
	dcva = dcv/100;
	dcvb = dcv%100;
	sprintf( gDbgBuf, "DCVoltage: %u.%.02u - 0x%02X", dcva, dcvb, dcv );
	WriteDBGLog( gDbgBuf );
	
	
	/* DC Current */
	dcc = (unsigned int)Swap2Endian(CRIS->DCCurrent);

	sprintf( locbuf, "&DCCurrent=%u", dcc );
	strcat( gPostVars, locbuf);

	dcca = dcc/100;
	dccb = dcc%100;
	sprintf( gDbgBuf, "DCCurrent: %u.%.02u - 0x%02X", dcca, dccb, dcc );
	WriteDBGLog( gDbgBuf );

	/* AC Voltage */
	acv = (unsigned int) Swap2Endian(CRIS->ACVoltage);

	sprintf( locbuf, "&ACVoltage=%u", acv );
	strcat( gPostVars, locbuf);

	acva = acv/100;
	acvb = acv%100;
	sprintf( gDbgBuf, "ACVoltage: %u.%.02u - 0x%02X", acva, acvb, acv );
	WriteDBGLog( gDbgBuf );
	
	/* ACCURRENT */
	acc = (unsigned int) Swap2Endian(CRIS->ACCurrent);

	sprintf( locbuf, "&ACCurrent=%u", acc );
	strcat( gPostVars, locbuf);

	acca = acc/100;
	accb = acc%100;	
	sprintf( gDbgBuf, "ACCurrent: %u.%.02u - 0x%02X", acca, accb, acc );
	WriteDBGLog( gDbgBuf );

	/* TEMP */
	t = (unsigned int) Swap2Endian(CRIS->Temp);

	sprintf( locbuf, "&Temp=%u", t );
	strcat( gPostVars, locbuf);
	
	ta = t/10;
	tb = t%10;	
	
	uf = (t * 18) + 3200;	
	
	fa = uf/100;
	fb = uf%100;	
	sprintf( gDbgBuf, "Temp: %u.%.01u C | %u.%.02u F - 0x%02X", ta, tb, fa, fb, t );
	WriteDBGLog( gDbgBuf );
	
	sprintf( gDbgBuf, "Status: 0x%02X", CRIS->status );
	WriteDBGLog( gDbgBuf );

	sprintf( gRunLogBuf, "%02X, %04X, %04X, %08X, %u, %02X, %02X, %u, %u, %u, %u, %02X, %u, %02X, ", 
		CRIS->head.cmd, 
		Swap2Endian(CRIS->head.databoxid), Swap2Endian(CRIS->head.areaid), Swap4Endian(CRIS->head.inverterid),
		Swap3Endian(CRIS->head.tw), CRIS->head.value, CRIS->check, 
		dcv,
		dcc,
		acv,
		acc,
		CRIS->unknown,
		t ,
		CRIS->status );
		
	gHexBuf[0] = 0;
	HexString ( (char *)_status, sizeof( struct CU_RecvInverterStatus ), gHexBuf , 0 );
		
	strcat( gRunLogBuf, gHexBuf);
		
	sprintf( locbuf, "&RawData=%s", gHexBuf );
	strcat( gPostVars, locbuf);	
	
	

	
	/* ============================== */
	/* WRITE To CSV file if turned on */	
	/* ============================== */
	if ( gCSVWrite )
		WriteCSV( gRunLogBuf );
		
	/* ============================== */
	/* Write to Web Server if ON      */
	/* ============================== */
	if ( gPostWrite )
		PostData( gPostVars );
	
}

/* ======================================================================================== */
/* POST the data to the server of choice													*/
/* ======================================================================================== */
int
PostData(char *_vars )
{
	CURL *curl;
	CURLcode res;
	
	// Always put a TS time stamp on 
	sprintf( gCurlVars, "TS=%lu&%s", (unsigned long)time(NULL), _vars);
	
	sprintf( gDbgBuf, "CurlVars: %s", gCurlVars );
	WriteDBGLog( gDbgBuf );

	/* In windows, this will init the winsock stuff */ 
	curl_global_init(CURL_GLOBAL_ALL);
 
	/* get a curl handle */ 
	curl = curl_easy_init();
	if(curl) 
	{
		/* First set the URL that is about to receive our POST. This URL can
		   just as well be a https:// URL if that is what should receive the
		   data. */ 
		curl_easy_setopt(curl, CURLOPT_URL, gPostHost );
		/* Now specify the POST data */ 
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, gCurlVars );
		
		curl_easy_setopt(curl, CURLOPT_USERAGENT, gAgent );
 
		/* Perform the request, res will get the return code */ 
		res = curl_easy_perform(curl);
		/* Check for errors */ 
		if(res != CURLE_OK)
		{
			sprintf(gDbgBuf, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
			  
			WriteDBGLog( gDbgBuf );
		}
		/* always cleanup */ 
		curl_easy_cleanup(curl);
	}
	else
	{
		WriteDBGLog( "Failed creating curl_easy_init" );
	}
	curl_global_cleanup();
	
	return 0;
}

/*--------------------------------------------------------------------------*/
/* WRITE to CSV File the data we received									*/
/*--------------------------------------------------------------------------*/
void
WriteCSV( char *_Str )
{
    FILE  *FH;
    time_t  tt;
    struct  tm      *tm;
    
    FH = fopen(gCSVFile, "a+");
    if (FH == NULL)
    {
        return;
    } /* endif */
    
    tt = time(NULL);
    tm = localtime(&tt);
    fprintf(FH, "%04d/%02d/%02d,%02d:%02d:%02d, ", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    fputs( _Str, FH );
    
    if ( _Str[ strlen( _Str) - 1] != '\n' )
        fprintf( FH, "\n" );
    
    fclose(FH);

}




/*==============================================================================*/
/* Quick binary to print hex function											*/
/*==============================================================================*/
void
HexString( char *_buffer, int _len, char *_rstr, int _split )
{
	unsigned char	locstr[64];
	
	_rstr[0] = 0;		// Put a null at the start of the string they passed in
	
	for (int i = 0; i < _len; i++)
	{
		if ( _split != 0 && i % _split == 0  && i != 0)
			strcat( _rstr, " " ); 

		memset( locstr, 0 , sizeof( locstr) );
		sprintf(locstr, "%02X", (unsigned char)_buffer[i] );
		strcat( _rstr, locstr ); 
		
		
	}
	
}


/*===========================================================================*/
/* SENDCMD - Send a command to the DataBox device							 */
/*===========================================================================*/
int
SendCmd( unsigned char _cmd, int _databoxid, int _areaid, int _inverterid, unsigned char _val )
{
	int		len;
	int		rlen;
	struct	CU_SendMsg		CU_SendMsg;
	int		bytes;
	int		i;

	memset( (char *)&CU_SendMsg, 0, sizeof( struct CU_SendMsg ));

	CU_SendMsg.head.head = 0x43;			// Hard coded not sure if there are other values
	CU_SendMsg.head.cmd = _cmd;
	
	
	sprintf( gDbgBuf, "CMD = %x", (unsigned char) _cmd);
	WriteDBGLog( gDbgBuf );
	
	
	/* set up the SEND message that we will send down the serial device	*/
	if ( _cmd != CMD_HELLO )
	{
		CU_SendMsg.head.databoxid = _databoxid;
		CU_SendMsg.head.areaid = _areaid;
		CU_SendMsg.head.inverterid = _inverterid;
		CU_SendMsg.head.value = _val;
		// Do stuff
	}
	else
	{
		sprintf( gDbgBuf, "Size of CU_SendMsg = %d", sizeof( CU_SendMsg ) );
		WriteDBGLog( gDbgBuf );
	}
	
	/* Calculate a checksum	*/
	CU_SendMsg.check = CheckSum( (unsigned char *)&CU_SendMsg, sizeof( CU_SendMsg ) - 1 );
	
	
	gHexBuf[0] = 0;
	HexString ( (char *)&CU_SendMsg, sizeof( struct CU_SendMsg ), gHexBuf , 2);
	WriteDBGLog( gHexBuf );

	/* Write to the COM Port to send the message */
	len = write( gFD, (void *)&CU_SendMsg, sizeof( struct CU_SendMsg ) );

	if ( len != sizeof( struct CU_SendMsg ) )
	{
		sprintf( gDbgBuf, "Error writing Command to port, %d != %d", len, sizeof( struct CU_SendMsg ) );
		WriteDBGLog( gDbgBuf );
		return( -1 );
	}
	else
	{
		/* call simple function to sleep the number of microseconds that a 9600 baud will take to xmit the data down the line */
		SleepBytes( len );

		ioctl(gFD, FIONREAD, &bytes);		// Query number of characters waiting
		sprintf(gDbgBuf, "ioctl bytes in buffer  FIONREAD. %d", bytes );
		WriteDBGLog( gDbgBuf );
		
		if ( bytes == 1 )			/* This is a catch all, on startup there can sometimes be a single byte in the buffer */
		{
			WriteDBGLog( "Reading 1 byte in because its not suppose to be here" );

			rlen = read( gFD, gReadBuf, 1 );
			HexString ( gReadBuf, 1, gHexBuf, 2 );
			WriteDBGLog( gHexBuf );
			
		}
		
	}


	return( len );
}


/*===========================================================================*/
/* RECVCMD - RECEIVE data from the com port 								 */
/*===========================================================================*/
int
RecvCmd( unsigned char *_buf, int _buflen, int _tries )
{
	int		len;
	int		bytes;
	int		loop = 1;

	memset( (char *)_buf, 0, _buflen);

	sprintf( gDbgBuf, "RecvCmd  reading buffer in, %d", _buflen );
	WriteDBGLog( gDbgBuf );


	ioctl(gFD, FIONREAD, &bytes);		// Query number of characters waiting
	sprintf(gDbgBuf, "ioctl bytes in buffer  FIONREAD. %d", bytes );
	WriteDBGLog( gDbgBuf );		

	while ( bytes < _buflen && loop < _tries )		// wait till the buffer is full with what we want
	{	
		SleepBytes( BYTETIME );			// Sleep time to xmit 1 byte across serial link
		
		ioctl(gFD, FIONREAD, &bytes);	// Query number of characters waiting
		sprintf(gDbgBuf, "ioctl bytes in buffer  FIONREAD. %d", bytes );
		WriteDBGLog( gDbgBuf );		
		
		loop++;
	}
	
	len = read( gFD, (void *)_buf, _buflen );

	if ( len != _buflen )
	{
		sprintf( gDbgBuf, "Error reading buffer in, %d != %d", len, _buflen );
		WriteDBGLog( gDbgBuf );
		
		HexString ( (char *)_buf, len, gHexBuf, 2 );
		WriteDBGLog( gHexBuf );
		
		return( -1 );
	}
	
	if ( _buf[0] != 0x43 )
	{
		sprintf( gDbgBuf, "Error buffer first byte != 0x43, %02x", _buf[0] );
		WriteDBGLog( gDbgBuf );
		HexString ( (char *)_buf, len, gHexBuf, 2 );
		WriteDBGLog( gHexBuf );

		return( -1 );		
	}

	WriteDBGLog( "Full buffer -----------------------||    |    |    |    |    |" );
	gHexBuf[0] = 0;
	HexString ( (char *)_buf, len, gHexBuf,2  );
	WriteDBGLog( gHexBuf );



	return( len );
}



/*===========================================================================*/
/* SleepBytes - Sleep for the number of bytes we are sending or receiving    */
/*===========================================================================*/
void 
SleepBytes( int _len )
{
	unsigned int	st;
	
	st = _len * BYTETIME;		// BYTETIME is the number of ms to xmit 10 bits
	sprintf(gDbgBuf, "Sleeping %d micro seconds ", st );
	WriteDBGLog( gDbgBuf );
	
	usleep ( st  ); // sleep the number of microsec to xmit the number of bytes
}


/*===========================================================================*/
/* Calculate CheckSum
/*===========================================================================*/
unsigned char
CheckSum( unsigned char *_sumstr, int _len )
{
	int i;
	int	sumup = 0;
	unsigned char	cs;
	
	for (int i = 0; i < _len; i++)
		sumup += _sumstr[i];
	
	
	cs = (char)(sumup & 0xff);
	
return( cs );
}


/*===========================================================================*/
/* LOADINIPARMS - Load in the INI file                                       */
/*===========================================================================*/
void
LoadINIParms( )
{
	char locbuf[256];

	int		rc = 0;
	char	*token;
	char	*delim = ",";
	int		var;

	GetIniString( gApp, "DebugLog", "./raven2db.log", gDebugLog, sizeof( gDebugLog ), gINIFile );
	GetIniString( gApp,   "Debug", "No", locbuf, sizeof( locbuf ), gINIFile );
	gDebug = CheckYes( locbuf );

	GetIniString( gApp, "USBDev", "/dev/ttyUSB1", gUSBDev, sizeof( gUSBDev ), gINIFile );
	
	GetIniString( gApp, "Inverters", "", gInvertersStr, sizeof( gInvertersStr ), gINIFile );
	
	token = strtok (gInvertersStr, delim);
	while (token != NULL)
	{
		sprintf(gDbgBuf, "Adding ==> [%s] to inverter list", token );
		WriteDBGLog( gDbgBuf );
		
		
		sscanf (token, "%X", &var);
		gInverters[gICnt] = var; //Swap4Endian(var);		// Store it so we just use it


		sprintf(gDbgBuf, "Added ==> icnt=%d [%04x] to inverter list", gICnt, gInverters[gICnt] );
		WriteDBGLog( gDbgBuf );

		token = strtok ( NULL, delim );
		
		gICnt++;
		if (gICnt >= MAX_INVERTERS )			// Only poll 32 inverters
			break;
	}
	
	GetIniString( gApp, "PollTimeMin", "5", locbuf, sizeof( locbuf ), gINIFile );
	gPollTimeMin = atoi( locbuf );

	GetIniString( gApp, "PostHost", "https://anbbb.com/cu.php", gPostHost, sizeof( gPostHost ), gINIFile );
	GetIniString( gApp, "PostWrite", "Yes", locbuf, sizeof( locbuf ), gINIFile );
	gPostWrite = CheckYes( locbuf );


	GetIniString( gApp, "CSVFile", "databox24.csv", gCSVFile, sizeof( gCSVFile ), gINIFile );
	GetIniString( gApp, "CSVWrite", "Yes", locbuf, sizeof( locbuf ), gINIFile );
	gCSVWrite = CheckYes( locbuf );


}

/*===========================================================================*/
/* OPENPORT - Open the USB Port for reading and writing						 */
/*===========================================================================*/
int OpenPort(void)
{
	int fd; // file description for the serial port
	int	status=0;
	int	bytes=0;
	struct termios options;
	
	sprintf( gDbgBuf, "Opening port [%s]", gUSBDev );
	WriteDBGLog( gDbgBuf );

	gFD = open(gUSBDev, O_RDWR | O_NDELAY );
	
	if(gFD == -1) // if open is unsucessful
	{
		sprintf(gDbgBuf, "open_port: Unable to open %s. 0x%0x - %s", gUSBDev, errno, strerror( errno ) );
		WriteDBGLog( gDbgBuf );
		perror( gDbgBuf );
		return( ERR_BADFDOPEN );
	}
	else
	{
		
		tcgetattr(gFD, &options);


		/* The next bunches of lines are to set to 9600,8,N,1 baud rate of the com port	*/
		cfsetispeed(&options, B9600);
		cfsetospeed(&options, B9600);
		options.c_cflag |= (CLOCAL | CREAD);
		
		options.c_cflag &= ~CSIZE; 	
		options.c_cflag |= CS8;    /* Select 8 data bits */

		options.c_iflag &= ~IGNBRK;
		options.c_lflag = 0;
		options.c_oflag = 0;
		options.c_cc[VMIN] = 1;
		options.c_cc[VTIME] = 5;
		options.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
		options.c_cflag &= ~(PARENB | PARODD);      // shut off parity
		
       	options.c_cflag |= 1;
       	options.c_cflag &= ~CSTOPB;
       	options.c_cflag &= ~CRTSCTS;

		tcsetattr(gFD, TCSANOW, &options);
		
		ioctl(gFD, FIONREAD, &bytes);
		sprintf(gDbgBuf, "readbytes. 0x%02x", bytes );
		WriteDBGLog( gDbgBuf );
		
		
		
		ioctl(gFD, TIOCMGET, &status);
		sprintf(gDbgBuf, "stats right after open. 0x%02x", status );
		WriteDBGLog( gDbgBuf );
	
		/* Set REQUEST TO SEND and DATA TERMINAL READY */
		status |= ( TIOCM_RTS | TIOCM_DTR );
		
		sprintf(gDbgBuf, "stats after bits fliped. 0x%02x", status );
		WriteDBGLog( gDbgBuf );
	
		ioctl(gFD, TIOCMSET, &status);//Set RTS pin 
		WriteDBGLog( "set bit" );
		
		ioctl(gFD, TIOCMGET, &status);
		sprintf(gDbgBuf, "stats after set. 0x%0x ", status );
		WriteDBGLog( gDbgBuf );
		
		fcntl(gFD, F_SETFL, FNDELAY);

		WriteDBGLog( "COM Port (ttyUSB#) is open" );

	}
	
	return(SUCCESSFUL);
} //open_port



/* The data that is returned in not reverse byte,  My INTEL NUC is so I have to swap bytes	*/

/* ==================================================== */
/* SWAP a 2 byte number									*/
/* ==================================================== */
unsigned short Swap2Endian(unsigned short _val)
{
    unsigned int se = (_val<<8) | (_val>>8);
	
	return( se );
}

/* ==================================================== */
/* SWAP a 3 byte number									*/
/* ==================================================== */
unsigned int Swap3Endian(unsigned char *_val)
{
	unsigned int rc=0;
	unsigned char *rcp;
	
	rcp = (unsigned char *)&rc;
	
	rcp[0] = _val[2];
	rcp[1] = _val[1];
	rcp[2] = _val[0];
	
	return (rc);

}


/* ==================================================== */
/* SWAP a 3 byte number									*/
/* ==================================================== */
unsigned int Swap4Endian(unsigned int _val)
{
	return (((_val>>24) & 0x000000ff) | ((_val>>8) & 0x0000ff00) | ((_val<<8) & 0x00ff0000) | ((_val<<24) & 0xff000000));
}

