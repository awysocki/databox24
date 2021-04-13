#pragma pack(1) 

#define	HEAD		0x43	// Don't know why this, when tracing their app, they always send the first by hex 43

#define CMD_HELLO	0xfc
#define CMD_DEVICE	0xc0
#define CMD_OOR		0xc1
#define CMD_UNK		0xc2
#define CMD_POWER	0xc3


#define	VALUE_STATUS 	0x00
#define VALUE_ON		0x01
#define VALUE_OFF		0x02
#define VALUE_REBOOT	0x03


struct CU_SendHead {
	unsigned char	head;			// 1 byte header code usually a 0x43
	unsigned char	cmd;			// 1 byte command function code
	short	databoxid;				// 2 byte databox ID
	short	areaid;					// 2 byte areaid this is just echoed back what we send in
	int		inverterid;				// 4 byte Inverter ID
	int		value;					// 4 byte parameter passed in
};

struct CU_RecvHead {
	unsigned char	head;			// 1 byte header code usually a 0x43
	unsigned char	cmd;			// 1 byte command function code
	short	databoxid;				// 2 byte databox ID
	short	areaid;					// 2 byte areaid this is just echoed back what we send in
	int		inverterid;				// 4 byte Inverter ID
	float	totalwatts;				// 4 byte total cumlative inverter generation
};



struct CU_SendMsg {
	struct CU_SendHead	head;		// 14 byte header 
	unsigned char	check;			// 1 byte check digit
};


struct CU_RecvHello {
	struct CU_RecvHead	head;		// 14 byte header
	unsigned char	check;			// 1 byte check digit
};
struct CU_RecvInverterStatus {
	struct CU_RecvHead	head;		// 14 byte header
	unsigned char	check;			// 1 byte check digit
	unsigned short	DCVoltage;		// 2 byte DC Voltage (divide by 100 - 2 places past decimal point)
	unsigned short	DCCurrent;		// 2 byte DC Current (divide by 100 - 2 places past decimal point)
	unsigned short	ACVoltage;		// 2 byte AC Voltage (divide by 100 - 2 places past decimal point)
	unsigned short	ACCurrent;		// 2 byte AC Current (divide by 100 - 2 places past decimal point)
	unsigned short 	reserved;		// 2 Byte reserved by company, no definition given
	unsigned char 	xx;				// 1 byte not defined by company
	unsigned char	Temp;			// 1 byte temperature
};


#define	MS20	21		// The number of characters for 200 ms

#define	RCV_HELLO_LEN sizeof( struct CU_RecvHello ) 
#define	RCV_STATUS_LEN sizeof( struct CU_RecvInverterStatus ) 
#define	SND_MSG_LEN sizeof( struct CU_SendMsg ) 
