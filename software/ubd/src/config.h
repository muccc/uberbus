#ifndef __CONFIG_H_
#define __CONFIG_H_
#include <glib.h>

struct ubconfig {
    gchar *interface;
    gchar *base;
    gint prefix;
    gchar *device;
    gint rate;
};

extern struct ubconfig config;
#endif