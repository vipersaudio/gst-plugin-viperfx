# Note: plugindir is set in configure

plugin_LTLIBRARIES = libgstviperfx.la

# sources used to compile this plug-in
libgstviperfx_la_SOURCES = gstviperfx.c viperfx_so.c

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstviperfx_la_CFLAGS = $(GST_CFLAGS)
libgstviperfx_la_LIBADD = $(GST_LIBS)
libgstviperfx_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS) -rdynamic -ldl
libgstviperfx_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstviperfx.h viperfx_so.h
