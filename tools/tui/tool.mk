ifeq ($(HAVE_CURSESW),)
    tools := $(filter-out tui,$(tools))
endif

$(out_dir)/zs-tui: $(tui.objects)
	$(CXX) $(EXTRA_LDFLAGS) $(tui.objects) $(lib) $(LDFLAGS) -o $@
