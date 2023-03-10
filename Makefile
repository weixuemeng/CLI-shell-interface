all: shell379

shell379: shell379.o pcb_handler.o cmd.o
	gcc -o shell379 shell379.o pcb_handler.o cmd.o

shell379.o: shell379.c 
	gcc -c shell379.c

pcb_handler.o:  pcb_handler.c pcb_handler.h
	gcc -c pcb_handler.c

cmd.o: cmd.c header.h
	gcc -c cmd.c


clean:
	rm -f shell379
