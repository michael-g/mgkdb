
libsnoop.so: type_snoop.c
	gcc -I $(PWD)/../kx_systems/include -fPIC -shared -o libsnoop.so -Wall type_snoop.c

clean: libsnoop.so
	rm -f libsnoop.so
