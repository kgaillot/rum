#
# A "real" project would use GNU autotools rather than a simple Makefile.
#
LDFLAGS=-L.
CFLAGS=-I. -Wall

# library
HEADERS=rump.h rum_language.h rum_document.h rump_private.h
LIBOBJS=rump.o rum_language.o rum_document.o
LIBRARY=librump.a

# application
CMD=rum
OBJS=rum.o

all: $(CMD) $(LIBRARY)

install:
	@A="y"; while [[ $$A == "y" ]]; do \
		echo "Are you sure you want to install? (y/n) \c"; \
		read A; \
	done

clean:
	rm -f $(CMD) $(OBJS) $(LIBRARY) $(LIBOBJS)

$(CMD): $(OBJS) $(LIBRARY)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(CMD) $(OBJS) -lrump

$(LIBRARY): $(LIBOBJS)
	ar $(ARFLAGS) $@ $^

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(CPPFLAGS) -c -o $@ $<
