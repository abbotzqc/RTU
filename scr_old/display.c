#include<signal.h>
#include<sys/time.h>
#include<stdio.h>
#include<string.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<sys/sem.h>
#include<sys/shm.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>
#include<fcntl.h>
#include<errno.h>
#include<signal.h>
#include<pthread.h>
#include"sem.h"
#include<regex.h>
#include<sys/stat.h>

#include <stdint.h>
#include <linux/input.h>

#ifndef EV_SYN
#define EV_SYN 0
#endif


#define DISPLAY 1

#define N sizeof(shm_s)
#define N_NODE 4
#define N_OPT 12
#define PATH_PARM "/rturun/parm.log"
#define PATH_SHM  "/rturun/app"

#define PRINTALL              21
#define PRINT_RTU             22
#define PRINT_NODE_ONE        23
#define PRINT_NODE_TWO        24
#define PRINT_NODE_THR        25
#define PRINT_NODE_FOU        26

#define REC_RTUADDR            11
#define REC_RTUTYPE            12
#define REC_SEQL               13
#define REC_SWITCH             14
#define REC_TIMEINT1           15
#define REC_TIMEINT2           16
#define REC_TIMEINT3           17
#define REC_NODE               18
#define REC_SEN                19
#define REC_DUR                21
#define REC_SERIP              22
#define REC_SERPORT            23
#define REC_CLIIP              24
#define REC_CLIPORT            25
#define REC_OTS                26
#define REC_OTC                27

#define N_NODE    4

/***********************************
 *  key used
 *
 * */

char *events[EV_MAX + 1] = {
        [0 ... EV_MAX] = NULL,
        [EV_SYN] = "Sync",                      [EV_KEY] = "Key",
        [EV_REL] = "Relative",                  [EV_ABS] = "Absolute",
        [EV_MSC] = "Misc",                      [EV_LED] = "LED",
        [EV_SND] = "Sound",                     [EV_REP] = "Repeat",
        [EV_FF] = "ForceFeedback",              [EV_PWR] = "Power",
        [EV_FF_STATUS] = "ForceFeedbackStatus",
};

char *keys[KEY_MAX + 1] = {
        [0 ... KEY_MAX] = NULL,
        [KEY_RESERVED] = "Reserved",            [KEY_ESC] = "Esc",
        [KEY_1] = "1",                          [KEY_2] = "2",
        [KEY_3] = "3",                          [KEY_4] = "4",
        [KEY_5] = "5",                          [KEY_6] = "6",
        [KEY_7] = "7",                          [KEY_8] = "8",
        [KEY_9] = "9",                          [KEY_0] = "0",
        [KEY_MINUS] = "Minus",                  [KEY_EQUAL] = "Equal",
        [KEY_BACKSPACE] = "Backspace",          [KEY_TAB] = "Tab",
        [KEY_Q] = "Q",                          [KEY_W] = "W",
        [KEY_E] = "E",                          [KEY_R] = "R",
        [KEY_T] = "T",                          [KEY_Y] = "Y",
        [KEY_U] = "U",                          [KEY_I] = "I",
        [KEY_O] = "O",                          [KEY_P] = "P",
        [KEY_LEFTBRACE] = "LeftBrace",          [KEY_RIGHTBRACE] = "RightBrace",
        [KEY_ENTER] = "Enter",                  [KEY_LEFTCTRL] = "LeftControl",
        [KEY_A] = "A",                          [KEY_S] = "S",
        [KEY_D] = "D",                          [KEY_F] = "F",
        [KEY_G] = "G",                          [KEY_H] = "H",
        [KEY_J] = "J",                          [KEY_K] = "K",
        [KEY_L] = "L",                          [KEY_SEMICOLON] = "Semicolon",
        [KEY_APOSTROPHE] = "Apostrophe",        [KEY_GRAVE] = "Grave",
        [KEY_LEFTSHIFT] = "LeftShift",          [KEY_BACKSLASH] = "BackSlash",
        [KEY_Z] = "Z",                          [KEY_X] = "X",
        [KEY_C] = "C",                          [KEY_V] = "V",
        [KEY_B] = "B",                          [KEY_N] = "N",
        [KEY_M] = "M",                          [KEY_COMMA] = "Comma",
        [KEY_DOT] = "Dot",                      [KEY_SLASH] = "Slash",
        [KEY_RIGHTSHIFT] = "RightShift",        [KEY_KPASTERISK] = "KPAsterisk",
        [KEY_LEFTALT] = "LeftAlt",              [KEY_SPACE] = "Space",
        [KEY_CAPSLOCK] = "CapsLock",            [KEY_F1] = "F1",
        [KEY_F2] = "F2",                        [KEY_F3] = "F3",
        [KEY_F4] = "F4",                        [KEY_F5] = "F5",
        [KEY_F6] = "F6",                        [KEY_F7] = "F7",
        [KEY_F8] = "F8",                        [KEY_F9] = "F9",
        [KEY_F10] = "F10",                      [KEY_NUMLOCK] = "NumLock",
        [KEY_SCROLLLOCK] = "ScrollLock",        [KEY_KP7] = "KP7",
        [KEY_KP8] = "KP8",                      [KEY_KP9] = "KP9",
        [KEY_KPMINUS] = "KPMinus",              [KEY_KP4] = "KP4",
        [KEY_KP5] = "KP5",                      [KEY_KP6] = "KP6",
        [KEY_KPPLUS] = "KPPlus",                [KEY_KP1] = "KP1",
        [KEY_KP2] = "KP2",                      [KEY_KP3] = "KP3",
        [KEY_KP0] = "KP0",                      [KEY_KPDOT] = "KPDot",
        [KEY_ZENKAKUHANKAKU] = "Zenkaku/Hankaku", [KEY_102ND] = "102nd",
        [KEY_F11] = "F11",                      [KEY_F12] = "F12",
        [KEY_RO] = "RO",                        [KEY_KATAKANA] = "Katakana",
        [KEY_HIRAGANA] = "HIRAGANA",            [KEY_HENKAN] = "Henkan",
        [KEY_KATAKANAHIRAGANA] = "Katakana/Hiragana", [KEY_MUHENKAN] = "Muhenkan",
        [KEY_KPJPCOMMA] = "KPJpComma",          [KEY_KPENTER] = "KPEnter",
        [KEY_RIGHTCTRL] = "RightCtrl",          [KEY_KPSLASH] = "KPSlash",
        [KEY_SYSRQ] = "SysRq",                  [KEY_RIGHTALT] = "RightAlt",
        [KEY_LINEFEED] = "LineFeed",            [KEY_HOME] = "Home",
        [KEY_UP] = "Up",                        [KEY_PAGEUP] = "PageUp",
        [KEY_LEFT] = "Left",                    [KEY_RIGHT] = "Right",
        [KEY_END] = "End",                      [KEY_DOWN] = "Down",
        [KEY_PAGEDOWN] = "PageDown",            [KEY_INSERT] = "Insert",
        [KEY_DELETE] = "Delete",                [KEY_MACRO] = "Macro",
        [KEY_MUTE] = "Mute",                    [KEY_VOLUMEDOWN] = "VolumeDown",
        [KEY_VOLUMEUP] = "VolumeUp",            [KEY_POWER] = "Power",
        [KEY_KPEQUAL] = "KPEqual",              [KEY_KPPLUSMINUS] = "KPPlusMinus",
        [KEY_PAUSE] = "Pause",                  [KEY_KPCOMMA] = "KPComma",
        [KEY_HANGUEL] = "Hanguel",              [KEY_HANJA] = "Hanja",
        [KEY_YEN] = "Yen",                      [KEY_LEFTMETA] = "LeftMeta",
        [KEY_RIGHTMETA] = "RightMeta",          [KEY_COMPOSE] = "Compose",
        [KEY_STOP] = "Stop",                    [KEY_AGAIN] = "Again",
        [KEY_PROPS] = "Props",                  [KEY_UNDO] = "Undo",
        [KEY_FRONT] = "Front",                  [KEY_COPY] = "Copy",
        [KEY_OPEN] = "Open",                    [KEY_PASTE] = "Paste",
        [KEY_FIND] = "Find",                    [KEY_CUT] = "Cut",
        [KEY_HELP] = "Help",                    [KEY_MENU] = "Menu",
        [KEY_CALC] = "Calc",                    [KEY_SETUP] = "Setup",
        [KEY_SLEEP] = "Sleep",                  [KEY_WAKEUP] = "WakeUp",
        [KEY_FILE] = "File",                    [KEY_SENDFILE] = "SendFile",
        [KEY_DELETEFILE] = "DeleteFile",        [KEY_XFER] = "X-fer",
        [KEY_PROG1] = "Prog1",                  [KEY_PROG2] = "Prog2",
        [KEY_WWW] = "WWW",                      [KEY_MSDOS] = "MSDOS",
        [KEY_COFFEE] = "Coffee",                [KEY_DIRECTION] = "Direction",
        [KEY_CYCLEWINDOWS] = "CycleWindows",    [KEY_MAIL] = "Mail",
        [KEY_BOOKMARKS] = "Bookmarks",          [KEY_COMPUTER] = "Computer",
        [KEY_BACK] = "Back",                    [KEY_FORWARD] = "Forward",
        [KEY_CLOSECD] = "CloseCD",              [KEY_EJECTCD] = "EjectCD",
        [KEY_EJECTCLOSECD] = "EjectCloseCD",    [KEY_NEXTSONG] = "NextSong",
        [KEY_PLAYPAUSE] = "PlayPause",          [KEY_PREVIOUSSONG] = "PreviousSong",
        [KEY_STOPCD] = "StopCD",                [KEY_RECORD] = "Record",
        [KEY_REWIND] = "Rewind",                [KEY_PHONE] = "Phone",
        [KEY_ISO] = "ISOKey",                   [KEY_CONFIG] = "Config",
        [KEY_HOMEPAGE] = "HomePage",            [KEY_REFRESH] = "Refresh",
        [KEY_EXIT] = "Exit",                    [KEY_MOVE] = "Move",
        [KEY_EDIT] = "Edit",                    [KEY_SCROLLUP] = "ScrollUp",
        [KEY_SCROLLDOWN] = "ScrollDown",        [KEY_KPLEFTPAREN] = "KPLeftParenthesis",
        [KEY_KPRIGHTPAREN] = "KPRightParenthesis", [KEY_F13] = "F13",
        [KEY_F14] = "F14",                      [KEY_F15] = "F15",
        [KEY_F16] = "F16",                      [KEY_F17] = "F17",
        [KEY_F18] = "F18",                      [KEY_F19] = "F19",
        [KEY_F20] = "F20",                      [KEY_F21] = "F21",
        [KEY_F22] = "F22",                      [KEY_F23] = "F23",
        [KEY_F24] = "F24",                      [KEY_PLAYCD] = "PlayCD",
        [KEY_PAUSECD] = "PauseCD",              [KEY_PROG3] = "Prog3",
        [KEY_PROG4] = "Prog4",                  [KEY_SUSPEND] = "Suspend",
        [KEY_CLOSE] = "Close",                  [KEY_PLAY] = "Play",
        [KEY_FASTFORWARD] = "Fast Forward",     [KEY_BASSBOOST] = "Bass Boost",
        [KEY_PRINT] = "Print",                  [KEY_HP] = "HP",
        [KEY_CAMERA] = "Camera",                [KEY_SOUND] = "Sound",
        [KEY_QUESTION] = "Question",            [KEY_EMAIL] = "Email",
        [KEY_CHAT] = "Chat",                    [KEY_SEARCH] = "Search",
        [KEY_CONNECT] = "Connect",              [KEY_FINANCE] = "Finance",
        [KEY_SPORT] = "Sport",                  [KEY_SHOP] = "Shop",
        [KEY_ALTERASE] = "Alternate Erase",     [KEY_CANCEL] = "Cancel",
        [KEY_BRIGHTNESSDOWN] = "Brightness down", [KEY_BRIGHTNESSUP] = "Brightness up",
        [KEY_MEDIA] = "Media",                  [KEY_UNKNOWN] = "Unknown",
        [BTN_0] = "Btn0",                       [BTN_1] = "Btn1",
        [BTN_2] = "Btn2",                       [BTN_3] = "Btn3",
        [BTN_4] = "Btn4",                       [BTN_5] = "Btn5",
        [BTN_6] = "Btn6",                       [BTN_7] = "Btn7",
        [BTN_8] = "Btn8",                       [BTN_9] = "Btn9",
        [BTN_LEFT] = "LeftBtn",                 [BTN_RIGHT] = "RightBtn",
        [BTN_MIDDLE] = "MiddleBtn",             [BTN_SIDE] = "SideBtn",
        [BTN_EXTRA] = "ExtraBtn",               [BTN_FORWARD] = "ForwardBtn",
        [BTN_BACK] = "BackBtn",                 [BTN_TASK] = "TaskBtn",
        [BTN_TRIGGER] = "Trigger",              [BTN_THUMB] = "ThumbBtn",
        [BTN_THUMB2] = "ThumbBtn2",             [BTN_TOP] = "TopBtn",
        [BTN_TOP2] = "TopBtn2",                 [BTN_PINKIE] = "PinkieBtn",
        [BTN_BASE] = "BaseBtn",                 [BTN_BASE2] = "BaseBtn2",
        [BTN_BASE3] = "BaseBtn3",               [BTN_BASE4] = "BaseBtn4",
        [BTN_BASE5] = "BaseBtn5",               [BTN_BASE6] = "BaseBtn6",
        [BTN_DEAD] = "BtnDead",                 [BTN_A] = "BtnA",
        [BTN_B] = "BtnB",                       [BTN_C] = "BtnC",
        [BTN_X] = "BtnX",                       [BTN_Y] = "BtnY",
        [BTN_Z] = "BtnZ",                       [BTN_TL] = "BtnTL",
        [BTN_TR] = "BtnTR",                     [BTN_TL2] = "BtnTL2",
        [BTN_TR2] = "BtnTR2",                   [BTN_SELECT] = "BtnSelect",
        [BTN_START] = "BtnStart",               [BTN_MODE] = "BtnMode",
        [BTN_THUMBL] = "BtnThumbL",             [BTN_THUMBR] = "BtnThumbR",
        [BTN_TOOL_PEN] = "ToolPen",             [BTN_TOOL_RUBBER] = "ToolRubber",
        [BTN_TOOL_BRUSH] = "ToolBrush",         [BTN_TOOL_PENCIL] = "ToolPencil",
        [BTN_TOOL_AIRBRUSH] = "ToolAirbrush",   [BTN_TOOL_FINGER] = "ToolFinger",
        [BTN_TOOL_MOUSE] = "ToolMouse",         [BTN_TOOL_LENS] = "ToolLens",
        [BTN_TOUCH] = "Touch",                  [BTN_STYLUS] = "Stylus",
        [BTN_STYLUS2] = "Stylus2",              [BTN_TOOL_DOUBLETAP] = "Tool Doubletap",
        [BTN_TOOL_TRIPLETAP] = "Tool Tripletap", [BTN_GEAR_DOWN] = "WheelBtn",
        [BTN_GEAR_UP] = "Gear up",              [KEY_OK] = "Ok",
        [KEY_SELECT] = "Select",                [KEY_GOTO] = "Goto",
        [KEY_CLEAR] = "Clear",                  [KEY_POWER2] = "Power2",
        [KEY_OPTION] = "Option",                [KEY_INFO] = "Info",
        [KEY_TIME] = "Time",                    [KEY_VENDOR] = "Vendor",
        [KEY_ARCHIVE] = "Archive",              [KEY_PROGRAM] = "Program",
        [KEY_CHANNEL] = "Channel",              [KEY_FAVORITES] = "Favorites",
        [KEY_EPG] = "EPG",                      [KEY_PVR] = "PVR",
        [KEY_MHP] = "MHP",                      [KEY_LANGUAGE] = "Language",
        [KEY_TITLE] = "Title",                  [KEY_SUBTITLE] = "Subtitle",
        [KEY_ANGLE] = "Angle",                  [KEY_ZOOM] = "Zoom",
        [KEY_MODE] = "Mode",                    [KEY_KEYBOARD] = "Keyboard",
        [KEY_SCREEN] = "Screen",                [KEY_PC] = "PC",
        [KEY_TV] = "TV",                        [KEY_TV2] = "TV2",
        [KEY_VCR] = "VCR",                      [KEY_VCR2] = "VCR2",
        [KEY_SAT] = "Sat",                      [KEY_SAT2] = "Sat2",
        [KEY_CD] = "CD",                        [KEY_TAPE] = "Tape",
        [KEY_RADIO] = "Radio",                  [KEY_TUNER] = "Tuner",
        [KEY_PLAYER] = "Player",                [KEY_TEXT] = "Text",
        [KEY_DVD] = "DVD",                      [KEY_AUX] = "Aux",
        [KEY_MP3] = "MP3",                      [KEY_AUDIO] = "Audio",
        [KEY_VIDEO] = "Video",                  [KEY_DIRECTORY] = "Directory",
        [KEY_LIST] = "List",                    [KEY_MEMO] = "Memo",
        [KEY_CALENDAR] = "Calendar",            [KEY_RED] = "Red",
        [KEY_GREEN] = "Green",                  [KEY_YELLOW] = "Yellow",
        [KEY_BLUE] = "Blue",                    [KEY_CHANNELUP] = "ChannelUp",
        [KEY_CHANNELDOWN] = "ChannelDown",      [KEY_FIRST] = "First",
        [KEY_LAST] = "Last",                    [KEY_AB] = "AB",
        [KEY_NEXT] = "Next",                    [KEY_RESTART] = "Restart",
        [KEY_SLOW] = "Slow",                    [KEY_SHUFFLE] = "Shuffle",
        [KEY_BREAK] = "Break",                  [KEY_PREVIOUS] = "Previous",
        [KEY_DIGITS] = "Digits",                [KEY_TEEN] = "TEEN",
        [KEY_TWEN] = "TWEN",                    [KEY_DEL_EOL] = "Delete EOL",
        [KEY_DEL_EOS] = "Delete EOS",           [KEY_INS_LINE] = "Insert line",
        [KEY_DEL_LINE] = "Delete line",
};

char *absval[5] = { "Value", "Min  ", "Max  ", "Fuzz ", "Flat " };

char *relatives[REL_MAX + 1] = {
        [0 ... REL_MAX] = NULL,
        [REL_X] = "X",                  [REL_Y] = "Y",
        [REL_Z] = "Z",                  [REL_HWHEEL] = "HWheel",
        [REL_DIAL] = "Dial",            [REL_WHEEL] = "Wheel", 
        [REL_MISC] = "Misc",    
};

char *absolutes[ABS_MAX + 1] = {
        [0 ... ABS_MAX] = NULL,
        [ABS_X] = "X",                  [ABS_Y] = "Y",
        [ABS_Z] = "Z",                  [ABS_RX] = "Rx",
        [ABS_RY] = "Ry",                [ABS_RZ] = "Rz",
        [ABS_THROTTLE] = "Throttle",    [ABS_RUDDER] = "Rudder",
        [ABS_WHEEL] = "Wheel",          [ABS_GAS] = "Gas",
        [ABS_BRAKE] = "Brake",          [ABS_HAT0X] = "Hat0X",
        [ABS_HAT0Y] = "Hat0Y",          [ABS_HAT1X] = "Hat1X",
        [ABS_HAT1Y] = "Hat1Y",          [ABS_HAT2X] = "Hat2X",
        [ABS_HAT2Y] = "Hat2Y",          [ABS_HAT3X] = "Hat3X",
        [ABS_HAT3Y] = "Hat 3Y",         [ABS_PRESSURE] = "Pressure",
        [ABS_DISTANCE] = "Distance",    [ABS_TILT_X] = "XTilt",
        [ABS_TILT_Y] = "YTilt",         [ABS_TOOL_WIDTH] = "Tool Width",
        [ABS_VOLUME] = "Volume",        [ABS_MISC] = "Misc",
};

char *misc[MSC_MAX + 1] = {
        [ 0 ... MSC_MAX] = NULL,
        [MSC_SERIAL] = "Serial",        [MSC_PULSELED] = "Pulseled",
        [MSC_GESTURE] = "Gesture",      [MSC_RAW] = "RawData",
        [MSC_SCAN] = "ScanCode",
};

char *leds[LED_MAX + 1] = {
        [0 ... LED_MAX] = NULL,
        [LED_NUML] = "NumLock",         [LED_CAPSL] = "CapsLock", 
        [LED_SCROLLL] = "ScrollLock",   [LED_COMPOSE] = "Compose",
        [LED_KANA] = "Kana",            [LED_SLEEP] = "Sleep", 
        [LED_SUSPEND] = "Suspend",      [LED_MUTE] = "Mute",
        [LED_MISC] = "Misc",
};

char *repeats[REP_MAX + 1] = {
        [0 ... REP_MAX] = NULL,
        [REP_DELAY] = "Delay",          [REP_PERIOD] = "Period"
};

char *sounds[SND_MAX + 1] = {
        [0 ... SND_MAX] = NULL,
        [SND_CLICK] = "Click",          [SND_BELL] = "Bell",
        [SND_TONE] = "Tone"
};

char **names[EV_MAX + 1] = {
        [0 ... EV_MAX] = NULL,
        [EV_SYN] = events,                      [EV_KEY] = keys,
        [EV_REL] = relatives,                   [EV_ABS] = absolutes,
        [EV_MSC] = misc,                        [EV_LED] = leds,
        [EV_SND] = sounds,                      [EV_REP] = repeats,
};

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)    ((array[LONG(bit)] >> OFF(bit)) & 1)



/**************************************/


unsigned char flag_tab=0;

unsigned short rtuaddr;
char rtutype;
unsigned char seq;
unsigned char switch_timeint_loc;
//unsigned char switch_timeint;                      //switch_timeint in parm.log 
int timeint_loc[3];
unsigned int n_node;
unsigned int n_sen;
unsigned int duration;
unsigned char serverIP[16];
unsigned char serverPORT[6];
unsigned char hostIP[16];
unsigned char hostPORT[6];
int overtime_server;
int overtime_client;

char optionname[N_OPT][96]={"(interval)(([1,2,3][0,1][0,1,2][0-9]){1,3})",                // number --1
					      "(serverIP)([0-2][0-9][0-9].[0-9]{1,3}.[0-9]{1,3}.[0-9]{1,3})",                // number --2
					      "(serverPORT)([0-9]{4,5})",              //3
					 	  "(hostIP)([0-2][0-9][0-9].[0-9]{1,3}.[0-9]{1,3}.[0-9]{1,3})",                //4
					      "(hostPORT)([0-9]{4,5})",              //5 
					      "(address)([0-9]{8})",                   //6
						  "(nodenumber)([0-9]{1,2})",         //7
						  "(sensornuber)([0-9]{1,2})",        //8
						  "(duration)([0-9]{1,2})",                 //9
						  "(rtutype)([0-9]{1,2})",                 //10
						  "(overtime_server)([0-9]{1,2})",          //11 
						  "(overtime_client)([0-9]{1,2})"           //12
				         };

pthread_t id_shm,id_parm,id_key;
//int msgid;
//int semid;
int shmid;

pthread_mutex_t mutex_shmbuf,mutex_parm_loc,mutex_tab;
pthread_cond_t cond_tab;

typedef struct afnbroadtser		
{
	//unsigned char nodetype;
	unsigned char nodevolt[2];
	unsigned char nodecir[2];
	unsigned char nodetime[7];
	unsigned char workmode;
	unsigned char runtime[2];
	unsigned char sleeptime[2];
	unsigned char timeset[3];
	//unsigned char timeset_two;
	//unsigned char timeset_three;
	unsigned char nodeversion[2]; 
} afnbroadtser_s;

typedef struct disply_node
{
	unsigned char node_addr;
	unsigned char ip[16];
	unsigned int port;
	
	//unsigned char switch_timeint;
	//int timeint[3];

	time_t t_login;
	time_t t_logoff;
	time_t t_login_bak;
	time_t t_logoff_bak;

	unsigned long rx;
	unsigned long tx;

	afnbroadtser_s afnbroadtser;
} disply_node_s;

typedef struct shm
{
	//RTU
	unsigned short volt;
	unsigned short curr;
	int timeint[3];
	unsigned char switch_timeint;
	unsigned char overtime_server;
	unsigned char time_repeat;

	//NODE
	unsigned char flag_online;
	disply_node_s disply_node[4];
} shm_s;

shm_s disply_shm;
/*******************************************************************************
 *	pthread key
 * ******************************************************************************/
void *pthread_key(void *arg)
{
	int fd, rd, i, j, k;
	struct input_event ev[64];
	int version;
	unsigned short id[4];
	unsigned long bit[EV_MAX][NBITS(KEY_MAX)];
	char name[256] = "Unknown";
	int abs[5];

	//	if (argc < 2) {
	//		printf("Usage: evtest /dev/input/eventX\n");
	//		printf("Where X = input device number\n");
	//		return 1;
	//	}
	//
	//	if ((fd = open(argv[argc - 1], O_RDONLY)) < 0) {
	//		perror("evtest");
	//		return 1;
	//	}
start:
	if((fd=open("/dev/event0",O_RDWR))<0)
		goto start;

	if (ioctl(fd, EVIOCGVERSION, &version)) 
	{
		goto start;
	}

	ioctl(fd, EVIOCGID, id);

	ioctl(fd, EVIOCGNAME(sizeof(name)), name);

//	memset(bit, 0, sizeof(bit));
//	ioctl(fd, EVIOCGBIT(0, EV_MAX), bit[0]);
//
//	for (i = 0; i < EV_MAX; i++)
//		if (test_bit(i, bit[0])) {
//			if (!i) continue;
//			ioctl(fd, EVIOCGBIT(i, KEY_MAX), bit[i]);
//			for (j = 0; j < KEY_MAX; j++) 
//				if (test_bit(j, bit[i])) {
//					if (i == EV_ABS) {
//						ioctl(fd, EVIOCGABS(j), abs);
//						for (k = 0; k < 5; k++)
//							if ((k < 3) || abs[k]);
//					}
//				}
//		}

	system("echo 'key ok' > /dev/tty0");

	while (1) 
	{
		rd = read(fd, ev, sizeof(struct input_event) * 64);

		//		if (rd < (int) sizeof(struct input_event)) {
		//			printf("yyy\n");
		//			perror("\nevtest: error reading");
		//			return 1;
		//		}
		//
		//		for (i = 0; i < rd / sizeof(struct input_event); i++)
		//
		//			if (ev[i].type == EV_SYN) {
		//				printf("Event: time %ld.%06ld, -------------- %s ------------\n",
		//						ev[i].time.tv_sec, ev[i].time.tv_usec, ev[i].code ? "Config Sync" : "Report Sync" );
		//			} else if (ev[i].type == EV_MSC && (ev[i].code == MSC_RAW || ev[i].code == MSC_SCAN)) {
		//				printf("Event: time %ld.%06ld, type %d (%s), code %d (%s), value %02x\n",
		//						ev[i].time.tv_sec, ev[i].time.tv_usec, ev[i].type,
		//						events[ev[i].type] ? events[ev[i].type] : "?",
		//						ev[i].code,
		//						names[ev[i].type] ? (names[ev[i].type][ev[i].code] ? names[ev[i].type][ev[i].code] : "?") : "?",
		//						ev[i].value);
		//			} else {
		//				printf("Event: time %ld.%06ld, type %d (%s), code %d (%s), value %d\n",
		//						ev[i].time.tv_sec, ev[i].time.tv_usec, ev[i].type,
		//						events[ev[i].type] ? events[ev[i].type] : "?",
		//						ev[i].code,
		//						names[ev[i].type] ? (names[ev[i].type][ev[i].code] ? names[ev[i].type][ev[i].code] : "?") : "?",
		//						ev[i].value);
		//			}       
		//	
		if(rd>0)
		{
			rd = read(fd, ev, sizeof(struct input_event) * 64);
			if(rd>0)
			{
				pthread_mutex_lock(&mutex_tab);
				(flag_tab=N_NODE+1)?(flag_tab=1):(flag_tab++);
				pthread_mutex_unlock(&mutex_tab);
				pthread_cond_signal(&cond_tab);
			}
		}

	}
	return;
}


/*******************************************************************************
 *	pthread of writing shm to send frame to server_down
 *******************************************************************************/

void *pthread_shm(void *arg)
{
	shm_s *pshmbuf;
	key_t key_info;
	unsigned char str[32];
#ifdef DEBUG
	printf("*********************pthread_shm     in************************\n");
#endif

	while ((key_info = ftok (PATH_SHM, 'i')) < 0);
	//{
	//	exit (-1);
	//}
#ifdef DEBUG
	printf ("client_uphost pthread_shm : key = %d\n", key_info);
#endif

//	if((semid = semget (key_info, 1, IPC_CREAT | IPC_EXCL |0666)) < 0)
//	{
//		if (errno == EEXIST)
//		{
//			semid = semget (key_info, 1, 0666);
//		}
//		else
//		{
//			perror ("client_uphost pthread_shm : semget\n");
//		//	exit (-1);
//		}
//	}
//	else
//	{
//		init_sem (semid, 0, 2);
//	}
#ifdef DEBUG
	printf("client_uphost pthread_shm : shm bellow\n");
#endif
	if ((shmid = shmget (key_info, sizeof(shm_s), IPC_CREAT | IPC_EXCL | 0666)) < 0)
	{
		if (errno == EEXIST)
		{
			shmid = shmget (key_info, N, 0666);
			pshmbuf = (shm_s*)shmat (shmid, NULL, 0);
			bzero (pshmbuf, sizeof(shm_s));
		}
		else
		{
			perror ("client_uphost pthread_shm : shmget\n");
		//	exit (-1);
		}
	}
	else
	{
		if ((pshmbuf = (shm_s*)shmat(shmid, NULL, 0)) == (void *)-1)
		{
			perror ("client_uphost pthread_shm : shmat\n");
		//	exit (-1);
		}
	}
	unsigned char flag_online_bak;
	while(1)
	{
//#ifdef DEBUG
//		printf("client_uphost pthread_shm : wait cond_shm\n");
//#endif
	
		pthread_mutex_lock(&mutex_shmbuf);
		//sem_p(semid,0);                                       //locate here ok?
	
		//pthread_cond_wait(&cond_shm,&mutex_shmbuf);
//#ifdef DEBUG
//		printf("client_uphost pthread_shm : wait cond_shm awake\n");
//#endif
		
		memcpy(&disply_shm,pshmbuf,sizeof(shm_s));
		if(disply_shm.flag_online!=flag_online_bak)
		{
			pthread_cond_signal(&cond_tab);
			flag_online_bak=disply_shm.flag_online;
		}

#ifdef DEBUG
		printf("!!!!!!!!flag_online %d\n",pshmbuf->flag_online);
		printf("!!!!!!!!workmode %02x\n",pshmbuf->disply_node[0].afnbroadtser.workmode);
#endif

		//sem_v(semid,0);
		pthread_mutex_unlock(&mutex_shmbuf);
		//usleep(10000);
	}
	return;
}

/*****************************************
 *	thread read parmgrams from parm.log
 ****************************************/
void *pthread_parm(void *arg)
{
	pthread_detach(pthread_self());
	unsigned char flag=0;

	if(arg!=NULL)
	{
		free(arg);
		flag=1;
	}

	int fd,cnt;
	unsigned char buf[256];

	unsigned char bak[8]={};
//	while((fd=open(PATH_PARM,O_RDWR|O_CREAT,0x666))<0);
	if((fd=open(PATH_PARM,O_RDWR|O_CREAT,0x666))<0)
		if((fd=open(PATH_PARM,O_RDWR|O_CREAT,0x666))<0)
			return;
	cnt=read(fd,buf,sizeof(buf));

	pthread_mutex_lock(&mutex_parm_loc);

	bzero(bak,sizeof(bak));
	memcpy(bak,buf+45,2);
	switch_timeint_loc=strtol(bak,NULL,16);
	if(flag==1)
		return ;
	bzero(bak,sizeof(bak));

	memcpy(bak,buf+8,2);
	rtuaddr=strtol(bak,NULL,10);
	bzero(bak,sizeof(bak));

	memcpy(bak,buf+19,2);
	rtutype=strtol(bak,NULL,16);
	bzero(bak,sizeof(bak));

//	memcpy(bak,buf+27,2);
//	seql=strtol(bak,NULL,16);
//	bzero(bak,sizeof(bak));

	memcpy(bak,buf+59,2);
	timeint_loc[0]=strtol(bak,NULL,10);
	bzero(bak,sizeof(bak));

	memcpy(bak,buf+73,2);
	timeint_loc[1]=strtol(bak,NULL,10);
	bzero(bak,sizeof(bak));

	memcpy(bak,buf+87,2);
	timeint_loc[2]=strtol(bak,NULL,10);
	bzero(bak,sizeof(bak));

	memcpy(bak,buf+97,2);
	n_node=strtol(bak,NULL,10);
	bzero(bak,sizeof(bak));

	memcpy(bak,buf+106,2);
	n_sen=strtol(bak,NULL,10);
	bzero(bak,sizeof(bak));

	memcpy(bak,buf+118,2);
	duration=strtol(bak,NULL,10);
	bzero(bak,sizeof(bak));

	memcpy(serverIP,buf+130,15);
	memcpy(serverPORT,buf+157,5);
	memcpy(hostIP,buf+170,15);
	memcpy(hostPORT,buf+195,5);

	memcpy(bak,buf+217,2);
	overtime_server=strtol(bak,NULL,10);
	bzero(bak,sizeof(bak));
	memcpy(bak,buf+236,2);
	overtime_client=strtol(bak,NULL,10);
	bzero(bak,sizeof(bak));

	pthread_mutex_unlock(&mutex_parm_loc);

	close(fd);
	return;
}

/*****************************
 * record parmgrams into parm.log
 * */
int rec_parm(int cmd)
{
	int fd;
	unsigned char bak[32]={};
//	while((fd=open(PATH_PARM,O_RDWR|O_CREAT,0x666))<0);
	if((fd=open(PATH_PARM,O_RDWR|O_CREAT,0x666))<0)
		if((fd=open(PATH_PARM,O_RDWR|O_CREAT,0x666))<0)
			return 0;

	switch(cmd)
	{
	case REC_RTUADDR:
		sprintf(bak,"%02d",rtuaddr);
		lseek(fd,8,SEEK_SET);
		break;
	case REC_RTUTYPE:
		sprintf(bak,"%02x",rtutype);
		lseek(fd,19,SEEK_SET);
		break;
//	case REC_SEQL:
//		sprintf(bak,"%02x",seql);
//		lseek(fd,27,SEEK_SET);
//		break;
	case REC_SWITCH:
		sprintf(bak,"%02x",switch_timeint_loc);
		lseek(fd,45,SEEK_SET);
		break;
	case REC_TIMEINT1:
		sprintf(bak,"%02d",timeint_loc[0]);
		lseek(fd,59,SEEK_SET);
		break;
	case REC_TIMEINT2:
		sprintf(bak,"%02d",timeint_loc[1]);
		lseek(fd,73,SEEK_SET);
		break;
	case REC_TIMEINT3:
		sprintf(bak,"%02d",timeint_loc[2]);
		lseek(fd,87,SEEK_SET);
		break;
	case REC_NODE:
		sprintf(bak,"%03d",n_node);
		lseek(fd,97,SEEK_SET);
		break;
	case REC_SEN:
		sprintf(bak,"%03d",n_sen);
		lseek(fd,106,SEEK_SET);
		break;
	case REC_DUR:
		sprintf(bak,"%04d",duration);
		lseek(fd,118,SEEK_SET);
		break;
	case REC_SERIP:
		sprintf(bak,"%-15s\n",serverIP);
		lseek(fd,130,SEEK_SET);
		break;
	case REC_SERPORT:
		sprintf(bak,"%-6s\n",serverPORT);
		lseek(fd,157,SEEK_SET);
		break;
	case REC_CLIIP:
		sprintf(bak,"%-15s\n",hostIP);
		lseek(fd,170,SEEK_SET);
		break;
	case REC_CLIPORT:
		sprintf(bak,"%-6s\n",hostPORT);
		lseek(fd,195,SEEK_SET);
		break;
	case REC_OTS:
		sprintf(bak,"%02d",overtime_server);
		lseek(fd,217,SEEK_SET);
		break;
	case REC_OTC:
		sprintf(bak,"%02d",overtime_client);
		lseek(fd,236,SEEK_SET);
		break;
	}
	if((write(fd,bak,strlen(bak)))==strlen(bak))
	{
		return 1;                                      //success
	}
	else
		return 0;
}

/**************************
 *	record the parmgrams according to the optionname
 *************************/

void analysis(int optn_no,char *cmd,regmatch_t *pmatch)
{
	int error=0;                         // 1--FAILD;
//	int fd;
//	if((fd=open(PATH,O_RDWR|O_CREAT,0x666))<0)
//		printf("open %s wrong",PATH);

	//printf("optn_no : %d\n",optn_no);

	switch(optn_no)
	{
	case 1:
		{
			int i=0;
			char parm[5];
			for(i=0;i<pmatch[2].rm_eo-pmatch[2].rm_so;i+=4)
			{
				bzero(parm,sizeof(parm));
				strncpy(parm,cmd+pmatch[2].rm_so+i,4);
				switch(parm[0])
				{
				case '1':
					timeint_loc[0]=strtol(parm+2,NULL,10);
					if(rec_parm(REC_TIMEINT1)==0)             //faild
						error=1;
					break;
				case '2':	
					timeint_loc[1]=strtol(parm+2,NULL,10);
					//printf("1111   %d\n",timeint_loc[1]);
					if(!rec_parm(REC_TIMEINT2))
						error=1;
					break;
				case '3':
					timeint_loc[2]=strtol(parm+2,NULL,10);
					if(!rec_parm(REC_TIMEINT3))
						error=1;
					break;
				}
				switch_timeint_loc|=((parm[1]-'0')<<(parm[0]-'0'-1));
				if(!rec_parm(REC_SWITCH))
					error=1;
			}	
			if(error)
				printf("Set RTU interval FAILD\n");
			else
				printf("Set RTU interval OK\n");
			break;
		}
	case 2:
		{
			bzero(serverIP,sizeof(serverIP));
			memcpy(serverIP,cmd+pmatch[2].rm_so,pmatch[2].rm_eo-pmatch[2].rm_so);
			if(!rec_parm(REC_SERIP))
			{
				error=1;
			}
			if(error)
				printf("Set RTU serverIP FAILD\n");
			else
				printf("Set RTU serverIP OK\n");
			break;
		}
	case 3:
		{
			bzero(serverPORT,sizeof(serverPORT));
			memcpy(serverPORT,cmd+pmatch[2].rm_so,pmatch[2].rm_eo-pmatch[2].rm_so);
			if(!rec_parm(REC_SERPORT))
			{
				error=1;
			}
			if(error)
				printf("Set RTU serverPORT FAILD\n");
			else
				printf("Set RTU serverPORT OK\n");
			break;
		}
	case 4:
		{
			bzero(hostIP,sizeof(hostIP));
			memcpy(hostIP,cmd+pmatch[2].rm_so,pmatch[2].rm_eo-pmatch[2].rm_so);
			if(!rec_parm(REC_CLIIP))
			{
				error=1;
			}
			if(error)
				printf("Set RTU hostIP FAILD\n");
			else
				printf("Set RTU hostIP OK\n");
			break;
		}
	case 5:
		{
			bzero(hostPORT,sizeof(hostPORT));
			memcpy(hostPORT,cmd+pmatch[2].rm_so,pmatch[2].rm_eo-pmatch[2].rm_so);
			if(!rec_parm(REC_CLIPORT))
			{
				error=1;
			}
			if(error)
				printf("Set RTU hostPORT FAILD\n");
			else
				printf("Set RTU hostPORT OK\n");
			break;
		}
	case 6:
		{
			unsigned char buf[3];
			strncmp(buf,cmd+pmatch[2].rm_so+3,2);
			rtuaddr=strtol(buf,NULL,10);
			if(!rec_parm(REC_RTUADDR))
				error=1;
			if(error)
				printf("Set RTU address FAILD\n");
			else
				printf("Set RTU address OK\n");
			break;
		}
	case 7:
		{
			unsigned char parm[4]={};
			memcpy(parm,cmd+pmatch[2].rm_so,pmatch[2].rm_eo-pmatch[2].rm_so);
			n_node=strtol(parm,NULL,10);
			if(!rec_parm(REC_NODE))
				error=1;
			if(error)
				printf("Set NODE number FAILD\n");
			else
				printf("Set NODE number OK\n");
			break;
		}
	case 8:
		{
			unsigned char parm[4]={};
			memcpy(parm,cmd+pmatch[2].rm_so,pmatch[2].rm_eo-pmatch[2].rm_so);
			n_sen=strtol(parm,NULL,10);
			if(!rec_parm(REC_SEN))
				error=1;
			if(error)
				printf("Set NODE number FAILD\n");
			else
				printf("Set NODE number OK\n");
			break;
		}
	case 9:
		{
			unsigned char parm[4]={};
			memcpy(parm,cmd+pmatch[2].rm_so,pmatch[2].rm_eo-pmatch[2].rm_so);
			duration=strtol(parm,NULL,10);
			if(!rec_parm(REC_DUR))
				error=1;
			if(error)
				printf("Set RTU duration FAILD\n");
			else
				printf("Set RTU duration OK\n");
			break;
		}
	case 10:
		{
			unsigned char parm[4]={};
			memcpy(parm,cmd+pmatch[2].rm_so,pmatch[2].rm_eo-pmatch[2].rm_so);
			rtutype=strtol(parm,NULL,16);
			if(!rec_parm(REC_RTUTYPE))
				error=1;
			if(error)
				printf("Set RTU device type FAILD\n");
			else
				printf("Set RTU device type number OK\n");
			break;
		}
	case 11:
		{
			unsigned char parm[4]={};
			memcpy(parm,cmd+pmatch[2].rm_so,pmatch[2].rm_eo-pmatch[2].rm_so);
			overtime_server=strtol(parm,NULL,10);
			if(!rec_parm(REC_OTS))
				error=1;
			if(error)
				printf("Set RTU overtime_server FAILD\n");
			else
				printf("Set RTU overtime_server OK\n");
			break;
		}
	case 12:
		{
			unsigned char parm[4]={};
			memcpy(parm,cmd+pmatch[2].rm_so,pmatch[2].rm_eo-pmatch[2].rm_so);
			overtime_client=strtol(parm,NULL,10);
			if(!rec_parm(REC_OTC))
				error=1;
			if(error)
				printf("Set RTU overtime_client FAILD\n");
			else
				printf("Set RTU overtime_client OK\n");
			break;
		}
	}
	return ;
}

/******************************************
 *	earn YYYY-MM-DD HH:MM:SS
 * * */
void earntime(time_t t,unsigned char *string)
{
	if(t==0)
	{
		sprintf(string,"0000-00-00 00:00:00");
		return;
	}
	struct tm *tim;
	//time(&t);
	tim=localtime(&t);
	sprintf(string,"%d-%02d-%02d %02d:%02d:%02d",tim->tm_year+1900,tim->tm_mon+1,tim->tm_mday,tim->tm_hour,tim->tm_min,tim->tm_sec);
	return;
}


/********************************************
 * print information of -l
 *******************************************/
void print(int which)
{
#ifdef DEBUG
	printf("print in\n");
#endif
	unsigned char *flag_switch[3];
	unsigned char flag_onlinenode[4];
	unsigned char rtutime[20];

//	unsigned char switch_timeint;
//	int timeint[3];
//	unsigned char str_workmode[2][]={"Self-reported","Called-measurement"};
//
//	unsigned char year[5];
//	unsigned char mon[3];
//	unsigned char day[3];
//	unsigned char hh[3];
//	unsigned char mm[3];
//	unsigned char ss[3];
//	unsigned short runtime;
//	unsigned short sleeptime;

	if(which==PRINT_RTU)
	{
		time_t t_loc;
		time(&t_loc);
		earntime(t_loc,rtutime);
		float valt;	
		float curr;

		pthread_mutex_lock(&mutex_shmbuf);
		valt=((float)disply_shm.volt)/100;
		curr=((float)disply_shm.curr)/100;
		pthread_mutex_unlock(&mutex_shmbuf);

		pthread_mutex_lock(&mutex_parm_loc);
		int i;
		for(i=0;i<3;i++)
		{
			if((switch_timeint_loc&(1<<i))!=0)
				flag_switch[i]="ON";
			else 
				flag_switch[i]="OFF";
		}
		for(i=0;i<N_NODE;i++)
		{
			if((disply_shm.flag_online&(1<<i))!=0)
			{
				flag_onlinenode[i]=0x76;
			}
			else
			{
				flag_onlinenode[i]=0x20;
			}
		}
		printf("===========================================================\n");
		printf("=  |||RTU|||  NODE 1  |  NODE 2  |  NODE 3  |  NODE 4  |  =\n");
		printf("=                                                         =\n");
		printf("=                                                         =\n");
		printf("=   RTU address : 00%02d0000                                =\n",rtuaddr);
		printf("=                                                         =\n");
		printf("=   RTU type : %02x                                         =\n",rtutype);
		printf("=                                                         =\n");
		printf("=   time interval   |   switch       hour                 =\n");
		printf("=         1         |   %-3s          %-2d                   =\n",flag_switch[0],timeint_loc[0]);
		printf("=         2         |   %-3s          %-2d                   =\n",flag_switch[1],timeint_loc[1]);
		printf("=         3         |   %-3s          %-2d                   =\n",flag_switch[2],timeint_loc[2]);
		printf("=                                                         =\n");
		printf("=   server IP : %-16s                          =\n",serverIP);
		printf("=   server PORT : %-5s                                   =\n",serverPORT);
		printf("=                                                         =\n");
		printf("=   host   IP : %-16s                          =\n",hostIP);
		printf("=   host   port : %-5s                                   =\n",hostPORT);
		printf("=                                                         =\n");
		printf("=   voltage : %-4.2f                                        =\n",valt);
		printf("=   circuit : %-4.2f                                        =\n",curr);
		printf("=                                                         =\n");
		printf("=               |  NODE 1   NODE 2   NODE 3   NODE 4      =\n");
		printf("=   online node |  %1c        %1c        %1c        %1c           =\n",flag_onlinenode[0],flag_onlinenode[1],flag_onlinenode[2],flag_onlinenode[3]);
		printf("=                                                         =\n");
		printf("=   local time : %-20s                     =\n",rtutime);
		printf("=                                                         =\n");
		printf("=                                                         =\n");
		printf("=                                                         =\n");
		printf("=                                                         =\n");
		printf("=                                                         =\n");
		printf("=                                                         =\n");
		printf("===========================================================\n");

		pthread_mutex_unlock(&mutex_parm_loc);
	
	}
	else
	{
		int i;
		i=which-23;
		
		shm_s buf;
		pthread_mutex_lock(&mutex_shmbuf);
		memcpy(&buf,&disply_shm,sizeof(buf));
		pthread_mutex_unlock(&mutex_shmbuf);

		unsigned char time_login[20];
		unsigned char time_logoff[20];
		unsigned char time_login_bak[20];
		unsigned char time_logoff_bak[20];


		earntime(buf.disply_node[i].t_login,time_login);
		earntime(buf.disply_node[i].t_logoff,time_logoff);
		earntime(buf.disply_node[i].t_login_bak,time_login_bak);
		earntime(buf.disply_node[i].t_logoff_bak,time_logoff_bak);

		unsigned char switch_timeint;
		int timeint[3];


		int l;
		for(l=0;l<3;l++)
		{
			unsigned char a[3];
			sprintf(a,"%02x",buf.disply_node[i].afnbroadtser.timeset[l]&0x7f);
			switch_timeint|=((buf.disply_node[i].afnbroadtser.timeset[l]>>7)<<l);
			timeint[l]=strtol(a,NULL,10);
		}

		for(l=0;l<3;l++)
		{
			if((switch_timeint&(1<<l))!=0)
				flag_switch[l]="ON";
			else 
				flag_switch[l]="OFF";
		}

		unsigned char str_workmode[2][24]={"Self-reported","Called-measurement"};

		unsigned char year[5];
		unsigned char mon[3];
		unsigned char day[3];
		unsigned char hh[3];
		unsigned char mm[3];
		unsigned char ss[3];

		sprintf(year,"%02x",buf.disply_node[i].afnbroadtser.nodetime[0]);
		sprintf(year+2,"%02x",buf.disply_node[i].afnbroadtser.nodetime[1]);
		sprintf(mon,"%02x",buf.disply_node[i].afnbroadtser.nodetime[2]);
		sprintf(day,"%02x",buf.disply_node[i].afnbroadtser.nodetime[3]);
		sprintf(hh,"%02x",buf.disply_node[i].afnbroadtser.nodetime[4]);
		sprintf(mm,"%02x",buf.disply_node[i].afnbroadtser.nodetime[5]);
		sprintf(ss,"%02x",buf.disply_node[i].afnbroadtser.nodetime[6]);

		unsigned short runtime;
		memcpy(&runtime,buf.disply_node[i].afnbroadtser.runtime,2);
		unsigned short sleeptime;
		memcpy(&sleeptime,buf.disply_node[i].afnbroadtser.sleeptime,2);

		//printf("=======================================================================\n");
		//printf("=   |   RTU   ||||NODE 1||||   NODE 2   |   NODE 3   |   NODE 4   |   =\n");
		//printf("=                                                                     =\n"); 
	
		printf("===========================================================\n");
		switch(which)
		{
		case PRINT_NODE_ONE:
			printf("=  |  RTU  |||NODE 1|||  NODE 2  |  NODE 3  |  NODE 4  |  =\n");			
			printf("=                                                         =\n"); 
			printf("=           %-3s Line                                      =\n",(((buf.flag_online&(1<<i))==(1<<i))?"ON":"OUT"));

			break;
		case PRINT_NODE_TWO:
			printf("=  |  RTU  |  NODE 1  |||NODE 2|||  NODE 3  |  NODE 4  |  =\n");			
			printf("=                                                         =\n"); 
			printf("=                      %-3s Line                           =\n",(((buf.flag_online&(1<<i))==(1<<i))?"ON":"OUT"));

			break;
		case PRINT_NODE_THR:
			printf("=  |  RTU  |  NODE 1  |  NODE 2  |||NODE 3|||  NODE 4  |  =\n");			
			printf("=                                                         =\n"); 
			printf("=                                 %-3s Line                =\n",(((buf.flag_online&(1<<i))==(1<<i))?"ON":"OUT"));

			break;
		case PRINT_NODE_FOU:
			printf("=   |  RTU  |  NODE 1  |  NODE 2  |  NODE 3  |||NODE 4||| =\n");			
			printf("=                                                         =\n"); 
			printf("=                                             %-3s Line    =\n",(((buf.flag_online&(1<<i))==(1<<i))?"ON":"OUT"));

			break;
		}
		
		printf("=                                                         =\n"); 
		printf("=   Address : 00%02d%02x00                                    =\n",rtuaddr,buf.disply_node[i].node_addr);
		printf("=                                                         =\n"); 
		printf("=   Vertion : %02x%02x                                        =\n",buf.disply_node[i].afnbroadtser.nodeversion[0],buf.disply_node[i].afnbroadtser.nodeversion[1]);
		printf("=   Work Mode : %-20s                      =\n",str_workmode[buf.disply_node[i].afnbroadtser.workmode-1]);
		printf("=   Login Time (NODE) : %s-%s-%s %s:%s:%s               =\n",year,mon,day,hh,mm,ss);
		printf("=   Running Time (minut): %2d                              =\n",runtime);
		printf("=   Sleeping Time (minut): %2d                             =\n",sleeptime);
		printf("=                                                         =\n");
		printf("=   IP : %-16s                                 =\n",buf.disply_node[i].ip);
		printf("=   PORT : %-6d                                         =\n",buf.disply_node[i].port);
		printf("=                                                         =\n");
		printf("=   Login Voltage : %-4.2f                                  =\n",((float)strtol(buf.disply_node[i].afnbroadtser.nodevolt,NULL,10))/100);
		printf("=   Login Cirruit : %-4.2f                                  =\n",((float)strtol(buf.disply_node[i].afnbroadtser.nodecir,NULL,10))/100);
		printf("=                                                         =\n");
		printf("=   time interval   |   switch       hour                 =\n");
		printf("=         1         |     %-3s        %02d                   =\n",flag_switch[0],timeint[0]);
		printf("=         2         |     %-3s        %02d                   =\n",flag_switch[1],timeint[1]);
		printf("=         3         |     %-3s        %02d                   =\n",flag_switch[2],timeint[2]);
		printf("=                                                         =\n");
		printf("=   Login  time : %-20s                    =\n",time_login);
		printf("=   Logoff time : %-20s                    =\n",time_logoff);
		printf("=   Last Login  time : %-20s               =\n",time_login_bak);
		printf("=   Last Logoff time : %-20s               =\n",time_logoff_bak);
		printf("=                                                         =\n");
		printf("=   RX byte : %-30ld              =\n",buf.disply_node[i].rx);
		printf("=   TX byte : %-30ld              =\n",buf.disply_node[i].tx);
		printf("=                                                         =\n");
		printf("===========================================================\n");






	}
	return;
}




/******************************
 * release resouce
 * */
void releaseresouce(void)
{
	pthread_mutex_destroy(&mutex_parm_loc);
	pthread_mutex_destroy(&mutex_shmbuf);
	pthread_mutex_destroy(&mutex_tab);

	pthread_cond_destroy(&cond_tab);

	shmctl(shmid,IPC_RMID,NULL);
//	msgctl(msgid,IPC_RMID,NULL);
//	semctl(semid,1,IPC_RMID,NULL);

	pthread_cancel(id_shm);
	pthread_cancel(id_key);
	return;
}

/*********************************************
 *	main function
 *
 *********************************************/
int count=0;
void handler(int signo)
{
	if(count==8)
	{
		count=0;
		pthread_mutex_lock(&mutex_tab);
		flag_tab=0;
		pthread_mutex_unlock(&mutex_tab);
	}
	else
		count++;
	pthread_cond_signal(&cond_tab);
	alarm(60);
}

void handler_parm(int signo)
{
	pthread_create(&id_parm,0,&pthread_parm,NULL);
	return;
}


int main(int argc, const char *argv[])
{
	signal(SIGALRM,handler);
	signal(SIGINT,handler_parm);
	freopen("/dev/tty0","a+",stdout);

	pthread_mutex_init(&mutex_shmbuf,NULL);
	pthread_mutex_init(&mutex_parm_loc,NULL);
	pthread_mutex_init(&mutex_tab,NULL);

	pthread_cond_init(&cond_tab,NULL);

	pthread_create(&id_key,0,&pthread_key,NULL);
	pthread_create(&id_shm,0,&pthread_shm,NULL);
	pthread_create(&id_parm,0,&pthread_parm,NULL);
	printf("Loading Data ...\n");
	sleep(8);

//	int l;
	pthread_mutex_lock(&mutex_tab);
	while(1)
	{
		switch(flag_tab)
		{
		case 0:
			system("clear > /dev/tty0");
			printf("Press K2 To Chech Information\n");
			alarm(60);
			break;
		case 1:
			print(PRINT_RTU);
			break;
		case 2:
			print(PRINT_NODE_ONE);
			break;
		case 3:
			print(PRINT_NODE_TWO);
			break;
		case 4:
			print(PRINT_NODE_THR);
			break;
		case 5:
			print(PRINT_NODE_FOU);
			break;
		}
		pthread_mutex_unlock(&mutex_tab);
		pthread_mutex_lock(&mutex_tab);
		pthread_cond_wait(&cond_tab,&mutex_tab);
	}

	pthread_mutex_destroy(&mutex_parm_loc);
	pthread_mutex_destroy(&mutex_shmbuf);

	atexit(&releaseresouce);

	return 0;
}
