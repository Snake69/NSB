/*
    process input from user to auto-create a new team
*/

#include <stdlib.h>
#include <dirent.h>
#include <syslog.h>
#include <string.h>
#include "sglobal.h"
#include "db.h"
#include "sproto.h"
#include "net.h"

#define STACKSIZE 20
#define CALC_FAIL -2
#define CALC_SUCCESS -1

int stack, tpit, tply, piter[14], multiplyr, closer1, closer1compare;
int tg, tab, tr, th, t2b, t3b, thr, trbi, tbb, tk, thbp, tgidp, tsb, tcs, tibb, tsh, tsf, tpo, ta, tpb, te;
int tpg, tgs, tip, tthirds, tw, tl, ts, tbfp, tph, tp2b, tp3b, tphr, tpr, ter, tprbi, tcg, tgf, tsho, tsvopp, tpsb, tpcs, tpbb, tpk, tpibb, tpsh, tpsf,
    twp, tb, thb, toppab, pscore[10];
struct {
    struct bttr batter;
    struct ptchr pitcher;
    float score;
} players[3][2001];

typedef enum CALC_SYMBOLS_TAG {
    OPEN,
    CLOSE,
    NUMBER,
    STAT,
    MULTIPLY,
    DIVIDE,
    ADD,
    SUBTRACT,
    INVALID
} CALC_SYMBOLS;

struct level {
    float var;
    CALC_SYMBOLS sym;
} top[STACKSIZE];

int
pushc (struct level *p1) {
    if (stack < STACKSIZE) {
        top[stack].var = p1->var;
        top[stack].sym = p1->sym;
        stack++;
        return 1;
    }
    return 0;
}

int
popc (struct level *p1) {
    if (stack > 0) {
        stack--;
        p1->var = top[stack].var;
        p1->sym = top[stack].sym;
        return 1;
    }
    return 0;
}

int
peekc (struct level *p1) {
    if (stack > 0) {
        p1->var = top[stack - 1].var;
        p1->sym = top[stack - 1].sym;
        return 1;
    }
    return 0;
}

int
calc_numberc (float num) {
    struct level temp;
    CALC_SYMBOLS precede;

    if (stack < 1) {
        temp.var = num;
        temp.sym = NUMBER;
        pushc (&temp);
        return CALC_SUCCESS;
    }

    peekc (&temp);
    precede = temp.sym;
    switch (precede) {
        case MULTIPLY:
        case DIVIDE:
        case ADD:
        case SUBTRACT:
            if (popc (&temp) != 1)
                return CALC_FAIL;
            if (popc (&temp) != 1)
                return CALC_FAIL;
            if (temp.sym != NUMBER && temp.sym != STAT)
                return CALC_FAIL;
            switch (precede) {
                case MULTIPLY:
                    temp.var = temp.var * num;
                    break;
                case DIVIDE:
                    if (num == 0)
                        /* substitute for 0 */
                        num = 0.00001;
                    temp.var = temp.var / num;
                    break;
                case ADD:
                    temp.var = temp.var + num;
                    break;
                case SUBTRACT:
                    temp.var = temp.var - num;
                    break;
                default:
                    return CALC_FAIL;
                    break;
            }
            temp.sym = NUMBER;
            if (pushc (&temp) != 1)
                return CALC_FAIL;
            break;
        case OPEN:
            temp.var = num;
            temp.sym = NUMBER;
            if (pushc (&temp) != 1)
                return CALC_FAIL;
            break;
        case CLOSE:
        case STAT:
        case NUMBER:
            return CALC_FAIL;
            break;
        default:
            return CALC_FAIL;
            break;
    }

    return CALC_SUCCESS;
}

int
check4statc (char *exp, int ind, int p, int iter) {
    int LAsw;
    float comp;
    struct level temp;
    CALC_SYMBOLS precede;

    if (!strncasecmp (&exp[ind], "LA", 2)) {
        LAsw = 1;
        ind += 2;
    }
    else
        LAsw = 0;

    if (!strncasecmp (&exp[ind], "SOPP", 4)) {
        if (!iter)               /* svopp is a pitching only stat */
            return -1;
        if (LAsw)
            comp = (float) tsvopp / (float) tpit;
        else
            comp = team.pitchers[p].pitching.svopp;
        ind += 3;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "OPAB", 4)) {
        if (!iter)               /* oppba is a pitching only stat */
            return -1;
        if (LAsw)
            comp = (float) toppab / (float) tpit;
        else
            comp = team.pitchers[p].pitching.opp_ab;
        ind += 3;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "OPBA", 4)) {
        if (!iter)               /* opba is a pitching only stat */
            return -1;
        if (LAsw)
            comp = (float) tph / (float) toppab;
        else
            comp = (float) team.pitchers[p].pitching.hits / (float) team.pitchers[p].pitching.opp_ab;
        ind += 3;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "GIDP", 4)) {
        if (iter)                /* gidp is an offense only stat */
            return -1;
        if (LAsw)
            comp = (float) tgidp / (float) tply;
        else
            comp = team.batters[p].hitting.gidp;
        ind += 3;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "RBI", 3)) {
        if (!iter)
            if (LAsw)
                comp = (float) trbi / (float) tply;
            else
                comp = team.batters[p].hitting.rbi;
        else
            if (LAsw)
                comp = (float) tprbi / (float) tpit;
            else
                comp = team.pitchers[p].pitching.rbi;
        ind += 2;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "IBB", 3)) {
        if (!iter)
            if (LAsw)
                comp = (float) tibb / (float) tply;
            else
                comp = team.batters[p].hitting.ibb;
        else
            if (LAsw)
                comp = (float) tpibb / (float) tpit;
            else
                comp = team.pitchers[p].pitching.ibb;
        ind += 2;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "HBP", 3)) {
        if (!iter)
            if (LAsw)
                comp = (float) thbp / (float) tply;
            else
                comp = team.batters[p].hitting.hbp;
        else
            if (LAsw)
                comp = (float) thb / (float) tpit;
            else
                comp = team.pitchers[p].pitching.hb;
        ind += 2;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "OBA", 3)) {
        if (iter)                /* oba is an offense only stat */
            return -1;
        if (LAsw)
            comp = (float) (th + tbb + thbp) / (float) (tab + tbb + tsh + tsf + thbp);
        else
            comp = (float) (team.batters[p].hitting.hits + team.batters[p].hitting.bb + team.batters[p].hitting.hbp) /
                   (float) (team.batters[p].hitting.atbats + team.batters[p].hitting.bb + team.batters[p].hitting.sh + team.batters[p].hitting.sf +
                                                                                                                        team.batters[p].hitting.hbp);
        ind += 2;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "BFP", 3)) {
        if (!iter)               /* bfp is a pitching only stat */
            return -1;
        if (LAsw)
            comp = (float) tbfp / (float) tpit;
        else
            comp = team.pitchers[p].pitching.bfp;
        ind += 2;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "ERA", 3)) {
        if (!iter)               /* era is a pitching only stat */
            return -1;
        if (LAsw)
            comp = (float) (ter * 9) / (float) (tip + ((float) tthirds / 3.0));
        else
            comp = (float) (team.pitchers[p].pitching.er * 9) /
                   (float) (team.pitchers[p].pitching.innings + ((float) team.pitchers[p].pitching.thirds / 3.0));
        ind += 2;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "PCT", 3)) {
        if (!iter)                /* pct is a pitching only stat */
            return -1;
        if (LAsw)
            comp = (float) tw / (float) (tw + tl);
        else
            comp = (float) team.pitchers[p].pitching.wins / (float) (team.pitchers[p].pitching.wins + team.pitchers[p].pitching.losses);
        ind += 2;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "SHO", 3)) {
        if (!iter)               /* sho is a pitching only stat */
            return -1;
        if (LAsw)
            comp = (float) tsho / (float) tpit;
        else
            comp = team.pitchers[p].pitching.sho;
        ind += 2;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "SG", 2)) {
        if (!iter)
            if (LAsw)
                comp = (float) (th - (t2b + t3b + thr)) / (float) tply;
            else
                comp = team.batters[p].hitting.hits - (team.batters[p].hitting.doubles + team.batters[p].hitting.triples + team.batters[p].hitting.homers);
        else
            if (LAsw)
                comp = (float) (tph - (tp2b + tp3b + tphr)) / (float) tpit;
            else
                comp = team.pitchers[p].pitching.hits - (team.pitchers[p].pitching.doubles + team.pitchers[p].pitching.triples +
                                                                                                team.pitchers[p].pitching.homers);
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "DB", 2)) {
        if (!iter)
            if (LAsw)
                comp = (float) t2b / (float) tply;
            else
                comp = team.batters[p].hitting.doubles;
        else
            if (LAsw)
                comp = (float) tp2b / (float) tpit;
            else
                comp = team.pitchers[p].pitching.doubles;
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "TP", 2)) {
        if (!iter)
            if (LAsw)
                comp = (float) t3b / (float) tply;
            else
                comp = team.batters[p].hitting.triples;
        else
            if (LAsw)
                comp = (float) tp3b / (float) tpit;
            else
                comp = team.pitchers[p].pitching.triples;
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "HR", 2)) {
        if (!iter)
            if (LAsw)
                comp = (float) thr / (float) tply;
            else
                comp = team.batters[p].hitting.homers;
        else
            if (LAsw)
                comp = (float) tphr / (float) tpit;
            else
                comp = team.pitchers[p].pitching.homers;
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "BB", 2)) {
        if (!iter)
            if (LAsw)
                comp = (float) tbb / (float) tply;
            else
                comp = team.batters[p].hitting.bb;
        else
            if (LAsw)
                comp = (float) tpbb / (float) tpit;
            else
                comp = team.pitchers[p].pitching.walks;
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "SB", 2)) {
        if (!iter)
            if (LAsw)
                comp = (float) tsb / (float) tply;
            else
                comp = team.batters[p].hitting.sb;
        else
            if (LAsw)
                comp = (float) tpsb / (float) tpit;
            else
                comp = team.pitchers[p].pitching.sb;
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "CS", 2)) {
        if (!iter)
            if (LAsw)
                comp = (float) tcs / (float) tply;
            else
                comp = team.batters[p].hitting.cs;
        else
            if (LAsw)
                comp = (float) tpcs / (float) tpit;
            else
                comp = team.pitchers[p].pitching.cs;
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "SF", 2)) {
        if (!iter)
            if (LAsw)
                comp = (float) tsf / (float) tply;
            else
                comp = team.batters[p].hitting.sf;
        else
            if (LAsw)
                comp = (float) tpsf / (float) tpit;
            else
                comp = team.pitchers[p].pitching.sf;
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "SH", 2)) {
        if (!iter)
            if (LAsw)
                comp = (float) tsh / (float) tply;
            else
                comp = team.batters[p].hitting.sh;
        else
            if (LAsw)
                comp = (float) tpsh / (float) tpit;
            else
                comp = team.pitchers[p].pitching.sh;
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "AB", 2)) {
        if (iter)                /* ab is an offense only stat */
            return -1;
        if (LAsw)
            comp = (float) tab / (float) tply;
        else
            comp = team.batters[p].hitting.atbats;
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "TB", 2)) {
        int singles;

        if (iter)                /* tb is an offense only stat */
            return -1;
        if (LAsw) {
            singles = th - (t2b + t3b + thr);
            comp = (float) (singles + (t2b * 2) + (t3b * 3) + (thr * 4)) / (float) tply;
        }
        else {
            singles = team.batters[p].hitting.hits - (team.batters[p].hitting.doubles + team.batters[p].hitting.triples + team.batters[p].hitting.homers);
            comp = singles + (team.batters[p].hitting.doubles * 2) + (team.batters[p].hitting.triples * 3) + (team.batters[p].hitting.homers * 4);
        }
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "BA", 2)) {
        if (iter)                /* ba is an offense only stat */
            return -1;
        if (LAsw)
            comp = (float) th / (float) tab;
        else
            comp = (float) team.batters[p].hitting.hits / (float) team.batters[p].hitting.atbats;
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "SA", 2)) {
        int singles, totb;

        if (iter)                /* sa is an offense only stat */
            return -1;
        if (LAsw) {
            singles = th - (t2b + t3b + thr);
            totb = (singles + (t2b * 2) + (t3b * 3) + (thr * 4));
            comp = (float) totb / (float) tab;
        }
        else {
            singles = team.batters[p].hitting.hits - (team.batters[p].hitting.doubles + team.batters[p].hitting.triples + team.batters[p].hitting.homers);
            totb = singles + (team.batters[p].hitting.doubles * 2) + (team.batters[p].hitting.triples * 3) + (team.batters[p].hitting.homers * 4);
            comp = (float) totb / (float) team.batters[p].hitting.atbats;
        }
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "PO", 2)) {
        int x;

        if (iter)                /* po is an offense only stat */
            return -1;
        if (LAsw)
            comp = (float) tpo / (float) tply;
        else
            for (comp = 0, x = 1; x < 11; x++)
                if (team.batters[p].fielding[x].games > 0) {
                    if (x >= 7 && x <= 9 && team.batters[p].fielding[10].games > 0)
                        /* if the OF fielding iteration has games then that iteration will contained all OF stats */
                        continue;
                    else
                        comp += team.batters[p].fielding[x].po;
                }
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "PB", 2)) {
        if (iter)                /* pb is an offense only stat */
            return -1;
        if (LAsw)
            comp = (float) tpb / (float) tply;
        else
            if (team.batters[p].fielding[2].games > 0)
                comp = team.batters[p].fielding[2].pb;
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "FA", 2)) {
        int x, chances, errors;

        if (iter)                /* fa is an offense only stat */
            return -1;
        if (LAsw) {
            chances = tpo + ta;
            comp = (float) chances / (float) (chances + te);
        }
        else
            for (chances = errors = 0, x = 1; x < 11; x++) {
                if (team.batters[p].fielding[x].games > 0) {
                    if (x >= 7 && x <= 9 && team.batters[p].fielding[10].games > 0)
                        /* if the OF fielding iteration has games then that iteration will contained all OF stats */
                        continue;
                    else {
                        chances += (team.batters[p].fielding[x].po + team.batters[p].fielding[x].a);
                        errors += team.batters[p].fielding[x].e;
                    }
                }
                if (!chances)
                    comp = 0.0;
                else
                    comp = (float) chances / (float) (chances + errors);
            }
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "CG", 2)) {
        if (!iter)               /* cg is a pitching only stat */
            return -1;
        if (LAsw)
            comp = (float) tcg / (float) tpit;
        else
            comp = team.pitchers[p].pitching.cg;
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "ER", 2)) {
        if (!iter)               /* er is a pitching only stat */
            return -1;
        if (LAsw)
            comp = (float) ter / (float) tpit;
        else
            comp = team.pitchers[p].pitching.er;
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "GF", 2)) {
        if (!iter)               /* gf is a pitching only stat */
            return -1;
        if (LAsw)
            comp = (float) tgf / (float) tpit;
        else
            comp = team.pitchers[p].pitching.gf;
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "GS", 2)) {
        if (!iter)               /* gs is a pitching only stat */
            return -1;
        if (LAsw)
            comp = (float) tgs / (float) tpit;
        else
            comp = team.pitchers[p].pitching.games_started;
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "IP", 2)) {
        if (!iter)               /* ip is a pitching only stat */
            return -1;
        if (LAsw)
            comp = (float) (tip + ((float) tthirds / 3.0)) / (float) tpit;
        else
            comp = team.pitchers[p].pitching.innings + ((float) team.pitchers[p].pitching.thirds / 3.0);
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "WP", 2)) {
        if (!iter)               /* wp is a pitching only stat */
            return -1;
        if (LAsw)
            comp = (float) twp / (float) tpit;
        else
            comp = team.pitchers[p].pitching.wp;
        ind++;
        goto AfterComp;
    }

    if (exp[ind + 1] == ' ' || exp[ind + 1] == '+' || exp[ind + 1] == '-' || exp[ind + 1] == '*' || exp[ind + 1] == '/' ||
                                               exp[ind + 1] == '(' || exp[ind + 1] == ')' || (ind + 1) >= strlen (&exp[0])) {
        if (exp[ind] == 'G' || exp[ind] == 'g') {
            if (!iter)
                if (LAsw)
                    comp = (float) tg / (float) tply;
                else
                    comp = team.batters[p].hitting.games;
            else
                if (LAsw)
                    comp = (float) tpg / (float) tpit;
                else
                    comp = team.pitchers[p].pitching.games;
        }

        if (exp[ind] == 'H' || exp[ind] == 'h') {
            if (!iter)
                if (LAsw)
                    comp = (float) th / (float) tply;
                else
                    comp = team.batters[p].hitting.hits;
            else
                if (LAsw)
                    comp = (float) tph / (float) tpit;
                else
                    comp = team.pitchers[p].pitching.hits;
        }

        if (exp[ind] == 'R' || exp[ind] == 'r') {
            if (!iter)
                if (LAsw)
                    comp = (float) tr / (float) tply;
                else
                    comp = team.batters[p].hitting.runs;
            else
                if (LAsw)
                    comp = (float) tpr / (float) tpit;
                else
                    comp = team.pitchers[p].pitching.runs;
        }

        if (exp[ind] == 'K' || exp[ind] == 'k') {
            if (!iter)
                if (LAsw)
                    comp = (float) tk / (float) tply;
                else
                    comp = team.batters[p].hitting.so;
            else
                if (LAsw)
                    comp = (float) tpk / (float) tpit;
                else
                    comp = team.pitchers[p].pitching.so;
        }

        if (exp[ind] == 'A' || exp[ind] == 'a') {
            int x;

            if (iter)                /* a is an offense only stat */
                return -1;
            if (LAsw)
                comp = (float) ta / (float) tply;
            else
                for (comp = 0, x = 1; x < 11; x++)
                    if (team.batters[p].fielding[x].games > 0) {
                        if (x >= 7 && x <= 9 && team.batters[p].fielding[10].games > 0)
                            /* if the OF fielding iteration has games then that iteration will contained all OF stats */
                            continue;
                        else
                            comp += team.batters[p].fielding[x].a;
                    }
        }

        if (exp[ind] == 'E' || exp[ind] == 'e') {
            int x;

            if (iter)                /* e is an offense only stat */
                return -1;
            if (LAsw)
                comp = (float) te / (float) tply;
            else
                for (comp = 0, x = 1; x < 11; x++)
                    if (team.batters[p].fielding[x].games > 0) {
                        if (x >= 7 && x <= 9 && team.batters[p].fielding[10].games > 0)
                            /* if the OF fielding iteration has games then that iteration will contained all OF stats */
                            continue;
                        else
                            comp += team.batters[p].fielding[x].e;
                    }
        }

        if (exp[ind] == 'B' || exp[ind] == 'b') {
            if (!iter)               /* b is a pitching only stat */
                return -1;
            if (LAsw)
                comp = (float) tb / (float) tpit;
            else
                comp = team.pitchers[p].pitching.balks;
        }

        if (exp[ind] == 'W' || exp[ind] == 'w') {
            if (!iter)               /* w is a pitching only stat */
                return -1;
            if (LAsw)
                comp = (float) tw / (float) tpit;
            else
                comp = team.pitchers[p].pitching.wins;
        }

        if (exp[ind] == 'L' || exp[ind] == 'l') {
            if (!iter)               /* l is a pitching only stat */
                return -1;
            if (LAsw)
                comp = (float) tl / (float) tpit;
            else
                comp = team.pitchers[p].pitching.losses;
        }

        if (exp[ind] == 'S' || exp[ind] == 's') {
            if (!iter)               /* s is a pitching only stat */
                return -1;
            if (LAsw)
                comp = (float) ts / (float) tpit;
            else
                comp = team.pitchers[p].pitching.saves;
        }
    }
AfterComp:
    if (stack < 1) {
        temp.var = comp;
        temp.sym = STAT;
        pushc (&temp);
        return ind;
    }

    peekc (&temp);
    precede = temp.sym;
    switch (precede) {
        case MULTIPLY:
        case DIVIDE:
        case ADD:
        case SUBTRACT:
            if (popc (&temp) != 1)
                return -1;
            if (popc (&temp) != 1)
                return -1;
            if (temp.sym != NUMBER && temp.sym != STAT)
                return -1;
            switch (precede) {
                case MULTIPLY:
                    temp.var = temp.var * comp;
                    break;
                case DIVIDE:
                    if (comp == 0)
                        /* substitute for 0 */
                        comp = 0.00001;
                    temp.var = temp.var / comp;
                    break;
                case ADD:
                    temp.var = temp.var + comp;
                    break;
                case SUBTRACT:
                    temp.var = temp.var - comp;
                    break;
                default:
                    return CALC_FAIL;
                    break;
            }
            temp.sym = STAT;
            if (pushc (&temp) != 1)
                return -1;
            break;
        case OPEN:
            temp.var = comp;
            temp.sym = STAT;
            if (pushc (&temp) != 1)
                return -1;
            break;
        case CLOSE:
        case STAT:
        case NUMBER:
            return -1;
            break;
        default:
            return -1;
            break;
    }

    return ind;
}

int
calc_binary_opc (CALC_SYMBOLS sym) {
    struct level temp;

    if (stack < 1)
        return CALC_FAIL;

    peekc (&temp);
    if (temp.sym != NUMBER && temp.sym != STAT)
        return CALC_FAIL;

    temp.sym = sym;
    if (pushc (&temp) != 1)
        return CALC_FAIL;

    return CALC_SUCCESS;
}

int
calc_parenc (CALC_SYMBOLS sym) {
    struct level temp;
    float result;

    switch (sym) {
        case OPEN:
            temp.sym = sym;
            if (pushc (&temp) != 1)
                return CALC_FAIL;
            break;
        case CLOSE:
            if (popc (&temp) != 1)
                return CALC_FAIL;
            if (temp.sym != NUMBER && temp.sym != STAT)
                return CALC_FAIL;
            result = temp.var;

            if (popc (&temp) != 1)
                return CALC_FAIL;
            if (temp.sym != OPEN)
                return CALC_FAIL;
            if (calc_numberc (result) != CALC_SUCCESS)
                return CALC_FAIL;
            break;
        default:
            return CALC_FAIL;
            break;
    }

    return CALC_SUCCESS;
}

void
CalculateCoverage () {
    int x, y;

    for (x = 0; x < 10; x++)
        pscore[x] = 0;

    for (x = 0; x < 14; x++)
        for (y = 0; y < 10; y++)
            if (players[0][piter[x]].batter.fielding[y].games)
                pscore[y]++;
}

int
CheckCoverage () {
    int x;

    /* we don't care about DH and pitcher */
    for (x = 2; x < 10; x++)
        if (pscore[x] < 2)
            return x;

    return -1;
}

int
FindOvercoverage () {
    int x, savex, savesc;

    savex = savesc = 0;

    for (x = 0; x < 10; x++)
        if (pscore[x] > 2 && pscore[x] > savesc) {
            savex = x;
            savesc = pscore[x];
        }

    if (!savex) {
        /* processing will very rarely (if ever) get here but if it does ... */
        for (x = 0; x < 10; x++)
            if (x == 1)
                continue;
            else
                if (pscore[x])
                    return x;
        /* should never get here */
        if (syslog_ent == YES)
            syslog (LOG_ERR, "There is something wrong in the CreateTeam process");
        return -1;
    }
    else
        return savex;
}

int
RemoveOvercover (int prem) {
    int x;

    for (x = 13; x > 0; x--)
        if (players[0][piter[x]].batter.fielding[prem].games)
            return x;

    return 13;
}

int
CheckPosition (int startp, int iter, int pos) {
    int x, y, matchSP;

    matchSP = 0;

    for (x = startp; x < 2000; x++)
        if (players[0][x].batter.fielding[pos].games) {
            if (multiplyr) {
                for (y = 0; y < 14; y++)
                    if (!strcmp (&players[0][x].batter.id.name[0], &players[0][piter[y]].batter.id.name[0]) &&
                                  players[0][x].batter.dob.month == players[0][piter[y]].batter.dob.month &&
                                  players[0][x].batter.dob.day == players[0][piter[y]].batter.dob.day &&
                                  players[0][x].batter.dob.year == players[0][piter[y]].batter.dob.year &&
                                  players[0][x].batter.id.year == players[0][piter[y]].batter.id.year)
                        matchSP = 1;
            }
            else
                for (y = 0; y < 14; y++)
                    if (!strcmp (&players[0][x].batter.id.name[0], &players[0][piter[y]].batter.id.name[0]) &&
                                  players[0][x].batter.dob.month == players[0][piter[y]].batter.dob.month &&
                                  players[0][x].batter.dob.day == players[0][piter[y]].batter.dob.day &&
                                  players[0][x].batter.dob.year == players[0][piter[y]].batter.dob.year)
                        matchSP = 1;
            if (matchSP)
                matchSP = 0;
            else {
                piter[iter] = x;
                return x;
            }
        }

    return -1;
}

int
CreateTeam () {
    /* user wants to auto-create a new team */
    int x, y, z, cindex[3], thold1, thold2, tindex, number, tgamespnt, loop, lp, pos, totgames, yrs[YEAR_SPREAD], yrsB[YEAR_SPREAD], yrsS[YEAR_SPREAD],
        yrsR[YEAR_SPREAD], yr, errformula[3], unavailstats, iter, nope;
    float minreq[3];
    char results[5000], work[500], dummy[256], dummy1[256], dummy2[256], tname[100], whichteam[256], svtname[256], iformula[3][4096], *pnt, dirt[256];
    DIR *fnames;
    struct dirent *dir;
    struct level temp;
    FILE *in, *out;

    struct {
        int teamid, games;
    } tgames[YEAR_SPREAD][50];

    for (x = 0; x < YEAR_SPREAD; x++)
        yrs[x] = yrsB[x] = yrsS[x] = yrsR[x] = 0;

    for (y = 0; y < 3; y++)
        for (x = 0; x < 2001; x++)
            players[y][x].score = -99999.0;

    /* get what-do-to-with-missing-stats indicator */
    unavailstats = buffer[1] - '0';
    /* get use-same-player-twice indicator */
    multiplyr = buffer[2] - '0';
    /* get more-than-1-closer indicator */
    closer1 = buffer[3] - '0';

    /* get team name */
    pnt = index (&buffer[4], ',');
    *pnt = '\0';
    strcpy (&svtname[0], &buffer[4]);
    pos = strlen (&buffer[0]) + 1;    /* first position of formula for Position Players */
    *pnt = ',';

    /* check if the team name the user wants to use is already being used */

    /* create the directory to store user-created teams if it doesn't already exist */
    strcpy (&dirt[0], "/var/NSB/");
    strcat (&dirt[0], &nsbdb[user].id[0]);
    strcat (&dirt[0], "/UserTeams");
    if ((fnames = opendir (&dirt[0])) == NULL) {
        if (mkdir (&dirt[0], 0700)) {
            if (syslog_ent == YES)
                syslog (LOG_INFO, "couldn't create %s: %s", dirt, strerror (errno));
            sock_puts (sock, "ERR\n");
            return -1;
        }
    }
    else
        closedir (fnames);

    /* check if team name is already being used */
    nope = 0;
    if ((fnames = opendir (&dirt[0])) != NULL) {
        while ((dir = readdir (fnames)))
            if (!strcmp (dir->d_name, &svtname[0])) {
                sock_puts (sock, "DUP\n");
                sock_gets (sock, &buffer1[0], sizeof (buffer1));
                if (!strcmp (&buffer1[0], "OK"))
                    break;
                else {
                    nope = 1;
                    break;
                }
            }
        closedir (fnames);
        if (nope)
            return -1;
    }
    else {
        sock_puts (sock, "ERR\n");
        return -1;
    }

    /* save the complete file pathname for saving later */
    strcat (&dirt[0], "/");
    strcat (&dirt[0], &svtname[0]);
    /* set a couple of values for this user-created team */
    dteam.id = dteam.year = 0;
    dteam.league = dteam.division = ' ';
    for (x = 0; x < 25; x++)
        strcpy (&dteam.batters[x].id.name[0], " ");
    for (x = 0; x < 11; x++)
        strcpy (&dteam.pitchers[x].id.name[0], " ");

    /* get formulas */
    pnt = index (&buffer[pos], ',');
    *pnt = '\0';
    strcpy (&iformula[0][0], &buffer[pos]);
    pos = strlen (&buffer[0]) + 1;    /* first position of formula for Starting Pitchers */
    *pnt = ',';
    pnt = index (&buffer[pos], ',');
    *pnt = '\0';
    strcpy (&iformula[1][0], &buffer[pos]);
    pos = strlen (&buffer[0]) + 1;    /* first position of formula for Relief Pitchers */
    *pnt = ',';
    pnt = index (&buffer[pos], ',');
    *pnt = '\0';
    strcpy (&iformula[2][0], &buffer[pos]);
    pos = strlen (&buffer[0]) + 1;    /* first position of minimum for Position Players */
    *pnt = ',';

    /* get minimum requirements */
    pnt = index (&buffer[pos], ' ');
    *pnt = '\0';
    minreq[0] = atof (&buffer[pos]);
    pos = strlen (&buffer[0]) + 1;    /* first position of minimum for Starting Pitchers */
    *pnt = ' ';
    pnt = index (&buffer[pos], ' ');
    *pnt = '\0';
    minreq[1] = atof (&buffer[pos]);
    pos = strlen (&buffer[0]) + 1;    /* first position of minimum for Relief Pitchers */
    *pnt = ' ';
    pnt = index (&buffer[pos], ' ');
    *pnt = '\0';
    minreq[2] = atof (&buffer[pos]);
    pos = strlen (&buffer[0]) + 1;    /* first position of which-teams-to-use indicator */
    *pnt = ' ';

    /* get which-teams-to-use indicator */
    pnt = index (&buffer[pos], ' ');
    *pnt = '\0';
    strcpy (&whichteam[0], &buffer[pos]);
    pos = strlen (&buffer[0]) + 1;    /* first position of years */
    *pnt = ' ';

    /* get years to include in search */
    for (x = 0; x < YEAR_SPREAD; x++, pos++)
        if (buffer[pos] == '1')
            yrs[x] = yrsB[x] = yrsS[x] = yrsR[x] = 1;

    if (!unavailstats) {
        /* omit years with unavailable stats */
        char *pnt;

        /* IBB is unavailable for both offense and pitching 1901 through 1954 */
        if (strcasestr (&iformula[0][0], "IBB"))
            for (x = 0; x < 54; x++)
                yrsB[x] = 0;
        if (strcasestr (&iformula[1][0], "IBB"))
            for (x = 0; x < 54; x++)
                yrsS[x] = 0;
        if (strcasestr (&iformula[2][0], "IBB"))
            for (x = 0; x < 54; x++)
                yrsR[x] = 0;

        if (strcasestr (&iformula[0][0], "GIDP"))
            /* GIDP is unavailable for offense 1901 through 1938 */
            for (x = 0; x < 38; x++)
                yrsB[x] = 0;
        if (strcasestr (&iformula[0][0], "SF"))
            /* SF is unavailable for offense 1901 through 1953 */
            for (x = 0; x < 53; x++)
                yrsB[x] = 0;
        if (strcasestr (&iformula[0][0], "K"))
            /* K is unavailable for offense 1901 through 1912 */
            for (x = 0; x < 12; x++)
                yrsB[x] = 0;
        if (strcasestr (&iformula[0][0], "CS")) {
            /* CS is unavailable for offense 1901 through 1919 and 1926 through 1950 */
            for (x = 0; x < 19; x++)
                yrsB[x] = 0;
            for (x = 26; x < 50; x++)
                yrsB[x] = 0;
        }

        /* SOPP, DB, TP, RBI, SB, CS, SF, & OPBA are unavailable for pitching for all years except 1998 */
        if (strcasestr (&iformula[1][0], "SOPP") || strcasestr (&iformula[1][0], "DB") || strcasestr (&iformula[1][0], "TP") ||
            strcasestr (&iformula[1][0], "RBI") || strcasestr (&iformula[1][0], "SB") || strcasestr (&iformula[1][0], "CS") ||
            strcasestr (&iformula[1][0], "SF") || strcasestr (&iformula[1][0], "OPBA")) {
            for (x = 0; x < 97; x++)
                yrsS[x] = 0;
            for (x = 98; x < YEAR_SPREAD; x++)
                yrsS[x] = 0;
        }
        if (strcasestr (&iformula[2][0], "SOPP") || strcasestr (&iformula[2][0], "DB") || strcasestr (&iformula[2][0], "TP") ||
            strcasestr (&iformula[2][0], "RBI") || strcasestr (&iformula[2][0], "SB") || strcasestr (&iformula[2][0], "CS") ||
            strcasestr (&iformula[2][0], "SF") || strcasestr (&iformula[2][0], "OPBA")) {
            for (x = 0; x < 97; x++)
                yrsR[x] = 0;
            for (x = 98; x < YEAR_SPREAD; x++)
                yrsR[x] = 0;
        }
        /* make sure SH isn't really SHO */
        pnt = strcasestr (&iformula[1][0], (char *) "SH");
        if (pnt && *(pnt + 2) != 'O' && *(pnt + 2) !='o') {
            for (x = 0; x < 97; x++)
                yrsS[x] = 0;
            for (x = 98; x < YEAR_SPREAD; x++)
                yrsS[x] = 0;
        }
        pnt = strcasestr (&iformula[2][0], (char *) "SH");
        if (pnt && *(pnt + 2) != 'O' && *(pnt + 2) !='o') {
            for (x = 0; x < 97; x++)
                yrsR[x] = 0;
            for (x = 98; x < YEAR_SPREAD; x++)
                yrsR[x] = 0;
        }
    }

    errformula[0] = errformula[1] = errformula[2] = 0;

    /* get data to use */
    strcpy (&dummy[0], "/var/NSB/RealLifeStats/");
    for (yr = 0; yr < YEAR_SPREAD; yr++) {
        if (yrs[yr]) {
            strcpy (&dummy1[0], &dummy[0]);
            strcat (&dummy1[0], (char *) cnvt_int2str (4, (1901 + yr)));

            for (x = 0; x < 50; x++) {
                tgames[yr][x].games = 0;
                tgames[yr][x].teamid = 999;
            }

            tg = tab = tr = th = t2b = t3b = thr = trbi = tbb = tk = thbp = tgidp = tsb = tcs = tibb = tsh = tsf = tpo = ta = tpb = te = 0;
            tpg = tgs = tip = tthirds = tw = tl = ts = tbfp = tph = tp2b = tp3b = tphr = tpr = ter = tprbi = tcg = tgf = tsho = tsvopp = tpsb = tpcs =
                tpbb = tpk = tpibb = tpsh = tpsf = twp = tb = thb = toppab = tply = tpit = tgamespnt = 0;

            /* read the Results file info for later processing */
            for (x = 0; x < 5000; x++)
                results[x] = '\0';

            strcpy (&dummy2[0], &dummy1[0]);
            strcat (&dummy2[0], "/Results");
            if ((in = fopen (dummy2, "r")) != NULL) {
                fread (&results, sizeof results, 1, in);
                fclose (in);
            }

            /* go through all teams first to cum all stats and to get the number of players and pitchers in the entire league
               in order to be ready to use league averages if the need arises */
            if ((fnames = opendir (&dummy1[0])) != NULL)
                while ((dir = readdir (fnames))) {
                    /* don't process . and .. files */
                    if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                        continue;
                    /* don't process the Results file again */
                    if (strstr (dir->d_name, "Results"))
                        continue;

                    strcpy (&dummy2[0], &dummy1[0]);
                    strcat (&dummy2[0], "/");
                    strcat (&dummy2[0], dir->d_name);

                    if ((in = fopen (dummy2, "r")) != NULL) {
                        fread (&team.id, sizeof team.id, 1, in);
                        fread (&team.year, sizeof team.year, 1, in);
                        fread (&team.league, sizeof team.league, 1, in);
                        fread (&team.division, sizeof team.division, 1, in);
                        for (x = 0; x < 25; x++) {
                            fread (&team.batters[x].id, sizeof team.batters[x].id, 1, in);
                            fread (&team.batters[x].dob, sizeof team.batters[x].dob, 1, in);
                            fread (&team.batters[x].hitting, sizeof team.batters[x].hitting, 1, in);
                            for (y = 0; y < 11; y++)
                                fread (&team.batters[x].fielding[y], sizeof team.batters[x].fielding[y], 1, in);
                        }
                        for (x = 0; x < 11; x++) {
                            fread (&team.pitchers[x].id, sizeof team.pitchers[x].id, 1, in);
                            fread (&team.pitchers[x].pitching, sizeof team.pitchers[x].pitching, 1, in);
                        }
                        fclose (in);
                    }
                    else {
                        if (syslog_ent == YES)
                            syslog (LOG_ERR, "There is something wrong with %s: %s", dummy2, strerror (errno));
                        return -1;
                    }
                    /* determine the number of players and pitchers this team has */
                    for (maxplayers[0] = 0; maxplayers[0] < 25; maxplayers[0]++)
                        if (team.batters[maxplayers[0]].id.name[0] == ' ' || !strlen (&team.batters[maxplayers[0]].id.name[0]))
                            break;
                    for (maxpitchers[0] = 0; maxpitchers[0] < 11; maxpitchers[0]++)
                        if (team.pitchers[maxpitchers[0]].id.name[0] == ' ' || !strlen (&team.pitchers[maxpitchers[0]].id.name[0]))
                            break;
                    /* cum total players and pitchers in league */
                    tply += maxplayers[0];
                    tpit += maxpitchers[0];

                    for (lp = 0; lp < maxplayers[0]; lp++) {
                        tg += team.batters[lp].hitting.games;
                        tab += team.batters[lp].hitting.atbats;
                        tr += team.batters[lp].hitting.runs;
                        th += team.batters[lp].hitting.hits;
                        t2b += team.batters[lp].hitting.doubles;
                        t3b += team.batters[lp].hitting.triples;
                        thr += team.batters[lp].hitting.homers;
                        trbi += team.batters[lp].hitting.rbi;
                        tbb += team.batters[lp].hitting.bb;
                        tk += team.batters[lp].hitting.so;
                        thbp += team.batters[lp].hitting.hbp;
                        tgidp += team.batters[lp].hitting.gidp;
                        tsb += team.batters[lp].hitting.sb;
                        tcs += team.batters[lp].hitting.cs;
                        tibb += team.batters[lp].hitting.ibb;
                        tsh += team.batters[lp].hitting.sh;
                        tsf += team.batters[lp].hitting.sf;
                        for (y = 1; y < 11; y++)
                            if (team.batters[lp].fielding[y].games > 0) {
                                if (x >= 7 && x <= 9 && team.batters[lp].fielding[10].games > 0)
                                    /* if the OF fielding iteration has games then that iteration will contained all OF stats */
                                    continue;
                                else {
                                    tpo += team.batters[lp].fielding[y].po;
                                    ta += team.batters[lp].fielding[y].a;
                                    if (y == 2)
                                        tpb += team.batters[lp].fielding[y].pb;
                                    te += team.batters[lp].fielding[y].e;
                                }
                            }
                    }

                    /* strikeout, gidp, caught stealing, intentional base on balls & sacrifice fly were not always kept */
                    if (tk < 0)
                        tk = 0;
                    if (tgidp < 0)
                        tgidp = 0;
                    if (tcs < 0)
                        tcs = 0;
                    if (tibb < 0)
                        tibb = 0;
                    if (tsf < 0)
                        tsf = 0;

                    for (lp = 0; lp < maxpitchers[0]; lp++) {
                        tpg += team.pitchers[lp].pitching.games;
                        tgs += team.pitchers[lp].pitching.games_started;
                        tip += team.pitchers[lp].pitching.innings;
                        tthirds += team.pitchers[lp].pitching.thirds;
                        tw += team.pitchers[lp].pitching.wins;
                        tl += team.pitchers[lp].pitching.losses;
                        ts += team.pitchers[lp].pitching.saves;
                        tbfp += team.pitchers[lp].pitching.bfp;
                        tph += team.pitchers[lp].pitching.hits;
                        tp2b += team.pitchers[lp].pitching.doubles;
                        tp3b += team.pitchers[lp].pitching.triples;
                        tphr += team.pitchers[lp].pitching.homers;
                        tpr += team.pitchers[lp].pitching.runs;
                        ter += team.pitchers[lp].pitching.er;
                        tprbi += team.pitchers[lp].pitching.rbi;
                        tcg += team.pitchers[lp].pitching.cg;
                        tgf += team.pitchers[lp].pitching.gf;
                        tsho += team.pitchers[lp].pitching.sho;
                        tsvopp += team.pitchers[lp].pitching.svopp;
                        tpsb += team.pitchers[lp].pitching.sb;
                        tpcs += team.pitchers[lp].pitching.cs;
                        tpbb += team.pitchers[lp].pitching.walks;
                        tpk += team.pitchers[lp].pitching.so;
                        tpibb += team.pitchers[lp].pitching.ibb;
                        tpsh += team.pitchers[lp].pitching.sh;
                        tpsf += team.pitchers[lp].pitching.sf;
                        twp += team.pitchers[lp].pitching.wp;
                        tb += team.pitchers[lp].pitching.balks;
                        thb += team.pitchers[lp].pitching.hb;
                        toppab += team.pitchers[lp].pitching.opp_ab;
                    }

                    /* doubles allowed, triples allowed, rbi's allowed, save opportunities, stolen bases allowed,
                       caught stealing allowed, IBB's given, sacrifice hits allowed & sacrifice flies allowed were not always kept */
                    if (tp2b < 0)
                        tp2b = 0;
                    if (tp3b < 0)
                        tp3b = 0;
                    if (tprbi < 0)
                        tprbi = 0;
                    if (tsvopp < 0)
                        tsvopp = 0;
                    if (tpsb < 0)
                        tpsb = 0;
                    if (tpcs < 0)
                        tpcs = 0;
                    if (tpibb < 0)
                        tpibb = 0;
                    if (tpsh < 0)
                        tpsh = 0;
                    if (tpsf < 0)
                        tpsf = 0;

                    /* get number of games team played during the regular season
                       in 1981 each team appears twice in the Results file, so in the scan loop of the results area below don't break out of it
                         when a team match is found and accumulate games rather than replacing them */
                    /* get the team name */
                    strcpy (&tname[0], (char *) GetTeamName (team.id));
                    /* the Cardinals and Browns show as "St." in Results files but as "St " in teamnames */
                    if (!strncmp (&tname[0], "St ", 3)) {
                        strcpy (&work[0], &tname[0]);
                        tname[2] = '.';
                        tname[3] = ' ';
                        strcpy (&tname[4], &work[3]);
                    }

                    for (pos = 0; pos < strlen (&results[0]); pos++) {
                        if (!strncmp (&results[pos], "Post Season", strlen ("Post Season")))
                            break;
                        if (!strncmp (&results[pos], &tname[0], strlen (&tname[0]))) {
                            /* find the first character 0 - 9, that will be the first position of team wins */
                            for ( ; pos < strlen (&results[0]); pos++)
                                if (results[pos] >= '0' && results[pos] <= '9')
                                    break;
                            pnt = index (&results[pos], ' ');
                            *pnt = '\0';
                            thold1 = atoi (&results[pos]);
                            *pnt = ' ';
                            /* get to first position of team losses */
                            for ( ; results[pos] != ' '; pos++);
                            for ( ; pos < strlen (&results[0]); pos++)
                                if (results[pos] >= '0' && results[pos] <= '9')
                                    break;
                            pnt = index (&results[pos], ' ');
                            *pnt = '\0';
                            thold2 = atoi (&results[pos]);
                            *pnt = ' ';
                            tgames[yr][tgamespnt].teamid = team.id;
                            tgames[yr][tgamespnt].games += (thold1 + thold2);
                        }
                    }
                    tgamespnt++;
                }
            closedir (fnames);

            /* go through all the teams three times getting the top 2000 position players, the top 2000 starting pitchers, and the top 2000 relief pitchers
               according to the formulas submitted by the user */
            for (iter = 0; iter < 3; iter++) {
                if (!iter) {
                    if (!yrsB[yr])
                        continue;
                }
                else
                    if (iter == 1) {
                        if (!yrsS[yr])
                            continue;
                    }
                    else
                        if (!yrsR[yr])
                            continue;
                if ((fnames = opendir (&dummy1[0])) != NULL) {
                    while ((dir = readdir (fnames))) {
                        /* don't process . and .. files */
                        if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                            continue;
                        /* don't process the Results file */
                        if (strstr (dir->d_name, "Results"))
                            continue;
                        /* are we using players from just one team? */
                        if (strncmp (&whichteam[0], "ALLT", 4))
                            if (strcmp (dir->d_name, &whichteam[0]))
                                continue;

                        strcpy (&dummy2[0], &dummy1[0]);
                        strcat (&dummy2[0], "/");
                        strcat (&dummy2[0], dir->d_name);

                        if ((in = fopen (dummy2, "r")) != NULL) {
                            fread (&team.id, sizeof team.id, 1, in);
                            fread (&team.year, sizeof team.year, 1, in);
                            fread (&team.league, sizeof team.league, 1, in);
                            fread (&team.division, sizeof team.division, 1, in);
                            for (x = 0; x < 25; x++) {
                                fread (&team.batters[x].id, sizeof team.batters[x].id, 1, in);
                                fread (&team.batters[x].dob, sizeof team.batters[x].dob, 1, in);
                                fread (&team.batters[x].hitting, sizeof team.batters[x].hitting, 1, in);
                                for (y = 0; y < 11; y++)
                                    fread (&team.batters[x].fielding[y], sizeof team.batters[x].fielding[y], 1, in);
                            }
                            for (x = 0; x < 11; x++) {
                                fread (&team.pitchers[x].id, sizeof team.pitchers[x].id, 1, in);
                                fread (&team.pitchers[x].pitching, sizeof team.pitchers[x].pitching, 1, in);
                            }
                            fclose (in);
                        }
                        else {
                            if (syslog_ent == YES)
                                syslog (LOG_ERR, "There is something wrong with %s: %s", dummy2, strerror (errno));
                            return -1;
                        }

                        /* get games team played during regular season */
                        for (x = 0; x < tgamespnt; x ++)
                            if (tgames[yr][x].teamid == team.id) {
                                totgames = tgames[yr][x].games;
                                break;
                            }

                        /* determine the number of batters and pitchers this team has */
                        for (maxplayers[0] = 0; maxplayers[0] < 25; maxplayers[0]++)
                            if (team.batters[maxplayers[0]].id.name[0] == ' ' || !strlen (&team.batters[maxplayers[0]].id.name[0]))
                                break;
                        for (maxpitchers[0] = 0; maxpitchers[0] < 11; maxpitchers[0]++)
                            if (team.pitchers[maxpitchers[0]].id.name[0] == ' ' || !strlen (&team.pitchers[maxpitchers[0]].id.name[0]))
                                break;

                        if (!iter)
                            loop = maxplayers[0];
                        else
                            loop = maxpitchers[0];

                        for (lp = 0; lp < loop; lp++) {
                            players[iter][2000].score = -99999.0;

                            /* change any stats that are -1 to 0 for calculating
                               if this year is supposed to be omitted, processing would not get here */
                            if (!iter) {
                                players[iter][2000].batter = team.batters[lp];
                                /* if the player happened to play pitcher wipe out those fielding stats ... we don't want them */
                                players[iter][2000].batter.fielding[1].games = players[iter][2000].batter.fielding[1].po =
                                  players[iter][2000].batter.fielding[1].dp = players[iter][2000].batter.fielding[1].a =
                                  players[iter][2000].batter.fielding[1].pb = players[iter][2000].batter.fielding[1].e = 0;

                                if (team.batters[lp].hitting.so == -1)
                                    team.batters[lp].hitting.so = 0;
                                if (team.batters[lp].hitting.gidp == -1)
                                    team.batters[lp].hitting.gidp = 0;
                                if (team.batters[lp].hitting.cs == -1)
                                    team.batters[lp].hitting.cs = 0;
                                if (team.batters[lp].hitting.ibb == -1)
                                    team.batters[lp].hitting.ibb = 0;
                                if (team.batters[lp].hitting.sf == -1)
                                    team.batters[lp].hitting.sf = 0;
                            }
                            else {
                                players[iter][2000].pitcher = team.pitchers[lp];
                                /* find hitting data for this pitcher */
                                for (x = 0; x < maxplayers[0]; x++)
                                    if (!strcmp (&team.pitchers[lp].id.name[0], &team.batters[x].id.name[0]))
                                        players[iter][2000].batter = team.batters[x];

                                if (team.pitchers[lp].pitching.doubles == -1)
                                    team.pitchers[lp].pitching.doubles = 0;
                                if (team.pitchers[lp].pitching.triples == -1)
                                    team.pitchers[lp].pitching.triples = 0;
                                if (team.pitchers[lp].pitching.rbi == -1)
                                    team.pitchers[lp].pitching.rbi = 0;
                                if (team.pitchers[lp].pitching.svopp == -1)
                                    team.pitchers[lp].pitching.svopp = 0;
                                if (team.pitchers[lp].pitching.sb == -1)
                                    team.pitchers[lp].pitching.sb = 0;
                                if (team.pitchers[lp].pitching.cs == -1)
                                    team.pitchers[lp].pitching.cs = 0;
                                if (team.pitchers[lp].pitching.ibb == -1)
                                    team.pitchers[lp].pitching.ibb = 0;
                                if (team.pitchers[lp].pitching.sh == -1)
                                    team.pitchers[lp].pitching.sh = 0;
                                if (team.pitchers[lp].pitching.sf == -1)
                                    team.pitchers[lp].pitching.sf = 0;
                            }

                            if (!iter) {
                                /* to be considered player must have a minimum number of plate appearances */
                                if ((team.batters[lp].hitting.atbats + team.batters[lp].hitting.bb + team.batters[lp].hitting.hbp +
                                                           team.batters[lp].hitting.sf + team.batters[lp].hitting.sh) < ((float) totgames * minreq[iter]))
                                    continue;
                            }
                            else
                                if (iter == 1) {
                                    /* to be considered starting pitcher must have a minimum number of innings pitched */
                                    if (team.pitchers[lp].pitching.innings < ((float) totgames * minreq[iter]))
                                        continue;
                                    /* to be considered starting pitcher at least 75% of his games must be Games Started */
                                    if (team.pitchers[lp].pitching.games_started < (team.pitchers[lp].pitching.games / 4 * 3))
                                        continue;
                                }
                                else {
                                    /* to be considered relief pitcher must have a minimum number of games pitched */
                                    if (team.pitchers[lp].pitching.games < ((float) totgames * minreq[iter]))
                                        continue;
                                    /* to be considered relief pitcher at least 75% of his games must not be as Games Started */
                                    if (team.pitchers[lp].pitching.games_started > (team.pitchers[lp].pitching.games / 4))
                                        continue;
                                }

                            for (x = 0; x < STACKSIZE; x++) {
                                top[x].var = 0.0;
                                top[x].sym = INVALID;
                            }
                            stack = cindex[iter] = errformula[iter] = 0;

                            while (cindex[iter] < strlen (&iformula[iter][0])) {
                                switch (iformula[iter][cindex[iter]]) {
                                    case '(':
                                        if (calc_parenc (OPEN) == CALC_FAIL)
                                            errformula[iter] = 1;
                                        break;
                                    case ')':
                                        if (calc_parenc (CLOSE) == CALC_FAIL)
                                            errformula[iter] = 1;
                                        break;
                                    case '*':
                                        if (calc_binary_opc (MULTIPLY) == CALC_FAIL)
                                            errformula[iter] = 2;
                                        break;
                                    case '/':
                                        if (calc_binary_opc (DIVIDE) == CALC_FAIL)
                                            errformula[iter] = 2;
                                        break;
                                    case '+':
                                        if (calc_binary_opc (ADD) == CALC_FAIL)
                                            errformula[iter] = 2;
                                        break;
                                    case '-':
                                        if (calc_binary_opc (SUBTRACT) == CALC_FAIL)
                                            errformula[iter] = 2;
                                        break;
                                    case '0':
                                    case '1':
                                    case '2':
                                    case '3':
                                    case '4':
                                    case '5':
                                    case '6':
                                    case '7':
                                    case '8':
                                    case '9':
                                        number = (iformula[iter][cindex[iter]] - 0x30);
                                        while ((iformula[iter][cindex[iter] + 1] >= 0x30) && (iformula[iter][cindex[iter] + 1] <= 0x39)) {
                                            cindex[iter]++;
                                            number = (number * 10) + (iformula[iter][cindex[iter]] - 0x30);
                                        }
                                        if (calc_numberc ((float) number) == CALC_FAIL)
                                            errformula[iter] = 3;
                                        break;    
                                    case ' ':
                                        break;
                                    default:
                                        tindex = check4statc (iformula[iter], cindex[iter], lp, iter);
                                        if (tindex == -1) {
                                            errformula[iter] = 4;
                                            break;
                                        }
                                        cindex[iter] = tindex;
                                        break;
                                }
                                if (errformula[iter])
                                    break;
                                cindex[iter]++;
                            }
                            if (stack != 1)
                                errformula[iter] = 9;

                            if (popc (&temp) != 1)
                                errformula[iter] = 9;
                            if (temp.sym != NUMBER && temp.sym != STAT)
                                errformula[iter] = 9;

                            if (errformula[iter])
                                break;

                            /* calculation complete, save score */
                            players[iter][2000].score = temp.var;

                            for (x = 0; x < 2000; x++)
                                if (players[iter][2000].score > players[iter][x].score) {
                                    for (z = 1998; z >= x; z--)
                                        players[iter][z + 1] = players[iter][z];
                                    players[iter][x] = players[iter][2000];

                                    break;
                                }
                        }
                        if (errformula[iter])
                            break;
                    }
                }
                closedir (fnames);
            }
        }
        else
            continue;
        if (errformula[0] || errformula[1] || errformula[2])
            break;
    }

    if (!errformula[0] && !errformula[1] && !errformula[2]) {
        /* build team */
        int saves, spot, pplace, bplace, pos, undercover, overcover, itp, matchSP;

        for (x = 0; x < 9; x++)
            pscore[x] = 0;

        /* first find a closer, we need only one */
        for (saves = 10; saves >= 0; saves -= 5) {
            for (x = 0; x < 2000; x++) {
                if (players[2][x].pitcher.pitching.saves > saves) {
                    dteam.pitchers[10] = players[2][x].pitcher;
                    dteam.batters[24] = players[2][x].batter;
                    break;
                }
                if (players[2][x].score == -99999.0)
                    x = 1999;
            }
            if (x != 2000)
                break;
        }
        if (x == 2000) {
            /* if this happens then no relief pitcher in the list had any saves */
            sock_puts (sock, "ERRFORMC\n");
            return -1;
        }

        /* find 6 starting pitchers */
        for (matchSP = pplace = 0, bplace = 14, x = 0; x < 2000 && pplace < 6; x++, matchSP = 0) {
            if (players[1][x].score == -99999.0)
                break;
            if (multiplyr) {
                for (y = 0; y < 25; y++)
                    if (!strcmp (&players[1][x].batter.id.name[0], &dteam.batters[y].id.name[0]) &&
                                  players[1][x].batter.dob.month == dteam.batters[y].dob.month &&
                                  players[1][x].batter.dob.day == dteam.batters[y].dob.day &&
                                  players[1][x].batter.dob.year == dteam.batters[y].dob.year &&
                                  players[1][x].batter.id.year == dteam.batters[y].id.year)
                        matchSP = 1;
            }
            else
                for (y = 0; y < 25; y++)
                    if (!strcmp (&players[1][x].batter.id.name[0], &dteam.batters[y].id.name[0]) &&
                                  players[1][x].batter.dob.month == dteam.batters[y].dob.month &&
                                  players[1][x].batter.dob.day == dteam.batters[y].dob.day &&
                                  players[1][x].batter.dob.year == dteam.batters[y].dob.year)
                        matchSP = 1;
            if (matchSP)
                continue;
            dteam.pitchers[pplace] = players[1][x].pitcher;
            dteam.batters[bplace] = players[1][x].batter;
            pplace++;
            bplace++;
        }
        if (x == 2000 || pplace < 6) {
            /* if this happens then we couldn't find enough starting pitchers */
            sock_puts (sock, "ERRFORMS\n");
            return -1;
        }

        /* find 4 relief pitchers */
        if (!closer1)
            /* user does not want to have more than 1 closer-type pitcher on the roster */
            closer1compare = 11;
        else
            /* okay to have more than 1 closer-type pitcher on the roster */
            closer1compare = 2000;
        for (matchSP = 0, pplace = 6, bplace = 20, x = 0; x < 2000 && pplace < 10; x++, matchSP = 0) {
            if (players[2][x].score == -99999.0)
                break;
            if (players[2][x].pitcher.pitching.saves < closer1compare) {
                if (multiplyr) {
                    for (y = 0; y < 25; y++)
                        if (!strcmp (&players[2][x].batter.id.name[0], &dteam.batters[y].id.name[0]) &&
                                      players[2][x].batter.dob.month == dteam.batters[y].dob.month &&
                                      players[2][x].batter.dob.day == dteam.batters[y].dob.day &&
                                      players[2][x].batter.dob.year == dteam.batters[y].dob.year &&
                                      players[2][x].batter.id.year == dteam.batters[y].id.year)
                            matchSP = 1;
                }
                else
                    for (y = 0; y < 25; y++)
                        if (!strcmp (&players[2][x].batter.id.name[0], &dteam.batters[y].id.name[0]) &&
                                      players[2][x].batter.dob.month == dteam.batters[y].dob.month &&
                                      players[2][x].batter.dob.day == dteam.batters[y].dob.day &&
                                      players[2][x].batter.dob.year == dteam.batters[y].dob.year)
                            matchSP = 1;
                if (matchSP)
                    continue;
                dteam.pitchers[pplace] = players[2][x].pitcher;
                dteam.batters[bplace] = players[2][x].batter;
                pplace++;
                bplace++;
            }
        }
        if (x == 2000 || pplace < 10) {
            /* if this happens then we couldn't find enough relief pitchers */
            sock_puts (sock, "ERRFORMR\n");
            return -1;
        }

        /* find enough position players ... there must be at least 2 players who can play each position */

        /* begin by selecting the highest ranking player on the list at each position */
        for (pos = 2; pos < 10; pos++) {                 /* we don't care about the DH or P positions */
            for (matchSP = x = 0; x < 2000; x++) {
                if (players[0][x].score == -99999.0) {
                    x = 1999;
                    continue;
                }
                /* if this player did not play any positions then go to the next player */
                for (y = 2; y < 10; y++)
                    if (players[0][x].batter.fielding[y].games)
                        break;
                if (y == 10)
                    continue;

                /* get games team played during regular season */
                for (y = 0; y < 50; y ++)
                    if (tgames[players[0][x].batter.id.year - 1901][y].teamid == players[0][x].batter.id.teamid)
                        totgames = tgames[players[0][x].batter.id.year - 1901][y].games;
                if (players[0][x].batter.fielding[pos].games < (totgames / 2))
                    /* the player needs to have played at least half of his team's games at the position in order to be considered */
                    continue;
                if (multiplyr) {
                    for (y = 0; y < (pos - 2); y++)
                        if (!strcmp (&players[0][x].batter.id.name[0], &players[0][piter[y]].batter.id.name[0]) &&
                                      players[0][x].batter.dob.month == players[0][piter[y]].batter.dob.month &&
                                      players[0][x].batter.dob.day == players[0][piter[y]].batter.dob.day &&
                                      players[0][x].batter.dob.year == players[0][piter[y]].batter.dob.year &&
                                      players[0][x].batter.id.year == players[0][piter[y]].batter.id.year)
                            matchSP = 1;
                }
                else
                    for (y = 0; y < (pos - 2); y++)
                        if (!strcmp (&players[0][x].batter.id.name[0], &players[0][piter[y]].batter.id.name[0]) &&
                                      players[0][x].batter.dob.month == players[0][piter[y]].batter.dob.month &&
                                      players[0][x].batter.dob.day == players[0][piter[y]].batter.dob.day &&
                                      players[0][x].batter.dob.year == players[0][piter[y]].batter.dob.year)
                            matchSP = 1;
                if (matchSP)
                    matchSP = 0;
                else {
                    piter[pos - 2] = x;
                    break;
                }
            }
            if (x == 2000) {
                /* if this happens then we couldn't find enough position players */
                sock_puts (sock, "ERRFORMP\n");
                return -1;
            }
        }
        /* fill in the remaining position player spots with the highest ranking players on the list */
        for (spot = 8; spot < 14; spot++) {
            for (x = 0; x < 2000; x++) {
                if (players[0][x].score == -99999.0) {
                    x = 1999;
                    continue;
                }
                /* if this player did not play any positions then go to the next player */
                for (y = 2; y < 10; y++)
                    if (players[0][x].batter.fielding[y].games)
                        break;
                if (y == 10)
                    continue;

                if (multiplyr) {
                    for (y = 0; y < spot; y++)
                        if (!strcmp (&players[0][x].batter.id.name[0], &players[0][piter[y]].batter.id.name[0]) &&
                                      players[0][x].batter.dob.month == players[0][piter[y]].batter.dob.month &&
                                      players[0][x].batter.dob.day == players[0][piter[y]].batter.dob.day &&
                                      players[0][x].batter.dob.year == players[0][piter[y]].batter.dob.year &&
                                      players[0][x].batter.id.year == players[0][piter[y]].batter.id.year)
                            matchSP = 1;
                }
                else
                    for (y = 0; y < spot; y++)
                        if (!strcmp (&players[0][x].batter.id.name[0], &players[0][piter[y]].batter.id.name[0]) &&
                                      players[0][x].batter.dob.month == players[0][piter[y]].batter.dob.month &&
                                      players[0][x].batter.dob.day == players[0][piter[y]].batter.dob.day &&
                                      players[0][x].batter.dob.year == players[0][piter[y]].batter.dob.year)
                            matchSP = 1;
                if (matchSP)
                    matchSP = 0;
                else {
                    piter[spot] = x;
                    break;
                }
            }
            if (x == 2000) {
                /* if this happens then we couldn't find enough position players */
                sock_puts (sock, "ERRFORMP\n");
                return -1;
            }
        }

        /* check that all positions are covered, if not go through remaining list of players one at a time until we have a proper team */
        for (x = 0; x < 2000; x++) {
            CalculateCoverage ();
            undercover = CheckCoverage ();
            if (undercover == -1)
                /* coverage is okay */
                break;
            overcover = FindOvercoverage ();
            if (overcover == -1) {
                sock_puts (sock, "ERR\n");
                return -1;
            }
            y = RemoveOvercover (overcover);
            itp = CheckPosition (x, y, undercover);
            if (itp == -1) {
                x = 2000;
                break;
            }
        }
        if (x == 2000) {
            /* if this happens then we couldn't find enough position players */
            sock_puts (sock, "ERRFORMP\n");
            return -1;
        }

        /* fill in team with position players */
        for (x = 0; x < 14; x++)
            dteam.batters[x] = players[0][piter[x]].batter;

        if ((out = fopen (dirt, "w")) != NULL) {
            fwrite (&dteam.id, sizeof dteam.id, 1, out);
            fwrite (&dteam.year, sizeof dteam.year, 1, out);
            fwrite (&dteam.league, sizeof dteam.league, 1, out);
            fwrite (&dteam.division, sizeof dteam.division, 1, out);
            for (x = 0; x < 25; x++) {
                fwrite (&dteam.batters[x].id, sizeof dteam.batters[x].id, 1, out);
                fwrite (&dteam.batters[x].dob, sizeof dteam.batters[x].dob, 1, out);
                fwrite (&dteam.batters[x].hitting, sizeof dteam.batters[x].hitting, 1, out);
                for (y = 0; y < 11; y++)
                    fwrite (&dteam.batters[x].fielding[y], sizeof dteam.batters[x].fielding[y], 1, out);
            }
            for (x = 0; x < 11; x++) {
                fwrite (&dteam.pitchers[x].id, sizeof dteam.pitchers[x].id, 1, out);
                fwrite (&dteam.pitchers[x].pitching, sizeof dteam.pitchers[x].pitching, 1, out);
            }
            fclose (out);
            strcpy (&buffer1[0], "OK");
        }
        else
            strcpy (&buffer1[0], "ERR");
    }
    else {
        strcpy (&buffer1[0], "ERROR");
        strcat (&buffer1[0], (char *) cnvt_int2str (1, errformula[0]));
        strcat (&buffer1[0], (char *) cnvt_int2str (1, errformula[1]));
        strcat (&buffer1[0], (char *) cnvt_int2str (1, errformula[2]));
        strcat (&buffer1[0], (char *) cnvt_int2str (4, cindex[0]));
        strcat (&buffer1[0], (char *) cnvt_int2str (4, cindex[1]));
        strcat (&buffer1[0], (char *) cnvt_int2str (4, cindex[2]));
    }

    if (!strlen (&buffer1[0]))
        strcat (&buffer1[0], "NODATA");
    strcat (&buffer1[0], "\n");
    sock_puts (sock, &buffer1[0]);

    return 0;
}

