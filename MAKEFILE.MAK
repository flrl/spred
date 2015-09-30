MEM = c
TC = c:\tc
C0x = $(TC)\lib\c0$(MEM)
Cx = $(TC)\lib\c$(MEM)

TARGET = spred.exe

OBJS = 			\
	geom.obj	\
	mouse.obj	\
	sprite.obj	\
	ui.obj 		\
	vga13h.obj	\
	main.obj

HEADERS = 		\
	geom.h		\
	mouse.h		\
	sprite.h	\
	types.h		\
	vga13h.h	\
	util.h		\
	ui.h

LIBS =

$(TARGET): $(OBJS) $(HEADERS)
	tlink /c $(LDFLAGS) $(C0x) $(OBJS), $*, $*, $(LIBS) $(Cx)

.c.obj:
	tcc -c -m$(MEM) $(CFLAGS) $*

clean:
	- del $(TARGET)
	- del *.map
	- del *.obj