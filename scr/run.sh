#!/bin/sh 
rm *.o
if [ $1 = ca ]; then
rm UPclient
	make -f makefile_arm
fi
if [ $1 = ch ]; then
rm UPclient 
	make -f makefile_host
fi
if [ $1 = sa ]; then
rm DWserver
	make -f makefileser_arm
fi
if [ $1 = sh ]; then
rm DWserver
	make -f makefileser_host
fi
if [ $1 = dh ]; then
rm debug
	gcc debug.c -o debug -lpthread -g #-DDEBUG
fi
if [ $1 = da ]; then
rm debug
	arm-none-linux-gnueabi-gcc debug.c -o debug -lpthread
fi
if [ $1 = dish ]; then
rm display
	gcc display.c -o display -lpthread -g #-Ddisplay
fi
if [ $1 = disa ]; then
rm display
	arm-none-linux-gnueabi-gcc display.c -o display -lpthread
fi
if [ $1 = all ]; then
rm DWserver
	make -f makefileser_arm
rm debug
	arm-none-linux-gnueabi-gcc debug.c -o debug -lpthread
rm display
	arm-none-linux-gnueabi-gcc display.c -o display -lpthread
make -f makefile_arm clean
	make -f makefile_arm

mv UPclient DWserver debug display ../rturun
make -f makefile_arm clean
fi

