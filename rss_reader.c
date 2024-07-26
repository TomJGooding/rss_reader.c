#define _GNU_SOURCE  // strptime
#include <curl/curl.h>
#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/xmlmemory.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_FEED_ITEMS 25

#define DOWNLOAD_OK 0

typedef struct {
    xmlChar *title;
    xmlChar *link;
    time_t pub_date;
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
    printf("\n%s\n", feed->description);

    for (size_t i = 0; i < feed->items_count; i++) {
        printf("\n%s\n", feed->items[i]->title);
        printf("<%s>\n", feed->items[i]->link);
        printf("%s", ctime(&feed->items[i]->pub_date));
    }
}

bool node_name_is(xmlNodePtr node, char *name) {
    return xmlStrcmp(node->name, (const xmlChar *)name) == 0;
}

time_t convert_rfc822_date(xmlChar *date_str) {
    struct tm tm = {};
    strptime((char *)date_str, "%a, %d %b %Y %T", &tm);
    return timegm(&tm);
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
        if (node_name_is(node, "pubDate")) {
            xmlChar *date_str = xmlNodeGetContent(node->children);
            item->pub_date = convert_rfc822_date(date_str);
            free(date_str);
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

int download_rss(char *url, char *filename) {
    FILE *rss_file = fopen(filename, "w");
    if (rss_file == NULL) {
        fprintf(stderr, "Error: failed to open \"%s\"\n", filename);
        return -1;
    }

    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        fprintf(stderr, "Error: failed to initialize libcurl\n");
        return -1;
    }
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http,https");
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "http,https");
    curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, rss_file);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(
            stderr,
            "Error: failed to download RSS: %s\n",
            curl_easy_strerror(res)
        );
    }

    curl_easy_cleanup(curl);

    fclose(rss_file);

    return res;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <url>\n", argv[0]);
        return 0;
    }

    char *url = argv[1];
    char *filename = "rss.xml";
    if (download_rss(url, filename) != DOWNLOAD_OK) {
        return 1;
    }

    xmlDocPtr doc = xmlParseFile(filename);
    if (doc == NULL) {
        fprintf(stderr, "Error: unable to parse file\n");
        return 1;
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
