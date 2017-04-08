all: oss slave

slave: slave.o
	gcc -g -o slave slave.o

oss: oss.o
	gcc -g -o oss oss.o
	
oss.o: oss.c
	gcc -g -c oss.c

slave.o: slave.c 
	gcc -g -c slave.c
	
clean: remove

remove:
	rm *.o oss slave *.out

clear: 
	clear
	
success: 
	$(info SUCCESS)
	
readme:
	cat README
