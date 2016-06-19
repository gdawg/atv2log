# asllog - appletv2 system log tool

This functions as a primitive replacement for the "syslog" command found
on osx.

It can be asked to tail the system log from boot with -B or from recent
messages onwards (default / -w).

```bash
appletv# asllog -B |head -20
Jun 18 19:27:25  AppleTV[21478] <Notice>: T:[0x2ff03000] ATVSystemLog : Reloading URL bag
Jun 18 10:12:13  freemem-watcher[36195] <Notice>: Check apsd memory usage...
Jun 18 10:12:13  freemem-watcher[36205] <Notice>: Process apsd real mem is 1864 KB
```


### implementation

Under the covers it uses apple's asl log facilities albeit an early version
of those for which I cobbled together headers from a few apple open source
projects to get going.

### building

Building *ought* be as easy as **Make** if you've got the xcode commandline
tools setup and an ios sdk installed. 

```bash
$ make
clang -c asllog.c  -isysroot *iPHONE_SDK_PATH* -arch armv7
clang -o asllog asllog.o  -isysroot *iPHONE_SDK_PATH* -arch armv7
```

The Makefile is supposed to figure things out but if it doesn't the
few flags above are all that's needed anyway.

### optional build flags

If local.mk is found it'll be sourced during build. 
Depending on the installed clang version you may wish for lto goodness
or other random flags.

```Makefile
# my local.mk
CFLAGS += -O3
CFLAGS += -flto -emit-llvm
LDFLAGS += -flto
LDFLAGS += -dead_strip

all_local: all

install: all
    scp asllog appletv:`ssh appletv which syslog`
```




