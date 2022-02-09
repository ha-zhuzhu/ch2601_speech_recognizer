CPRE := @
ifeq ($(V),1)
CPRE :=
VERB := --verbose
endif

MK_GENERATED_IMGS_PATH:=generated

.PHONY:startup
startup: all

all:
	@echo "Build Solution by $(BOARD) "
	$(CPRE) scons $(VERB) --board=$(BOARD) -j4
	@echo YoC SDK Done

	@echo [INFO] Create bin files
	$(CPRE) product image $(MK_GENERATED_IMGS_PATH)/images.zip -i $(MK_GENERATED_IMGS_PATH)/data -l -p
	$(CPRE) product image $(MK_GENERATED_IMGS_PATH)/images.zip -e $(MK_GENERATED_IMGS_PATH) -x

.PHONY:flash
flash:
	$(CPRE) product flash $(MK_GENERATED_IMGS_PATH)/images.zip -w prim -f ./ch2601_flash.elf -x gdbinit

.PHONY:flashall
flashall:
	$(CPRE) product flash $(MK_GENERATED_IMGS_PATH)/images.zip -a -f ./ch2601_flash.elf -x gdbinit

sdk:
	$(CPRE) yoc sdk

.PHONY:clean
clean:
	$(CPRE) scons -c
	$(CPRE) find . -name "*.[od]" -delete
	$(CPRE) rm yoc_sdk yoc.* generated out -rf
