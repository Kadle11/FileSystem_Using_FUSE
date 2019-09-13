Implementation of a Simple Filesystem (Using FUSE)
=======================================

### Installing Dependencies
This file system was built on Ubuntu 16.04 (Xenial Xerus) <br />
FUSE Version 2.9.4

To Install the Library, <br />
``` 
sudo apt-get update
sudo apt-get install libfuse-dev
```

### Features supported by the Filesystem

1. Create a Directory
2. Remove a non-empty Directory
3. Create a File
4. Delete a File
5. Access, Modified and Status Change Times
6. Open and Close Files
7. Read from and Write into Files
8. Persistent Storage (When FS is Remounted data is retrived and Most recent state of FS is restored)
9. Data backup and Hierarchy of FS is also created and updated every time program is terminated.
10. After Program Termination, the user can still access all the Directories and Files (__2 Levels of Persistence__)

### Cloning and Execution

Clone the Reposirory <br/>
```git clone https://github.com/Kadle11/FileSystem_Using_FUSE.git TFS```

Compiling the code <br/>
``` make ```

Mounting the Filesystem <br/>
``` ./TFS -f <Mount Dir> ```

### The Second Level of Persistence

To Implement the Second Level of Persistence, an additional requirement is to have a Folder called 'The_File_System' that has non-root read and write permissions as a Sibling of the TFS Directory.
