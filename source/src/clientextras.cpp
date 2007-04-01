// clientextras.cpp: stuff that didn't fit in client.cpp or clientgame.cpp :)

#include "cube.h"

void renderclient(playerent *d, char *mdlname, char *vwepname, int tex)
{
    int varseed = (int)(size_t)d;
    int anim = ANIM_IDLE|ANIM_LOOP;
    float speed = 100.0f;
    float mz = d->o.z-d->eyeheight;
    int basetime = -((int)(size_t)d&0xFFF);
    if(d->state==CS_DEAD)
    {
		loopv(bounceents) if(bounceents[i]->bouncestate==GIB && bounceents[i]->owner==d) return;
        d->pitch = 0.1f;
        int r = 6;
        anim = ANIM_DEATH;
        varseed += d->lastaction;
        basetime = d->lastaction;
        int t = lastmillis-d->lastaction;
        if(t<0 || t>20000) return;
        if(t>(r-1)*100-50) 
		{ 
			anim = ANIM_LYING_DEAD|ANIM_NOINTERP|ANIM_LOOP;
			if(t>(r+10)*100) 
			{ 
				t -= (r+10)*100; 
				mz -= t*t/10000000000.0f*t; 
			}
		}
        //if(mz<-1000) return;
    }
    else if(d->state==CS_EDITING)                   { anim = ANIM_JUMP|ANIM_END; }
    else if(d->state==CS_LAGGED)                    { anim = ANIM_SALUTE|ANIM_LOOP; }
    else if(lastmillis-d->lastpain<300)             { anim = ANIM_PAIN; speed = 300.0f/4; varseed += d->lastpain; basetime = d->lastpain; }
    else if(!d->onfloor && d->timeinair>0)          { anim = ANIM_JUMP|ANIM_END; }
    else if(d->gunselect==d->lastattackgun && lastmillis-d->lastaction<300)
                                                    { anim = ANIM_ATTACK; speed = 300.0f/8; basetime = d->lastaction; }
    else if(!d->move && !d->strafe)                 { anim = ANIM_IDLE|ANIM_LOOP; }
    else                                            { anim = ANIM_RUN|ANIM_LOOP; speed = 1860/d->maxspeed; }
    rendermodel(mdlname, anim, tex, 1.5f, d->o.x, mz, d->o.y, d->yaw+90, d->pitch/4, speed, basetime, d, vwepname);
}

extern int democlientnum;

void renderplayer(playerent *d)
{
    if(!d) return;
   
    int team = team_int(d->team);
    s_sprintfd(skin)("packages/models/playermodels/%s/0%i.jpg", d->team, 1 + max(0, min(d->skin, (team==TEAM_CLA ? 3 : 5))));
    string vwep;
    if(d->gunselect>=0 && d->gunselect<NUMGUNS) s_sprintf(vwep)("weapons/%s/world", hudgunnames[d->gunselect]);
    else vwep[0] = 0;
    renderclient(d, "playermodels", vwep[0] ? vwep : NULL, -textureload(skin)->id);
}

void renderclients()
{
    playerent *d;
    loopv(players) if((d = players[i]) && (!demoplayback || i!=democlientnum)) renderplayer(d);
    if(player1->state==CS_DEAD) renderplayer(player1);
}

// creation of scoreboard pseudo-menu

void *scoremenu = NULL, *ctfmenu = NULL;

void showscores(bool on)
{
    menuset(on ? (m_ctf ? ctfmenu : scoremenu) : NULL);
}

struct sline { string s; };
vector<sline> scorelines;

void renderscore(void *menu, playerent *d, int cn)
{
    const char *status = "";
    if(d->ismaster) status = "\f0";
    if(d->state==CS_DEAD) status = "\f4";
    s_sprintfd(lag)("%d", d->plag);
	if(m_ctf) s_sprintf(scorelines.add().s)("%d\t%d\t%s\t%d\t%s\t%s%s\t%d", d->flagscore, d->frags, d->state==CS_LAGGED ? "LAG" : lag, d->ping, d->team, status, d->name, cn);
	else s_sprintf(scorelines.add().s)("%d\t%s\t%d\t%s\t%s%s\t%d", d->frags, d->state==CS_LAGGED ? "LAG" : lag, d->ping, m_teammode ? d->team : "", status, d->name, cn);
    menumanual(menu, scorelines.length()-1, scorelines.last().s);
}

struct teamscore
{
    char *team;
    int score, flagscore;
    teamscore() {}
    teamscore(char *s, int n, int f = 0) : team(s), score(n), flagscore(f) {}
};

static int teamscorecmp(const teamscore *x, const teamscore *y)
{
    if(x->flagscore > y->flagscore) return -1;
    if(x->flagscore < y->flagscore) return 1;
    if(x->score > y->score) return -1;
    if(x->score < y->score) return 1;
    return 0;
}

vector<teamscore> teamscores;

void addteamscore(playerent *d)
{
    if(!d || !d->team[0]) return;
    loopv(teamscores) if(!strcmp(teamscores[i].team, d->team))
    {
        teamscores[i].score += d->frags;
        if(m_ctf) teamscores[i].flagscore += d->flagscore;
        return;
    }
    teamscores.add(teamscore(d->team, d->frags, m_ctf ? d->flagscore : 0));
}

void renderscores()
{
    void *menu = m_ctf ? ctfmenu : scoremenu;
    scorelines.setsize(0);
    if(!demoplayback) renderscore(menu, player1, getclientnum());
    loopv(players) if(players[i]) renderscore(menu, players[i], i);

    sortmenu(menu, 0, scorelines.length());
    if(m_teammode)
    {
        menumanual(menu, scorelines.length(), "");
        teamscores.setsize(0);
        loopv(players) addteamscore(players[i]);
        if(!demoplayback) addteamscore(player1);
        teamscores.sort(teamscorecmp);
		string &teamline = scorelines.add().s;
		teamline[0] = 0;
        loopv(teamscores)
        {
            string s;
            if(m_ctf) s_sprintf(s)("[ %s: %d flags  %d frags ]", teamscores[i].team, teamscores[i].flagscore, teamscores[i].score);
            else s_sprintf(s)("[ %s: %d ]", teamscores[i].team, teamscores[i].score);
            s_strcat(teamline, s);
        }
		menumanual(menu, scorelines.length(), teamline);
    }
}

// sendmap/getmap commands, should be replaced by more intuitive map downloading

void sendmap(char *mapname)
{
    if(*mapname)
    {
        save_world(mapname);
        changemap(mapname);
    }    
    mapname = getclientmap();
    int mapsize;
    uchar *mapdata = readmap(mapname, &mapsize); 
    if(!mapdata) return;
    ENetPacket *packet = enet_packet_create(NULL, MAXTRANS + mapsize, ENET_PACKET_FLAG_RELIABLE);
    ucharbuf p(packet->data, packet->dataLength);
    putint(p, SV_SENDMAP);
    sendstring(mapname, p);
    putint(p, mapsize);
    if(65535 - p.length() < mapsize)
    {
        conoutf("map %s is too large to send", mapname);
        delete[] mapdata;
        enet_packet_destroy(packet);
        return;
    }
    p.put(mapdata, mapsize);
    delete[] mapdata; 
    enet_packet_resize(packet, p.length());
    sendpackettoserv(2, packet);
    conoutf("sending map %s to server...", mapname);
    s_sprintfd(msg)("[map %s uploaded to server, \"getmap\" to receive it]", mapname);
    toserver(msg);
}

void getmap()
{
    ENetPacket *packet = enet_packet_create(NULL, MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
    ucharbuf p(packet->data, packet->dataLength);
    putint(p, SV_RECVMAP);
    enet_packet_resize(packet, p.length());
    sendpackettoserv(2, packet);
    conoutf("requesting map from server...");
}

COMMAND(sendmap, ARG_1STR);
COMMAND(getmap, ARG_NONE);

