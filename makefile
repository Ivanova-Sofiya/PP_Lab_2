
start: use_mutex.cpp
	gcc -D_REENTRANT use_mutex.cpp -lm -lstdc++ -lpthread -o start

time: get_time.cpp
	gcc -D_REENTRANT get_time.cpp -lm -lstdc++ -lpthread -o get_time

map_reduce: map_reduce.cpp
	gcc -D_REENTRANT map_reduce.cpp -lm -lstdc++ -lpthread -o map_reduce

find_string: find_string.cpp
	g++ -std=c++11 find_string.cpp -lpthread -lstdc++fs -o find_string

clean_start:
	rm start

clean_time:
	rm get_time

clean_map:
	rm map_reduce

clean_find:
	rm find_string


