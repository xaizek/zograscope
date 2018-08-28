gdiff.abs_out := $(abspath $(out_dir))/tools/gdiff

gdiff.extradeps := $(addprefix tools/gdiff/, gdiff.pro help.html resources.qrc)
gdiff.extradeps += $(wildcard $(addprefix tools/gdiff/, *.ui *.hpp))

gdiff.qmake_args := OUT="$(abspath $(out_dir))"
ifeq ($(is_release),0)
    gdiff.qmake_args += CONFIG+=debug
endif

$(out_dir)/zs-gdiff:
	@cd $(gdiff.abs_out)/ && \
	    qmake-qt5 $(gdiff.qmake_args) $(abspath .)/tools/gdiff/gdiff.pro
	+@$(MAKE) -C $(gdiff.abs_out)/
	@mv $(gdiff.abs_out)/zs-gdiff $(out_dir)/zs-gdiff

clean: gdiff.clean

gdiff.clean:
	-@make -C $(gdiff.abs_out)/ distclean

.PHONY: gdiff.clean
