# Build REPL executable
bin_PROGRAMS = ditty

if HAVE_BOBA
ditty_SOURCES = main.c repl_app.c flare_style_charmtones.c
else
ditty_SOURCES = main.c
endif

ditty_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src \
    -I$(top_builddir)/src \
    $(GC_CFLAGS) \
    $(PCRE2_CFLAGS)

# ditty_version.h is generated in $(top_builddir)/src. Force it to exist
# before any repl objects compile.
$(ditty_OBJECTS): $(top_builddir)/src/ditty_version.h
$(top_builddir)/src/ditty_version.h:
	$(MAKE) -C $(top_builddir)/src ditty_version.h

ditty_LDADD = \
    $(top_builddir)/src/libditty.a \
    $(GC_LIBS) \
    $(PCRE2_LIBS)

# Windows resource file — embed application icon into the .exe via windres.
# The .rc file lives in $(srcdir) and references icons/ditty.ico relative
# to the top source directory, so we pass --preprocessor defines for that.
if HAVE_WINDRES
noinst_DATA = ditty-res.o
ditty_LDADD += ditty-res.o

ditty-res.o: $(srcdir)/ditty.rc $(top_srcdir)/icons/ditty.ico
	$(WINDRES) \
	    --define _UNICODE --define UNICODE \
	    -I$(top_srcdir) \
	    -I$(srcdir) \
	    $(srcdir)/ditty.rc -O coff -o $@
endif

if HAVE_BOBA
ditty_CPPFLAGS += $(BOBA_CFLAGS)
ditty_LDADD += $(BOBA_LIBS)
endif

EXTRA_DIST = colors.h repl_app.h
