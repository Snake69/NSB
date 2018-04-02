/*
   some initialization is done here
*/
#include "db.h"
#include "sproto.h"
#include "cglobal.h"

/* populate the teaminfo structure */
void
populate () {
    int x;

    for (x = 0; x <= NUMBER_OF_TEAMS; x++) {
        teaminfo[x].id = x;                   /* a team id of 0 is for user-created teams */
        switch (x) {
            case 0:
                strcpy (&teaminfo[x].filename[0], "UserCreated");
                strcpy (&teaminfo[x].teamabbrev[0], "U-C");
                strcpy (&teaminfo[x].teamname[0], "User-Created");
                teaminfo[x].yrspan[0] = teaminfo[x].yrspan[1] = teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 1:
                strcpy (&teaminfo[x].filename[0], "AnaheimAngels");
                strcpy (&teaminfo[x].teamabbrev[0], "Ana");
                strcpy (&teaminfo[x].teamname[0], "Anaheim Angels");
                teaminfo[x].yrspan[0] = 1997;
                teaminfo[x].yrspan[1] = 2004;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 2:
                strcpy (&teaminfo[x].filename[0], "BaltimoreOrioles");
                strcpy (&teaminfo[x].teamabbrev[0], "Bal");
                strcpy (&teaminfo[x].teamname[0], "Baltimore Orioles");
                teaminfo[x].yrspan[0] = 1901;
                teaminfo[x].yrspan[1] = 1902;
                teaminfo[x].yrspan[2] = 1954;
                teaminfo[x].yrspan[3] = MAX_YEAR;
                break;
            case 3:
                strcpy (&teaminfo[x].filename[0], "BostonRedSox");
                strcpy (&teaminfo[x].teamabbrev[0], "Bos");
                strcpy (&teaminfo[x].teamname[0], "Boston Red Sox");
                teaminfo[x].yrspan[0] = 1908;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 4:
                strcpy (&teaminfo[x].filename[0], "ChicagoWhiteSox");
                strcpy (&teaminfo[x].teamabbrev[0], "ChiSox");
                strcpy (&teaminfo[x].teamname[0], "Chicago White Sox");
                teaminfo[x].yrspan[0] = 1901;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 5:
                strcpy (&teaminfo[x].filename[0], "ClevelandIndians");
                strcpy (&teaminfo[x].teamabbrev[0], "Cle");
                strcpy (&teaminfo[x].teamname[0], "Cleveland Indians");
                teaminfo[x].yrspan[0] = 1915;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 6:
                strcpy (&teaminfo[x].filename[0], "DetroitTigers");
                strcpy (&teaminfo[x].teamabbrev[0], "Det");
                strcpy (&teaminfo[x].teamname[0], "Detroit Tigers");
                teaminfo[x].yrspan[0] = 1901;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 7:
                strcpy (&teaminfo[x].filename[0], "KansasCityRoyals");
                strcpy (&teaminfo[x].teamabbrev[0], "KC");
                strcpy (&teaminfo[x].teamname[0], "Kansas City Royals");
                teaminfo[x].yrspan[0] = 1969;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 8:
                strcpy (&teaminfo[x].filename[0], "MinnesotaTwins");
                strcpy (&teaminfo[x].teamabbrev[0], "Min");
                strcpy (&teaminfo[x].teamname[0], "Minnesota Twins");
                teaminfo[x].yrspan[0] = 1961;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 9:
                strcpy (&teaminfo[x].filename[0], "NewYorkYankees");
                strcpy (&teaminfo[x].teamabbrev[0], "NYY");
                strcpy (&teaminfo[x].teamname[0], "New York Yankees");
                teaminfo[x].yrspan[0] = 1913;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 10:
                strcpy (&teaminfo[x].filename[0], "OaklandAthletics");
                strcpy (&teaminfo[x].teamabbrev[0], "Oak");
                strcpy (&teaminfo[x].teamname[0], "Oakland Athletics");
                teaminfo[x].yrspan[0] = 1968;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 11:
                strcpy (&teaminfo[x].filename[0], "SeattleMariners");
                strcpy (&teaminfo[x].teamabbrev[0], "Sea");
                strcpy (&teaminfo[x].teamname[0], "Seattle Mariners");
                teaminfo[x].yrspan[0] = 1977;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 12:
                strcpy (&teaminfo[x].filename[0], "TampaBayDevilRays");
                strcpy (&teaminfo[x].teamabbrev[0], "Tam");
                strcpy (&teaminfo[x].teamname[0], "Tampa Bay Devil Rays");
                teaminfo[x].yrspan[0] = 1998;
                teaminfo[x].yrspan[1] = 2007;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 13:
                strcpy (&teaminfo[x].filename[0], "TexasRangers");
                strcpy (&teaminfo[x].teamabbrev[0], "Tex");
                strcpy (&teaminfo[x].teamname[0], "Texas Rangers");
                teaminfo[x].yrspan[0] = 1972;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 14:
                strcpy (&teaminfo[x].filename[0], "TorontoBlueJays");
                strcpy (&teaminfo[x].teamabbrev[0], "Tor");
                strcpy (&teaminfo[x].teamname[0], "Toronto Blue Jays");
                teaminfo[x].yrspan[0] = 1977;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 15:
                strcpy (&teaminfo[x].filename[0], "ArizonaDiamondbacks");
                strcpy (&teaminfo[x].teamabbrev[0], "Ari");
                strcpy (&teaminfo[x].teamname[0], "Arizona Diamondbacks");
                teaminfo[x].yrspan[0] = 1998;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 16:
                strcpy (&teaminfo[x].filename[0], "AtlantaBraves");
                strcpy (&teaminfo[x].teamabbrev[0], "Atl");
                strcpy (&teaminfo[x].teamname[0], "Atlanta Braves");
                teaminfo[x].yrspan[0] = 1966;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 17:
                strcpy (&teaminfo[x].filename[0], "ChicagoCubs");
                strcpy (&teaminfo[x].teamabbrev[0], "Cubs");
                strcpy (&teaminfo[x].teamname[0], "Chicago Cubs");
                teaminfo[x].yrspan[0] = 1902;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 18:
                strcpy (&teaminfo[x].filename[0], "CincinnatiReds");
                strcpy (&teaminfo[x].teamabbrev[0], "Cin");
                strcpy (&teaminfo[x].teamname[0], "Cincinnati Reds");
                teaminfo[x].yrspan[0] = 1901;
                teaminfo[x].yrspan[1] = 1953;
                teaminfo[x].yrspan[2] = 1960;
                teaminfo[x].yrspan[3] = MAX_YEAR;
                break;
            case 19:
                strcpy (&teaminfo[x].filename[0], "ColoradoRockies");
                strcpy (&teaminfo[x].teamabbrev[0], "Col");
                strcpy (&teaminfo[x].teamname[0], "Colorado Rockies");
                teaminfo[x].yrspan[0] = 1993;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 20:
                strcpy (&teaminfo[x].filename[0], "FloridaMarlins");
                strcpy (&teaminfo[x].teamabbrev[0], "Fla");
                strcpy (&teaminfo[x].teamname[0], "Florida Marlins");
                teaminfo[x].yrspan[0] = 1993;
                teaminfo[x].yrspan[1] = 2011;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 21:
                strcpy (&teaminfo[x].filename[0], "HoustonAstros");
                strcpy (&teaminfo[x].teamabbrev[0], "Hou");
                strcpy (&teaminfo[x].teamname[0], "Houston Astros");
                teaminfo[x].yrspan[0] = 1965;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 22:
                strcpy (&teaminfo[x].filename[0], "LosAngelesDodgers");
                strcpy (&teaminfo[x].teamabbrev[0], "LA");
                strcpy (&teaminfo[x].teamname[0], "Los Angeles Dodgers");
                teaminfo[x].yrspan[0] = 1958;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 23:
                strcpy (&teaminfo[x].filename[0], "MilwaukeeBrewers");
                strcpy (&teaminfo[x].teamabbrev[0], "Mil");
                strcpy (&teaminfo[x].teamname[0], "Milwaukee Brewers");
                teaminfo[x].yrspan[0] = teaminfo[x].yrspan[1] = 1901;
                teaminfo[x].yrspan[2] = 1970;
                teaminfo[x].yrspan[3] = MAX_YEAR;
                break;
            case 24:
                strcpy (&teaminfo[x].filename[0], "MontrealExpos");
                strcpy (&teaminfo[x].teamabbrev[0], "Mon");
                strcpy (&teaminfo[x].teamname[0], "Montreal Expos");
                teaminfo[x].yrspan[0] = 1969;
                teaminfo[x].yrspan[1] = 2004;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 25:
                strcpy (&teaminfo[x].filename[0], "NewYorkMets");
                strcpy (&teaminfo[x].teamabbrev[0], "NYM");
                strcpy (&teaminfo[x].teamname[0], "New York Mets");
                teaminfo[x].yrspan[0] = 1962;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 26:
                strcpy (&teaminfo[x].filename[0], "PhiladelphiaPhillies");
                strcpy (&teaminfo[x].teamabbrev[0], "Phi");
                strcpy (&teaminfo[x].teamname[0], "Philadelphia Phillies");
                teaminfo[x].yrspan[0] = 1901;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 27:
                strcpy (&teaminfo[x].filename[0], "PittsburghPirates");
                strcpy (&teaminfo[x].teamabbrev[0], "Pit");
                strcpy (&teaminfo[x].teamname[0], "Pittsburgh Pirates");
                teaminfo[x].yrspan[0] = 1901;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 28:
                strcpy (&teaminfo[x].filename[0], "SanDiegoPadres");
                strcpy (&teaminfo[x].teamabbrev[0], "SD");
                strcpy (&teaminfo[x].teamname[0], "San Diego Padres");
                teaminfo[x].yrspan[0] = 1969;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 29:
                strcpy (&teaminfo[x].filename[0], "SanFranciscoGiants");
                strcpy (&teaminfo[x].teamabbrev[0], "SF");
                strcpy (&teaminfo[x].teamname[0], "San Francisco Giants");
                teaminfo[x].yrspan[0] = 1958;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 30:
                strcpy (&teaminfo[x].filename[0], "StLouisCardinals");
                strcpy (&teaminfo[x].teamabbrev[0], "StL");
                strcpy (&teaminfo[x].teamname[0], "St Louis Cardinals");
                teaminfo[x].yrspan[0] = 1901;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 31:
                strcpy (&teaminfo[x].filename[0], "StLouisBrowns");
                strcpy (&teaminfo[x].teamabbrev[0], "StLB");
                strcpy (&teaminfo[x].teamname[0], "St Louis Browns");
                teaminfo[x].yrspan[0] = 1902;
                teaminfo[x].yrspan[1] = 1953;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 32:
                strcpy (&teaminfo[x].filename[0], "NewYorkHighlanders");
                strcpy (&teaminfo[x].teamabbrev[0], "NYH");
                strcpy (&teaminfo[x].teamname[0], "New York Highlanders");
                teaminfo[x].yrspan[0] = 1903;
                teaminfo[x].yrspan[1] = 1912;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 33:
                strcpy (&teaminfo[x].filename[0], "PhiladelphiaAthletics");
                strcpy (&teaminfo[x].teamabbrev[0], "PhiA");
                strcpy (&teaminfo[x].teamname[0], "Philadelphia Athletics");
                teaminfo[x].yrspan[0] = 1901;
                teaminfo[x].yrspan[1] = 1954;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 34:
                strcpy (&teaminfo[x].filename[0], "KansasCityAthletics");
                strcpy (&teaminfo[x].teamabbrev[0], "KCA");
                strcpy (&teaminfo[x].teamname[0], "Kansas City Athletics");
                teaminfo[x].yrspan[0] = 1955;
                teaminfo[x].yrspan[1] = 1967;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 35:
                strcpy (&teaminfo[x].filename[0], "WashingtonSenators");
                strcpy (&teaminfo[x].teamabbrev[0], "Sen");
                strcpy (&teaminfo[x].teamname[0], "Washington Senators");
                teaminfo[x].yrspan[0] = 1901;
                teaminfo[x].yrspan[1] = 1971;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 36:
                strcpy (&teaminfo[x].filename[0], "LosAngelesAngels");
                strcpy (&teaminfo[x].teamabbrev[0], "LAAng");
                strcpy (&teaminfo[x].teamname[0], "Los Angeles Angels");
                teaminfo[x].yrspan[0] = 1961;
                teaminfo[x].yrspan[1] = 1964;
                teaminfo[x].yrspan[2] = 2017;
                teaminfo[x].yrspan[3] = MAX_YEAR;
                break;
            case 37:
                strcpy (&teaminfo[x].filename[0], "CaliforniaAngels");
                strcpy (&teaminfo[x].teamabbrev[0], "Cal");
                strcpy (&teaminfo[x].teamname[0], "California Angels");
                teaminfo[x].yrspan[0] = 1965;
                teaminfo[x].yrspan[1] = 1996;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 38:
                strcpy (&teaminfo[x].filename[0], "SeattlePilots");
                strcpy (&teaminfo[x].teamabbrev[0], "SeaP");
                strcpy (&teaminfo[x].teamname[0], "Seattle Pilots");
                teaminfo[x].yrspan[0] = teaminfo[x].yrspan[1] = 1969;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 39:
                strcpy (&teaminfo[x].filename[0], "OaklandAs");
                strcpy (&teaminfo[x].teamabbrev[0], "OakA");
                strcpy (&teaminfo[x].teamname[0], "Oakland A's");
                teaminfo[x].yrspan[0] = teaminfo[x].yrspan[1] = teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = -1;
                break;
            case 40:
                strcpy (&teaminfo[x].filename[0], "BostonPilgrims");
                strcpy (&teaminfo[x].teamabbrev[0], "BosPil");
                strcpy (&teaminfo[x].teamname[0], "Boston Pilgrims");
                teaminfo[x].yrspan[0] = teaminfo[x].yrspan[1] = teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = -1;
                break;
            case 41:
                strcpy (&teaminfo[x].filename[0], "BostonPuritans");
                strcpy (&teaminfo[x].teamabbrev[0], "BosPur");
                strcpy (&teaminfo[x].teamname[0], "Boston Puritans");
                teaminfo[x].yrspan[0] = teaminfo[x].yrspan[1] = teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = -1;
                break;
            case 42:
                strcpy (&teaminfo[x].filename[0], "BostonPlymouthRocks");
                strcpy (&teaminfo[x].teamabbrev[0], "BosPly");
                strcpy (&teaminfo[x].teamname[0], "Boston Plymouth Rocks");
                teaminfo[x].yrspan[0] = teaminfo[x].yrspan[1] = teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = -1;
                break;
            case 43:
                strcpy (&teaminfo[x].filename[0], "BostonSomersets");
                strcpy (&teaminfo[x].teamabbrev[0], "BosSom");
                strcpy (&teaminfo[x].teamname[0], "Boston Somersets");
                teaminfo[x].yrspan[0] = teaminfo[x].yrspan[1] = teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = -1;
                break;
            case 44:
                strcpy (&teaminfo[x].filename[0], "ClevelandBronchos");
                strcpy (&teaminfo[x].teamabbrev[0], "CleBro");
                strcpy (&teaminfo[x].teamname[0], "Cleveland Bronchos");
                teaminfo[x].yrspan[0] = teaminfo[x].yrspan[1] = 1902;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 45:
                strcpy (&teaminfo[x].filename[0], "ClevelandBlues");
                strcpy (&teaminfo[x].teamabbrev[0], "CleBlu");
                strcpy (&teaminfo[x].teamname[0], "Cleveland Blues");
                teaminfo[x].yrspan[0] = teaminfo[x].yrspan[1] = 1901;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 46:
                strcpy (&teaminfo[x].filename[0], "ClevelandNaps");
                strcpy (&teaminfo[x].teamabbrev[0], "CleNap");
                strcpy (&teaminfo[x].teamname[0], "Cleveland Naps");
                teaminfo[x].yrspan[0] = 1903;
                teaminfo[x].yrspan[1] = 1914;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 47:
                strcpy (&teaminfo[x].filename[0], "ClevelandMollyMcGuires");
                strcpy (&teaminfo[x].teamabbrev[0], "CleMol");
                strcpy (&teaminfo[x].teamname[0], "Cleveland Molly McGuires");
                teaminfo[x].yrspan[0] = teaminfo[x].yrspan[1] = teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = -1;
                break;
            case 48:
                strcpy (&teaminfo[x].filename[0], "WashingtonNationals");
                strcpy (&teaminfo[x].teamabbrev[0], "Nat");
                strcpy (&teaminfo[x].teamname[0], "Washington Nationals");
                teaminfo[x].yrspan[0] = 2005;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 49:
                strcpy (&teaminfo[x].filename[0], "LosAngelesAngelsofAnaheim");
                strcpy (&teaminfo[x].teamabbrev[0], "LAA");
                strcpy (&teaminfo[x].teamname[0], "Los Angeles Angels of Anaheim");
                teaminfo[x].yrspan[0] = 2005;
                teaminfo[x].yrspan[1] = 2016;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 50:
                strcpy (&teaminfo[x].filename[0], "BostonBraves");
                strcpy (&teaminfo[x].teamabbrev[0], "BosBra");
                strcpy (&teaminfo[x].teamname[0], "Boston Braves");
                teaminfo[x].yrspan[0] = 1912;
                teaminfo[x].yrspan[1] = 1935;
                teaminfo[x].yrspan[2] = 1941;
                teaminfo[x].yrspan[3] = 1952;
                break;
            case 51:
                strcpy (&teaminfo[x].filename[0], "MilwaukeeBraves");
                strcpy (&teaminfo[x].teamabbrev[0], "MilBra");
                strcpy (&teaminfo[x].teamname[0], "Milwaukee Braves");
                teaminfo[x].yrspan[0] = 1953;
                teaminfo[x].yrspan[1] = 1965;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 52:
                strcpy (&teaminfo[x].filename[0], "BrooklynDodgers");
                strcpy (&teaminfo[x].teamabbrev[0], "Brk");
                strcpy (&teaminfo[x].teamname[0], "Brooklyn Dodgers");
                teaminfo[x].yrspan[0] = 1911;
                teaminfo[x].yrspan[1] = 1912;
                teaminfo[x].yrspan[2] = 1932;
                teaminfo[x].yrspan[3] = 1957;
                break;
            case 53:
                strcpy (&teaminfo[x].filename[0], "NewYorkGiants");
                strcpy (&teaminfo[x].teamabbrev[0], "NYG");
                strcpy (&teaminfo[x].teamname[0], "New York Giants");
                teaminfo[x].yrspan[0] = 1901;
                teaminfo[x].yrspan[1] = 1957;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 54:
                strcpy (&teaminfo[x].filename[0], "HoustonColt45s");
                strcpy (&teaminfo[x].teamabbrev[0], "HouCol");
                strcpy (&teaminfo[x].teamname[0], "Houston Colt 45's");
                teaminfo[x].yrspan[0] = 1962;
                teaminfo[x].yrspan[1] = 1964;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 55:
                strcpy (&teaminfo[x].filename[0], "BostonBeaneaters");
                strcpy (&teaminfo[x].teamabbrev[0], "BosBea");
                strcpy (&teaminfo[x].teamname[0], "Boston Beaneaters");
                teaminfo[x].yrspan[0] = 1901;
                teaminfo[x].yrspan[1] = 1906;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 56:
                strcpy (&teaminfo[x].filename[0], "BostonDoves");
                strcpy (&teaminfo[x].teamabbrev[0], "BosDov");
                strcpy (&teaminfo[x].teamname[0], "Boston Doves");
                teaminfo[x].yrspan[0] = 1907;
                teaminfo[x].yrspan[1] = 1910;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 57:
                strcpy (&teaminfo[x].filename[0], "BostonBees");
                strcpy (&teaminfo[x].teamabbrev[0], "BosBee");
                strcpy (&teaminfo[x].teamname[0], "Boston Bees");
                teaminfo[x].yrspan[0] = 1936;
                teaminfo[x].yrspan[1] = 1940;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 58:
                strcpy (&teaminfo[x].filename[0], "BrooklynSuperbas");
                strcpy (&teaminfo[x].teamabbrev[0], "BrkSup");
                strcpy (&teaminfo[x].teamname[0], "Brooklyn Superbas");
                teaminfo[x].yrspan[0] = 1901;
                teaminfo[x].yrspan[1] = 1910;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 1913;
                break;
            case 59:
                strcpy (&teaminfo[x].filename[0], "BrooklynRobins");
                strcpy (&teaminfo[x].teamabbrev[0], "BrkRob");
                strcpy (&teaminfo[x].teamname[0], "Brooklyn Robins");
                teaminfo[x].yrspan[0] = 1914;
                teaminfo[x].yrspan[1] = 1931;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 60:
                strcpy (&teaminfo[x].filename[0], "CincinnatiRedLegs");
                strcpy (&teaminfo[x].teamabbrev[0], "CinRL");
                strcpy (&teaminfo[x].teamname[0], "Cincinnati Red Legs");
                teaminfo[x].yrspan[0] = teaminfo[x].yrspan[1] = teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = -1;
                break;
            case 61:
                strcpy (&teaminfo[x].filename[0], "CincinnatiRedlegs");
                strcpy (&teaminfo[x].teamabbrev[0], "CinRl");
                strcpy (&teaminfo[x].teamname[0], "Cincinnati Redlegs");
                teaminfo[x].yrspan[0] = 1954;
                teaminfo[x].yrspan[1] = 1959;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 62:
                strcpy (&teaminfo[x].filename[0], "BostonAmericans");
                strcpy (&teaminfo[x].teamabbrev[0], "BosAm");
                strcpy (&teaminfo[x].teamname[0], "Boston Americans");
                teaminfo[x].yrspan[0] = 1901;
                teaminfo[x].yrspan[1] = 1907;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 63:
                strcpy (&teaminfo[x].filename[0], "ChicagoOrphans");
                strcpy (&teaminfo[x].teamabbrev[0], "ChiOrp");
                strcpy (&teaminfo[x].teamname[0], "Chicago Orphans");
                teaminfo[x].yrspan[0] = 1901;
                teaminfo[x].yrspan[1] = 1902;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 64:
                strcpy (&teaminfo[x].filename[0], "BostonRustlers");
                strcpy (&teaminfo[x].teamabbrev[0], "BosRu");
                strcpy (&teaminfo[x].teamname[0], "Boston Rustlers");
                teaminfo[x].yrspan[0] = teaminfo[x].yrspan[1] = 1911;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 65:
                strcpy (&teaminfo[x].filename[0], "PhiladelphiaBlueJays");
                strcpy (&teaminfo[x].teamabbrev[0], "PhiB");
                strcpy (&teaminfo[x].teamname[0], "Philadelphia Blue Jays");
                teaminfo[x].yrspan[0] = teaminfo[x].yrspan[1] = teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = -1;
                break;
            case 66:
                strcpy (&teaminfo[x].filename[0], "TampaBayRays");
                strcpy (&teaminfo[x].teamabbrev[0], "TamR");
                strcpy (&teaminfo[x].teamname[0], "Tampa Bay Rays");
                teaminfo[x].yrspan[0] = 2008;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
                break;
            case 67:
                strcpy (&teaminfo[x].filename[0], "MiamiMarlins");
                strcpy (&teaminfo[x].teamabbrev[0], "Mia");
                strcpy (&teaminfo[x].teamname[0], "Miami Marlins");
                teaminfo[x].yrspan[0] = 2012;
                teaminfo[x].yrspan[1] = MAX_YEAR;
                teaminfo[x].yrspan[2] = teaminfo[x].yrspan[3] = 0;
        }
    }
}

