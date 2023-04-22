all: map_handler first_gardener second_gardener

map_handler: map_handler.c common.h
	gcc map_handler.c -o map_handler -lrt -lpthread
first_gardener: first_gardener.c common.h
	gcc first_gardener.c -o first_gardener -lrt -lpthread
second_gardener: second_gardener.c common.h
	gcc second_gardener.c -o second_gardener -lrt -lpthread

clean:
	rm map_handler first_gardener second_gardener
