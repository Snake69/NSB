/*
    process a formula submitted by user to calculate players' seasons
*/

#include <stdlib.h>
#include <dirent.h>
#include <syslog.h>
#include "sglobal.h"
#include "db.h"
#include "sproto.h"
#include "net.h"

#define STACKSIZE 20
#define CALC_FAIL -2
#define CALC_SUCCESS -1

int stack, tpit, tply, divideby0, divide0;
int tg, tab, tr, th, t2b, t3b, thr, trbi, tbb, tk, thbp, tgidp, tsb, tcs, tibb, tsh, tsf, tpo, ta, tpb, te;
int tpg, tgs, tip, tthirds, tw, tl, ts, tbfp, tph, tp2b, tp3b, tphr, tpr, ter, tprbi, tcg, tgf, tsho, tsvopp, tpsb, tpcs, tpbb, tpk, tpibb, tpsh, tpsf, twp, tb, thb, toppab;

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
push (struct level *p1) {
    if (stack < STACKSIZE) {
        top[stack].var = p1->var;
        top[stack].sym = p1->sym;
        stack++;
        return 1;
    }
    return 0;
}

int
pop (struct level *p1) {
    if (stack > 0) {
        stack--;
        p1->var = top[stack].var;
        p1->sym = top[stack].sym;
        return 1;
    }
    return 0;
}

int
peek (struct level *p1) {
    if (stack > 0) {
        p1->var = top[stack - 1].var;
        p1->sym = top[stack - 1].sym;
        return 1;
    }
    return 0;
}

int
calc_number (float num) {
    struct level temp;
    CALC_SYMBOLS precede;

    if (stack < 1) {
        temp.var = num;
        temp.sym = NUMBER;
        push (&temp);
        return CALC_SUCCESS;
    }

    peek (&temp);
    precede = temp.sym;
    switch (precede) {
        case MULTIPLY:
        case DIVIDE:
        case ADD:
        case SUBTRACT:
            if (pop (&temp) != 1)
                return CALC_FAIL;
            if (pop (&temp) != 1)
                return CALC_FAIL;
            if (temp.sym != NUMBER && temp.sym != STAT)
                return CALC_FAIL;
            switch (precede) {
                case MULTIPLY:
                    temp.var = temp.var * num;
                    break;
                case DIVIDE:
                    if (num == 0) {
                        if (divide0 == 1) 
                            /* user wants to substitute for 0 */
                            num = 0.00001;
                        else {
                            divideby0 = 1;
                            return CALC_FAIL;
                        }
                    }
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
            if (push (&temp) != 1)
                return CALC_FAIL;
            break;
        case OPEN:
            temp.var = num;
            temp.sym = NUMBER;
            if (push (&temp) != 1)
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
check4stat (char *exp, int ind, int p) {
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
        if (buffer[0] == 'v')    /* svopp is a pitching only stat */
            return -1;
        if (LAsw)
            comp = (float) tsvopp / (float) tpit;
        else
            comp = team.pitchers[p].pitching.svopp;
        ind += 3;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "OPAB", 4)) {
        if (buffer[0] == 'v')    /* oppab is a pitching only stat */
            return -1;
        if (LAsw)
            comp = (float) toppab / (float) tpit;
        else
            comp = team.pitchers[p].pitching.opp_ab;
        ind += 3;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "OPBA", 4)) {
        if (buffer[0] == 'v')    /* opba is a pitching only stat */
            return -1;
        if (LAsw)
            comp = (float) tph / (float) toppab;
        else
            comp = (float) team.pitchers[p].pitching.hits / (float) team.pitchers[p].pitching.opp_ab;
        ind += 3;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "GIDP", 4)) {
        if (buffer[0] == 'V')    /* gidp is an offense only stat */
            return -1;
        if (LAsw)
            comp = (float) tgidp / (float) tply;
        else
            comp = team.batters[p].hitting.gidp;
        ind += 3;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "RBI", 3)) {
        if (buffer[0] == 'v')
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
        if (buffer[0] == 'v')
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
        if (buffer[0] == 'v')
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
        if (buffer[0] == 'V')    /* oba is an offense only stat */
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
        if (buffer[0] == 'v')    /* bfp is a pitching only stat */
            return -1;
        if (LAsw)
            comp = (float) tbfp / (float) tpit;
        else
            comp = team.pitchers[p].pitching.bfp;
        ind += 2;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "ERA", 3)) {
        if (buffer[0] == 'v')    /* era is a pitching only stat */
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
        if (buffer[0] == 'v')    /* pct is a pitching only stat */
            return -1;
        if (LAsw)
            comp = (float) tw / (float) (tw + tl);
        else
            comp = (float) team.pitchers[p].pitching.wins / (float) (team.pitchers[p].pitching.wins + team.pitchers[p].pitching.losses);
        ind += 2;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "SHO", 3)) {
        if (buffer[0] == 'v')    /* sho is a pitching only stat */
            return -1;
        if (LAsw)
            comp = (float) tsho / (float) tpit;
        else
            comp = team.pitchers[p].pitching.sho;
        ind += 2;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "SG", 2)) {
        if (buffer[0] == 'v')
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
        if (buffer[0] == 'v')
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
        if (buffer[0] == 'v')
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
        if (buffer[0] == 'v')
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
        if (buffer[0] == 'v')
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
        if (buffer[0] == 'v')
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
        if (buffer[0] == 'v')
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
        if (buffer[0] == 'v')
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
        if (buffer[0] == 'v')
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
        if (buffer[0] == 'V')    /* ab is an offense only stat */
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

        if (buffer[0] == 'V')    /* tb is an offense only stat */
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
        if (buffer[0] == 'V')    /* ba is an offense only stat */
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

        if (buffer[0] == 'V')    /* sa is an offense only stat */
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

        if (buffer[0] == 'V')    /* po is an offense only stat */
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
        if (buffer[0] == 'V')    /* pb is an offense only stat */
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
        int x;
        float chances, errors;

        if (buffer[0] == 'V')    /* fa is an offense only stat */
            return -1;
        if (LAsw) {
            chances = tpo + ta;
            comp = chances / (chances + te);
        }
        else {
            for (chances = errors = 0, x = 1; x < 11; x++)      /* nothing for DH */
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
                comp = chances / (chances + errors);
        }
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "CG", 2)) {
        if (buffer[0] == 'v')    /* cg is a pitching only stat */
            return -1;
        if (LAsw)
            comp = (float) tcg / (float) tpit;
        else
            comp = team.pitchers[p].pitching.cg;
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "ER", 2)) {
        if (buffer[0] == 'v')    /* er is a pitching only stat */
            return -1;
        if (LAsw)
            comp = (float) ter / (float) tpit;
        else
            comp = team.pitchers[p].pitching.er;
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "GF", 2)) {
        if (buffer[0] == 'v')    /* gf is a pitching only stat */
            return -1;
        if (LAsw)
            comp = (float) tgf / (float) tpit;
        else
            comp = team.pitchers[p].pitching.gf;
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "GS", 2)) {
        if (buffer[0] == 'v')    /* gs is a pitching only stat */
            return -1;
        if (LAsw)
            comp = (float) tgs / (float) tpit;
        else
            comp = team.pitchers[p].pitching.games_started;
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "IP", 2)) {
        if (buffer[0] == 'v')    /* ip is a pitching only stat */
            return -1;
        if (LAsw)
            comp = (float) (tip + ((float) tthirds / 3.0)) / (float) tpit;
        else
            comp = team.pitchers[p].pitching.innings + ((float) team.pitchers[p].pitching.thirds / 3.0);
        ind++;
        goto AfterComp;
    }

    if (!strncasecmp (&exp[ind], "WP", 2)) {
        if (buffer[0] == 'v')    /* wp is a pitching only stat */
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
            if (buffer[0] == 'v')
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
            if (buffer[0] == 'v')
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
            if (buffer[0] == 'v')
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
            if (buffer[0] == 'v')
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

            if (buffer[0] == 'V')    /* a is an offense only stat */
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

            if (buffer[0] == 'V')    /* e is an offense only stat */
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
            if (buffer[0] == 'v')    /* b is a pitching only stat */
                return -1;
            if (LAsw)
                comp = (float) tb / (float) tpit;
            else
                comp = team.pitchers[p].pitching.balks;
        }

        if (exp[ind] == 'W' || exp[ind] == 'w') {
            if (buffer[0] == 'v')    /* w is a pitching only stat */
                return -1;
            if (LAsw)
                comp = (float) tw / (float) tpit;
            else
                comp = team.pitchers[p].pitching.wins;
        }

        if (exp[ind] == 'L' || exp[ind] == 'l') {
            if (buffer[0] == 'v')    /* l is a pitching only stat */
                return -1;
            if (LAsw)
                comp = (float) tl / (float) tpit;
            else
                comp = team.pitchers[p].pitching.losses;
        }

        if (exp[ind] == 'S' || exp[ind] == 's') {
            if (buffer[0] == 'v')    /* s is a pitching only stat */
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
        push (&temp);
        return ind;
    }

    peek (&temp);
    precede = temp.sym;
    switch (precede) {
        case MULTIPLY:
        case DIVIDE:
        case ADD:
        case SUBTRACT:
            if (pop (&temp) != 1)
                return -1;
            if (pop (&temp) != 1)
                return -1;
            if (temp.sym != NUMBER && temp.sym != STAT)
                return -1;
            switch (precede) {
                case MULTIPLY:
                    temp.var = temp.var * comp;
                    break;
                case DIVIDE:
                    if (comp == 0) {
                        if (divide0 == 1)
                            /* user wants to substitute for 0 */
                            comp = 0.00001;
                        else {
                            divideby0 = 1;
                            return -1;
                        }
                    }
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
            if (push (&temp) != 1)
                return -1;
            break;
        case OPEN:
            temp.var = comp;
            temp.sym = STAT;
            if (push (&temp) != 1)
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
calc_binary_op (CALC_SYMBOLS sym) {
    struct level temp;

    if (stack < 1)
        return CALC_FAIL;

    peek (&temp);
    if (temp.sym != NUMBER && temp.sym != STAT)
        return CALC_FAIL;

    temp.sym = sym;
    if (push (&temp) != 1)
        return CALC_FAIL;

    return CALC_SUCCESS;
}

int
calc_paren (CALC_SYMBOLS sym) {
    struct level temp;
    float result;

    switch (sym) {
        case OPEN:
            temp.sym = sym;
            if (push (&temp) != 1)
                return CALC_FAIL;
            break;
        case CLOSE:
            if (pop (&temp) != 1)
                return CALC_FAIL;
            if (temp.sym != NUMBER && temp.sym != STAT)
                return CALC_FAIL;
            result = temp.var;

            if (pop (&temp) != 1)
                return CALC_FAIL;
            if (temp.sym != OPEN)
                return CALC_FAIL;
            if (calc_number (result) != CALC_SUCCESS)
                return CALC_FAIL;
            break;
        default:
            return CALC_FAIL;
            break;
    }

    return CALC_SUCCESS;
}

int
formula () {
    /* user wants to see the results of a formula applied to player seasons over certain years */
    int x, y, z, cindex, thold1, thold2, tindex, number, tgamespnt, loop, lp, pos, totgames, yrs[YEAR_SPREAD], yr, errformula, rtype, listlen, b2w;
    float minreq;
    char results[5000], work[500], dummy[256], dummy1[256], dummy2[256], tname[100], iformula[4096], *pnt;
    DIR *fnames;
    struct dirent *dir;
    struct level temp;
    FILE *in;

    struct {
        int year, teamid;
        char pname[100];
        float score;
    } players[501];

    struct {
        int teamid, games;
    } tgames[50];

    divideby0 = 0;
    for (x = 0; x < YEAR_SPREAD; x++)
        yrs[x] = 0;

    /* get what-do-to-with-missing-stats indicator */
    rtype = buffer[1] - '0';

    /* get divide-by-0 indicator */
    divide0 = buffer[2] - '0';

    /* get length of returned list */
    pnt = index (&buffer[4], ',');
    *pnt = '\0';
    listlen = atoi (&buffer[4]);
    pos = strlen (&buffer[0]) + 1;    /* best to worst or worst to best indicator */
    *pnt = ',';

    /* get best to worst indicator */
    b2w = buffer[pos] - '0';
    pos++;                            /* first position of min req */

    for (x = 0; x < 501; x++)
        if (!b2w)
            players[x].score = -99999.0;
        else
            players[x].score = 99999.0;

    /* get minimum requirements and formula */
    pnt = index (&buffer[pos], ',');
    *pnt = '\0';
    minreq = atof (&buffer[pos]);
    pos = strlen (&buffer[0]) + 1;    /* first position of formula */
    *pnt = ',';

    pnt = index (&buffer[pos], ',');
    *pnt = '\0';
    strcpy (&iformula[0], &buffer[pos]);
    pos = strlen (&buffer[0]) + 1;    /* first position of years */
    *pnt = ',';

    /* get years to include in search */
    for (x = 0; x < YEAR_SPREAD; x++, pos++)
        if (buffer[pos] == '1')
            yrs[x] = 1;

    if (!rtype) {
        /* omit years with unavailable stats */
        char *pnt;

        /* IBB is unavailable for both offense and pitching 1901 through 1954 */
        if (strcasestr (&iformula[0], "IBB"))
            for (x = 0; x < 54; x++)
                yrs[x] = 0;

        if (buffer[0] == 'v') {
            if (strcasestr (&iformula[0], "GIDP"))
                /* GIDP is unavailable for offense 1901 through 1938 */
                for (x = 0; x < 38; x++)
                    yrs[x] = 0;
            if (strcasestr (&iformula[0], "SF"))
                /* SF is unavailable for offense 1901 through 1953 */
                for (x = 0; x < 53; x++)
                    yrs[x] = 0;
            if (strcasestr (&iformula[0], "K"))
                /* K is unavailable for offense 1901 through 1912 */
                for (x = 0; x < 12; x++)
                    yrs[x] = 0;
            if (strcasestr (&iformula[0], "CS")) {
                /* CS is unavailable for offense 1901 through 1919 and 1926 through 1950 */
                for (x = 0; x < 19; x++)
                    yrs[x] = 0;
                for (x = 26; x < 50; x++)
                    yrs[x] = 0;
            }
        }

        if (buffer[0] == 'V') {
            /* SOPP, DB, TP, RBI, SB, CS, SF, & OPBA are unavailable for pitching for all years except 1998 */
            if (strcasestr (&iformula[0], "SOPP") || strcasestr (&iformula[0], "DB") || strcasestr (&iformula[0], "TP") ||
                strcasestr (&iformula[0], "RBI") || strcasestr (&iformula[0], "SB") || strcasestr (&iformula[0], "CS") ||
                strcasestr (&iformula[0], "SF") || strcasestr (&iformula[0], "OPBA")) {
                for (x = 0; x < 97; x++)
                    yrs[x] = 0;
                for (x = 98; x < YEAR_SPREAD; x++)
                    yrs[x] = 0;
            }
            /* make sure SH isn't really SHO */
            pnt = strcasestr (&iformula[0], (char *) "SH");
            if (pnt && *(pnt + 2) != 'O' && *(pnt + 2) !='o') {
                for (x = 0; x < 97; x++)
                    yrs[x] = 0;
                for (x = 98; x < YEAR_SPREAD; x++)
                    yrs[x] = 0;
            }
        }
    }

    /* get data to use */
    strcpy (&dummy[0], "/var/NSB/RealLifeStats/");
    for (yr = 0; yr < YEAR_SPREAD; yr++) {
        if (yrs[yr]) {
            strcpy (&dummy1[0], &dummy[0]);
            strcat (&dummy1[0], (char *) cnvt_int2str (4, (1901 + yr)));

            for (x = 0; x < 50; x++) {
                tgames[x].games = 0;
                tgames[x].teamid = 999;
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

            /* go through all teams twice ... this first time is to cum all stats and to get the number of players and pitchers in the entire league
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
                        for (x = 0; x < 28; x++) {
                            fread (&team.batters[x].id, sizeof team.batters[x].id, 1, in);
                            fread (&team.batters[x].dob, sizeof team.batters[x].dob, 1, in);
                            fread (&team.batters[x].hitting, sizeof team.batters[x].hitting, 1, in);
                            for (y = 0; y < 11; y++)
                                fread (&team.batters[x].fielding[y], sizeof team.batters[x].fielding[y], 1, in);
                        }
                        for (x = 0; x < 13; x++) {
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
                    for (maxplayers[0] = 0; maxplayers[0] < 28; maxplayers[0]++)
                        if (team.batters[maxplayers[0]].id.name[0] == ' ' || !strlen (&team.batters[maxplayers[0]].id.name[0]))
                            break;
                    for (maxpitchers[0] = 0; maxpitchers[0] < 13; maxpitchers[0]++)
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
                        for (y = 1; y < 11; y++)      /* nothing for DH */
                            if (team.batters[lp].fielding[y].games > 0) {
                                if (y >= 7 && y <= 9 && team.batters[lp].fielding[10].games > 0)
                                    /* if the OF fielding iteration has games then that iteration will contained all OF stats */
                                    continue;
                                else {
                                    tpo += team.batters[lp].fielding[y].po;
                                    ta += team.batters[lp].fielding[y].a;
                                    te += team.batters[lp].fielding[y].e;
                                    if (y == 2)
                                        tpb += team.batters[lp].fielding[y].pb;
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
                            tgames[tgamespnt].teamid = team.id;
                            tgames[tgamespnt].games += (thold1 + thold2);
                        }
                    }
                    tgamespnt++;
                }
            closedir (fnames);

            if ((fnames = opendir (&dummy1[0])) != NULL) {
                while ((dir = readdir (fnames))) {
                    /* don't process . and .. files */
                    if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                        continue;
                    /* don't process the Results file */
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
                        for (x = 0; x < 28; x++) {
                            fread (&team.batters[x].id, sizeof team.batters[x].id, 1, in);
                            fread (&team.batters[x].dob, sizeof team.batters[x].dob, 1, in);
                            fread (&team.batters[x].hitting, sizeof team.batters[x].hitting, 1, in);
                            for (y = 0; y < 11; y++)
                                fread (&team.batters[x].fielding[y], sizeof team.batters[x].fielding[y], 1, in);
                        }
                        for (x = 0; x < 13; x++) {
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
                        if (tgames[x].teamid == team.id)
                            totgames = tgames[x].games;

                    /* determine the number of players and pitchers this team has */
                    for (maxplayers[0] = 0; maxplayers[0] < 28; maxplayers[0]++)
                        if (team.batters[maxplayers[0]].id.name[0] == ' ' || !strlen (&team.batters[maxplayers[0]].id.name[0]))
                            break;
                    for (maxpitchers[0] = 0; maxpitchers[0] < 13; maxpitchers[0]++)
                        if (team.pitchers[maxpitchers[0]].id.name[0] == ' ' || !strlen (&team.pitchers[maxpitchers[0]].id.name[0]))
                            break;

                    if (buffer[0] == 'v')
                        loop = maxplayers[0];
                    else
                        loop = maxpitchers[0];

                    for (lp = 0; lp < loop; lp++) {
                        players[500].year = team.year;
                        players[500].teamid = team.id;
                        if (!b2w)
                            players[500].score = -99999.0;
                        else
                            players[500].score = 99999.0;

                        /* change any stats that are -1 to 0 for calculating
                           if this year is supposed to be omitted it would have been omitted in the client */
                        if (buffer[0] == 'v') {
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

                        if (buffer[0] == 'v') {
                            float totpa;

                            strcpy (&players[500].pname[0], &team.batters[lp].id.name[0]);
                            /* to be considered player must have a minimum number of plate appearances */
                            /* sacrifice flies wasn't always a recorded stat */
                            if (minreq == 0.0)
                                /* there needs to be a min PA of at least 1, many real life players have 0 */
                                totpa = 1.0;
                            else
                                totpa = (float) totgames * minreq;
                            if ((team.batters[lp].hitting.atbats + team.batters[lp].hitting.bb + team.batters[lp].hitting.hbp +
                                                             team.batters[lp].hitting.sf + team.batters[lp].hitting.sh) < totpa)
                                continue;

                        }
                        else {
                            strcpy (&players[500].pname[0], &team.pitchers[lp].id.name[0]);
                            /* to be considered pitcher must have a minimum number of innings pitched */
                            if (team.pitchers[lp].pitching.innings < ((float) totgames * minreq))
                                continue;
                        }

                        for (x = 0; x < STACKSIZE; x++) {
                            top[x].var = 0.0;
                            top[x].sym = INVALID;
                        }
                        stack = cindex = errformula = 0;

                        while (cindex < strlen (&iformula[0])) {
                            switch (iformula[cindex]) {
                                case '(':
                                    if (calc_paren (OPEN) == CALC_FAIL)
                                        errformula = 1;
                                    break;
                                case ')':
                                    if (calc_paren (CLOSE) == CALC_FAIL)
                                        errformula = 1;
                                    break;
                                case '*':
                                    if (calc_binary_op (MULTIPLY) == CALC_FAIL)
                                        errformula = 2;
                                    break;
                                case '/':
                                    if (calc_binary_op (DIVIDE) == CALC_FAIL)
                                        errformula = 2;
                                    break;
                                case '+':
                                    if (calc_binary_op (ADD) == CALC_FAIL)
                                        errformula = 2;
                                    break;
                                case '-':
                                    if (calc_binary_op (SUBTRACT) == CALC_FAIL)
                                        errformula = 2;
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
                                    number = (iformula[cindex] - 0x30);
                                    while ((iformula[cindex + 1] >= 0x30) && (iformula[cindex + 1] <= 0x39)) {
                                        cindex++;
                                        number = (number * 10) + (iformula[cindex] - 0x30);
                                    }
                                    if (calc_number ((float) number) == CALC_FAIL)
                                        errformula = 3;
                                    break;    
                                case ' ':
                                    break;
                                default:
                                    tindex = check4stat (iformula, cindex, lp);
                                    if (tindex == -1) {
                                        errformula = 4;
                                        break;
                                    }
                                    cindex = tindex;
                                    break;
                            }
                            if (errformula)
                                break;
                            cindex++;
                        }
                        if (stack != 1)
                            errformula = 9;

                        if (pop (&temp) != 1)
                            errformula = 9;
                        if (temp.sym != NUMBER && temp.sym != STAT)
                            errformula = 9;

                        if (errformula)
                            break;

                        /* calculation complete, save score */
                        players[500].score = temp.var;

                        for (x = 0; x < 500; x++)
                            if ((!b2w && players[500].score > players[x].score) || (b2w && players[500].score < players[x].score)) {
                                for (z = 498; z >= x; z--)
                                    players[z + 1] = players[z];
                                players[x].score = players[500].score;
                                players[x].year = players[500].year;
                                players[x].teamid = players[500].teamid;
                                strcpy (&players[x].pname[0], &players[500].pname[0]);

                                break;
                            }
                    }
                    if (errformula)
                        break;
                }
            }
            closedir (fnames);
        }
        else
            continue;
        if (errformula)
            break;
    }

    if (!errformula)
        for (buffer1[0] = '\0', x = 0; x < listlen; x++)
            if ((!b2w && players[x].score != -99999.0) || (b2w && players[x].score != 99999.0)) {
                sprintf (work, "%d %d %s %10.10f ", players[x].year, players[x].teamid, players[x].pname, players[x].score);
                strcat (&buffer1[0], &work[0]);
            }
            else
                break;
    else {
        if (divideby0)
            strcpy (&buffer1[0], "ERR00");
        else
            strcpy (&buffer1[0], "ERROR");
        strcat (&buffer1[0], (char *) cnvt_int2str (1, errformula));
        strcat (&buffer1[0], (char *) cnvt_int2str (4, cindex));
    }

    if (!strlen (&buffer1[0]))
        strcat (&buffer1[0], "NODATA");
    strcat (&buffer1[0], "\n");
    sock_puts (sock, &buffer1[0]);

    return 0;
}

