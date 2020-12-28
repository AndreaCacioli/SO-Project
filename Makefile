# flags per la compilazione
CFLAGS = -std=c89 -pedantic

grid.o: grid.c grid.h
	gcc -c  $(CFLAGS) grid.c

taxi.o: taxi.c taxi.h
	gcc -c  $(CFLAGS) taxi.c

mapgenerator.o: mapgenerator.c mapgenerator.h
	gcc -c  $(CFLAGS) mapgenerator.c

cell.o: cell.h cell.c
	gcc -c  $(CFLAGS) cell.c

Master.o: Master.c
	gcc -c -$(CFLAGS) Master.c

TheGame : Master.o taxi.o cell.o mapgenerator.o grid.o
	gcc Master.o taxi.o cell.o mapgenerator.o grid.o -lm -o TheGame

clean:
	rm -f *.o grid*~
	rm -f *.o taxi*~
	rm -f *.o mapgenerator*~
	rm -f *.o cell*~
	rm -f *.o Master*~
	rm -f *.o TheGame*~
run:  TheGame
	./TheGame
