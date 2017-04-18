/*
 * XBar: simple X11 indicator bar
 *       2014:
             PiXy <ashtoreth@ukr.net>
 *
 * May be redistributed under the terms of the MIT license.
 *
 * Code originators:
 *       1998-2001:
 *           Suguru Yamaguchi
 *           Akira Kato
 *           Noriyuki Shigechika
 *           Susumu Sano
 *           Hiroshi Ura
 *           Yuji Sekiya
 *       2009:
 *           Dmitry E. Oboukhov
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <X11/Xlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include "xbari.h"

/*
 * Global variables
 */

extern FILE *stdin;

int xib_online = -1;		/* online status */
int xib_level = -1;		/* indicator level */

unsigned long onin, onout;	/* indicator colors for online */
unsigned long offin, offout;	/* indicator colors for offline */

struct itimerval IntervalTimer;	/* polling interval timer */

Display *disp;
Window winbar;			/* bar indicator window */
Window winstat = -1;		/* pop-up status window */
GC gcbar;
GC gcstat;
unsigned int width, height;
XEvent theEvent;

/*
 * function prototypes
 */
void InitDisplay (void);
Status AllocColor (char *, unsigned long *);
void check_input (void);
void process_bar(int, int);
void redraw (void);
void showdiagbox (void);
void disposediagbox (void);
void usage (char **);
void about_this_program (void);

/*
 * usage of this command
 */
void
about_this_program ()
{
  fprintf (stderr,
	   "This is %s version %s "
	   "for YaWM project (c) 2014 PiXy \n", XBARNAME, VERSION);
}

void
usage (char **argv)
{
  fprintf (stderr,
	   "\n"
	   "usage:\t%s [-a] [-h|v] [-p sec] [-t thickness]\n"
	   "\t\t[-I color] [-O color] [-i color] [-o color]\n"
	   "\t\t[ top | bottom | left | right ]\n"
	   "-a:         always on top.\n"
	   "-v, -h:     show this message.\n"
	   "-t:         bar (indicator) thickness. [def: 3 pixels]\n"
	   "-p:         polling interval. [def: 10 sec.]\n"
	   "-I, -O:     bar colors in on-line. [def: \"green\" & \"olive drab\"]\n"
	   "-i, -o:     bar colors in off-line. [def: \"blue\" and \"red\"]\n"
	   "top, bottom, left, right: bar localtion. [def: \"bottom\"]\n"
	   "\n"
	   "-f:         string format for input data\n"
	   "-s script:  use external script for getting battery status (FFU)\n",
	   argv[0]);
  _exit (0);
}

/*
 * AllocColor:
 * convert color name to pixel value
 */
Status
AllocColor (char *name, unsigned long *pixel)
{
  XColor color, exact;
  int status;

  status = XAllocNamedColor (disp, DefaultColormap (disp, 0),
			     name, &color, &exact);
  *pixel = color.pixel;

  return (status);
}

/*
 * InitDisplay:
 * create small window in top or bottom
 */
void
InitDisplay (void)
{
  Window root;
  int x, y;
  unsigned int border, depth;
  XSetWindowAttributes att;

  if ((disp = XOpenDisplay (NULL)) == NULL)
    {
      fprintf (stderr, "%s: can't open display.\n", XBARNAME);
      _exit (1);
    }

  if (XGetGeometry (disp, DefaultRootWindow (disp), &root, &x, &y,
		    &width, &height, &border, &depth) == 0)
    {
      fprintf (stderr, "%s: can't get window geometry\n", XBARNAME);
      _exit (1);
    }

  if (!AllocColor (ONIN_C, &onin) ||
      !AllocColor (OFFOUT_C, &offout) ||
      !AllocColor (OFFIN_C, &offin) || !AllocColor (ONOUT_C, &onout))
    {
      fprintf (stderr, "%s: can't allocate color resources\n", XBARNAME);
      _exit (1);
    }

  switch (xib_direction)
    {
    case XIB_Top:		/* (0,0) - (width, xib_thick) */
      xib_width = width;
      xib_height = xib_thick;
      xib_x = 0;
      xib_y = 0;
      break;
    case XIB_Bottom:
      xib_width = width;
      xib_height = xib_thick;
      xib_x = 0;
      xib_y = height - xib_thick;
      break;
    case XIB_Left:
      xib_width = xib_thick;
      xib_height = height;
      xib_x = 0;
      xib_y = 0;
      break;
    case XIB_Right:
      xib_width = xib_thick;
      xib_height = height;
      xib_x = width - xib_thick;
      xib_y = 0;
    }

  winbar = XCreateSimpleWindow (disp, DefaultRootWindow (disp),
				xib_x, xib_y, xib_width, xib_height,
				0, BlackPixel (disp, 0), WhitePixel (disp,
								     0));

  /* make this window without its titlebar */
  att.override_redirect = True;
  XChangeWindowAttributes (disp, winbar, CWOverrideRedirect, &att);

  XMapWindow (disp, winbar);

  gcbar = XCreateGC (disp, winbar, 0, 0);
}

main (int argc, char **argv)
{
  extern char *optarg;
  extern int optind;
  int ch;

  about_this_program ();
  while ((ch = getopt (argc, argv, "at:f:hI:i:O:o:p:vs:f:")) != -1)
    switch (ch)
      {
      case 'f':
	FORMAT_STRING = optarg;
	break;

      case 's':
	EXTERNAL_CHECK = optarg;
	break;

      case 'a':
	alwaysontop = True;
	break;

      case 't':
	xib_thick = atoi (optarg);
	break;

      case 'I':
	ONIN_C = optarg;
	break;
      case 'i':
	OFFIN_C = optarg;
	break;
      case 'O':
	ONOUT_C = optarg;
	break;
      case 'o':
	OFFOUT_C = optarg;
	break;

      case 'p':
	xib_interval = atoi (optarg);
	break;

      case 'h':
      case 'v':
	usage (argv);
	break;
      }
  argc -= optind;
  argv += optind;

  if (argc > 0)
    {
      if (strcasecmp (*argv, "top") == 0)
	xib_direction = XIB_Top;
      else if (strcasecmp (*argv, "bottom") == 0)
	xib_direction = XIB_Bottom;
      else if (strcasecmp (*argv, "left") == 0)
	xib_direction = XIB_Left;
      else if (strcasecmp (*argv, "right") == 0)
	xib_direction = XIB_Right;
    }

  /*
   * set APM polling interval timer
   */
  if (!xib_interval)
    {
      fprintf (stderr, "%s: can't set interval timer\n", XBARNAME);
      _exit (1);
    }
  alarm (xib_interval);

  /*
   * X Window main loop
   */
  InitDisplay ();
  check_input ();
  XSelectInput (disp, winbar, myEventMask);
  while (1)
    {
      XWindowEvent (disp, winbar, myEventMask, &theEvent);
      switch (theEvent.type)
	{
	case Expose:
	  /* we redraw our window since our window has been exposed. */
	  redraw ();
	  break;

	case EnterNotify:
	  /* create battery status message */
	  showdiagbox ();
	  break;

	case LeaveNotify:
	  /* destroy status window */
	  disposediagbox ();
	  break;

	case VisibilityNotify:
	  if (alwaysontop)
	    XRaiseWindow (disp, winbar);
	  break;

	default:
	  /* for debugging */
	  fprintf (stderr,
		   "%s: unknown event (%d) captured\n",
		   XBARNAME, theEvent.type);
	}
    }
}

void
redraw (void)
{
  process_bar(xib_level, xib_online);
}


void
showdiagbox (void)
{
  XSetWindowAttributes att;
  XFontStruct *fontp;
  XGCValues theGC;
  int pixw, pixh;
  int boxw, boxh;
  char diagmsg[64];

  /* compose diag message and calculate its size in pixels */
  sprintf (diagmsg,
	   "Input is %s-line. Level is %d%%",
	   xib_online ? "on" : "off", xib_level);
  fontp = XLoadQueryFont (disp, DefaultFont);
  pixw = XTextWidth (fontp, diagmsg, strlen (diagmsg));
  pixh = fontp->ascent + fontp->descent;
  boxw = pixw + DiagXMergin * 2;
  boxh = pixh + DiagYMergin * 2;

  /* create status window */
  if (winstat != -1)
    disposediagbox ();
  winstat = XCreateSimpleWindow (disp, DefaultRootWindow (disp),
				 (width - boxw) / 2, (height - boxh) / 2,
				 boxw, boxh,
				 2, BlackPixel (disp, 0), WhitePixel (disp,
								      0));

  /* make this window without time titlebar */
  att.override_redirect = True;
  XChangeWindowAttributes (disp, winstat, CWOverrideRedirect, &att);
  XMapWindow (disp, winstat);
  theGC.font = fontp->fid;
  gcstat = XCreateGC (disp, winstat, GCFont, &theGC);
  XDrawString (disp, winstat,
	       gcstat,
	       DiagXMergin, fontp->ascent + DiagYMergin,
	       diagmsg, strlen (diagmsg));
}

void
disposediagbox (void)
{
  if (winstat != -1)
    {
      XDestroyWindow (disp, winstat);
      winstat = -1;
    }
}

void
process_bar (int left, int xib_state)
{
  int pos;
  if (XIB_Horizontal)
    {
      pos = width * left / 100;
      XSetForeground (disp, gcbar, xib_state ? onin : offin);
      XFillRectangle (disp, winbar, gcbar, 0, 0, pos, xib_thick);
      XSetForeground (disp, gcbar, xib_state? onout : offout);
      XFillRectangle (disp, winbar, gcbar, xib_state ? (pos + 1) : pos, 0, width, xib_thick);
    }
  else
    {
      pos = height * left / 100;
      XSetForeground (disp, gcbar, xib_state ? onin : offin);
      XFillRectangle (disp, winbar, gcbar, 0, height - pos, xib_thick,
		      height);
      XSetForeground (disp, gcbar, xib_state ? onout : offout);
      XFillRectangle (disp, winbar, gcbar, 0, 0, xib_thick, height - pos);
    }
  XFlush (disp);
}

inline static int
read_input (int fd, char *buffer)
{
  int rd = read (fd, buffer, TEMP_BUFFER_SIZE - 1);
  if (rd == -1)
    {
      perror ("read");
      return 0;
    }
  buffer[rd] = 0;
  return rd;
}

void
print_script_error (void)
{
  fprintf (stderr, "\nExternal script must print exactly the string,\n"
	   "specified by '-f <format string>' argument:\n"
	   "\t '%s' where first value between 0 and 100\n"
	   "\tand second if a boolean type 1|0, means on|off\n",
	   FORMAT_STRING);

}

void
check_input (void)
{
  int len, value, status;
  char buffer[TEMP_BUFFER_SIZE];

  len = read_input (fileno (stdin), buffer);

#ifdef DEBUG
  fprintf (stderr, "raw input: %s", buffer);
#endif

  if (!len)
    {
      print_script_error ();
      goto exit_check;
    }

  sscanf (buffer, FORMAT_STRING, &value, &status);

  xib_level = value;
  if (xib_level > 100)
    fprintf (stderr, "Incorrect input level "
	     " has been received: %d%%\n", xib_level);

  xib_online = (status >= 1) ? 1 : 0;

#ifdef DEBUG
  printf ("Received level: %d%%, xib_online: %s\n",
	  xib_level, xib_online ? "true" : "false");
#endif

exit_check:
  redraw ();
  signal (SIGALRM, (void *) (check_input));
  alarm (xib_interval);
}
