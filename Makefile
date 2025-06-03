# GAME Language Compiler , target machine=8080 with CP/M2.2
#  by Shozo TAKEOKA take@takeoka.org
# game80.GGG is runtime-routines, include by the compiler
#

OBJS = gc80
ALL = $(OBJS)

GAMESRC = qq.g test.g xx.g xxx.g xz.g
ARC = gc80.c game80.GGG $(GAMESRC)


gc80: gc80.c
	cc -g -o gc80 gc80.c

clean:
	rm $(OBJS)

tar:
	tar cvzf gc80.tgz $(ARC) Makefile
