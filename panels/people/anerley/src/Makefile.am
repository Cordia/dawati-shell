libexec_PROGRAMS = anerley-account-starter

%.c: %.list Makefile.am
	$(QUIET_GEN)glib-genmarshal --body --prefix=sw_marshal $< > $@

%.h: %.list Makefile.am
	$(QUIET_GEN)glib-genmarshal --header --prefix=sw_marshal $< > $@

BUILT_SOURCES = sw-marshals.c sw-marshals.h

anerley_account_starter_SOURCES = anerley-account-starter.c \
				  sw-online.c \
				  sw-online.h \
				  sw-marshals.list \
				  $(BUILT_SOURCES)
anerley_account_starter_CFLAGS = $(PANEL_PEOPLE_CFLAGS)
anerley_account_starter_LDADD = $(PANEL_PEOPLE_LIBS)
