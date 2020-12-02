taxi.o: taxi.c taxi.h
	gcc -c  -std=c89 -pedantic taxi.c

mapgenerator.o: mapgenerator.c mapgenerator.h
	gcc -c  -std=c89 -pedantic mapgenerator.c

cell.o: cell.h cell.c
	gcc -c  -std=c89 -pedantic cell.c

Master.o: Master.c
	gcc -c -std=c89 -pedantic Master.c

TheGame : Master.o taxi.o cell.o mapgenerator.o
	gcc Master.o taxi.o cell.o mapgenerator.o -o TheGame
