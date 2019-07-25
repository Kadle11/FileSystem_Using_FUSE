COMPILER = gcc
FILESYSTEM_FILES = MyFS.c

build: $(FILESYSTEM_FILES)
	$(COMPILER) $(FILESYSTEM_FILES) -Wall -g -o ssfs `pkg-config fuse --cflags --libs`
	echo 'To Mount: ./ssfs -f /home/vishal/Desktop/College/Sem 5/IOS-Lab/FUSE/SSFS-master'

clean:
	rm ssfs
