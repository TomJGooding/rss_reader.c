#include <libxml2/libxml/parser.h>
#include <stdio.h>

void parse_rss(xmlNodePtr rss) {
    xmlNodePtr channel;
    for (channel = rss->children; channel; channel = channel->next) {
        if (!xmlStrcmp(channel->name, (const xmlChar *)"channel")) {
            break;
        }
    }

    xmlNodePtr cur;
    for (cur = channel->children; cur; cur = cur->next) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"title")) {
            xmlChar *title = xmlNodeGetContent(cur->children);
            printf("=== %s ====\n", title);
            xmlFree(title);
        }
        if (!xmlStrcmp(cur->name, (const xmlChar *)"description")) {
            xmlChar *description = xmlNodeGetContent(cur->children);
            printf("%s\n", description);
            xmlFree(description);
        }
    }
}

int main() {
    char *filename = "sample-rss-2.xml";

    xmlDocPtr doc = xmlParseFile(filename);
    xmlNodePtr rss = xmlDocGetRootElement(doc);

    parse_rss(rss);

    xmlFreeDoc(doc);

    return 0;
}
