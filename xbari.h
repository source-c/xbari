#ifndef __XBARI_H
#define __XBARI_H

static const char *VERSION = "1.0.0";
static const char *XBARNAME = "xbar-indicator";

#define PollingInterval  10	/* polling interval in sec */
#define XIB_THICKNESS    3	/* indicator thickness in pixels */

#define XIB_Bottom	0
#define XIB_Top		1
#define XIB_Left	2
#define XIB_Right	3
#define XIB_Horizontal	((xib_direction & 2) == 0)
#define XIB_Vertical	((xib_direction & 2) == 2)

#define myEventMask (ExposureMask|EnterWindowMask|LeaveWindowMask|VisibilityChangeMask)
#define DefaultFont "fixed"
#define DiagXMergin 20
#define DiagYMergin 5

#define CriticalLevel  5

#define TEMP_BUFFER_SIZE 4096

char *EXTERNAL_CHECK = NULL;
char *FORMAT_STRING = "battery=%d, ac_line=%d";

/* indicator default colors */
char *ONIN_C = "green";
char *ONOUT_C = "olive drab";
char *OFFIN_C = "blue";
char *OFFOUT_C = "red";

int alwaysontop = False;

int xib_direction = XIB_Bottom;	/* status bar location */
int xib_height;			/* height of Indicator */
int xib_width;			/* width of Indicator */
int xib_x;			/* x coordinate of upper left corner */
int xib_y;			/* y coordinate of upper left corner */
int xib_thick = XIB_THICKNESS;	/* thickness of  Indicator */
int xib_interval = PollingInterval;	/* interval of polling subsystem */

#endif
