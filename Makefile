OS_TYPE		?=	linux
PROJECT		=	xbari
DESTDIR		?=	/
CC          =  gcc

TARGET		=	xbari
CPPFLAGS	=	-D_FORTIFY_SOURCE=2 -DDEBUG
CFLAGS		=	-g -O2 -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security $(CPPFLAGS)
LDFLAGS		=	-Wl,-z,relro

HEADERS = xbari.h
TESTS = test-input-acpi test-input-sys
SRC = xbari.c
OBJ = ${SRC:.c=.o}

all: $(TARGET)

.c.o:
	${CC} -c $< ${CFLAGS}

${OBJ}: ${HEADERS}

$(TARGET): ${OBJ}
	${CC} -o $@ $< -lX11 $(LDFLAGS)

clean:
	rm -fr *~ ${OBJ}
	rm -f $(TARGET)


install: $(TARGET)
	install -d -m 0755 $(DESTDIR)/usr/lib/$(PROJECT)
	install -d -m 0755 $(DESTDIR)/usr/share/$(PROJECT)
	install -d -m 0755 $(DESTDIR)/usr/bin
	install -d -m 0755 $(DESTDIR)/usr/share/man/man1
	install -m 0755 $(TARGET) $(DESTDIR)/usr/bin/
	install -m 0755 $(TESTS)  $(DESTDIR)/usr/share/$(PROJECT)/
	install -m 0644 $(PROJECT).man $(DESTDIR)/usr/share/man/man1/$(PROJECT).1 

