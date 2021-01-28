DATABOX24 = databox24.o parse.o dbglog.c 
DATABOX24REBOOT = databox24reboot.o parse.o dbglog.c 
HFILES = databox24.h parse.h dbglog.h
ALLOBJ = (DATABOX24) (DATABOX24REBOOT)


ALLPGM =  databox24 databox24reboot


CC = gcc

IFLAGS = -I /usr/include/mysql -I /usr/include 
CFLAGS = -ggdb -c $(IFLAGS) 
CURLFLAGS = curl-config --cflags

LFLAGS = -L /usr/lib64/mysql -L /usr/lib64 -L/usr/local/ssl/lib
SQLLIBS = -lmysqlclient -lz 
CURLIBS = -lcurl -lnsl -lssl -lcrypto

all : $(ALLPGM)


databox24 : $(DATABOX24) 
	gcc $(LFLAGS)  -ggdb -o $@ $(DATABOX24) $(CURLIBS)

databox24reboot : $(DATABOX24REBOOT) 
	gcc $(LFLAGS)  -ggdb -o $@ $(DATABOX24REBOOT) $(CURLIBS)

%.o:%.c makefile $(HFILES)
	$(CC) $(CFLAGS) $< -ggdb -o $@

.PHONY : clean

clean:
	rm -f *.o $(ALLPGM)
