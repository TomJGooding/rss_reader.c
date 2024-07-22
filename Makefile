P=rss_reader
OBJECTS=
CFLAGS=-Wall -Wextra `pkg-config --cflags libxml-2.0`
LDLIBS=`pkg-config --libs libxml-2.0`

$(P): $(OBJECTS)
