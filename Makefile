all:
	clang++ -Wall -Werror -std=c++17 -lpthread -Ofast src/*.cpp -o main

debug:
	clang++ -Wall -Werror -std=c++17 -lpthread -O0 -g src/*.cpp -o main