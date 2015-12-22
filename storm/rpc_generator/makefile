
generator: lexical.l parser.cpp syntax.y
	flex -o lex.yy.cpp --header-file=lex.yy.hpp lexical.l
	bison -d -o syntax.tab.cpp syntax.y
	g++ -std=c++11 lex.yy.cpp parser.cpp syntax.tab.cpp -lfl -o $@
