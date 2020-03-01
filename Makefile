CC = gcc
DEPS = ubash.h
OBJ = ubash.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< 

ubash: $(OBJ)
	gcc -o $@ $^ 