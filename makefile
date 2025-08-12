# TODO: make a proper makefile

kk: kk.c thidparty/flag.h 
	cc -o kk kk.c flag.h -lncurses -ggdb
