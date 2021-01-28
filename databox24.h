#pragma pack(1) 

#define	HEAD		0x43	// Don't know why this, when tracing their app, they always send the first by hex 43

#define CMD_HELLO	0xfc
#define CMD_DEVICE	0xc0
#define CMD_OOR		0xc1
#define CMD_UNK		0xc2
#define CMD_POWER	0xc3


#define	VALUE_STATUS 	0x0000
#define VALUE_ON		0x0001
#define VALUE_OFF		0x0002
#define VALUE_REBOOT	0x0003


struct CU_Head {
	unsigned char	head;
	unsigned char	cmd;
	short	databoxid;
	short	areaid;
	int		inverterid;
	unsigned char	tw[3];			// Guess this is the total Watts ever created in the unit it  increments
	unsigned char	value;
};

struct CU_SendMsg {
	struct CU_Head	head;
	unsigned char	check;
};


struct CU_RecvHello {
	struct CU_Head	head;
	unsigned char	check;
};
struct CU_RecvInverterStatus {
	struct CU_Head	head;
	unsigned char	check;
	unsigned short	DCVoltage;
	unsigned short	DCCurrent;
	unsigned short	ACVoltage;
	unsigned short	ACCurrent;
	unsigned char 	unknown;
	unsigned short	Temp;
	unsigned char	status;
};


#define	MS20	21		// The number of characters for 200 ms

#define	RCV_HELLO_LEN sizeof( struct CU_RecvHello ) 
#define	RCV_STATUS_LEN sizeof( struct CU_RecvInverterStatus ) 
#define	SND_MSG_LEN sizeof( struct CU_SendMsg ) 
