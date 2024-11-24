###################################################
#
# file: Makefile
#
# @Author:   Ioannis Chatziantwoiou
# @Version:  18-10-2024
# @email:    csd5193@csd.uoc.gr
#
# Makefile
#
####################################################

all : prompt

prompt: prompt.o
	gcc prompt.o -o prompt

prompt.o: prompt.c
	gcc -c prompt.c

clean:
	rm -f *.o
	rm -f prompt

