CFLAGS=-g -Wall -Werror # -O3

# main.o: main.c schema.h

tests: sjp_lexer_test sjp_parser_test

clean:
	rm -f *.o sjp_lexer_test sjp_parser_test

sjp_lexer.o: sjp_lexer.c sjp_lexer.h sjp_common.h

sjp_parser.o: sjp_parser.c sjp_parser.h sjp_lexer.h sjp_common.h

sjp_testing.o: sjp_testing.c sjp_testing.h sjp_lexer.h sjp_parser.h sjp_common.h
sjp_lexer_test.o: sjp_lexer_test.c sjp_lexer.h sjp_testing.h sjp_common.h
sjp_parser_test.o: sjp_parser_test.c sjp_testing.h sjp_lexer.h sjp_parser.h sjp_common.h

sjp_lexer_test: sjp_lexer_test.o sjp_lexer.o sjp_testing.o
	$(CC) $(CFLAGS) -o $@ $+

sjp_parser_test: sjp_parser_test.o sjp_parser.o sjp_lexer.o sjp_testing.o

#jsane: main.o
#	gcc $(CFLAGS) -o jsane $
