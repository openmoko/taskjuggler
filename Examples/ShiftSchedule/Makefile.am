TASKJUGGLER=../../taskjuggler/taskjuggler
TJX2GANTT=../../tjx2gantt/tjx2gantt
TARGETNAME=ShiftSchedule

EXTRA_DIST = *.html *.tjp *.tjx
CLEANFILES = *.html *.tjx

docprefix = @PACKAGES_DIR@Examples/$(TARGETNAME)

install-data-local:
	$(mkinstalldirs) -m 755 $(DESTDIR)$(docprefix)
	$(INSTALL_DATA) $(srcdir)/*.html $(DESTDIR)$(docprefix)
	$(INSTALL_DATA) $(srcdir)/*.tjp $(DESTDIR)$(docprefix)

uninstall-local:
	-rm -f $(DESTDIR)$(docprefix)

$(TARGETNAME).tjx : $(TARGETNAME).tjp $(TASKJUGGLER)
	$(TASKJUGGLER) --debug 1 $(TARGETNAME).tjp

all-local: $(TARGETNAME).tjx

