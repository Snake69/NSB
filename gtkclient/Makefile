
OBJS = gtknsbc.o about.o init.o misc.o creatensbid.o getservername.o help.o users.o stats.o setupleague.o gps.o showlineups.o \
       injuries.o play.o selectteams.o playgame.o select1team.o lineup.o gameactions.o createteam.o getteamfname.o nacc.o hof.o \
       awards.o preferences.o bestteams.o getyear.o setupleaguechoices.o series.o tdib.o bestseasons.o autoplay.o setupteamchoices.o \
       ../common/net.o ../common/init.o

CFLAGS = -g -O2 -I../include -Wall `pkg-config --cflags gtk+-2.0`

gtknsbclient: $(OBJS)
	$(CC) $(CFLAGS) -o gtknsbclient $(OBJS) `pkg-config --libs gtk+-2.0 gthread-2.0`

clean:
	rm $(OBJS) gtknsbclient 2>/dev/null || /bin/true 

install::
	mkdir -p /usr/local/bin
	mkdir -p /usr/local/man/man6
	mkdir -p /usr/local/share/NSB
	cp -f gtknsbclient /usr/local/bin
	cp -f ../docs/gtknsbclient.6 /usr/local/man/man6
	cp -f ../docs/gtk-user-manual /usr/local/share/NSB/user-manual
	cp -f ../docs/abbreviations /usr/local/share/NSB/abbreviations
	cp -f ../Misc/SomeGoodPlayerSeasons.txt /usr/local/share/NSB/SomeGoodPlayerSeasons.txt
	cp -f ../Misc/NotableAccomplishments.txt /usr/local/share/NSB/NotableAccomplishments.txt
	cp -f ../Misc/HOF.txt /usr/local/share/NSB/HOF.txt
	cp -f ../Misc/MVP.txt /usr/local/share/NSB/MVP.txt
	cp -f ../Misc/CyYoung.txt /usr/local/share/NSB/CyYoung.txt
	cp -f ../Misc/SilverSlugger.txt /usr/local/share/NSB/SilverSlugger.txt
	cp -f ../Misc/GoldGlove.txt /usr/local/share/NSB/GoldGlove.txt
	cp -f ../Misc/Rookie.txt /usr/local/share/NSB/Rookie.txt
	cp -f ../Misc/FormulaChars.txt /usr/local/share/NSB/FormulaChars.txt
	cp -f ../Misc/nsb-icon.jpg /usr/local/share/NSB
	cp -rf ../Misc/PlayerPics /usr/local/share/NSB
	cp -rf ../Misc/TDIB /usr/local/share/NSB
	cp -f ../Audio/* /usr/local/share/NSB

