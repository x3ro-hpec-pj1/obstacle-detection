include Makefile.shared

run: $(NAME)
	setenv DYLD_LIBRARY_PATH $(JANSSON_LIB) ; ./$^ ../res/scanner2.out

run_local: $(NAME)
	setenv DYLD_LIBRARY_PATH $(JANSSON_LIB) ; ./$^ ./scanner.out

debug: $(NAME)
	setenv DYLD_LIBRARY_PATH $(JANSSON_LIB) ; lldb ./$^ ../res/scanner.out

run_scanner: $(NAME)
	setenv DYLD_LIBRARY_PATH $(JANSSON_LIB) ; ./$^ /dev/cu.usbmodemfa131

jansson_configure:
	cd $(JANSSON_DIR); [ ! -e "configure" ] && autoreconf -fi; \
		[ ! -e "Makefile" ] && ./configure; \
		[ -e "Makefile" ]
