#ifndef _LEDLIB_H
# define _LEDLIB_H

#ifndef LED_API
# define LED_API
#endif

struct led_tab {
    char name[8];
    unsigned char value;
};

extern struct led_tab led_tab[];
LED_API int led_ctrl (char *name, int onoff);

int fd_ledsix,fd_lednine;

//fd_ledsix = open ("/sys/class/leds/d6/brightness", O_WRONLY);
//fd_lednine = open ("/sys/class/leds/d9/brightness", O_WRONLY);
#endif /* _LEDLIB_H */
