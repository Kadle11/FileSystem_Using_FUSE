COMPILER = gcc
FILESYSTEM_FILES = MyFS.c

build: $(FILESYSTEM_FILES)
	$(COMPILER) $(FILESYSTEM_FILES) -Wall -g -o TFS `pkg-config fuse --cflags --libs`
	echo 'To Mount: ./TFS -f <Mount Directory>'

clean:
	rm TFS
