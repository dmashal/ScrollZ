
/* C-OperVision, for ScrollZ. Coded by Zakath. */
/* Thanks to Sheik and Flier for assistance.   */
/*
 * Implemented caching for OVgetword, fixed for various ircd versions   -Flier
 *
 * There seems to be a problem when you have OperVision turned ON and
 * either server closes connection or you reconnect yourself.  Client
 * resets window levels and by doing that, messes things up (not just
 * OperVision, messages go to wrong window too). I have fixed that by
 * patching /SERVER. It checks whether OperVision was turned ON prior
 * to reconnect, and if it was it is turned OFF and immediately after
 * that it is turned back ON. This seems to fix the problem.            -Flier
 *
 * I also had to patch my functions that take care of joining channels
 * to deal with OperVision correctly. Channels were going to wrong wi-
 * ndow when OperVision was active. I just changed the way I send JOIN
 * command to server (I'm using ircII function now) and it seems to be
 * working as expected.                                                 -Flier
 *
 * When user chooses to kill OperVision window with ^WK or WINDOW KILL
 * command, we disable OperVision since they probably wanted that.      -Flier
 *
 * $Id: operv.c,v 1.16 1999-10-31 11:05:39 f Exp $
 */

#include "irc.h"
#include "ircaux.h"
#include "window.h"
#include "edit.h"
#include "output.h"
#include "server.h"
#include "screen.h"
#include "myvars.h"

#if defined(OPERVISION) && defined(WANTANSI)

extern void PrintUsage _((char *));

/* Variables needed for caching */
static  int  NewNotice;   /* 1 if we are parsing new notice, 0 otherwise */
static  int  OldWord;     /* holds number  for previous word, if NewNotice is 0 */
static  char *OldPtr;     /* holds pointer for previous word, if NewNotice is 0 */

void CreateMode(tmpbuf,sizeofbuf)
char *tmpbuf;
int  sizeofbuf;
{
    /* we need to send aditional usermodes (+swfuckrn), for ircd 2.9/2.10 only send +w */
    if (get_server_version(from_server)==Server2_9 || 
        get_server_version(from_server)==Server2_10)
        strmcpy(tmpbuf,"w",sizeofbuf);
    else strmcpy(tmpbuf,"swfuckrn",sizeofbuf);
}

void OperVision(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tmp=(char *) 0;
    char tmpbuf[mybufsize/4+1];
    unsigned int display;

    tmp=next_arg(args,&args);
    if (tmp) {
	if (!my_stricmp("ON",tmp)) {
	    if (OperV) say("OperVision is already turned on");
            else {
		OperV=1;
                ServerNotice=1;
                /* turn on additional user modes */
                CreateMode(tmpbuf,mybufsize/4);
                send_to_server("MODE %s :+%s",get_server_nickname(from_server),tmpbuf);
                /* made one window command, made it jump back to current window when
                   it's done, all output from /WINDOW command is supressed   -Flier */
                strcpy(tmpbuf,"NEW NAME OV DOUBLE OFF LEVEL OPNOTE,SNOTE,WALLOP REFNUM 1 GROW 6");
                display=window_display;
                window_display=0;
                windowcmd(NULL,tmpbuf,NULL);
                window_display=display;
                say("OperVision is now enabled");
	    }
	}
	else if (!my_stricmp("OFF",tmp)) {
	    if (!OperV) say("OperVision is not currently active");
	    else {
		OperV=0;
                /* we need to undo aditional usermodes (-swfuckrn) */
                CreateMode(tmpbuf,mybufsize/4);
                send_to_server("MODE %s :-%s",get_server_nickname(from_server),tmpbuf);
                /* made one window command, all output from /WINDOW command is
                   supressed   -Flier */
		strcpy(tmpbuf,"REFNUM OV KILL");
                display=window_display;
                window_display=0;
		windowcmd(NULL,tmpbuf,NULL);
                window_display=display;
		say("OperVision is now disabled");
	    }
	}
	else PrintUsage("OV on/off");
    }
    else PrintUsage("OV on/off");
}

/* Takes (u@h), removes (), colorizes, returns u@h */
/* Also works with [u@h]   -Flier */
/* Also works with n!u@h   -Pier  */
char *OVuh(word)
char *word;
{
    int i;
    int sht=1;
    char *tmpstr;
    char *tmphost;
    char tmpbuf1[mybufsize/4];
    static char tmpbuf2[mybufsize/4];

    /* I added sht (number-of-chars-to-short the line) -Pier */
    /* Remove the ()'s from *word (pointer +1, cat length -1) */
    tmphost=index(word,'(');
    /* We need to check for []'s if there are no ()'s since ircd that I run on
       my box reports client connecting/exiting with [] ?????   -Flier */
    if (!tmphost) tmphost=index(word,'[');
    /* Hybrid 5.1b26 seems to use n!u@h in /quote HTM notices -Pier */
    if (!tmphost) {
        tmphost=index(word,'!');
        sht=0;
    }
    if (tmphost) tmphost++;
    else return(word);
    i=strlen(tmphost);
    strmcpy(tmpbuf1,tmphost,i-sht);
    /* tmpbuf1 is the u@h, now colorize it */
    if ((tmpstr=index(tmpbuf1,'@'))) {
	*tmpstr='\0';
	tmpstr++;
	sprintf(tmpbuf2,"%s(%s%s%s%s%s@%s%s%s%s%s)%s",
	        CmdsColors[COLMISC].color2,Colors[COLOFF],
	        CmdsColors[COLSETTING].color4,tmpbuf1,Colors[COLOFF],
	        CmdsColors[COLMISC].color1,Colors[COLOFF],
	        CmdsColors[COLSETTING].color4,tmpstr,Colors[COLOFF],
	        CmdsColors[COLMISC].color2,Colors[COLOFF]);
    }
    return(tmpbuf2);
}

/* Returns domain, minus host and top */
/* fixed by Flier to work on hostname.domain (like irc.net) */
char *OVsvdmn(string)
char *string;
{
    int  i,l;
    char *c;
    char *d;
    char tmpstr[mybufsize/8];
    static char tmpbuf[mybufsize/4];

    c=rindex(string,'.');
    if (!c) return(string);
    i=strlen(string);
    l=strlen(c);
    i-=l;  /* Length of top */
    strmcpy(tmpstr,string,i);
    if (!(d=rindex(tmpstr,'.'))) d=c; /* Extract domain */
    d++;
    sprintf(tmpbuf,"%s",d);
    return(tmpbuf);
}

/* Gets word(s) from string. Similar to $word() in IrcII */
/* Added caching   -Flier */
char *OVgetword(a,b,string)
int a;
int b;
char *string;
{
    int  i=1;
    static char tmpbuf1[mybufsize/4];
    static char tmpbuf2[mybufsize/4];
    char *tmpstr=tmpbuf1;
    char *tmpbuf=tmpbuf1;

    /* Caching works like this:
       You have to call this function with incrementing a or b, i.e.:
       OVgetword(0,2,blah); OVgetword(0,4,blah); OVgetword(5,0,blah);
       This should speed things up and reduce CPU usage.
       First check if this is new notice, and if it is copy entire string to buffer.
       Else, copy old pointer and work from there on, using new indexes   -Flier */
    if (NewNotice) strcpy(tmpbuf1,string);
    else {
        strcpy(tmpbuf1,OldPtr);
        i=OldWord+1;
    }
    /* If a=0, find and return word #b */
    if ((a==0) && (b>0)) {
	for(;i<=b;i++) tmpstr=next_arg(tmpbuf,&tmpbuf);
        /* Made it crash proof since my ircd formats some messages differently */
        if (tmpstr) strcpy(tmpbuf2,tmpstr);
        /* so if there is no word #b we copy empty string   -Flier */
        else *tmpbuf2='\0';
        /* Store current word number */
        OldWord=b;
        /* Store current word pointer */
        OldPtr=tmpbuf;
    }
    /* If a>0 and b=0, return from word #a to end */
    else if ((a>0) && (b==0)) {
        for(;i<a;i++) {
            tmpstr=index(tmpstr,' ');
            if (tmpstr) tmpstr++;
        }
        /* Made it crash proof since my ircd formats some messages differently */
	if (tmpstr) strcpy(tmpbuf2,tmpstr);
        /* so if there is no word #a we copy empty string   -Flier */
        else *tmpbuf2='\0';
        /* Store current word number */
        OldWord=a;
        /* Store current word pointer */
        OldPtr=tmpstr;
    }
    /* Update caching variables
       If there was no word #a or #b start from scratch since we're at the end of
       the string   -Flier */
    if (!OldPtr) NewNotice=1; 
    else NewNotice=0;
    return(tmpbuf2);
}

/* Gets nick form nick!user@host */
char *OVgetnick(nuh)
char *nuh;
{
    char *tmpstr;
    static char tmpbuf[mybufsize/4+1];

    strmcpy(tmpbuf,nuh,mybufsize/4);
    if ((tmpstr=index(tmpbuf,'!'))) *tmpstr='\0';
    return(tmpbuf);
}

void OVformat(line,from)
char *line;
char *from;
{
    char *tmp;
    char *tmpline;
    char *servername;
    char word1[mybufsize/4];
    char word2[mybufsize/4];
    char word3[mybufsize/2];
    char word4[mybufsize/4];
    char tmpbuf[mybufsize];

    /* Set up tmpline to be just the message to parse */
    if (strstr(line,"Notice --")) tmpline=line+14;
    else if (strstr(line,"***")) tmpline=line+4; 
    else tmpline=line;
    strcpy(tmpbuf,tmpline); /* Default if no match is found */
    tmpline=tmpbuf;
    /* If from has '.' in it is is server */
    if (from && index(from,'.')) from=(char *) 0;
    /* We got new notice, needed for caching */
    NewNotice=1;
    /* Now we got the message, use strstr() to match it up */
    if (!strncmp(tmpline,"Connecting to",12)) {
        strcpy(word1,OVgetword(0,3,tmpline));  /* Server */
        sprintf(tmpbuf,"Connecting to %s%s%s",CmdsColors[COLOV].color2,word1,Colors[COLOFF]);
    }
    else if (!strncmp(tmpline,"Entering high-traffic mode: Forced by",36)) {
        strcpy(word2,OVgetword(0,6,tmpline));  /* Nick!User@Host */
        strcpy(word1,OVgetnick(word2));        /* Nick */
        sprintf(tmpbuf,"Entering high traffic mode: forced by %s%s%s %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
    }
    else if (!strncmp(tmpline,"Entering high-traffic mode",25)) {
        strcpy(word1,OVgetword(0,5,tmpline));  /* High speed */
        strcpy(word2,OVgetword(0,7,tmpline));  /* Low speed */
#ifdef CELECOSM
        sprintf(tmpbuf,"high-traffic mode: %s%s%s � %s%s%s",
#else
        sprintf(tmpbuf,"Entering high traffic mode: %s%s%s > %s%s%s",
#endif
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                CmdsColors[COLOV].color2,word2,Colors[COLOFF]);
    }
    else if (!strncmp(tmpline,"Resuming standard operation: Forced by",37)) {
        strcpy(word2,OVgetword(0,6,tmpline));  /* Nick!User@Host */
        strcpy(word1,OVgetnick(word2));        /* Nick */
	sprintf(tmpbuf,"Resuming standard operation: forced by %s%s%s %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
    }
    else if (!strncmp(tmpline,"Resuming standard operation",26)) {
        strcpy(word1,OVgetword(0,4,tmpline));  /* Low speed */
        strcpy(word2,OVgetword(0,6,tmpline));  /* High speed */
#ifdef CELECOSM
        sprintf(tmpbuf,"standard-traffic mode: %s%s%s %s%s%s",
#else
        sprintf(tmpbuf,"Entering standard traffic mode: %s%s%s  %s%s%s",
#endif
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],
		CmdsColors[COLOV].color1,word2,Colors[COLOFF]);
    }
    else if (!strncmp(tmpline,"Client exiting",14)) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Nick */
        if (strstr(tmpline+14," from ")) {
            /* ircd 2.9/2.10 */
            strcpy(word3,OVgetword(0,5,tmpline));  /* user */
	    sprintf(word2,"(%s@%s)",word3,
                    OVgetword(0,7,tmpline));       /* host */
            strcpy(word3,OVgetword(9,0));
        }
        else {
	    strcpy(word2,OVgetword(0,4,tmpline));  /* user@host */
	    strcpy(word3,OVgetword(5,0,tmpline));  /* Reason */
        }
#ifdef CELECOSM
        sprintf(tmpbuf,"clnt/%sexit%s  %s%s%s %s",
                CmdsColors[COLOV].color4,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#else
	sprintf(tmpbuf,"Client %sexiting%s: %s%s%s %s %s",
		CmdsColors[COLOV].color4,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),word3);
#endif
    }
    else if (!strncmp(tmpline,"Client connecting on port",25)) {
        strcpy(word1,OVgetword(0,5,tmpline));  /* port */
        if ((tmp=index(word1,':'))) *tmp='\0';
	strcpy(word2,OVgetword(0,6,tmpline));  /* Nick */
	strcpy(word3,OVgetword(0,7,tmpline));  /* user@host */
#ifdef CELECOSM
        sprintf(tmpbuf,"clnt/%sconnect%s [p%s%s%s]  %s%s%s %s",
#else
        sprintf(tmpbuf,"Client %sconnecting%s on port %s%s%s: %s%s%s %s",
#endif
		CmdsColors[COLOV].color4,Colors[COLOFF],
                CmdsColors[COLOV].color5,word1,Colors[COLOFF],
                CmdsColors[COLOV].color1,word2,Colors[COLOFF],OVuh(word3));
    }
    else if (!strncmp(tmpline,"Client connecting",17)) {
        strcpy(word1,OVgetword(0,3,tmpline));  /* Nick */
        if (strstr(tmpline+17," from ")) {
            /* ircd 2.9/2.10 */
            strcpy(word3,OVgetword(0,5,tmpline));  /* user */
	    sprintf(word2,"(%s@%s)",word3,
                    OVgetword(0,7,tmpline));       /* host */
        }
        else strcpy(word2,OVgetword(0,4,tmpline));  /* user@host */
#ifdef CELECOSM
        sprintf(tmpbuf,"clnt/%sconnect%s  %s%s%s %s",
#else
        sprintf(tmpbuf,"Client %sconnecting%s: %s%s%s %s",
#endif
		CmdsColors[COLOV].color4,Colors[COLOFF],
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
    }
    else if (!strncmp(tmpline,"LINKS requested by",18) ||
             !strncmp(tmpline,"LINKS '",7)) {
        if (*(tmpline+6)=='\'') {
	    sprintf(word3," %s",OVgetword(0,2,tmpline));  /* filter */
            strcpy(word1,OVgetword(0,5,tmpline));  /* Nick */
	    strcpy(word2,OVgetword(0,6,tmpline));  /* user@host */
        }
        else {
            strcpy(word1,OVgetword(0,4,tmpline));  /* Nick */
	    strcpy(word2,OVgetword(0,5,tmpline));  /* user@host */
            *word3='\0';
        }
#ifdef CELECOSM
        sprintf(tmpbuf,"%slinks%s%s from %s%s%s %s",
#else
        sprintf(tmpbuf,"%sLinks%s%s request from %s%s%s %s",
#endif
		CmdsColors[COLOV].color4,Colors[COLOFF],word3,
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
    }
    else if (!strncmp(tmpline,"TRACE requested by",18)) {
        strcpy(word1,OVgetword(0,4,tmpline));  /* Nick */
	strcpy(word2,OVgetword(0,5,tmpline));  /* user@host */
#ifdef CELECOSM
        sprintf(tmpbuf,"%strace%s from %s%s%s %s",
#else
        sprintf(tmpbuf,"%sTrace%s request from %s%s%s %s",
#endif
		CmdsColors[COLOV].color4,Colors[COLOFF],
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
    }
    else if (!strncmp(tmpline,"Rejecting vlad",14)) {
	strcpy(word1,OVgetword(0,4,tmpline));  /* Bot nick */
	strcpy(word2,OVgetword(0,5,tmpline));  /* user@host */
	sprintf(tmpbuf,"Rejecting vlad/joh/com bot: %s%s%s %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
    }
    else if (!strncmp(tmpline,"Rejecting clonebot",17)) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Bot nick */
	strcpy(word2,OVgetword(0,4,tmpline));  /* user@host */
	sprintf(tmpbuf,"Rejecting clonebot: %s%s%s %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
    }
    else if (!strncmp(tmpline,"Identd response differs",22)) {
	strcpy(word1,OVgetword(0,4,tmpline));  /* Nick */
	strcpy(word2,OVgetword(0,5,tmpline));  /* Attemtped IRCUSER */
	sprintf(tmpbuf,"Fault identd response for %s%s%s %c%s%c",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],
		bold,word2,bold);
    }
    else if (!strncmp(tmpline,"Kill line active for",19)) {
	strcpy(word1,OVgetword(0,5,tmpline));  /* Banned client */
#ifdef CELECOSM
        sprintf(tmpbuf,"k-line active: %s%s%s",
#else
        sprintf(tmpbuf,"Active K-Line for %s%s%s",
#endif
                CmdsColors[COLOV].color1,word1,Colors[COLOFF]);
    }
    else if (!strncmp(tmpline,"K-lined ",8)) {
	strcpy(word1,OVgetword(0,2,tmpline));  /* Banned client */
        if (*word1) {
            tmp=word1+strlen(word1)-1;
            *tmp='\0';
        }
#ifdef CELECOSM
        sprintf(tmpbuf,"k-line active: %s%s%s",
#else
        sprintf(tmpbuf,"Active K-line for %s%s%s",
#endif
                CmdsColors[COLOV].color1,word1,Colors[COLOFF]);
    }
    else if (!my_strnicmp(tmpline,"stats ",6)) {
        strcpy(word1,OVgetword(0,2,tmpline));  /* Stat type */
        if (strstr(tmpline+6," from ")) {
            /* ircd 2.9/2.10 */
	    strcpy(word2,OVgetword(0,4,tmpline));  /* Nick */
            strcpy(word4,OVgetword(0,6,tmpline));  /* user */
	    sprintf(word3,"(%s@%s)",word4,
                    OVgetword(0,8,tmpline));       /* host */
            *word4='\0';
        }
        else {
	    strcpy(word2,OVgetword(0,5,tmpline));  /* Nick */
	    strcpy(word3,OVgetword(0,6,tmpline));  /* user@host */
	    strcpy(word4,OVgetword(0,7,tmpline));  /* Server */
        }
#ifdef CELECOSM
        sprintf(tmpbuf,"stats %s%s%s from %s%s%s %s %s%s%s",
#else
        sprintf(tmpbuf,"Stats %s%s%s request from %s%s%s %s %s%s%s",
#endif
		CmdsColors[COLOV].color4,word1,Colors[COLOFF],
		CmdsColors[COLOV].color1,word2,Colors[COLOFF],OVuh(word3),
                CmdsColors[COLOV].color3,word4,Colors[COLOFF]);
    }
    else if (!strncmp(tmpline,"Nick change collision",21)) {
	strcpy(word1,OVgetword(0,5,tmpline));
	strcpy(word2,OVgetword(0,7,tmpline));
	strcpy(word3,OVgetword(8,0,tmpline));
#ifdef CELECOSM
        sprintf(tmpbuf,"nick collide: %s%s%s [%s] %s",
#else
        sprintf(tmpbuf,"Nick change collision: [%s%s%s] [%s] %s",
#endif
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],word2,word3);
    }
    else if (!strncmp(tmpline,"Nick collision on",17)) {
	strcpy(word1,OVgetword(4,0,tmpline));
#ifdef CELECOSM
        sprintf(tmpbuf,"nick collide: %s%s%s",
#else
        sprintf(tmpbuf,"Nick collision: %s%s%s",
#endif
		CmdsColors[COLOV].color2,word1,Colors[COLOFF]);
    }
    else if (!strncmp(tmpline,"Fake ",5)) {
	strcpy(word1,OVgetword(0,2,tmpline));  /* Nick/Server */
	strcpy(word2,OVgetword(0,4,tmpline));  /* channel */
	strcpy(word3,OVgetword(5,0,tmpline));  /* fake modes */
#ifdef CELECOSM
        sprintf(tmpbuf,"fake mode in %s%s%s: \"%s%s%s\" by %s",
                CmdsColors[COLOV].color1,word2,Colors[COLOFF],
                CmdsColors[COLOV].color4,word3,Colors[COLOFF],word1);
#else
        sprintf(tmpbuf,"Fake mode: \"%s%s%s\" in %s%s%s by %s",
                CmdsColors[COLOV].color4,word3,Colors[COLOFF],
                CmdsColors[COLOV].color1,word2,Colors[COLOFF],word1);
#endif
    }
    else if (!strncmp(tmpline,"Too many connect",16)) strcpy(tmpbuf,line);
    else if (!strncmp(tmpline,"Possible bot",12)) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Botnick */
	strcpy(word2,OVgetword(0,4,tmpline));  /* user@host */
	sprintf(tmpbuf,"Possible Bot: %s%s%s %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
    }
    else if (!strncmp(tmpline,"Link with",9)) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Server */
	strcpy(word2,OVgetword(4,0,tmpline));  /* Connect info */
        if ((tmp=index(word2,'('))) {
            *word2='\0';
        }
#ifdef CELECOSM
        sprintf(tmpbuf,"link/%sconnect%s  %s%s%s (%s)",
                CmdsColors[COLOV].color3,Colors[COLOFF],
#else
        sprintf(tmpbuf,"Link: connected to %s%s%s %s%s%s",
#endif
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],
                *word2?"(":"",*word2?word2:"",*word2?")":"");
    }
    else if (!strncmp(tmpline,"Write error to",14)) {
	strcpy(word1,OVgetword(0,4,tmpline));  /* Server */
#ifdef CELECOSM
        sprintf(tmpbuf,"link/%sw.error%s  %s%s%s (closing)",
                CmdsColors[COLOV].color3,Colors[COLOFF],
#else
        sprintf(tmpbuf,"Link: Write error to %s%s%s - closing.",
#endif
		CmdsColors[COLOV].color2,word1,Colors[COLOFF]);
    }
    else if (!strncmp(line,"Message",7)) strcpy(tmpbuf,line);
    else if (!strncmp(tmpline,"Received SQUIT",14)) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Server */
	strcpy(word2,OVgetword(0,5,tmpline));  /* SQUITer */
        strcpy(word3,OVgetword(6,0,tmpline));  /* Reason */
#ifdef CELECOSM
        sprintf(tmpbuf,"link/%ssquit%s  %s%s%s from %s %s",
                CmdsColors[COLOV].color3,Colors[COLOFF],
                CmdsColors[COLOV].color2,word1,Colors[COLOFF],word2,word3);
#else
	sprintf(tmpbuf,"Link: %s%s%s recieved %sSQUIT%s from %s %s",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],
                CmdsColors[COLOV].color4,Colors[COLOFF],word2,word3);
#endif
    }
    else if (!strncmp(tmpline,"Received KILL message for",25)) {
        strcpy(word1,OVgetword(0,5,tmpline));  /* nick */
        if (strlen(word1) && word1[strlen(word1)-1]=='.')
            word1[strlen(word1)-1]='\0';
        strcpy(word2,OVgetword(0,7,tmpline));  /* killer  */
        strcpy(word3,OVgetword(0,7,tmpline));  /* path  */
        strcpy(word4,OVgetword(10,0));         /* reason */
        /* check for server kill first */
        if (index(word2,'.'))
            sprintf(tmpbuf,"Server kill received for %s%s%s from %s%s%s (%s)",
                    CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                    CmdsColors[COLOV].color2,word2,Colors[COLOFF],word4);
        else {
            int foreign=0;
            char *tmp=word3;
            char *tmpuh=NULL;
            char *userhost=word3;

            /* check for foreing kill (more than one !) */
            while (*tmp && (tmp=index(tmp,'!'))) {
                foreign++;
                *tmp++='\0';
                userhost=tmpuh;
                tmpuh=tmp;
            }
            if (foreign>1)
                sprintf(tmpbuf,"Foreign kill received for %s%s%s from %s%s%s %s",
                        CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                        CmdsColors[COLOV].color1,word2,Colors[COLOFF],word4);
            else {
                int locop=strstr(tmpuh,"(L")?1:0;

                sprintf(tmpbuf,"Local kill received for %s%s%s from %s%s%s%s %s",
                        CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                        locop?"local operator ":"",
                        CmdsColors[COLOV].color1,word2,Colors[COLOFF],word4);
            }
        }
    }
    else if (!strncmp(tmpline,"Possible Eggdrop:",17)) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* BotNick */
	strcpy(word2,OVgetword(0,4,tmpline));  /* user@host */
	strcpy(word3,OVgetword(0,5,tmpline));  /* b-line notice */
	sprintf(tmpbuf,"Possible eggdrop: %s%s%s %s %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),word3);
    }
    else if (!strncmp(tmpline,"Nick change",11)) {
        if (strstr(tmpline+6," from ")) {
            /* ircd 2.9/2.10 */
	    strcpy(word1,OVgetword(0,3,tmpline));  /* oldnick */
            strcpy(word2,OVgetword(0,5,tmpline));  /* newnick */
            strcpy(word4,OVgetword(0,7,tmpline));  /* user */
	    sprintf(word3,"(%s@%s)",word4,
                    OVgetword(0,9,tmpline));       /* host */
        }
        else {
            strcpy(word1,OVgetword(0,4,tmpline));  /* oldnick */
	    strcpy(word2,OVgetword(0,6,tmpline));  /* newnick */
            strcpy(word3,OVgetword(0,7,tmpline));  /* user@host */
        }
	sprintf(tmpbuf,"Nick change: %s%s%s to %s%s%s %s",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],
		CmdsColors[COLOV].color1,word2,Colors[COLOFF],OVuh(word3));
    }
    else if (!strncmp(tmpline,"added K-Line",12)) {
	strcpy(word1,OVgetword(0,1,tmpline));  /* Nick */
        strcpy(word2,OVgetword(5,0,tmpline));  /* K-line */
        sprintf(tmpbuf,"K-Line added by %s%s%s - %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],word2);
    }
    else if (!strncmp(tmpline,"Added K-Line [",14)) return;
    else if (!strncmp(tmpline,"Bogus server name",17)) {
	strcpy(word1,OVgetword(0,4,tmpline));  /* Bogus name */
	strcpy(word2,OVgetword(0,6,tmpline));  /* Nick */
	sprintf(tmpbuf,"Bogus server name %s%s%s from %s%s%s",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],
		CmdsColors[COLOV].color1,word2,Colors[COLOFF]);
    }
    else if (!strncmp(tmpline,"No response from",16)) {
	strcpy(word1,OVgetword(0,4,tmpline));  /* Server */
	sprintf(tmpbuf,"Link error: %s%s%s is not responding",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF]);
    }
    else if (!strncmp(tmpline,"IP# Mismatch",12)) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Real IP */
	strcpy(word2,OVgetword(0,5,tmpline));  /* Mismatched IP */
	sprintf(tmpbuf,"IP mismatch detected: %s%s%s != %s%s%s",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],
		CmdsColors[COLOV].color2,word2,Colors[COLOFF]);
    }
    else if (!strncmp(tmpline,"Unauthorized connection from",28)) {
	strcpy(word1,OVgetword(0,4,tmpline));  /* Nick!user@host */
	sprintf(tmpbuf,"Unauthorized connect from %s%s%s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF]);
    }
    else if (!strncmp(tmpline,"Invalid username",16)) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Nick */
	strcpy(word2,OVgetword(0,4,tmpline));  /* username */
	sprintf(tmpbuf,"Invalid username: %s%s%s %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
    }
    else if (!strncmp(tmpline,"Cannot accept connect",21)) {
	strcpy(word1,OVgetword(0,4,tmpline));  /* Nick? */
	strcpy(word2,OVgetword(5,0,tmpline));  /* Stuff? */
	sprintf(tmpbuf,"Unlinkable connection: %s%s%s [%s]",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],word2);
    }
    else if (!strncmp(tmpline,"Lost connection to",18)) {
	strcpy(word1,OVgetword(4,0,tmpline));  /* Server */
	sprintf(tmpbuf,"Link: lost connection to %s%s%s",
                CmdsColors[COLOV].color2,word1,Colors[COLOFF]);
    }
    else if (!my_strnicmp(tmpline,"failed oper",11)) {
        strcpy(word1,OVgetword(0,6,tmpline));  /* n!u@h for ircd 2.9/2.10 */
        if ((tmp=index(word1,'!')) && index(word1,'@')) {
            *tmp++='\0';
            sprintf(word2,"(%s)",tmp);
            strcpy(word3,OVgetword(0,3));
        }
        else {
            strcpy(word1,OVgetword(0,5,tmpline));  /* Nick */
            strcpy(word2,OVgetword(0,6,tmpline));  /* user@host */
            strcpy(word3,OVgetword(0,7,tmpline));  /* OPER nick */
        }
	sprintf(tmpbuf,"Failed OPER attempt: %s%s%s %s %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),word3);
    }
    else if (!strncmp(tmpline,"Global -- Failed OPER attempt",29)) {
        strcpy(word1,OVgetword(0,7,tmpline));  /* Nick */
	strcpy(word2,OVgetword(0,8,tmpline));  /* user@host */
	sprintf(tmpbuf,"Failed OPER attempt: %s%s%s %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
    }
    else if (!strncmp(tmpline,"Sending SQUIT ",14) ||
             !strncmp(tmpline,"Sending SERVER ",15)) {
        strcpy(word1,OVgetword(0,2,tmpline));
        strcpy(word2,OVgetword(0,3,tmpline));
        sprintf(tmpbuf,"Sending %s%s%s %s",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],word2);
    }
    else if (!strncmp(tmpline,"Rejecting connection from ",26)) {
        strcpy(word1,OVgetword(0,4,tmpline));
        sprintf(tmpbuf,"Rejecting connection from %s%s%s",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF]);
    }
    else if (strstr(tmpline,"whois on you")) {
        strcpy(word1,OVgetword(0,1,tmpline));  /* nick */
        strcpy(word2,OVgetword(0,2,tmpline));  /* user@host */
	sprintf(tmpbuf,"%s%s%s %s is doing a %sWhois%s on you.",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),
		CmdsColors[COLOV].color4,Colors[COLOFF]);
    }
    else if (strstr(tmpline,"Flooder") && strstr(tmpline,"target")) {
        strcpy(word1,OVgetword(0,2,tmpline)); /* nick */
        strcpy(word2,OVgetword(0,3,tmpline)); /* user@host */
        strcpy(word3,OVgetword(0,5,tmpline)); /* Server */
        strcpy(word4,OVgetword(0,7,tmpline)); /* Channel */
#ifdef CELECOSM
        sprintf(tmpbuf,"clnt/%sflood%s %s%s%s -> %s%s%s [%s]",
                CmdsColors[COLOV].color4,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                CmdsColors[COLOV].color2,word4,Colors[COLOFF],word3);
#else
        sprintf(tmpbuf,"Flooder %s%s%s %s -> %s%s%s [%s]",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),
                CmdsColors[COLOV].color2,word4,Colors[COLOFF],word3);
#endif
    }
    else if (strstr(tmpline,"is now operator")) {
	strcpy(word1,OVgetword(0,1,tmpline));  /* Nick */
        strcpy(word2,OVgetword(0,2,tmpline));  /* user@host */
        strcpy(word3,OVgetword(0,6,tmpline));  /* o/O */
        tmp=word3;
        if (*tmp) tmp++;
        if (get_server_version(from_server)==Server2_9 || 
            get_server_version(from_server)==Server2_10) {
            if (*tmp=='o') *tmp='O';
            else *tmp='o';
        }
#ifdef CELECOSM
        sprintf(tmpbuf,"%s%s%s %s is an IRC warrior %s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),
                *tmp?(*tmp=='O'?"(global)":"(local)"):"");
#else
	sprintf(tmpbuf,"%s%s%s %s is now %sIRC Operator.",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),
                *tmp?(*tmp=='O'?"global ":"local "):"");
#endif
    }
    else if (strstr(tmpline,"is rehashing Server config")) {
        strcpy(word1,OVgetword(0,1,tmpline));  /* Nick */
#ifdef CELECOSM
        sprintf(tmpbuf,"config rehash by %s%s%s",
#else
        sprintf(tmpbuf,"%s%s%s is rehashing the server config file.",
#endif
                CmdsColors[COLOV].color1,word1,Colors[COLOFF]);
    }
    else if (strstr(tmpline,"tried to msg")) {
        strcpy(word1,OVgetword(0,2,tmpline));  /* nick */
        strcpy(word2,OVgetword(0,3,tmpline));  /* user@host */
        strcpy(word3,OVgetword(0,7,tmpline));  /* number */
        sprintf(tmpbuf,"User %s%s%s %s tried to message %s%s%s users",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),
                CmdsColors[COLOV].color2,word3,Colors[COLOFF]);
    }
    else if ((strstr(tmpline,"Possible")) && (strstr(tmpline,"bot"))) {
	strcpy(word1,OVgetword(0,4,tmpline));  /* Bot Nick */
	strcpy(word2,OVgetword(0,5,tmpline));  /* User@Host */
	sprintf(tmpbuf,"Possible IrcBot: %s%s%s %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
    }
    else if (strstr(tmpline,"ERROR :from")) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Error Source */
	strcpy(word2,OVgetword(0,7,tmpline));  /* ? */
	strcpy(word3,OVgetword(8,0,tmpline));  /* Error */
	sprintf(tmpbuf,"Error: %s%s%s - close link %s%s%s %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],
		CmdsColors[COLOV].color2,word2,Colors[COLOFF],word3);
    }
    else if (strstr(tmpline,"closed the connection")) {
	strcpy(word1,OVgetword(0,2,tmpline));  /* Server */
	sprintf(tmpbuf,"Link Error: %s%s%s closed the connection.",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF]);
    }
    else if ((strstr(tmpline,"Connection to")) && (strstr(tmpline,"activated"))) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Server */
	sprintf(tmpbuf,"Link: Connecting to %s%s%s",CmdsColors[COLOV].color2,word1,Colors[COLOFF]);
    }
    else if (strstr(tmpline,"connect failure:")) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Failed Server */
	strcpy(word2,OVgetword(4,0,tmpline));  /* Reason */
	sprintf(tmpbuf,"Failed connect from %s%s%s [%s]",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],word2);
    }
    else if ((tmp=strstr(tmpline," added a ")) && strstr(tmp,"kline")) {
	strcpy(word1,OVgetword(0,1,tmpline));  /* Nick */
        strcpy(word2,OVgetword(6,0,tmpline));  /* K-line */
        sprintf(tmpbuf,"K-Line added by %s%s%s - %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],word2);
    }
    else if (strstr(tmpline," added K-Line ")) {
	strcpy(word1,OVgetword(0,1,tmpline));  /* Nick */
        strcpy(word2,OVgetword(5,0,tmpline));  /* K-line */
        sprintf(tmpbuf,"K-Line added by %s%s%s - %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],word2);
    }
    servername=server_list[from_server].itsname;
    if (!servername) servername=server_list[from_server].name;
    if (from) put_it("[%s%s%s] Opermsg from %s%s%s: %s",
                     CmdsColors[COLOV].color6,OVsvdmn(servername),Colors[COLOFF],
                     CmdsColors[COLOV].color1,from,Colors[COLOFF],tmpbuf);
    else put_it("[%s%s%s] %s",
                CmdsColors[COLOV].color6,OVsvdmn(servername),Colors[COLOFF],tmpbuf);
}
#endif
