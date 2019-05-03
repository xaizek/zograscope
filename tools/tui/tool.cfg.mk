$(tui.bin): EXTRA_LDFLAGS += -lcursesw
$(tui.objects): EXTRA_CXXFLAGS += -Itools/tui/libs

$(out_dir)/zs-tui: $(tui.objects)
