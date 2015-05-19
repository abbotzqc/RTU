#ifndef _BEEPLIB_H
# define _BEEPLIB_H

#ifndef BEEP_API
# define BEEP_API
#endif

struct beep_tab {
    char name[8];
    unsigned char value;
};

extern struct beep_tab beep_tab[];
BEEP_API int beep_ctrl (char *name, int onoff);

int fd_beep;
#endif /* _LEDLIB_H */
