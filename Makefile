#
# A "real" project would use GNU autotools rather than a simple Makefile.
#
HEADERS=rump.h
OBJS=rum.o rump.o
CFLAGS=-I. -Wall
CMD=rum

all: $(OBJS)
	$(CC) -o $(CMD) $(OBJS)

install:
	@A="y"; while [[ $$A == "y" ]]; do \
		echo "Are you sure you want to install? (y/n) \c"; \
		read A; \
	done

clean:
	rm -f $(CMD) $(OBJS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
