all:
	gcc server.c volkov.c -o server -lpthread -lm
	gcc server.c volkov.c -o serversan -fsanitize=address -lpthread -lm
	gcc client.c volkov.c -o client -lpthread -lm
	gcc client.c volkov.c -o clientsan -fsanitize=address -lpthread -lm
	cppcheck --enable=all --inconclusive --std=posix server.c client.c volkov.c
	/usr/src/linux-source-4.4.0/scripts/checkpatch.pl -f server.c client.c volkov.c
