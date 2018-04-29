fsck: fscheck.c fs.h types.h param.h stat.h
	gcc -Werror -Wall -o fscheck fscheck.c

corruptfs: corruptfs.c fs.h types.h param.h stat.h
	gcc -Werror -Wall -o corruptfs corruptfs.c

corrupttest: corruptfs fs-origin.img fsck
	cp fs-origin.img fs-test.img
	./corruptfs fs-test.img $(test) 
	./fscheck fs-test.img

clean:
	rm fs-test.img corruptfs fscheck

all: fsck
