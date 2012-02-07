vcftool: vcftool.o vcutil.o
	gcc vcftool.o vcutil.o -Wall -std=c99 -g -o vcftool
	gcc -I/user/include/python2.5 -fPIC -c A3module.c 
	gcc -shared A3module.o vcutil.o -o Vcf.so

vcutil.o : vcutil.c vcutil.h
	gcc -c vcutil.c -Wall -fPIC -std=c99 -g -lpython2.6

vcftool.o : vcftool.c vcftool.h
	gcc -c vcftool.c -Wall -std=c99 -g

clean : 
	rm *.o *.so vcftool