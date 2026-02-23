BOFNAME		:= wpd_com
CC_X64		:= x86_64-w64-mingw32-g++
STRIP_X64	:= x86_64-w64-mingw32-strip
CFLAGS		:= -fno-asynchronous-unwind-tables -Os -w -fno-ident

all:
	@ $(CC_X64) -o /tmp/$(BOFNAME).x64.o -c $(BOFNAME).cpp $(CFLAGS)
	@ $(STRIP_X64) --strip-unneeded /tmp/$(BOFNAME).x64.o
	@ echo "[*] Compiled $(BOFNAME)"

debug:
	@ $(CC_X64) -o /tmp/$(BOFNAME).x64.o -c $(BOFNAME).cpp $(CFLAGS) -DDEBUG
	@ $(STRIP_X64) --strip-unneeded /tmp/$(BOFNAME).x64.o
	@ echo "[*] Compiled $(BOFNAME) (DEBUG)"