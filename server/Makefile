CFLAGS          = -I../include -Wall -g
all:	nsbserver nsbpoolmngr

nsbserver: server.o ../common/net.o smisc.o game.o ../common/init.o records.o ai.o human.o formula.o createteam.o
	$(CC) $(CFLAGS) -o nsbserver server.o ../common/net.o smisc.o game.o ../common/init.o records.o ai.o human.o formula.o createteam.o

nsbpoolmngr: nsbpoolmngr.o ../common/nsbpoolnet.o
	$(CC) $(CFLAGS) -o nsbpoolmngr nsbpoolmngr.o ../common/nsbpoolnet.o -lpthread

clean::
	rm -f nsbserver nsbpoolmngr *.o ../common/nsbpoolnet.o

install::
	mkdir -p /var/NSB
	mkdir -p /usr/local/bin
	mkdir -p /usr/local/man/man6
	cp -f nsbserver /usr/local/bin
	cp -f nsbpoolmngr /usr/local/bin
	cp -f ../docs/nsbserver.6 /usr/local/man/man6
	cp -f ../docs/nsbpoolmngr.6 /usr/local/man/man6

