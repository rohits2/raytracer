all:
	clang++ -Wall -Werror -std=c++17 -lpthread -Ofast src/*.cpp -o main

debug:
	clang++ -Wall -Werror -std=c++17 -lpthread -O1 -g src/*.cpp -o main