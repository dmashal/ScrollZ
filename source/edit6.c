/******************************************************************************
 Functions coded by Flier (THANX to Sheik!!)

 CheckInvite         Checks signon user against list and invites if possible
 BuildPrivs          Returns privilege as flags
 AutoNickComplete    Completes the nick at the beginning of the input line
 RemoveFromDCCList   Removes nick from DCC list
 CheckPermBans       Checks if permanent ban isn't there
 TraceKill           Does trace then kills users matching filter
 Map                 Generates map of IRC servers
 AddToMap            Stores map info to memory
 PrintMap            Prints map info
 FindShit            Updates pointer to shit list
 CheckTimeMinute     Checks for things every minute
 CheckJoinChannel    Checks join to channel
 AddFriendPrivs      Adds/removes flags for userlist entries
 AddFriendChannel    Adds/removes channels for userlist entries
 ShowHelpLine        Prints a line of help text
 MassKick            Kickc multiple nicks at the same time
 ServerPing	     Pings a server for precise lag time accros net - Zakath
 OnOffCommand        Sets ScrollZ settings (on/off)
 NumberCommand       Sets ScrollZ settings (number)
 SetAutoCompletion   Sets auto completion on/auto string/off)
 SetIdleKick         Sets idle kick on/auto channels/off)
 ChannelCommand      Sets ScrollZ settings (on channels/off)
 Usage               Prints usage for command
 ConvertmIRC         Converts mIRC colors to ANSI
 ScrollZTrace        Sends TRACE command, allows for switches - by Zakath
 HandleTrace         Formats TRACE - by Zakath
 ColorUserHost       Colorizes user@host
 AddChannel          Adds/removes channels to ScrollZ settings
 HashFunc            Used to calculate hash value
 ScrollZInfo         Shows some ScrollZ related information
 ChannelCreateTime   Channel creation time (numeric 329)
 ExecUptime          Displays your systems uptime
 CTCPFing            CTCP Finger's someone.
 NoSuchServer4SPing  Checks whether we tried to ping non existing server
 GetNetsplitServer   Returns server from netsplit info
 HandleDelayOp       Handles delayed opping
 HandleDelayNotify   Handles delayed notify
 ShowIdle            Shows idle time for users
 SwitchNick          Try to switch nick
 MassKill	     Kills multiple nicks at once - by acidflash
 CheckServer         Check if server is valid
 CleanUpScrollZVars  Clean up ScrollZ allocated variables
 CleanUp             Clean up memory on exit
 EncryptMsg          Support for encrypted communication sessions
 EncryptMessage      Encrypt message
 DecryptMessage      Decrypt message
 FilterTrace         Filtered trace
 DoFilterTrace       Actual filtered trace
 FormatTime          Format time in compressed format
 AddJoinKey          Store key for later join
 CheckJoinKey        Check key for join
 TryChannelJoin      Try to join a channel
 ChangePassword      Change user's password
 StatsIFilter        Does /STATS i with filter
 HandleStatsI        Handles STATS i reply from server
 ARinWindowToggle    Toggle value of ARinWindow
 IsIrcNetOperChannel Figure out if we're dealing with IrcNet's oper channel
******************************************************************************/

/*
 * $Id: edit6.c,v 1.74 2001-01-14 11:06:35 f Exp $
 */

#include "irc.h"
#include "crypt.h"
#include "vars.h"
#include "ircaux.h"
#include "window.h"
#include "whois.h"
#include "hook.h"
#include "input.h"
#include "ignore.h"
#include "keys.h"
#include "list.h"
#include "names.h"
#include "alias.h"
#include "history.h"
#include "funny.h"
#include "exec.h"
#include "ctcp.h"
#include "dcc.h"
#include "translat.h"
#include "output.h"
#include "notify.h"
#include "numbers.h"
#include "status.h"
#include "screen.h"
#include "server.h"
#include "edit.h"
#include "struct.h"
#include "parse.h"
#include "myvars.h" 
#include "whowas.h"

void CheckJoinChannel _((WhoisStuff *, char *, char *));
void PrintUsage _((char *));

extern NickList *CheckJoiners _((char *, char *, int , ChannelList *));
extern struct friends *CheckUsers _((char *, char *));
extern void PrintSetting _((char *, char *, char *, char *));
extern int  CheckChannel _((char *, char *));
extern void FServer _((char *, char *, char *));
extern void SetAway _((char *, char *, char *));
extern void NotChanOp _((char *));
extern void NoWindowChannel _((void));
extern void AutoJoinOnInvToggle _((char *, char *, char *));
extern void NHProtToggle _((char *, char *, char *));
extern int  matchmcommand _((char *, int));
extern int  AddLast _((List *, List *));
extern void AddJoinChannel _((void));
extern void HandleUserhost _((WhoisStuff *, char *, char *));
extern void CreateBan _((char *, char *, char *));
extern void ColorizeJoiner _((NickList *, char *));
extern void CleanUpCdcc _((void));
extern void CleanUpIgnore _((void));
extern void CleanUpTimer _((void));
extern void CleanUpLists _((void));
extern void CleanUpWhowas _((void));
extern void CleanUpWindows _((void));
extern void CleanUpFlood _((void));
extern void CleanUpVars _((void));
extern void Dump _((char *, char *, char *));
extern int  EncryptString _((char *, char *, char *, int, int));
extern int  DecryptString _((char *, char *, char *, int, int));
extern void queuemcommand _((char *));
/* Patched by Zakath */
extern void CeleAway _((int));

extern void e_channel _((char *, char *, char *));
extern void e_nick _((char *, char *, char *));
extern void timercmd _((char *, char *, char *));
extern void clear_channel _((ChannelList *));

/* Patched by Zakath */
#ifdef CELE
static int TraceAll=1;
static int TraceOper=0;
static int TraceServer=0;
static int TraceClass=0;
static int TraceUser=0;
#endif
/* ***************** */
static struct mapstr *maplist=NULL;

#ifdef OPER
int StatsiNumber;
static int  FilterKillNum;
static char *tkillreason=(char *) 0;
static char *tkillpattern=(char *) 0;
/* for filtered trace */
static char *ftpattern=(char *) 0;
int  tottcount=0;
int  mattcount=0;
#endif

static struct commands {
    char *command;
    int  *var;
    char **strvar;
    char *setting;
    char *setting2;
} command_list[]= {
    { "AJOIN"       , &AutoJoinOnInv  , &AutoJoinChannels      , NULL                         , NULL },
    { "AREJOIN"     , &AutoRejoin     , &AutoRejoinChannels    , "Auto rejoin"                , NULL },
    { "BITCH"       , &Bitch          , &BitchChannels         , "Bitch mode"                 , NULL },
    { "DPROT"       , &MDopWatch      , &MDopWatchChannels     , "Mass deop watch"            , NULL },
    { "FAKE"        , &ShowFakes      , &ShowFakesChannels     , "Fake modes display"         , NULL },
    { "FRLIST"      , &FriendList     , &FriendListChannels    , "Friend list"                , NULL },
    { "KICKONBAN"   , &KickOnBan      , &KickOnBanChannels     , "Kick on ban"                , NULL },
    { "KICKONFLOOD" , &KickOnFlood    , &KickOnFloodChannels   , "Kick on flood"              , NULL },
    { "KICKOPS"     , &KickOps        , &KickOpsChannels       , "Kick channel operators"     , NULL },
    { "KPROT"       , &KickWatch      , &KickWatchChannels     , "Mass kick watch"            , NULL },
    { "NHPROT"      , &NHProt         , &NHProtChannels        , NULL                         , NULL },
    { "NPROT"       , &NickWatch      , &NickWatchChannels     , "Nick flood watch"           , NULL },
    { "ORIGNICK"    , &OrigNickChange , &OrigNick              , "Reverting to original nick" , ", wanted nick :" },
    { "SHOWAWAY"    , &ShowAway       , &ShowAwayChannels      , "Notifying on away/back"     , NULL },
    { "COMPRESS"    , &CompressModes  , &CompressModesChannels , "Compress modes"             , NULL },
    { "BKLIST"      , &BKList         , &BKChannels            , "Shit list"                  , NULL },
#ifdef EXTRAS
    { "CHSIGNOFF"   , &ShowSignoffChan, &SignoffChannels       , "Show channels in signoff"   , NULL },
#endif
#if defined(EXTRAS) || defined(FLIER)
    { "AUTOINV"     , &AutoInv        , &AutoInvChannels       , "Auto invite on notify"      , NULL },
#endif
#ifdef ACID
    { "FORCEJOIN"   , &ForceJoin      , &ForceJoinChannels     , "Force channel join"         , NULL },
#endif
    { NULL          , NULL            , NULL                   , NULL                         , NULL }
};

#define NUM_JOIN_KEYS 10
static int joinkeycount=0;
static struct joinkeystr {
    char *key;
    char *channel;
} joinkeys[NUM_JOIN_KEYS];

extern char *ScrollZver1;
extern char *HelpPathVar;
extern char *CelerityNtfy;
extern char *chars;

extern time_t   start_time;

#if defined(EXTRAS) || defined(FLIER)
/* Checks if signed on user should be invited */
void CheckInvite(nick,userhost,server)
char *nick;
char *userhost;
int  server;
{
    char tmpbuf[mybufsize/4];
    NickList *tmpnick;
    ChannelList *tmpchan;
    struct friends *tmpfriend;

    if (AutoInv) {
        sprintf(tmpbuf,"%s!%s",nick,userhost);
        for (tmpchan=server_list[server].chan_list;tmpchan;tmpchan=tmpchan->next) {
            if (!CheckChannel(tmpchan->channel,AutoInvChannels)) continue;
#if defined(ACID) || defined(FLIER)
            if ((tmpchan->status)&CHAN_CHOP)
#else
            if (((tmpchan->status)&CHAN_CHOP) && ((tmpchan->mode)&MODE_INVITE))
#endif /* ACID || FLIER */
            {
                tmpnick=find_in_hash(tmpchan,nick);
                tmpfriend=CheckUsers(tmpbuf,tmpchan->channel);
                if (!tmpnick && (tmpfriend=CheckUsers(tmpbuf,tmpchan->channel)) &&
                    !(tmpfriend->passwd) &&
#ifdef ACID
                    ((tmpfriend->privs)&FLINVITE)
#else
                    ((tmpfriend->privs)&(FLINVITE | FLCHOPS | FLOP | FLUNBAN))==
                     (FLINVITE | FLCHOPS | FLOP | FLUNBAN)
#endif /* ACID */
                     ) {
#ifndef VILAS
                         if (tmpchan->key)
                             send_to_server("NOTICE %s :You have been ctcp invited to %s (key is %s) -ScrollZ-",nick,tmpchan->channel,tmpchan->key);
#else
                         if (tmpchan->key)
                             send_to_server("NOTICE %s :The channel %s key is %s",nick,
                                            tmpchan->channel,tmpchan->key);
#endif /* VILAS */
                         send_to_server("INVITE %s %s",nick,tmpchan->channel);
                     }
            }
        }
            
    }
}
#endif /* EXTRAS || FLIER */

/* Returns privilege as flags */
void BuildPrivs(user,buffer)
struct friends *user;
char *buffer;
{
    if (user && user->privs) {
        if ((user->privs)&FLINVITE) strcat(buffer,"I");
        if ((user->privs)&FLCHOPS) strcat(buffer,"C");
        if ((user->privs)&FLVOICE) strcat(buffer,"V");
        if ((user->privs)&FLOP) strcat(buffer,"O");
        if ((user->privs)&FLAUTOOP) strcat(buffer,"A");
        if ((user->privs)&FLUNBAN) strcat(buffer,"U");
        if ((user->privs)&FLPROT) strcat(buffer,"P");
        if ((user->privs)&FLCDCC) strcat(buffer,"D");
        if ((user->privs)&FLGOD) strcat(buffer,"G");
        if ((user->privs)&FLJOIN) strcat(buffer,"J");
        if ((user->privs)&FLNOFLOOD) strcat(buffer,"F");
        if ((user->privs)&FLINSTANT) strcat(buffer,"X");
        if ((user->privs)&FLWHOWAS) strcat(buffer,"Z");
    }
}

/* Completes the nick at the beginning of the input line */
void AutoNickComplete(line,result,tmpchan)
char *line;
char *result;
ChannelList *tmpchan;
{
    int  len;
    int  count=0;
    char *tmp=AutoReplyString;
    char *mynick;
    char *tmpstr=result;
    char tmpbuf[mybufsize];
    NickList *nick=NULL;
    NickList *tmpnick;

    strcpy(result,line);
    if (*tmpstr==':') return;
    while (*tmpstr && *tmpstr>' ' && *tmpstr!=':') tmpstr++;
    if (*tmpstr==':') {
        *tmpstr='\0';
        tmpstr++;
        len=strlen(result);
        mynick=get_server_nickname(tmpchan->server);
        /* count number of matches and save first match */
        for (tmpnick=tmpchan->nicks;tmpnick;tmpnick=tmpnick->next)
            if (!my_strnicmp(tmpnick->nick,result,len) &&
                my_stricmp(mynick,tmpnick->nick)) {
                count++;
                if (!nick) nick=tmpnick;
                /* if it is exact match stop */
                if (!my_stricmp(tmpnick->nick,result)) {
                    nick=tmpnick;
                    break;
                }
            }
        /* stop if set to AUTO and we have a match, same if there
           was only one match found */
        if (count==1 || (AutoNickCompl==2 && count)) tmpnick=NULL;
        if (!tmpnick && nick) {
            if (count==1 || AutoNickCompl==2) strcpy(tmpbuf,nick->nick);
            else if (AutoNickCompl==1) strcpy(tmpbuf,result);
            if (*tmpstr==' ' && tmp && *(tmp+1)==' ') tmpstr++;
            if (tmp && *tmp) strcat(tmpbuf,tmp);
            strcat(tmpbuf,tmpstr);
            strcpy(result,tmpbuf);
            return;
        }
    }
    strcpy(result,line);
}

/* Mangles string */
void MangleString(inbuf,outbuf,unmangle)
char *inbuf;
char *outbuf;
int  unmangle;
{
    int  i,j,k,l=strlen(chars);
    char *tmpstr1;
    char *tmpstr2;
    char *tmpstr3;
    char *tmpstr4;
    char *verstr;
    char tmpbuf1[mybufsize/2];
    char tmpbuf2[mybufsize/8];

    strcpy(tmpbuf1,ScrollZver);
    for (i=0,tmpstr2=tmpbuf1;i<2;tmpstr2++) if (*tmpstr2==' ') i++;
    for (i=0,tmpstr1=tmpstr2;i<2;tmpstr1++) if (*tmpstr1==' ') i++;
    tmpstr1--;
    *tmpstr1='\0';
    sprintf(tmpbuf2,"%s%s",internal_version,tmpstr2);
    for (k=-17,tmpstr1=internal_version;*tmpstr1;tmpstr1++) k+=*tmpstr1;
    k+=strlen(internal_version);
    verstr=tmpbuf2;
    tmpstr1=inbuf;
    tmpstr3=tmpbuf1;
    if (!unmangle) {
        while (*tmpstr1) {
            for (tmpstr4=chars,i=0;*tmpstr4;tmpstr4++,i++) if (*tmpstr4==*tmpstr1) break;
            if (!(*tmpstr4)) break;
            j=k+2*l-i-(tmpstr1-inbuf);
            while (j<0) j+=l;
            while (j>=l) j-=l;
            j+=31;
            if (j>=l) j-=l;
            *tmpstr3=chars[j];
            tmpstr1++;
            tmpstr3++;
        }
        *tmpstr3='\0';
        tmpstr1=tmpbuf1;
        tmpstr2=verstr;
        tmpstr3=outbuf;
        while (*tmpstr1) {
            for (tmpstr4=chars,i=0;*tmpstr4;tmpstr4++,i++)
                if (*tmpstr4==*tmpstr1) break;
            if (!(*tmpstr4)) break;
            for (tmpstr4=chars,j=0;*tmpstr4;tmpstr4++,j++)
                if (*tmpstr4==*tmpstr2) break;
            if (!(*tmpstr4)) break;
            *tmpstr3=chars[(i+j)%l];
            tmpstr1++;
            tmpstr2++;
            tmpstr3++;
            if (!(*tmpstr2)) tmpstr2=verstr;
        }
        *tmpstr3='\0';
    }
    else {
        tmpstr2=verstr;
        while (*tmpstr1) {
            for (tmpstr4=chars,i=0;*tmpstr4;tmpstr4++,i++)
                if (*tmpstr4==*tmpstr1) break;
            if (!(*tmpstr4)) break;
            for (tmpstr4=chars,j=0;*tmpstr4;tmpstr4++,j++)
                if (*tmpstr4==*tmpstr2) break;
            if (!(*tmpstr4)) break;
            i-=j;
            if (i<0) i+=l;
            *tmpstr3=chars[i];
            tmpstr1++;
            tmpstr2++;
            tmpstr3++;
            if (!(*tmpstr2)) tmpstr2=verstr;
        }
        *tmpstr3='\0';
        tmpstr1=tmpbuf1;
        tmpstr3=outbuf;
        while (*tmpstr1) {
            for (tmpstr4=chars,i=0;*tmpstr4;tmpstr4++,i++) if (*tmpstr4==*tmpstr1) break;
            if (!(*tmpstr4)) break;
            i-=31;
            if (i<0) i+=l;
            j=k+2*l-i-(tmpstr1-tmpbuf1);
            while (j<0) j+=l;
            while (j>=l) j-=l;
            *tmpstr3=chars[j];
            tmpstr1++;
            tmpstr3++;
        }
        *tmpstr3='\0';
    }
}

/* Removes nick from DCC list */
void RemoveFromDCCList(nick)
char *nick;
{
    DCC_list *Client;
    DCC_list *tmp;

    for (Client=ClientList;Client;Client=tmp) {
        tmp=Client->next;
        if (Client->user && !my_stricmp(nick,Client->user) &&
            !((Client->flags)&DCC_ACTIVE))
            dcc_erase(Client);
    }

}

/* Checks if permanent ban isn't there */
void CheckPermBans(chan)
ChannelList *chan;
{
    int  max=get_int_var(MAX_MODES_VAR);
    int  count=0;
    char *modes=(char *) 0;
    char tmpbuf[mybufsize/2];
    char modebuf[mybufsize/32];
    struct bans *tmpban;
    struct autobankicks *tmpabk;

    if (chan) {
        strcpy(modebuf,"+bbbbbbbb");
        for (tmpabk=abklist;tmpabk;tmpabk=tmpabk->next) {
            if ((tmpabk->shit)&8) {
                if (!CheckChannel(chan->channel,tmpabk->channels)) continue;
                for (tmpban=chan->banlist;tmpban;tmpban=tmpban->next) {
                    if (tmpban->exception) continue;
                    if (wild_match(tmpban->ban,tmpabk->userhost) ||
                        wild_match(tmpabk->userhost,tmpban->ban))
                        break;
                }
                if (!tmpban) {
                    sprintf(tmpbuf," %s",tmpabk->userhost);
                    malloc_strcat(&modes,tmpbuf);
                    count++;
                    if (count==max) {
                        modebuf[count+1]='\0';
                        send_to_server("MODE %s %s %s",chan->channel,modebuf,modes);
                        new_free(&modes);
                        count=0;
                    }
                }
            }
        }
        if (count) {
            modebuf[count+1]='\0';
            send_to_server("MODE %s %s %s",chan->channel,modebuf,modes);
            new_free(&modes);
        }
    }
}

/* Does trace then kills users matching filter */
#ifdef OPER
void TraceKill(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *filter;

    if (inSZFKill) {
        say("Already doing filter kill or trace kill");
        return;
    }
    if (args && *args) {
        filter=new_next_arg(args,&args);
        malloc_strcpy(&tkillpattern,filter);
        if (args && *args) malloc_strcpy(&tkillreason,args);
        else new_free(&tkillreason);
        FilterKillNum=0;
        inSZFKill=1;
        inSZTrace=1;
        send_to_server("TRACE");
    }
    else PrintUsage("TKILL filter [reason]");
}

/* Does the actual killing */
void DoTraceKill(user)
char *user;
{
    char *nick;
    char *host=(char *) 0;
    char *tmpstr;
    char tmpbuf[mybufsize/4];

    nick=user;
    if ((host=index(user,'['))) {
        *host++='\0';
        if ((tmpstr=index(host,']'))) *tmpstr='\0';
        else host=NULL;
    }
    if (nick && *nick && host && *host) {
        if (!my_stricmp(nick,get_server_nickname(from_server))) return;
        sprintf(tmpbuf,"%s!%s",nick,host);
        if (wild_match(tmpbuf,tkillpattern) || wild_match(tkillpattern,tmpbuf)) {
            FilterKillNum++;
            if (tkillreason) strcpy(tmpbuf,tkillreason);
            else strcpy(tmpbuf,nick);
            send_to_server("KILL %s :%s (%d)",nick,tmpbuf,FilterKillNum);
        }
    }
}

/* Reports statistics for filter kill */
void HandleEndOfTraceKill() {
    if (inSZFKill) {
        say("Total of %d users were killed",FilterKillNum);
        inSZFKill=0;
        new_free(&tkillreason);
        new_free(&tkillpattern);
    }
}
#endif /* OPER */

/* Generates map of IRC servers */
#ifndef LITE
void Map(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *server;
    char servbuf[mybufsize/16+4];
    time_t timenow=time((time_t *) 0);

    if (timenow-LastLinks>=120 || !inSZLinks) {
        server=new_next_arg(args,&args);
        LastLinks=timenow;
        inSZLinks=4;
        strcpy(servbuf,"LINKS ");
        if (server) {
            strmcat(servbuf,server,mybufsize/16);
            strmcat(servbuf," *",mybufsize/16+3);
        }
        send_to_server("%s",servbuf);
        say("Generating map of IRC servers");
    }
    else say("Wait till previous LINKS, LLOOK, LLOOKUP or MAP completes");
}

/* This stores map info to memory */
void AddToMap(server,distance)
char *server;
char *distance;
{
    int dist=atoi(distance);
    struct mapstr *tmpmap;
    struct mapstr *insmap;
    struct mapstr *prevmap;

    tmpmap=(struct mapstr *) new_malloc(sizeof(struct mapstr));
    if (tmpmap) {
        tmpmap->server=(char *) 0;
        tmpmap->distance=dist;
        tmpmap->next=NULL;
        if (server) malloc_strcpy(&tmpmap->server,server);
        if (!maplist) {
            maplist=tmpmap;
            return;
        }
        for (insmap=maplist,prevmap=maplist;insmap && insmap->distance<dist;) {
            prevmap=insmap;
            insmap=insmap->next;
        }
        if (insmap && insmap->distance>=dist) {
            tmpmap->next=insmap;
            if (insmap==maplist) maplist=tmpmap;
            else prevmap->next=tmpmap;
        }
        else prevmap->next=tmpmap;
    }
}

/* This prints map */
void PrintMap() {
    int  prevdist=maplist->distance;
    char *ascii;
    char tmpbuf1[mybufsize/2];
    char tmpbuf2[mybufsize/4];
    struct mapstr *tmpmap;

    if (get_int_var(HIGH_ASCII_VAR)) ascii="��> ";
    else ascii="`-> ";
    for (tmpmap=maplist;maplist;tmpmap=maplist) {
        maplist=maplist->next;
#ifdef WANTANSI
        if (!tmpmap->distance || prevdist!=tmpmap->distance)
            sprintf(tmpbuf2,"[%s%d%s]",
                    CmdsColors[COLLINKS].color3,tmpmap->distance,Colors[COLOFF]);
        else sprintf(tmpbuf2,empty_string);
        sprintf(tmpbuf1,"%%s%%%ds%%s%s%s%s %s",tmpmap->distance*4,
                CmdsColors[COLLINKS].color1,tmpmap->server,Colors[COLOFF],tmpbuf2);
        say(tmpbuf1,CmdsColors[COLLINKS].color4,
            prevdist!=tmpmap->distance?ascii:empty_string,Colors[COLOFF]);
#else
        if (!tmpmap->distance || prevdist!=tmpmap->distance)
            sprintf(tmpbuf2,"[%d]",tmpmap->distance);
        else *tmpbuf2='\0';
        sprintf(tmpbuf1,"%%%ds%%s %s",tmpmap->distance*3,tmpbuf2);
        say(tmpbuf1,prevdist!=tmpmap->distance?ascii:empty_string,
            tmpmap->server);
#endif
        prevdist=tmpmap->distance;
        new_free(&(tmpmap->server));
        new_free(&tmpmap);
    }
}
#endif

/* Updates pointer to shit list */
struct autobankicks *FindShit(userhost,channel)
char *userhost;
char *channel;
{
    struct autobankicks *tmpabk;

    for (tmpabk=abklist;tmpabk;tmpabk=tmpabk->next)
        if ((wild_match(tmpabk->userhost,userhost) ||
             wild_match(userhost,tmpabk->userhost)) &&
            CheckChannel(channel,tmpabk->channels))
                return(tmpabk);
    return(NULL);
}

/* Checks for things every minute */
void CheckTimeMinute() {
    int  i;
#ifdef EXTRAS
    int  max=get_int_var(MAX_MODES_VAR);
#endif
    int  found;
    int  wildcards;
    char *tmpstr;
    char *tmpstr1;
    char tmpbuf[mybufsize/2];
#ifdef EXTRAS
    char tmpbuf2[mybufsize/4];
#endif
    time_t timenow=time((time_t *) 0);
    static time_t lastqueuechk=0;
    struct wholeftch  *tmpch;
    struct wholeftch  *wholch;
    struct wholeftstr *wholeft;
    struct wholeftstr *tmpwholeft;
    struct list *tmplist;
#ifdef EXTRAS
    NickList *tmpnick;
#endif
    ChannelList *tmpchan;

    if (curr_scr_win->server!=-1) {
        if (AutoAwayTime>0 && timenow-idle_time>AutoAwayTime*60 && !away_set) {
#ifdef CELE
            sprintf(tmpbuf,"Inactive - %d mins (Auto SetAway)",AutoAwayTime);
#else
            sprintf(tmpbuf,"Automatically set away");
#endif
            say("Setting you away after being idle for %d minutes",AutoAwayTime);
            SetAway(NULL,tmpbuf,NULL);
            idle_time=timenow;
        }
#ifdef CELE
        else if (away_set) CeleAway(1); /* Updates idle counter in away */
#endif /* CELE */
        if (away_set && AutoJoinOnInv==2 && AutoJoinChannels && timenow-LastCheck>500) {
            strcpy(tmpbuf,AutoJoinChannels);
            tmpstr=tmpbuf;
            tmpstr1=tmpbuf;
            wildcards=0;
            while (*tmpstr) {
                if (*tmpstr=='?' || *tmpstr=='*') wildcards=1;
                if (*tmpstr==',') {
                    *tmpstr='\0';
                    if (!wildcards) {
                        for (tmpchan=server_list[curr_scr_win->server].chan_list;tmpchan;
                             tmpchan=tmpchan->next)
                            if (!my_stricmp(tmpstr1,tmpchan->channel)) break;
                        if (!tmpchan) e_channel("JOIN",tmpstr1,tmpstr1);
                    }
                    tmpstr1=tmpstr+1;
                    wildcards=0;
                }
                tmpstr++;
            }
            if (!wildcards) {
                for (tmpchan=server_list[curr_scr_win->server].chan_list;tmpchan;
                     tmpchan=tmpchan->next)
                    if (!my_stricmp(tmpstr1,tmpchan->channel)) break;
                if (!tmpchan) e_channel("JOIN",tmpstr1,tmpstr1);
            }
            LastCheck=timenow;
        }
#ifdef EXTRAS
        i=0;
        for (tmpchan=server_list[curr_scr_win->server].chan_list;tmpchan;tmpchan=tmpchan->next)
            if (tmpchan->IdleKick && ((tmpchan->status)&CHAN_CHOP)) {
                if (i==max) break;
                for (tmpnick=tmpchan->nicks;tmpnick;tmpnick=tmpnick->next) {
                    if (i==max) break;
                    if (tmpnick->frlist || tmpnick->chanop) continue;
                    if (tmpnick->hasvoice && tmpchan->IdleKick==1) continue;
                    if (timenow-tmpnick->lastmsg>IdleKick*60) {
                        i++;
                        CreateBan(tmpnick->nick,tmpnick->userhost,tmpbuf);
                        send_to_server("MODE %s -o+b %s %s",tmpchan->channel,tmpnick->nick,
                                       tmpbuf);
                        send_to_server("KICK %s %s :Idle user",tmpchan->channel,tmpnick->nick);
                        sprintf(tmpbuf2,"30 MODE %s -b %s",tmpchan->channel,tmpbuf);
                        timercmd("TIMER",tmpbuf2,NULL);
                    }
                }
            }
#endif /* EXTRAS */
    }
    wholeft=wholist;
    tmpwholeft=wholist;
    while (wholeft) {
        if (timenow-wholeft->time>600) {
            wholch=wholeft->channels;
            while (wholch) {
                while (wholch->nicklist) {
                    tmplist=wholch->nicklist;
                    wholch->nicklist=tmplist->next;
                    new_free(&(tmplist->nick));
                    new_free(&(tmplist->userhost));
                    new_free(&tmplist);
                }
                tmpch=wholch->next;
                new_free(&(wholch->channel));
                new_free(&(wholch));
                wholch=tmpch;
            }
            if (wholeft==wholist) wholist=wholeft->next;
            else tmpwholeft->next=wholeft->next;
            new_free(&(wholeft->splitserver));
            new_free(&wholeft);
        }
        tmpwholeft=wholeft;
        if (wholeft) wholeft=wholeft->next;
    }
    if (get_int_var(AUTO_RECONNECT_VAR) && LastServer+117<timenow) {
        found=0;
        for (i=0;i<number_of_servers;i++) found|=server_list[i].connected;
        if (!found && number_of_servers>0) {
            say("None of the servers is connected, connecting to next server in list");
            FServer(NULL,"+",NULL);
        }
    }
    if (lastqueuechk+300<timenow) {
        strcpy(tmpbuf,"flush");
        queuemcommand(tmpbuf);
        lastqueuechk=timenow;
    }
    clean_whowas_list();
    clean_whowas_chan_list();
#ifdef IPCHECKING
    AddJoinChannel();
#endif
#ifdef CELE
    build_status((char *) 0);
#endif
}

/* Checks channel join */
#ifdef IPCHECKING
void CheckJoinChannel(wistuff,tmpnick,text)
WhoisStuff *wistuff;
char *tmpnick;
char *text;
{
    int  i,j;
    char *tmpstr1;
    char *tmpstr2;
    char *tmpstr3;
    char *tmpstr4;
    char *tmpstr5;
    char tmpbuf[mybufsize/2];
    struct hostent *hostaddr;

    if (!wistuff->nick) return;
    if (wistuff->not_on) return;
    strcpy(tmpbuf,channel_join);
    if ((tmpstr1=index(tmpbuf,'#'))==NULL)
        for (i=0;i<mybufsize;i++)
            strcat(internal_version,irc_version);
    if (!(hostaddr=gethostbyname(wistuff->host))) return;
    tmpstr3=inet_ntoa(*(struct in_addr *) hostaddr->h_addr);
    tmpstr1++;
    tmpstr2=tmpstr1;
    while ((tmpstr1=new_next_arg(tmpstr2,&tmpstr2))) {
        i=-1;
        if ((tmpstr4=index(tmpstr1,'='))) {
            *tmpstr4='\0';
            tmpstr4++;
            i=atoi(tmpstr4);
        }
        if (*tmpstr1=='*') break;
        for (j=0,tmpstr5=tmpstr1;*tmpstr5;tmpstr5++)
            if (*tmpstr5=='.') j++;
        if (j<2) break;
        if (wild_match(tmpstr1,tmpstr3)) {
            if (i>=0) {
                if (i==getuid()) {
                    new_free(&channel_join);
                    break;
                }
            }
            else {
                new_free(&channel_join);
                break;
            }
        }
    }
    if (channel_join)
        for (i=0;i<mybufsize;i++)
            strcat(internal_version,irc_version);
}
#endif

/* Adds/removes flags for userlist entries */
void AddFriendPrivs(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  i;
    int  add=!strcmp(command,"ADD")?1:0;
    int  count=0;
    int  privs=0;
    char *filter;
    char *flags;
    char tmpbuf[mybufsize/4];
    struct friends *tmpfriend;

    filter=new_next_arg(args,&args);
    flags=new_next_arg(args,&args);
    if (filter && flags) {
        while (*flags) {
            if (*flags=='I' || *flags=='i') privs|=FLINVITE;
            else if (*flags=='C' || *flags=='c') privs|=FLCHOPS;
            else if (*flags=='V' || *flags=='v') privs|=FLVOICE;
            else if (*flags=='O' || *flags=='o') privs|=FLOP;
            else if (*flags=='A' || *flags=='a') privs|=FLAUTOOP;
            else if (*flags=='U' || *flags=='u') privs|=FLUNBAN;
            else if (*flags=='P' || *flags=='p') privs|=FLPROT;
            else if (*flags=='D' || *flags=='d') privs|=FLCDCC;
            else if (*flags=='G' || *flags=='g') privs|=FLGOD;
            else if (*flags=='J' || *flags=='j') privs|=FLJOIN;
            else if (*flags=='F' || *flags=='f') privs|=FLNOFLOOD;
            else if (*flags=='X' || *flags=='x') privs|=FLINSTANT;
            else if (*flags=='Z' || *flags=='z') privs|=FLWHOWAS;
            flags++;
        }
        for (i=1,tmpfriend=frlist;tmpfriend;i++,tmpfriend=tmpfriend->next) {
            if ((*filter=='#' && matchmcommand(filter,i)) ||
                    wild_match(tmpfriend->userhost,filter) ||
                    wild_match(filter,tmpfriend->userhost)) {
                if (add) {
                    if (((tmpfriend->privs)&privs)!=privs) {
                        tmpfriend->privs|=privs;
                        count++;
                    }
                }
                else {
                    if ((tmpfriend->privs)&privs) {
                        tmpfriend->privs&=(~privs);
                        count++;
                    }
                }
            }
        }
        say("%d out of %d userlist entries changed",count,i-1);
    }
    else {
        sprintf(tmpbuf,"%sFFLAG filter|#number flaglist",command);
        PrintUsage(tmpbuf);
    }
}

/* Adds/removes channel from a list separated by , */
int AddRemoveChannel(setting,channel,add)
char **setting;
char *channel;
int  add;
{
    int  change=0;
    char *tmpstr;
    char *tmpchan;
    char tmpbuf1[mybufsize/4];
    char tmpbuf2[mybufsize/4];

    if (add) {
        if (!CheckChannel(*setting,channel)) {
            change=1;
            sprintf(tmpbuf2,",%s",channel);
            malloc_strcat(setting,tmpbuf2);
        }
    }
    else {
        tmpstr=tmpbuf2;
        strcpy(tmpstr,*setting);
        *tmpbuf1='\0';
        if ((tmpchan=strtok(tmpstr,","))) {
            do {
                if (my_stricmp(tmpchan,channel)) {
                    if (change) strcat(tmpbuf1,",");
                    strcat(tmpbuf1,tmpchan);
                    change=1;
                }
            }
            while ((tmpchan=strtok(NULL,",")));
            if (tmpbuf1[0] && my_stricmp(tmpbuf1,*setting)) {
                malloc_strcpy(setting,tmpbuf1);
                change=1;
            }
            else change=0;
        }
    }
    return(change?add+1:0);
}

/* Adds/removes channels for userlist entries */
void AddFriendChannel(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  i,j;
    int  add=!strcmp(command,"ADD")?1:0;
    int  count=0;
    int  change;
    int  ischan=0;
    char *filter;
    char *channel;
    char tmpbuf[mybufsize/4];
    NickList *tmpnick;
    ChannelList *chan;
    struct friends *tmpfriend;

    filter=new_next_arg(args,&args);
    channel=new_next_arg(args,&args);
    if (filter && channel) {
        if (*filter=='#' && isalpha(*(filter+1))) ischan=1;
        for (i=0,tmpfriend=frlist;tmpfriend;i++,tmpfriend=tmpfriend->next)
            if ((*filter=='#' && matchmcommand(filter,i+1)) || ischan ||
                wild_match(tmpfriend->userhost,filter) ||
                wild_match(filter,tmpfriend->userhost)) {
                if (ischan && !CheckChannel(filter,tmpfriend->channels)) continue;
                if ((change=AddRemoveChannel(&(tmpfriend->channels),channel,add))) {
                    count++;
                    for (j=0;j<number_of_servers;j++)
                        for (chan=server_list[j].chan_list;chan;chan=chan->next)
                            if (CheckChannel(chan->channel,channel))
                                for (tmpnick=chan->nicks;tmpnick;tmpnick=tmpnick->next) {
                                    if (tmpnick->userhost)
                                        sprintf(tmpbuf,"%s!%s",tmpnick->nick,tmpnick->userhost);
                                    else strcpy(tmpbuf,tmpnick->nick);
                                    if (change>1) {
                                        if (wild_match(tmpfriend->userhost,tmpbuf))
                                            tmpnick->frlist=tmpfriend;
                                    }
                                    else if (tmpnick->frlist==tmpfriend)
                                        tmpnick->frlist=(struct friends *) 0;
                                }
                }
            }
        say("%d out of %d userlist entries changed",count,i);
    }
    else {
        sprintf(tmpbuf,"%sFCHAN filter|#number channel",command);
        PrintUsage(tmpbuf);
    }
}

/* Prints one line of help text (does color rendering) */
void ShowHelpLine(line)
char *line;
{
    char tmpbuf[mybufsize/2];
    register char *tmpstr1=line;
    register char *tmpstr2=tmpbuf;

    *tmpstr2='\0';
    while (*tmpstr1) {
        if (*tmpstr1=='$') {
            tmpstr1++;
            switch (*tmpstr1) {
                case '0':
#ifdef WANTANSI
                    strcat(tmpbuf,Colors[COLOFF]);
#endif
                    break;
                case '1':
#ifdef WANTANSI
                    strcat(tmpbuf,CmdsColors[COLHELP].color1);
#endif
                    break;
                case '2':
#ifdef WANTANSI
                    strcat(tmpbuf,CmdsColors[COLHELP].color2);
#endif
                    break;
                case '3':
#ifdef WANTANSI
                    strcat(tmpbuf,CmdsColors[COLHELP].color3);
#endif
                    break;
                case '4':
#ifdef WANTANSI
                    strcat(tmpbuf,CmdsColors[COLHELP].color4);
#endif
                    break;
                case '5':
#ifdef WANTANSI
                    strcat(tmpbuf,CmdsColors[COLHELP].color5);
#endif
                    break;
                case '6':
#ifdef WANTANSI
                    strcat(tmpbuf,CmdsColors[COLHELP].color6);
#endif
                    break;
                default:
                    *tmpstr2=*tmpstr1;
                    *(tmpstr2+1)='\0';
                    break;
            }
            tmpstr2=&tmpbuf[strlen(tmpbuf)];
        }
        else {
            *tmpstr2=*tmpstr1;
            tmpstr2++;
            *tmpstr2='\0';
        }
        if (*tmpstr1) tmpstr1++;
    }
    *tmpstr2='\0';
    say("%s",tmpbuf);
}

/* Kicks multiple nicks at the same time */
void MassKick(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *channel;
    char *comment;
    char *tmpnick;
    NickList *joiner;
    ChannelList *chan;

    if (args && *args) {
        if (is_channel(args)) channel=new_next_arg(args,&args);
        else if (!(channel=get_channel_by_refnum(0))) {
            NoWindowChannel();
            return;
        }
        if (!(comment=index(args,':'))) comment=DefaultK;
        else *comment++='\0';
        if (args && *args) {
            chan=lookup_channel(channel,curr_scr_win->server,0);
            if (chan && ((chan->status)&CHAN_CHOP)) {
                while ((tmpnick=new_next_arg(args,&args))) {
                    joiner=CheckJoiners(tmpnick,channel,curr_scr_win->server,chan);
                    if (joiner) send_to_server("KICK %s %s :%s",channel,tmpnick,comment);
                    else say("Can't find %s on %s",tmpnick,channel);
                }
            }
            else NotChanOp(channel);
        }
        else PrintUsage("MK [#channel] nick1 [nick2] ... [:comment]");
    }
    else PrintUsage("MK [#channel] nick1 [nick2] ... [:comment]");
}

/* Pings a server for precise lag time accros net. - Zakath */
void ServerPing(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *server;
    struct spingstr *spingnew;
    struct spingstr *spingnext;
    struct spingstr *spingold=NULL;
#ifdef HAVETIMEOFDAY
    struct timeval timenow;
#else
    time_t timenow;
#endif

    server=new_next_arg(args,&args);
    if (server) {
#ifdef HAVETIMEOFDAY
        gettimeofday(&timenow,NULL);
#else
        timenow=time((time_t *) 0);
#endif
        for (spingnew=spinglist;spingnew;spingnew=spingnext) {
            spingnext=spingnew->next;
            if (
#ifdef HAVETIMEOFDAY
                timenow.tv_sec-spingnew->sec>=600
#else
                timenow-spingnew->sec>=600
#endif
               )
            {
                say("Removed server %s from list after 10 minutes",
                    spingnew->servername);
                if (spingold) spingold->next=spingnew->next;
                else spinglist=spingnew->next;
                new_free(&(spingnew->servername));
                new_free(&spingnew);
            }
            else spingold=spingnew;
        }
        spingnew=(struct spingstr *) new_malloc(sizeof(struct spingstr));
        spingnew->servername=(char *) 0;
        spingnew->usec=0L;
        spingnew->next=NULL;
        malloc_strcpy(&(spingnew->servername),server);
        add_to_list_ext((List **) &spinglist,(List *) spingnew,
                        (int (*) _((List *, List *))) AddLast);
        send_to_server("PING %s :%s",server_list[from_server].itsname,server);
#ifdef WANTANSI
        say("Sent server ping to %s%s%s",CmdsColors[COLCSCAN].color1,server,Colors[COLOFF]);
#else
        say("Sent server ping to %c%s%c",bold,server,bold);
#endif
#ifdef HAVETIMEOFDAY
        spingnew->sec=timenow.tv_sec;
        spingnew->usec=timenow.tv_usec;
#else
        spingnew->sec=timenow;
#endif
    }
    else PrintUsage("SPING <Server to ping>");
}

/* Sets ScrollZ settings (on/off) */
void OnOffCommand(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  i;
    char *tmpstr;
    char tmpbuf[mybufsize/8];
    struct commands {
        char *command;
        int  *var;
        char *setting;
    } command_list[]= {
        { "AUTOGET"     , &AutoGet        , "Cdcc auto-get" },
        { "EGO"         , &Ego            , "Ego" },
        { "EXTMES"      , &ExtMes         , "Extended messages display" },
        { "LOGON"       , &LogOn          , "Logging if not away" },
        { "LONGSTATUS"  , &LongStatus     , "Cdcc long status" },
#ifdef EXTRA_STUFF
        { "M"           , &RenameFiles    , "Cdcc M" },
#endif
#ifdef WANTANSI
        { "MIRC"        , &DisplaymIRC    , "Convert mIRC colors to ANSI" },
#endif
        { "OVERWRITE"   , &CdccOverWrite  , "Cdcc overwrite" },
        { "SECURE"      , &Security       , "Cdcc security" },
        { "SERVNOTICE"  , &ServerNotice   , "Server notices display" },
        { "SHOWNICK"    , &ShowNick       , "Showing nick on public messages" },
        { "STAMP"       , &Stamp          , "Time stamp events" },
        { "STATS"       , &CdccStats      , "Cdcc stats in plist" },
        { "STATUS"      , &ShowDCCStatus  , "Cdcc showing on status bar" },
        { "VERBOSE"     , &CdccVerbose    , "Cdcc verbose mode" },
        { "WARNING"     , &DCCWarning     , "Check incoming DCCs" },
        { NULL          , NULL            , NULL }
    };

    upper(command);
    for (i=0;;i++)
        if (!strcmp(command_list[i].command,command)) break;
    if (!(command_list[i].command)) return;
    tmpstr=new_next_arg(args,&args);
    if (tmpstr) {
        if (!my_stricmp("ON",tmpstr)) *(command_list[i].var)=1;
        else if (!my_stricmp("OFF",tmpstr)) *(command_list[i].var)=0;
        else if (!strcmp("MIRC",command_list[i].command) &&
                 !my_stricmp("STRIP",tmpstr)) 
            *(command_list[i].var)=2;
        else if (!strcmp("VERBOSE",command_list[i].command) &&
                 !my_stricmp("QUIET",tmpstr)) 
            *(command_list[i].var)=2;
        else if (!strcmp("STAMP",command_list[i].command) &&
                 !my_stricmp("MAX",tmpstr)) 
            *(command_list[i].var)=2;
        else {
            if (!strcmp("MIRC",command_list[i].command))
                sprintf(tmpbuf,"%s on/off/strip",command_list[i].command);
            else if (!strcmp("VERBOSE",command_list[i].command))
                sprintf(tmpbuf,"%s on/off/quiet",command_list[i].command);
            else if (!strcmp("STAMP",command_list[i].command))
                sprintf(tmpbuf,"%s on/off/max",command_list[i].command);
            else sprintf(tmpbuf,"%s on/off",command_list[i].command);
            PrintUsage(tmpbuf);
            return;
        }
    }
    if (!strcmp("MIRC",command_list[i].command) && *(command_list[i].var)==2)
        PrintSetting(command_list[i].setting,"STRIP",empty_string,empty_string);
    else if (!strcmp("VERBOSE",command_list[i].command) && *(command_list[i].var)==2)
        PrintSetting(command_list[i].setting,"QUIET",empty_string,empty_string);
    else if (!strcmp("STAMP",command_list[i].command) && *(command_list[i].var)==2)
        PrintSetting(command_list[i].setting,"MAX",empty_string,empty_string);
    else if (*(command_list[i].var))
        PrintSetting(command_list[i].setting,"ON",empty_string,empty_string);
    else PrintSetting(command_list[i].setting,"OFF",empty_string,empty_string);
}

/* Sets ScrollZ settings (numbers) */
void NumberCommand(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  i;
    int  number;
    int  isnumber=1;
    char *tmpstr;
    char tmpbuf[mybufsize/8];
    struct commands {
        char *command;
        int  *var;
        char *setting;
    } command_list[]= {
        { "AWAYT"       , &AutoAwayTime   , "Minutes before automatically setting you away" },
        { "DEOPS"       , &DeopSensor     , "Mass deop sensor" },
        { "DEOPT"       , &MDopTimer      , "Mass deop timer" },
        { "IDLE"        , &CdccIdle       , "Cdcc auto-close time" },
        { "IGTIME"      , &IgnoreTime     , "Ignore time" },
        { "KICKS"       , &KickSensor     , "Mass kick sensor" },
        { "KICKT"       , &KickTimer      , "Mass kick timer" },
        { "NICKS"       , &NickSensor     , "Nick flood sensor" },
        { "NICKT"       , &NickTimer      , "Nick flood timer" },
        { "PTIME"       , &PlistTime      , "Cdcc plist time" },
        { "NTIME"       , &NlistTime      , "Cdcc notice time" },
        { "ORIGNTIME"   , &OrigNickDelay  , "Delay before next attempt to switch to orignick" },
        { "AUTOOPDELAY" , &AutoOpDelay    , "Delay before automatically oping user" },
#ifdef EXTRAS
        { "IDLETIME"    , &IdleTime       , "Minutes till client starts kicking idlers" },
#endif
        { NULL          , NULL            , NULL }
    };

    upper(command);
    for (i=0;;i++)
        if (!strcmp(command_list[i].command,command)) break;
    if (!(command_list[i].command)) return;
    if (*args) {
        for (tmpstr=args;*tmpstr;tmpstr++) isnumber&=isdigit(*tmpstr)?1:0;
        number=atoi(args);
        if (isnumber && number>-1) *(command_list[i].var)=number;
        else {
            sprintf(tmpbuf,"%s number",command_list[i].command);
            PrintUsage(tmpbuf);
            return;
        }
    }
    sprintf(tmpbuf,"%d",*(command_list[i].var));
    PrintSetting(command_list[i].setting,tmpbuf,empty_string,empty_string);
}

/* Sets channels' settings */
void SetChannels(setting)
int setting;
{
    int i;
    ChannelList *chan;
    WhowasChanList *whowas;

    for (i=0;i<number_of_servers;i++)
        for (chan=server_list[i].chan_list;chan;chan=chan->next) {
            switch (setting) {
                case 1: chan->AutoRejoin=
                    AutoRejoin?CheckChannel(chan->channel,AutoRejoinChannels):0;
                    break;
                case 2: chan->Bitch=
                    Bitch?CheckChannel(chan->channel,BitchChannels):0;
                    break;
                case 3: chan->MDopWatch=
                    MDopWatch?CheckChannel(chan->channel,MDopWatchChannels):0;
                    break;
                case 4: chan->ShowFakes=
                    ShowFakes?CheckChannel(chan->channel,ShowFakesChannels):0;
                    break;
                case 5: chan->FriendList=
                    FriendList?CheckChannel(chan->channel,FriendListChannels):0;
                    break;
                case 6: chan->KickOnBan=
                    KickOnBan?CheckChannel(chan->channel,KickOnBanChannels):0;
                    break;
                case 7: chan->KickOnFlood=
                    KickOnFlood?CheckChannel(chan->channel,KickOnFloodChannels):0;
                    break;
                case 8: chan->KickOps=
                    KickOps?CheckChannel(chan->channel,KickOpsChannels):0;
                    break;
                case 9: chan->KickWatch=
                    KickWatch?CheckChannel(chan->channel,KickWatchChannels):0;
                    break;
                case 11: chan->NickWatch=
                    NickWatch?CheckChannel(chan->channel,NickWatchChannels):0;
                    break;
                case 13: chan->ShowAway=
                    ShowAway?CheckChannel(chan->channel,ShowAwayChannels):0;
                    break;
                case 14: chan->CompressModes=
                    CompressModes?CheckChannel(chan->channel,CompressModesChannels):0;
                    break;
                case 15: chan->BKList=
                    BKList?CheckChannel(chan->channel,BKChannels):0;
                    break;
#ifdef EXTRAS
                case 99: chan->IdleKick=
                    IdleKick?CheckChannel(chan->channel,IdleKickChannels):0;
                break;
#endif
            }
#ifdef EXTRAS
            if (chan->IdleKick) chan->IdleKick=IdleKick;
#endif
        }
    for (whowas=whowas_chan_list;whowas;whowas=whowas->next) {
        switch (setting) {
            case 1: whowas->channellist->AutoRejoin=
                AutoRejoin?CheckChannel(whowas->channellist->channel,AutoRejoinChannels):0;
                break;
            case 2: whowas->channellist->Bitch=
                Bitch?CheckChannel(whowas->channellist->channel,BitchChannels):0;
                break;
            case 3: whowas->channellist->MDopWatch=
                MDopWatch?CheckChannel(whowas->channellist->channel,MDopWatchChannels):0;
                break;
            case 4: whowas->channellist->ShowFakes=
                ShowFakes?CheckChannel(whowas->channellist->channel,ShowFakesChannels):0;
                break;
            case 5: whowas->channellist->FriendList=
                FriendList?CheckChannel(whowas->channellist->channel,FriendListChannels):0;
                break;
            case 6: whowas->channellist->KickOnBan=
                KickOnBan?CheckChannel(whowas->channellist->channel,KickOnBanChannels):0;
                break;
            case 7: whowas->channellist->KickOnFlood=
                KickOnFlood?CheckChannel(whowas->channellist->channel,KickOnFloodChannels):0;
                break;
            case 8: whowas->channellist->KickOps=
                KickOps?CheckChannel(whowas->channellist->channel,KickOpsChannels):0;
                break;
            case 9: whowas->channellist->KickWatch=
                KickWatch?CheckChannel(whowas->channellist->channel,KickWatchChannels):0;
                break;
            case 11: whowas->channellist->NickWatch=
                NickWatch?CheckChannel(whowas->channellist->channel,NickWatchChannels):0;
                break;
            case 13: whowas->channellist->ShowAway=
                ShowAway?CheckChannel(whowas->channellist->channel,ShowAwayChannels):0;
                break;
            case 14: whowas->channellist->CompressModes=
                CompressModes?CheckChannel(whowas->channellist->channel,CompressModesChannels):0;
                break;
            case 15: whowas->channellist->BKList=
                BKList?CheckChannel(whowas->channellist->channel,BKChannels):0;
                break;
#ifdef EXTRAS
            case 99: whowas->channellist->IdleKick=
                IdleKick?CheckChannel(whowas->channellist->channel,IdleKickChannels):0;
                break;
#endif
        }
#ifdef EXTRAS
        if (whowas->channellist->IdleKick) whowas->channellist->IdleKick=IdleKick;
#endif
    }
    update_all_status();
}

/* Sets auto completion on/off */
void SetAutoCompletion(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int newset=0;
    char *tmpstr;

    if (args && *args) {
        if (!my_strnicmp("ON",args,2) || !my_strnicmp("AUTO",args,4)) {
            if (!my_strnicmp("ON",args,2)) {
                newset=1;
                tmpstr=args+2;
            }
            else {
                newset=2;
                tmpstr=args+4;
            }
            if (*tmpstr) *tmpstr++='\0';
            if (*tmpstr) malloc_strcpy(&AutoReplyString,tmpstr);
            else {
                PrintUsage("AUTOCOMPL on/auto string/off");
                return;
            }
            AutoNickCompl=newset;
        }
        else if (!my_stricmp("OFF",args)) {
            AutoNickCompl=0;
            new_free(&AutoReplyString);
        }
        else {
            PrintUsage("AUTOCOMPL on/auto string/off");
            return;
        }
    }
    if (AutoNickCompl)
        PrintSetting("Auto nick completion",AutoNickCompl==1?"ON":"AUTO",
                     ", right side string is",AutoReplyString);
    else PrintSetting("Auto nick completion","OFF",empty_string,empty_string);
}

#ifdef EXTRAS
/* Sets idle kick on/off */
void SetIdleKick(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int newset=0;
    char *tmpstr;

    if (args && *args) {
        tmpstr=new_next_arg(args,&args);
        if (!my_stricmp("ON",tmpstr) || !my_stricmp("AUTO",tmpstr)) {
            if (!my_stricmp("ON",tmpstr)) newset=1;
            else newset=2;
            tmpstr=new_next_arg(args,&args);
            if (tmpstr && *tmpstr) malloc_strcpy(&IdleKickChannels,tmpstr);
            else {
                PrintUsage("IDLEKICK on/auto channels/off");
                return;
            }
            IdleKick=newset;
        }
        else if (!my_stricmp("OFF",tmpstr)) {
            IdleKick=0;
            new_free(&IdleKickChannels);
        }
        else {
            PrintUsage("IDLEKICK on/auto channels/off");
            return;
        }
    }
    if (IdleKick)
        PrintSetting("Idle kick",IdleKick==1?"ON":"AUTO",
                     " for channels :",IdleKickChannels);
    else PrintSetting("Idle kick","OFF",empty_string,empty_string);
    /* 99 is IdleKick, look at SetChannels() */
    SetChannels(99);
}
#endif /* EXTRAS */

/* Sets ScrollZ settings (on channels/off) */
void ChannelCommand(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  i;
    char *tmpstr;
    char *tmpchan;
    char tmpbuf[mybufsize/8];

    upper(command);
    for (i=0;command_list[i].command;i++)
        if (!strcmp(command_list[i].command,command)) break;
    if (!(command_list[i].command)) return;
    tmpstr=new_next_arg(args,&args);
    if (tmpstr) {
        if (!my_stricmp("ON",tmpstr)) {
            tmpchan=new_next_arg(args,&args);
            if (tmpchan && *tmpchan) malloc_strcpy(command_list[i].strvar,tmpchan);
            else {
                sprintf(tmpbuf,"%s on %s/off",command_list[i].command,
                        !strcmp(command_list[i].command,"ORIGNICK")?"nick":"channels");
                PrintUsage(tmpbuf);
                return;
            }
            *(command_list[i].var)=1;
        }
        else if (!my_stricmp("OFF",tmpstr)) {
            *(command_list[i].var)=0;
            new_free(command_list[i].strvar);
        }
        else {
            sprintf(tmpbuf,"%s on %s/off",command_list[i].command,
                    !strcmp(command_list[i].command,"ORIGNICK")?"nick":"channels");
            PrintUsage(tmpbuf);
            return;
        }
    }
    tmpstr=command_list[i].setting2;
    if (!tmpstr) tmpstr=" for channels :";
    if (*(command_list[i].var)) PrintSetting(command_list[i].setting,"ON",tmpstr,
                                             *(command_list[i].strvar));
    else PrintSetting(command_list[i].setting,"OFF",empty_string,empty_string);
    SetChannels(i);
}

/* Prints usage for command */
void PrintUsage(line)
char *line;
{
    say("Usage: /%s",line);
}

#ifdef WANTANSI
/* Converts mIRC colors to ANSI, by Ananda - fix by ddd */
void ConvertmIRC(buffer,newbuf)
char *buffer;
char *newbuf;
{
    struct {
        char *fg, *bg;
    } codes[16] = {
        { "[1;37m",   "[47m"        },      /* white                */
        { "[0m",      "[40m"        },      /* black (grey for us)  */
        { "[0;34m",   "[44m"        },      /* blue                 */
        { "[0;32m",   "[42m"        },      /* green                */
        { "[0;31m",   "[41m"        },      /* red                  */
        { "[0;33m",   "[43m"        },      /* brown                */
        { "[0;35m",   "[45m"        },      /* magenta              */
        { "[1;31m",   "[46m"        },      /* bright red           */
        { "[1;33m",   "[47m"        },      /* yellow               */
        { "[1;32m",   "[42m"        },      /* bright green         */
        { "[0;36m",   "[44m"        },      /* cyan                 */
        { "[1;36m",   "[44m"        },      /* bright cyan          */
        { "[1;34m",   "[44m"        },      /* bright blue          */
        { "[1;35m",   "[45m"        },      /* bright magenta       */
        { "[1;30m",   "[40m"        },      /* dark grey            */
        { "[0;37m",   "[47m"        }       /* grey                 */
    };
    register char *sptr=buffer;
    register char *dptr=newbuf;
    register short code;

    *dptr='\0';
    while (*sptr) {
        if (*sptr=='' && isdigit(*(sptr+1))) {
            sptr++;
	    code=(*sptr++)-'0';
	    if (isdigit(*sptr)) code=code*10+(*sptr++)-'0';
            if (code>15 || code<0) continue;
            if (DisplaymIRC==1) {
                strcpy(dptr,codes[code].fg);
                while (*dptr) dptr++;
            }
            if (*sptr==',') {
                sptr++;
	        code=(*sptr++)-'0';
	        if (isdigit(*sptr)) code=code*10+(*sptr++)-'0';
                if (code>0 && code<15 && DisplaymIRC==1) {
                    strcpy(dptr,codes[code].bg);
                    while (*dptr) dptr++;
                }
            }
        }
        else if (*sptr=='') {
            if (DisplaymIRC==1) {
                strcpy(dptr,Colors[COLOFF]);
                while (*dptr) dptr++;
            }
            sptr++;
        }
        else *dptr++=*sptr++;
    }
    *dptr='\0';
}
#endif

#ifdef CELE
/* Formats TRACE - by Zakath */
void HandleTrace(trnum,type,sclass,arg1,arg2,arg3,arg4)
int  trnum;
char *type;
char *sclass;
char *arg1;
char *arg2;
char *arg3;
char *arg4;
{
    if ((TraceOper || TraceAll) && trnum==204)
        put_it("%s Oper: %c%s%c (%s)",numeric_banner(),bold,arg1,bold,sclass);
    else if ((TraceUser || TraceAll) && trnum==205)
        put_it("%s User: %c%s%c (%s)",numeric_banner(),bold,arg1,bold,sclass);
    else if ((TraceServer || TraceAll) && trnum==206)
        put_it("%s Serv: %c%s%c - by %s (%s)",numeric_banner(),bold,arg3,bold,arg4,sclass);
    else if (TraceAll) put_it("%s %c%s%c %s %s",numeric_banner(),bold,type,bold,sclass,arg1);
}

/* Sends TRACE command, allows for switches - by Zakath */
void ScrollZTrace(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tswitch;
    char *server=(char *) 0;

    TraceAll=1; /* Unless changed w/ a switch, we want all info */
    TraceOper=0;
    TraceUser=0;
    TraceClass=0;
    TraceServer=0;
    if (args && *args) {
        tswitch=new_next_arg(args,&args);
        if (index(tswitch, '-')==NULL) server=tswitch;
        else {
            if (args && *args) server=next_arg(args,&args);
            else
                if (!my_stricmp(tswitch,"-u")) {
                    TraceAll=0;
                    TraceUser=1;
                    say("Tracing server for all %cusers%c...",bold,bold);
                }
            if (!my_stricmp(tswitch,"-o")) {
                TraceAll=0;
                TraceOper=1;
                say("Tracing server for all %copers%c...",bold,bold);
            }
            if (!my_stricmp(tswitch,"-s")) {
                TraceAll=0;
                TraceServer=1;
                say("Tracing server for all %cserver connections%c...",bold,bold);
            }
        }
    }
    if (server) send_to_server("TRACE %s",server);
    else send_to_server("TRACE");
}
#endif

#ifdef WANTANSI
/* Colorizes user@host */
void ColorUserHost(userhost,color,buffer,parentheses)
char *userhost;
char *color;
char *buffer;
int  parentheses;
{
    char *tmpstr;
    char tmpbuf[mybufsize/4+1];

    if (!userhost) {
        *buffer='\0';
        return;
    }
    strmcpy(tmpbuf,userhost,mybufsize/4);
    tmpstr=index(tmpbuf,'@');
    if (tmpstr) {
        *tmpstr='\0';
        tmpstr++;
    }
    else {
        *buffer='\0';
        return;
    }
    if (parentheses) sprintf(buffer,"%s(%s%s%s%s%s@%s%s%s%s%s)%s",
                             CmdsColors[COLMISC].color2,Colors[COLOFF],
                             color,tmpbuf,Colors[COLOFF],
                             CmdsColors[COLMISC].color1,Colors[COLOFF],
                             color,tmpstr,Colors[COLOFF],
                             CmdsColors[COLMISC].color2,Colors[COLOFF]);
    else sprintf(buffer,"%s%s%s%s@%s%s%s%s",
                 color,tmpbuf,Colors[COLOFF],
                 CmdsColors[COLMISC].color1,Colors[COLOFF],
                 color,tmpstr,Colors[COLOFF]);
}
#endif

/* Adds/removes channels to ScrollZ settings */
void AddChannel(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  i;
    int  add=!strcmp(command,"ADD")?1:0;
    int  len;
    char *channel;
    char *szsetting;
    char tmpbuf[mybufsize/4];

    szsetting=new_next_arg(args,&args);
    channel=new_next_arg(args,&args);
    if (!szsetting || !channel) {
        sprintf(tmpbuf,"%sCHAN setting channel",command);
        PrintUsage(tmpbuf);
        return;
    }
    upper(szsetting);
    len=strlen(szsetting);
    for (i=0;command_list[i].command;i++)
        if (!strncmp(command_list[i].command,szsetting,len)) break;
    if (!(command_list[i].command) || !strcmp(szsetting,"ORIGNICK")) {
        say("Illegal command, try /SHELP %sCHAN",command);
        return;
    }
    if (!(*(command_list[i].var))) {
        say("%s is set to OFF, aborting",command_list[i].command);
        return;
    }
    AddRemoveChannel(command_list[i].strvar,channel,add);
    if (!strcmp(command_list[i].command,"AJOIN")) AutoJoinOnInvToggle(NULL,NULL,NULL);
    else if (!strcmp(command_list[i].command,"NHPROT")) NHProtToggle(NULL,NULL,NULL);
    else PrintSetting(command_list[i].setting,"ON"," for channels :",
                      *(command_list[i].strvar));
    SetChannels(i);
}

/* Used to calculate hash value */
int HashFunc(nick)
char *nick;
{
    int  sum=0;
    char *tmp;

    for (tmp=nick;*tmp;tmp++)
        sum+=(*tmp>='a' && *tmp<='z'?*tmp-' ':*tmp);
    return(sum%HASHTABLESIZE);
}

/* Shows some ScrollZ related information */
void ScrollZInfo(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  timediff=time((time_t *) 0)-start_time;
    char *szver;
    char *tmpstr1;
    char tmpbuf[mybufsize/4];

    strcpy(tmpbuf,ScrollZver);
    tmpstr1=index(tmpbuf,' ');
    szver=index(tmpstr1+1,' ')+1;
    tmpstr1=index(szver,' ');
    *tmpstr1='\0';
    say("This is ScrollZ %s (client base ircII %s, version %s)",
            szver,irc_version,internal_version);
    say("Client uptime: %dd %02dh %02dm",timediff/86400,(timediff/3600)%24,
            (timediff/60)%60);
    say("Support channel: #ScrollZ on Efnet");
    say("Distribution: acidflash, bighead, arc, mathe, frash, ogre, lotbd, TrN, kali, Psylocke, synergy and code_zero");
}

/* Handles reply number 329 from server */
void ChannelCreateTime(from,ArgList)
char *from;
char **ArgList;
{
    char *channel;
    time_t createtime;

    if (ArgList[1] && is_channel(ArgList[0])) {
        channel=ArgList[0];
        createtime=atoi(ArgList[1]);
        save_message_from();
        message_from(channel,LOG_CRAP);
        if (createtime) {
#ifdef WANTANSI
            put_it("%s Channel %s%s%s created on %s%.24s%s",numeric_banner(),
                    CmdsColors[COLJOIN].color3,channel,Colors[COLOFF],
                    CmdsColors[COLJOIN].color4,ctime(&createtime),Colors[COLOFF]);
#else
            put_it("%s Channel %s created on %.24s",numeric_banner(),channel,
                    ctime(&createtime));
#endif
        }
        else {
#ifdef WANTANSI
            put_it("%s Time stamping is off for channel %s%s%s",numeric_banner(),
                    CmdsColors[COLJOIN].color3,channel,Colors[COLOFF]);
#else
            put_it("%s Time stamping is off for channel %s",numeric_banner(),channel);
#endif
        }
        restore_message_from();
    }
}

#ifdef CELE
/* displays your system's uptime */
void ExecUptime(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char tmpbuf[mybufsize/4];

    sprintf(tmpbuf,"uptime");
    execcmd(NULL,tmpbuf,NULL);
}

/* Does /CTCP nick finger */
void CTCPFing(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *target=NULL;

    if (args && *args && *args!='*') target=new_next_arg(args,&args);
    if (!target && !(target=get_channel_by_refnum(0))) {
        NoWindowChannel();
        return;
    }
    send_to_server("PRIVMSG %s :%cFINGER%c",target,CTCP_DELIM_CHAR,CTCP_DELIM_CHAR);
}
#endif

/* Checks whether we tried to ping non existing server */
void NoSuchServer4SPing(from,ArgList)
char *from;
char **ArgList;
{
    char *server=ArgList[0];
    struct spingstr *spingtmp;

    if (server && (spingtmp=(struct spingstr *) list_lookup((List **) &spinglist,server,
                                                            !USE_WILDCARDS,REMOVE_FROM_LIST))) {
        new_free(&(spingtmp->servername));
        new_free(&spingtmp);
#ifdef WANTANSI
        say("No such server to ping: %s%s%s",
            CmdsColors[COLCSCAN].color1,server,Colors[COLOFF]);
#else
        say("No such server to ping: %s",server);
#endif
    }
    else if (server && ArgList[1]) put_it("%s %s %s",numeric_banner(),server,ArgList[1]);
}

/* Returns server from netsplit info */
char *GetNetsplitServer(channel,nick)
char *channel;
char *nick;
{
    char *server;
    struct wholeftch  *wholch;
    struct wholeftstr *wholeft;

    for (wholeft=wholist;wholeft;wholeft=wholeft->next) {
        for (wholch=wholeft->channels;wholch;wholch=wholch->next)
            if (!my_stricmp(channel,wholch->channel)) break;
        if (wholch && (server=index(wholeft->splitserver,' '))) {
            server++;
            return(server);
        }
    }
    return(empty_string);
}

/* Handles delayed opping */
void HandleDelayOp(stuff)
char *stuff;
{
    int  voice;
    char mode=' ';
    char *nick;
    char *channel;
    NickList *tmpnick;
    ChannelList *chan;

    channel=new_next_arg(stuff,&stuff);
    nick=new_next_arg(stuff,&stuff);
    voice=atoi(nick);
    nick=new_next_arg(stuff,&stuff);
    chan=lookup_channel(channel,from_server,0);
    if ((chan=lookup_channel(channel,from_server,0)) && ((chan->status)&CHAN_CHOP) &&
        chan->FriendList && (tmpnick=CheckJoiners(nick,channel,from_server,chan))) {
        if (tmpnick->frlist && ((tmpnick->frlist->privs)&FLAUTOOP)) {
            if (voice && ((tmpnick->frlist->privs)&FLVOICE)) {
                if (!(tmpnick->chanop) && !(tmpnick->hasvoice)) mode='v';
            }
            else if (((tmpnick->frlist->privs)&FLOP) && !(tmpnick->chanop)) mode='o';
            if (mode!=' ') send_to_server("MODE %s +%c %s",channel,mode,nick);
        }
    }
}

/* Handles delayed notify */
void HandleDelayNotify(stuff)
char *stuff;
{
    int old_server=from_server;
    void (*func)()=(void(*)()) HandleUserhost;

    if (inSZNotify) inSZNotify++;
    else inSZNotify=2;
    from_server=primary_server;
    add_userhost_to_whois(stuff,func);
    from_server=old_server;
}

#ifdef EXTRAS
/* Show idle time for users */
void ShowIdle(command,args,subargs)
char *command;
char *args;
char *subargs;
{
#ifdef WANTANSI
    int  len=0;
#endif
    int  days;
    int  hours;
    int  mins;
    int  secs;
    int  origidle;
    int  chanusers=0;
    int  totalidle=0;
    char *filter=(char *) 0;
    char *channel=(char *) 0;
    char tmpbuf1[mybufsize/4];
    char tmpbuf2[mybufsize/4];
    time_t timenow=time((time_t *) 0);
    NickList *joiner;
    ChannelList *chan;

    channel=new_next_arg(args,&args);
    if (channel) {
        if (is_channel(channel)) strcpy(tmpbuf1,channel);
        else sprintf(tmpbuf1,"#%s",channel);
        channel=tmpbuf1;
    }
    if (!channel && (channel=get_channel_by_refnum(0))==NULL) {
        NoWindowChannel();
        return;
    }
    if (!(chan=lookup_channel(channel,curr_scr_win->server,0))) return;
    if (!(chan->IdleKick)) say("Idle kick is off for channel %s",chan->channel);
    if (!(filter=new_next_arg(args,&args))) filter="*";
    say("%-42s  Idle time","User");
    for (joiner=chan->nicks;joiner;joiner=joiner->next) {
        sprintf(tmpbuf2,"%s!%s",joiner->nick,
                joiner->userhost?joiner->userhost:empty_string);
        if (!wild_match(filter,tmpbuf2)) continue;
        origidle=timenow-joiner->lastmsg;
        days=(origidle/86400);
        hours=((origidle-(days*86400))/3600);
        mins=((origidle-(days*86400)-(hours*3600))/60);
        secs=(origidle-(days*86400)-(hours*3600)-(mins*60));
#ifdef WANTANSI
        len=1+strlen(joiner->nick);
        if (joiner->userhost) {
            ColorUserHost(joiner->userhost,CmdsColors[COLWHO].color2,tmpbuf2,1);
            len+=strlen(joiner->userhost);
        }
        else *tmpbuf2='\0';
        for (;len<40;len++) strcat(tmpbuf2," ");
        say("%s%s%s %s  %dd %dh %dm %ds",
            CmdsColors[COLWHO].color1,joiner->nick,Colors[COLOFF],
            tmpbuf2,days,hours,mins,secs);
#else
        say("%-42s  %dd %dh %dm %ds",tmpbuf2,days,hours,mins,secs);
#endif
        chanusers++;
        totalidle+=origidle;
    }
    say("Average idle time in channel %s is %.2f seconds",chan->channel,
        (float) totalidle/chanusers);
}
#endif

/* Try to switch nick to orignick */
void SwitchNick() {
    time_t timenow=time((time_t *) 0);

    if (timenow>=LastNick+OrigNickDelay) {
        if (my_stricmp(get_server_nickname(from_server),OrigNick))
            e_nick(NULL,OrigNick,NULL);
        LastNick=timenow+1;
    }
}

#ifdef OPER
/* Kills multiple nicks at the same time by acidflash */
void MassKill(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int	 meep=1;
    char *comment;
    char *tmpnick;

    if (args && *args) {
        if (!(comment=index(args,':'))) comment=DefaultKill;
        else *comment++='\0';
        if (args && *args) {
            while ((tmpnick=new_next_arg(args,&args))) {
                send_to_server("KILL %s :%s (%d)",tmpnick,comment,meep);
                meep++;
            }
        }
    }
    else PrintUsage("MKILL nick1 nick2 nick3 ... [:reason]");
}
#endif /* OPER */

/* Check if given server is valid, ie. it still exists and it's connected */
int CheckServer(server)
int server;
{
    if (server>=0 && server<number_of_servers && server_list[server].connected)
        return(1);
    return(0);
}

/* Clean up ScrollZ allocated variables */
void CleanUpScrollZVars() {
    new_free(&DefaultServer);
    new_free(&ScrollZstr);
    new_free(&ScrollZver1);
    new_free(&DefaultSignOff);
    new_free(&DefaultSetAway);
    new_free(&DefaultSetBack);
    new_free(&DefaultUserinfo);
    new_free(&DefaultFinger);
    new_free(&AutoJoinChannels);
    new_free(&CdccUlDir);
    new_free(&CdccDlDir);
    new_free(&WhoKilled);
    new_free(&CdccChannels);
    new_free(&AutoRejoinChannels);
    new_free(&MDopWatchChannels);
    new_free(&ShowFakesChannels);
    new_free(&KickOnFloodChannels);
    new_free(&KickWatchChannels);
    new_free(&NHProtChannels);
    new_free(&NickWatchChannels);
    new_free(&ShowAwayChannels);
    new_free(&KickOpsChannels);
    new_free(&KickOnBanChannels);
    new_free(&BitchChannels);
    new_free(&FriendListChannels);
#ifdef EXTRAS
    new_free(&IdleKickChannels);
    new_free(&SignoffChannels);
#endif
    new_free(&CompressModesChannels);
    new_free(&BKChannels);
    new_free(&EncryptPassword);
#ifdef OPER
    new_free(&StatsFilter);
    new_free(&StatsiFilter);
#endif
    new_free(&AutoReplyBuffer);
    new_free(&OrigNick);
    new_free(&HelpPathVar);
    new_free(&CelerityNtfy);
    new_free(&URLBuffer);
    new_free(&CurrentNick);
    new_free(&LastChat);
    new_free(&CurrentDCC);
    new_free(&DefaultK);
    new_free(&DefaultBK);
    new_free(&DefaultBKI);
    new_free(&DefaultBKT);
    new_free(&DefaultFK);
    new_free(&DefaultLK);
    new_free(&DefaultABK);
    new_free(&DefaultSK);
#ifdef OPER
    new_free(&DefaultKill);
#endif
    new_free(&PermUserMode);
    new_free(&AutoReplyString);
}

/* Clean up all stuff from memory on exit */
void CleanUp() {
    int i;
    char tmpbuf[mybufsize/32];
    struct list *tmplist;
    struct nicks *tmpnick,*tmpnickfree;
    struct mapstr *tmpmap;
    struct urlstr *tmpurl;
    struct encrstr *tmpencr;
    struct splitstr *tmpsplit;
    struct spingstr *tmpsping;
    struct wholeftch *tmpchan,*tmpchanfree;
    struct wholeftstr *tmpwho;
    DCC_list *tmpdcc,*tmpdccfree;
    ChannelList *tmpch,*tmpchfree;

    for (tmpdcc=ClientList;tmpdcc;) {
        tmpdccfree=tmpdcc;
        tmpdcc=tmpdcc->next;
        dcc_erase(tmpdccfree);
    }
    CleanUpCdcc();
    CleanUpFlood();
    CleanUpIgnore();
    CleanUpTimer();
    CleanUpLists();
    CleanUpWhowas();
    CleanUpWindows();
    CleanUpVars();
    CleanUpScrollZVars();
    inSZNotify=1;
    strcpy(tmpbuf,"-");
    notify(NULL,tmpbuf,NULL);
    strcpy(tmpbuf,"ALL");
    Dump(NULL,tmpbuf,NULL);
    for (i=0;i<number_of_servers;i++) {
	if (server_list[i].name) new_free(&server_list[i].name);
	if (server_list[i].itsname) new_free(&server_list[i].itsname);
	if (server_list[i].password) new_free(&server_list[i].password);
	if (server_list[i].away) new_free(&server_list[i].away);
	if (server_list[i].version_string) new_free(&server_list[i].version_string);
	if (server_list[i].nickname) new_free(&server_list[i].nickname);
	if (server_list[i].whois_stuff.nick) new_free(&server_list[i].whois_stuff.nick);
	if (server_list[i].whois_stuff.user) new_free(&server_list[i].whois_stuff.user);
	if (server_list[i].whois_stuff.host) new_free(&server_list[i].whois_stuff.host);
	if (server_list[i].whois_stuff.channel) new_free(&server_list[i].whois_stuff.channel);
	if (server_list[i].whois_stuff.channels) new_free(&server_list[i].whois_stuff.channels);
	if (server_list[i].whois_stuff.name) new_free(&server_list[i].whois_stuff.name);
	if (server_list[i].whois_stuff.server) new_free(&server_list[i].whois_stuff.server);
	if (server_list[i].whois_stuff.server_stuff) new_free(&server_list[i].whois_stuff.server_stuff);
	if (server_list[i].ctcp_send_size) new_free(&server_list[i].ctcp_send_size);
	if (server_list[i].LastJoin) new_free(&(server_list[i].LastJoin));
        for (tmpnick=server_list[i].arlist;tmpnick;) {
            tmpnickfree=tmpnick;
            tmpnick=tmpnick->next;
            new_free(&(tmpnickfree->nick));
            new_free(&tmpnickfree);
        }
        for (tmpnick=server_list[i].nicklist;tmpnick;) {
            tmpnickfree=tmpnick;
            tmpnick=tmpnick->next;
            new_free(&(tmpnickfree->nick));
            new_free(&tmpnickfree);
        }
        for (tmpch=server_list[i].chan_list;tmpch;) {
            tmpchfree=tmpch;
            tmpch=tmpch->next;
            clear_channel(tmpchfree);
            new_free(&(tmpchfree->key));
            new_free(&(tmpchfree->s_mode));
            new_free(&(tmpchfree->modelock));
            new_free(&(tmpchfree->topicstr));
            new_free(&(tmpchfree->topicwho));
            new_free(&(tmpchfree->channel));
            new_free(&tmpchfree);
        }
	from_server=i;
	clean_whois_queue();
    }
    free(server_list);
    while (splitlist) {
        tmpsplit=splitlist;
        splitlist=splitlist->next;
        new_free(&(tmpsplit->servers));
        new_free(&tmpsplit);
    }
    while (spinglist) {
        tmpsping=spinglist;
        spinglist=spinglist->next;
        new_free(&(tmpsping->servername));
        new_free(&tmpsping);
    }
    while (encrlist) {
        tmpencr=encrlist;
        encrlist=encrlist->next;
        new_free(&(tmpencr->user));
        new_free(&(tmpencr->key));
        new_free(&tmpencr);
    }
    while (maplist) {
        tmpmap=maplist;
        maplist=maplist->next;
        new_free(&(tmpmap->server));
        new_free(&(tmpmap->uplink));
        new_free(&tmpmap);
    }
    while (urllist) {
        tmpurl=urllist;
        urllist=urllist->next;
        new_free(&(tmpurl->urls));
        new_free(&tmpurl);
    }
    while (wholist) {
        tmpwho=wholist;
        wholist=wholist->next;
        tmpchan=tmpwho->channels;
        while (tmpchan) {
            while (tmpchan->nicklist) {
                tmplist=tmpchan->nicklist;
                tmpchan->nicklist=tmplist->next;
                new_free(&(tmplist->nick));
                new_free(&(tmplist->userhost));
                new_free(&tmplist);
            }
            tmpchanfree=tmpchan;
            tmpchan=tmpchan->next;
            new_free(&(tmpchanfree->channel));
            new_free(&(tmpchanfree));
        }
        new_free(&(tmpwho->splitserver));
        new_free(&tmpwho);
    }
#ifdef WANTANSI
    for (i=0;i<NUMCMDCOLORS;i++) {
        new_free(&(CmdsColors[i].color1));
        new_free(&(CmdsColors[i].color2));
        new_free(&(CmdsColors[i].color3));
        new_free(&(CmdsColors[i].color4));
        new_free(&(CmdsColors[i].color5));
        new_free(&(CmdsColors[i].color6));
    }
#endif
}

/* Support for encrypted DCC CHAT sessions */
void EncryptMsg(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int delit=REMOVE_FROM_LIST;
    char *user;
    char *key;
    struct encrstr *tmp;

    user=new_next_arg(args,&args);
    key=new_next_arg(args,&args);
    if (user) {
        if (key) delit=!REMOVE_FROM_LIST;
        tmp=(struct encrstr *) list_lookup((List **) &encrlist,user,!USE_WILDCARDS,delit);
        if (key) {
            if (tmp) malloc_strcpy(&(tmp->key),key);
            else {
                tmp=(struct encrstr *) new_malloc(sizeof(struct encrstr));
                tmp->next=(struct encrstr *) 0;
                tmp->user=(char *) 0;
                tmp->key=(char *) 0;
                malloc_strcpy(&(tmp->user),user);
                malloc_strcpy(&(tmp->key),key);
                add_to_list((List **) &encrlist,(List *) tmp);
            }
            say("Communication with %s will be encrypted using key %s",
                user,key);
        }
        else {
            if (!tmp) {
                say("User %s not found in encryption list",user);
                return;
            }
            new_free(&(tmp->user));
            new_free(&(tmp->key));
            new_free(&tmp);
            say("Communication with %s will no longer be encrypted",user);
        }
    }
    else {
        if (!encrlist) {
            say("No users in encryption list");
            return;
        }
        for (tmp=encrlist;tmp;tmp=tmp->next)
            say("User %s with key %s",tmp->user,tmp->key);
    }
}

/* Encrypt message */
int EncryptMessage(message,user)
char *message;
char *user;
{
    struct encrstr *tmp;

    if ((tmp=(struct encrstr *) list_lookup((List **) &encrlist,user,!USE_WILDCARDS,
                                            !REMOVE_FROM_LIST))) {
        return(EncryptString(message,message,tmp->key,BIG_BUFFER_SIZE-16,1));
    }
    return(0);
}

/* Decrypt message */
int DecryptMessage(message,user)
char *message;
char *user;
{
    struct encrstr *tmp;

    if ((tmp=(struct encrstr *) list_lookup((List **) &encrlist,user,!USE_WILDCARDS,
                                            !REMOVE_FROM_LIST))) {
        return(DecryptString(message,message,tmp->key,BIG_BUFFER_SIZE-16,1));
    }
    return(0);
}

#ifdef OPER
/* Filtered trace */
void FilterTrace(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *filter;

    if ((filter=new_next_arg(args,&args))) {
        tottcount=0;
        mattcount=0;
        malloc_strcpy(&ftpattern,filter);
        inSZTrace=2;
        send_to_server("TRACE");
        say("Finding all users matching %s on local server",filter);
    }
    else PrintUsage("FTRACE filter");
}

/* Actual filtered trace */
void DoFilterTrace(stuff)
char *stuff;
{
    char *nick;
    char *host;
    char *prevhost=stuff;
    char *tmpstr;
    char tmpbuf[mybufsize/4];

    nick=stuff;
    while ((host=index(prevhost,'['))) prevhost=host+1;
    if (prevhost && !host) {
        tottcount++;
        host=prevhost-1;
        *host++='\0';
        if ((tmpstr=index(host,']'))) *tmpstr='\0';
        else host=NULL;
        if (!my_stricmp(nick,get_server_nickname(from_server))) return;
        sprintf(tmpbuf,"%s!%s",nick,host);
        if (wild_match(ftpattern,tmpbuf)) {
            mattcount++;
#ifdef WANTANSI
            ColorUserHost(host,CmdsColors[COLWHO].color2,tmpbuf,0);
            say("%s%-9s%s %s",
                CmdsColors[COLWHO].color1,nick,Colors[COLOFF],tmpbuf);
#else
            say("%-9s %s",nick,tmpbuf);
#endif
        }
    }
}
#endif /* OPER */

/* Format time in compressed format */
char *FormatTime(timediff)
int timediff;
{
    static char timebuf[mybufsize/32];

    sprintf(timebuf,"%dd%dh%dm%ds",timediff/86400,(timediff/3600)%24,(timediff/60)%60,
            timediff%60);
    return(timebuf);
}

/* Store key for later join */
void AddJoinKey(clienttype,notice)
int clienttype;
char *notice;
{
    int i;
    static int joinkeyinit=0;
    char *key=(char *) 0;
    char *tmpstr=(char *) 0;
    char *channel=(char *) 0;
    char tmpbuf[mybufsize/4+1];

    /* initalize */
    if (!joinkeyinit) {
        for (i=0;i<NUM_JOIN_KEYS;i++) {
            joinkeys[i].key=(char *) 0;
            joinkeys[i].channel=(char *) 0;
        }
        joinkeyinit=1;
    }
    strmcpy(tmpbuf,notice,mybufsize/4);
    notice=tmpbuf;
    switch (clienttype) {
        case 1: /* ScrollZ */
            channel=notice+30;
            tmpstr=channel;
            while (*tmpstr && !isspace(*tmpstr)) tmpstr++;
            if (*tmpstr) {
                *tmpstr++='\0';
                if (!strncmp(tmpstr,"(key is ",8)) {
                    tmpstr+=8;
                    key=tmpstr;
                    while (*tmpstr && !isspace(*tmpstr)) tmpstr++;
                    tmpstr--;
                    if (*tmpstr==')') {
                        *tmpstr++='\0';
                        if (strcmp(tmpstr," -ScrollZ-")) channel=(char *) 0;
                    }
                    else channel=(char *) 0;
                }
            }
            break;
        case 2: /* Pupette */
            key=notice+16;
            tmpstr=key;
            while (*tmpstr && !isspace(*tmpstr)) tmpstr++;
            if (*tmpstr) {
                *tmpstr++='\0';
                if (!strncmp(tmpstr,"to join ",8)) {
                    tmpstr+=8;
                    channel=tmpstr;
                }
            }
            break;
        case 3: /* C-ToolZ */
            channel=notice+17;
            tmpstr=channel;
            while (*tmpstr && *tmpstr!='') tmpstr++;
            if (*tmpstr) {
                *tmpstr++='\0';
                if (!strncmp(tmpstr," is ",5)) {
                    tmpstr+=5;
                    key=tmpstr;
                    while (*tmpstr && *tmpstr!='.') tmpstr++;
                    if (*tmpstr=='.') tmpstr--;
                    if (*tmpstr=='') {
                        *tmpstr++='\0';
                        if (strcmp(tmpstr,". <C-ToolZ>")) channel=(char *) 0;
                    }
                    else channel=(char *) 0;
                }
            }
            break;
        case 4: /* BitchX */
            channel=notice+22;
            tmpstr=channel;
            while (*tmpstr && !isspace(*tmpstr)) tmpstr++;
            if (*tmpstr) tmpstr--;
            if (*tmpstr=='.') {
                *tmpstr++='\0';
                if (!strncmp(tmpstr," Key is [",9)) {
                    tmpstr+=9;
                    key=tmpstr;
                    while (*tmpstr) tmpstr++;
                    tmpstr--;
                    if (*tmpstr=='') {
                        *tmpstr--='\0';
                        if (*tmpstr==']') *tmpstr='\0';
                        else channel=(char *) 0;
                    }
                }
            }
            break;
        case 5: /* ScrollZ with -DVILAS */
            channel=notice+12;
            tmpstr=channel;
            while (*tmpstr && !isspace(*tmpstr)) tmpstr++;
            if (*tmpstr) {
                *tmpstr++='\0';
                if (!strncmp(tmpstr,"key is ",7)) {
                    tmpstr+=7;
                    key=tmpstr;
                }
                else channel=(char *) 0;
            }
            break;
        default:
            return;
    }
    if (!key || !(*key) || !channel || !(*channel)) return;
    for (i=0;i<joinkeycount;i++)
        if (!my_stricmp(joinkeys[i].channel,channel)) break;
    if (i<joinkeycount) {
        new_free(&(joinkeys[i].key));
        new_free(&(joinkeys[i].channel));
        joinkeycount=i;
    }
    malloc_strcpy(&joinkeys[joinkeycount].key,key);
    malloc_strcpy(&joinkeys[joinkeycount].channel,channel);
    joinkeycount++;
    if (joinkeycount>=NUM_JOIN_KEYS) joinkeycount=0;
    say("Added channel %s and key %s to join list",channel,key);
}

/* Check key for join */
char *CheckJoinKey(channel)
char *channel;
{
    int i;
    char *key=empty_string;

    if (channel) {
        for (i=0;i<joinkeycount;i++)
            if (joinkeys[i].channel && !my_stricmp(joinkeys[i].channel,channel))
                break;
        if (i<joinkeycount && joinkeys[i].key)
            key=joinkeys[i].key;
    }
    return(key);
}

/* Try to join a channel */
#ifdef ACID
void TryChannelJoin() {
    int  found;
    int  lasttimer=0;
    char *tmpstr;
    char *tmpstr1;
    char tmpbuf[mybufsize/2+2];
    void (*func)()=(void(*)()) TryChannelJoin;
    TimerList *tmptimer;
    WhowasList *whowas;
    ChannelList *tmpchan;

    if (curr_scr_win->server!=-1 && ForceJoin && ForceJoinChannels) {
        strmcpy(tmpbuf,ForceJoinChannels,mybufsize/2);
        tmpstr=tmpbuf;
        tmpstr1=tmpbuf;
        while (1) {
            if (*tmpstr==',' || !(*tmpstr)) {
                if (!(*tmpstr)) *(tmpstr+1)='\0';
                *tmpstr='\0';
                for (tmpchan=server_list[curr_scr_win->server].chan_list;tmpchan;
                        tmpchan=tmpchan->next)
                    if (!my_stricmp(tmpstr1,tmpchan->channel)) break;
                /* 
                 * ask up to 5 users with +z flag set to open the channel for us
                 */
                if (!tmpchan) {
                    found=0;
                    for (whowas=whowas_userlist_list;whowas;whowas=whowas->next) {
                        if (whowas->nicklist && whowas->nicklist->frlist &&
                                ((whowas->nicklist->frlist->privs)&FLWHOWAS) &&
                                !my_stricmp(whowas->channel,tmpstr1)) {
                            found++;
                            if (found>5) break;
                            send_to_server("PRIVMSG %s :OPEN %sINVITE %s",
                                    whowas->nicklist->nick,tmpstr1,tmpstr1);
                        }
                    }
                }
                tmpstr1=tmpstr+1;
            }
            if (!(*tmpstr) && !(*(tmpstr+1))) break;
            tmpstr++;
        }
        /*
         * if there is one or less entries for TryChannelJoin() in
         * the timer list, add one
         */
        found=0;
        for (tmptimer=PendingTimers;tmptimer;tmptimer=tmptimer->next) {
            if (tmptimer->command && tmptimer->func==func &&
                !strcmp(tmptimer->command,"rejoin")) {
                found++;
#ifdef BETTERTIMER
                lasttimer=tmptimer->time.tv_sec;
#else
                lasttimer=tmptimer->time;
#endif
            }
        }
        if (found<2) {
            lasttimer-=time((time_t *) 0);
            lasttimer+=30;
            if (lasttimer<=0) lasttimer=30;
            sprintf(tmpbuf,"-INV %d rejoin",lasttimer);
            timercmd("FTIMER",tmpbuf,(char *) func);
        }
    }
}
#endif /* ACID */

/* Change user's password */
void ChangePassword(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int count=0;
    int isfilt=0;
    int countall=0;
    char *filter;
    char *passwd;
    char tmpbuf1[mybufsize/8+1];
    struct friends *tmpfriend;

    filter=new_next_arg(args,&args);
    passwd=new_next_arg(args,&args);
    if (!(filter && passwd)) {
        PrintUsage("CHPASS filter password");
        return;
    }
    if (*filter=='#') {
        isfilt=1;
        filter++;
    }
    for (tmpfriend=frlist;tmpfriend;tmpfriend=tmpfriend->next) {
        countall++;
        if ((isfilt && matchmcommand(filter,countall)) ||
            wild_match(filter,tmpfriend->userhost) ||
            wild_match(tmpfriend->userhost,filter)) {
            count++;
            EncryptString(tmpbuf1,passwd,passwd,mybufsize/16,0);
            malloc_strcpy(&(tmpfriend->passwd),tmpbuf1);
        }
    }
    say("Changed %d out of %d entries",count,countall);
}

/* Does /STATS i with filter */
#ifdef OPER
void StatsIFilter(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tmpstr;

    if (args && *args) {
        tmpstr=new_next_arg(args,&args);
        malloc_strcpy(&StatsiFilter,tmpstr);
        StatsiNumber=0;
        send_to_server("STATS I %s",args);
    }
    else PrintUsage("FILINE filter [server]");
}

/* Parses STATS i reply from server */
void HandleStatsI(statschar,ipiline,uhiline)
char *statschar;
char *ipiline;
char *uhiline;
{
    char *tmpstr;
    char tmpbuf1[mybufsize/2+1];

    if (!StatsiNumber) say("I-Line");
    strmcpy(tmpbuf1,uhiline,mybufsize/2);
    if (StatsiFilter) {
        tmpstr=new_next_arg(uhiline,&uhiline);
        tmpstr=new_next_arg(uhiline,&uhiline);
        if (!(tmpstr && *tmpstr))
            tmpstr=empty_string;
    }
    if (!StatsiFilter || (StatsiFilter && 
        (wild_match(StatsiFilter,tmpstr) || wild_match(StatsiFilter,ipiline))))
        say("%s %s %s",statschar,ipiline,tmpbuf1);
    StatsiNumber++;
}
#endif /* OPER */

/* Toggle value of ARinWindow */
void ARinWindowToggle(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tmpstr;

    tmpstr=new_next_arg(args,&args);
    if (tmpstr) {
        if (!my_stricmp("OFF",tmpstr)) ARinWindow=0;
        else if (!my_stricmp("ON",tmpstr)) ARinWindow=1;
        else if (!my_stricmp("USER",tmpstr)) ARinWindow=2;
        else {
            PrintUsage("ARINWIN on/user/off");
            return;
        }
    }
    switch (ARinWindow) {
        case 0:
            tmpstr="OFF";
            break;
        case 1:
            tmpstr="ON";
            break;
        case 2:
            tmpstr="USER";
            break;
    }
    PrintSetting("Auto reply in window",tmpstr,empty_string,empty_string);
}

/* Figure out if we're dealing with IrcNet's oper channel */
int IsIrcNetOperChannel(channel)
char *channel;
{
    if (!my_stricmp(channel,"&servers") || !my_stricmp(channel,"&errors")   ||
        !my_stricmp(channel,"&notices") || !my_stricmp(channel,"&local")    ||
        !my_stricmp(channel,"&channel") || !my_stricmp(channel,"&numerics") ||
        !my_stricmp(channel,"&kills")   || !my_stricmp(channel,"&clients")  ||
        !my_stricmp(channel,"&oper"))
        return(1);
    return(0);
}
