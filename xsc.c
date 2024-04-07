/*xsc-x selection clipboard.
reads stdin and makes it an X CLIPBOARD selection.
deps:libX11.
build:cc -o xsc xsc.c -lX11
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <X11/Xlib.h>

#define BUFLEN 4096

static Atom CLIPATM;
static Atom STRATM;
static Atom XSTRATM;
static Atom UTF8STRATM;
static Atom TXTATM;
static Atom TRGTSATM;

static unsigned char* DATA=NULL;

//obtain atoms.
void
obtatms(Display* dpl)
{
	CLIPATM=XInternAtom(dpl,"CLIPBOARD",False);
	STRATM=XInternAtom(dpl,"STRING",False);
	XSTRATM=XInternAtom(dpl,"XA_STRING",False);
	UTF8STRATM=XInternAtom(dpl,"UTF8_STRING",False);
	TXTATM=XInternAtom(dpl,"TEXT",False);
	TRGTSATM=XInternAtom(dpl,"TARGETS",False);	
};

//selection notify.
void
selntf(XSelectionRequestEvent* srev,Atom prop)
{
	XSelectionEvent snev;
	snev.type=SelectionNotify;
	snev.serial=srev->serial;
	snev.send_event=srev->send_event;
	snev.display=srev->display;
	snev.requestor=srev->requestor;
	snev.selection=srev->selection;
	snev.target=srev->target;
	snev.property=prop;
	snev.time=srev->time;
	XSendEvent(srev->display,srev->requestor,False,NoEventMask,(XEvent*)&snev);
};

//selection targets reply.
void
seltrgtsrpl(XSelectionRequestEvent* srev)
{
	Atom trgts[]={UTF8STRATM,XSTRATM,STRATM,TXTATM};
	XChangeProperty(srev->display,srev->requestor,srev->property,srev->target,32,PropModeReplace,(unsigned char*)trgts,(int)(sizeof(trgts)/sizeof(Atom)));
	selntf(srev,srev->property);
};

//selection reply.
void
selrpl(XSelectionRequestEvent* srev,unsigned char* data)
{
	XChangeProperty(srev->display,srev->requestor,srev->property,srev->target,8,PropModeReplace,data,strlen(data));
	selntf(srev,srev->property);
};

//selection deny.
void
seldeny(XSelectionRequestEvent* srev)
{
	selntf(srev,None);
};

void
wdie(Display* dpl)
{
	XCloseDisplay(dpl);
	free(DATA);
	exit(0);
};

void
wrun(void)
{
	Display* dpl=XOpenDisplay(NULL);
	if(dpl==NULL)
	{
		dprintf(2,"err: cannot open a display.\n");
		exit(1);
	}
	obtatms(dpl);
	Window w=XCreateSimpleWindow(dpl,DefaultRootWindow(dpl),0,0,1,1,0,0,0);
	while(XGetSelectionOwner(dpl,CLIPATM)!=w)
	{
		XSetSelectionOwner(dpl,CLIPATM,w,CurrentTime);
	};
	XEvent ev;
	for(;;)
	{
		XNextEvent(dpl,&ev);
		switch(ev.type)
		{
			case SelectionRequest:
			{
				XSelectionRequestEvent* srev=&(ev.xselectionrequest);
				if(srev->property==None)
				{
					seldeny(srev);
					break;
				}
				if(srev->target==TRGTSATM)
				{
					seltrgtsrpl(srev);
					break;
				}
				else if((srev->target==STRATM)||(srev->target==XSTRATM)||(srev->target==UTF8STRATM)||(srev->target==TXTATM))
				{
					selrpl(srev,DATA);
					break;
				}
				else
				{
					seldeny(srev);
					break;
				}
			};
			case SelectionClear:
			{
				wdie(dpl);
			};
		};
	};
};

int
main(void)
{
	char buf[BUFLEN];
	int inlen=0;
	char* in=NULL;
	ssize_t rdr;
	while((rdr=read(0,&buf,BUFLEN))>0)
	{
		buf[rdr]=0;
		inlen+=rdr;
		in=realloc(in,inlen);
		if(in==NULL)
		{
			dprintf(2,"err: error during allocating memory for input.\n");
			exit(1);
		}
		strcat(in,buf);
	};
	DATA=in;
	pid_t forkr=fork();
	if(forkr==-1)
	{
		dprintf(2,"err: fork error.\n");
		free(DATA);
		exit(1);
	}
	else if(forkr==0)
	{
		wrun();
	}
	else
	{
		free(DATA);
		exit(0);
	}
};