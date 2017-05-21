CFLAGS=-g -Wall -Werror -O3

# main.o: main.c schema.h

tests: json_lexer_test json_parser_test

clean:
	rm -f *.o json_lexer_test json_parser_test

json_lexer.o: json_lexer.c json_lexer.h

json_parser.o: json_parser.c json_parser.h json_lexer.h

json_lexer_test.o: json_lexer_test.c json_lexer.h
json_parser_test.o: json_parser_test.c json_parser.h json_lexer.h

json_lexer_test: json_lexer_test.o json_lexer.o
	$(CC) $(CFLAGS) -o $@ $+

json_parser_test: json_parser_test.o json_parser.o json_lexer.o

#jsane: main.o
#	gcc $(CFLAGS) -o jsane $
