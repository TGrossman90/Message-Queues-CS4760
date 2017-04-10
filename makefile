all: oss user readme

user: user.o
	gcc -g -o user user.o

oss: oss.o
	gcc -g -o oss oss.o
	
oss.o: oss.c
	gcc -g -c oss.c

user.o: user.c 
	gcc -g -c user.c
	
clean: remove

remove:
	rm *.o oss user *.out

clear: 
	clear
	
success: 
	$(info SUCCESS)
	
readme:
	cat README
