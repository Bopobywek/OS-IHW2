all: map_handler first_gardener second_gardener system_V_key2 special_semaphores_key exit_key

map_handler: map_handler.c common.h
	gcc map_handler.c -o map_handler -lrt -lpthread
first_gardener: first_gardener.c common.h
	gcc first_gardener.c -o first_gardener -lrt -lpthread
second_gardener: second_gardener.c common.h
	gcc second_gardener.c -o second_gardener -lrt -lpthread
system_V_key2:
	touch system_V_key2
special_semaphores_key:
	touch special_semaphores_key
exit_key:
	touch exit_key
clean:
	rm map_handler first_gardener second_gardener system_V_key2 special_semaphores_key exit_key
