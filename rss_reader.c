#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/xmlmemory.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FEED_ITEMS 25

typedef struct {
    xmlChar *title;
    xmlChar *link;
} Item;

typedef struct {
    xmlChar *title;
    xmlChar *description;
    Item *items[MAX_FEED_ITEMS];
    size_t items_count;
} Feed;

void free_feed(Feed *feed) {
    if (feed == NULL) {
        return;
    }
    xmlFree(feed->title);
    xmlFree(feed->description);

    for (size_t i = 0; i < feed->items_count; i++) {
        xmlFree(feed->items[i]->title);
        xmlFree(feed->items[i]->link);
        free(feed->items[i]);
    }

    free(feed);
}

void print_feed(Feed *feed) {
    printf("%s\n", feed->title);
    for (size_t i = 0; i < strlen((char *)feed->title); i++) {
        printf("=");
    }
    printf("\n%s\n\n", feed->description);

    for (size_t i = 0; i < feed->items_count; i++) {
        printf("- %s\n", feed->items[i]->title);
        printf("  <%s>\n", feed->items[i]->link);
    }
}

bool node_name_is(xmlNodePtr node, char *name) {
    return xmlStrcmp(node->name, (const xmlChar *)name) == 0;
}

Item *parse_item(xmlNodePtr item_node) {
    Item *item = malloc(sizeof(Item));
    if (item == NULL) {
        fprintf(stderr, "Error: out of memory\n");
        return NULL;
    }
    *item = (Item){};

    for (xmlNodePtr node = item_node->children; node != NULL;
         node = node->next) {
        if (node_name_is(node, "title")) {
            item->title = xmlNodeGetContent(node->children);
        }
        if (node_name_is(node, "link")) {
            item->link = xmlNodeGetContent(node->children);
        }
    }

    return item;
}

Feed *parse_rss(xmlNodePtr rss) {
    if (rss == NULL) {
        fprintf(stderr, "Error: empty document\n");
        return NULL;
    }
    if (!node_name_is(rss, "rss")) {
        fprintf(stderr, "Error: invalid RSS document, missing <rss> element\n");
        return NULL;
    }

    xmlNodePtr channel = NULL;
    for (channel = rss->children; channel != NULL; channel = channel->next) {
        if (node_name_is(channel, "channel")) {
            break;
        }
    }
    if (channel == NULL) {
        fprintf(
            stderr, "Error: invalid RSS document, missing <channel> element\n"
        );
        return NULL;
    }

    Feed *feed = malloc(sizeof(Feed));
    if (feed == NULL) {
        fprintf(stderr, "Error: out of memory\n");
        return NULL;
    }
    *feed = (Feed){};

    for (xmlNodePtr node = channel->children; node != NULL; node = node->next) {
        if (node_name_is(node, "title")) {
            feed->title = xmlNodeGetContent(node->children);
        }
        if (node_name_is(node, "description")) {
            feed->description = xmlNodeGetContent(node->children);
        }
        if (node_name_is(node, "item")) {
            if (feed->items_count < MAX_FEED_ITEMS) {
                Item *item = parse_item(node);
                if (item != NULL) {
                    feed->items[feed->items_count] = item;
                    feed->items_count++;
                }
            }
        }
    }

    return feed;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Missing filename\n");
        return 0;
    }
    char *filename = argv[1];

    xmlDocPtr doc = xmlParseFile(filename);
    if (doc == NULL) {
        fprintf(stderr, "Error: unable to parse file\n");
        return 0;
    }

    xmlNodePtr rss = xmlDocGetRootElement(doc);

    Feed *feed = parse_rss(rss);
    if (feed != NULL) {
        print_feed(feed);
    }

    free_feed(feed);
    xmlFreeDoc(doc);

    return 0;
}
