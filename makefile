CC = gcc
OBJS = chessTournament.o chessSystem.o chessGame.o chessPlayer.o chessSystemTestsExample.o
EXEC = chess
DEBUG_FLAG = -DNDEBUG
COMP_FLAG = -std=c99 -Wall -pedantic-errors -Werror

$(EXEC) : $(OBJS)
	$(CC) $(OBJS) $(DEBUG_FLAG) -o $@ libmap.a -L -lmap
chessSystemTestsExample.o: tests/chessSystemTestsExample.c \
 tests/../chessSystem.h tests/../test_utilities.h
	$(CC) -c $(DEBUG_FLAG) $(COMP_FLAG) tests/$*.c
chessSystem.o: chessSystem.c chessSystem.h chessTournament.h \
 chessPlayer.h map.h chessGame.h utils.h
	$(CC) -c $(DEBUG_FLAG) $(COMP_FLAG) $*.c
chessTournament.o: chessTournament.c chessTournament.h chessPlayer.h \
 map.h chessGame.h
	$(CC) -c $(DEBUG_FLAG) $(COMP_FLAG) $*.c
chessGame.o: chessGame.c chessGame.h chessPlayer.h map.h
	$(CC) -c $(DEBUG_FLAG) $(COMP_FLAG) $*.c
chessPlayer.o: chessPlayer.c chessPlayer.h map.h
	$(CC) -c $(DEBUG_FLAG) $(COMP_FLAG) $*.c
clean:
	rm -f $(OBJS) $(EXEC)
	
