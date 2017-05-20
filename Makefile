CFLAGS=-g -Wall -Werror

# main.o: main.c schema.h

tests: json_lexer_test

json_lexer.o: json_lexer.c json_lexer.h
json_lexer_test.o: json_lexer_test.c

json_lexer_test: json_lexer_test.o json_lexer.o
	$(CC) $(CFLAGS) -o $@ $+

#jsane: main.o
#	gcc $(CFLAGS) -o jsane $
