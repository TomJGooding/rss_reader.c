P=rss_reader
OBJECTS=
CFLAGS=-Wall -Wextra `curl-config --cflags` `pkg-config --cflags libxml-2.0`
LDLIBS=`curl-config --libs` `pkg-config --libs libxml-2.0`

$(P): $(OBJECTS)
