Speedtest.o:speedtest.cc ttree.h ttree_multimap.h ttree_set.h
	g++ -g speedtest.cc -lpmem -lc -fpermissive -Wno-deprecated -o test
clean:
	rm -f my-file test
	echo 1 > /proc/sys/vm/drop_caches
