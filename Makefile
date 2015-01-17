include Makefile.shared

run: $(NAME)
	export DYLD_LIBRARY_PATH="$(JANSSON_LIB)"; \
	 ./$^ $(RUN_PARAMS)

debug: $(NAME)
	export DYLD_LIBRARY_PATH="$(JANSSON_LIB)"; \
	gdb ./$^ $(RUN_PARAMS)

jansson_configure:
	cd $(JANSSON_DIR); [ ! -e "configure" ] && autoreconf -fi; \
		[ ! -e "Makefile" ] && ./configure; \
		[ -e "Makefile" ]
