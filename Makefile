########################
#         Args         #

VERBOSE ?= 1
TARGET ?= DEBUG
CONFIRM ?= 0

ifeq ("$(VERBOSE)","1")
	NO_ECHO :=
else
	NO_ECHO := @
endif

########################
#        Define        #

OUTPUT_DIR				:= _output
BUILD_DIR				:= _build
PROJECT_APP_NAME		:= izn
PROJECT_APP_CONFIG      ?= nrf52832_xxaa
VERSION_FILE			:= application/config/app_version.h
#MAKEFILE_CACHE			:= Makefile.cache
#MAKEFILE_VERSION		:= Makefile.version
VERSION_TEMPLATE_FILE 	:= application/config/app_version.hxx

PROJECT_BOOT_NAME		:= izn_bootloader
PROJECT_BOOT_CONFIG     := Release

########################
#    Build version     #

# Load existing cache if any
#ifneq (,$(wildcard $(MAKEFILE_CACHE)*))
#  include $(MAKEFILE_CACHE)
#else
#  include $(MAKEFILE_CACHE)
#  $(info Remote fetch...)
#  include $(MAKEFILE_VERSION)
#  GIT_FETCHED:= 1
#  EXPECTED_LOCAL_MODIFICATION:= $(shell uname | grep -v "NT")$(LOCAL_MODIFICATION)
#endif

#ifeq ("$(LOCAL_MODIFICATION)$(GIT_FETCHED)", "0")
#  EXPECTED_LOCAL_MODIFICATION:= recheck
#else
#  EXPECTED_LOCAL_MODIFICATION?= $(LOCAL_MODIFICATION)
#endif


# Check if a refresh is required
#dummy := $(shell git describe --long --abbrev=40)
#CURRENT_VERSION_ID := $(shell echo $(word 3, $(subst -, ,$(dummy))) | cut -c 2-)

# If repository was already not clean AND we are on windows AND same commit, then we skip version
#SYSTEM_STATUS=$(shell uname | grep -v "NT")$(LOCAL_MODIFICATION)$(CURRENT_VERSION_ID)
#EXPECTED_STATUS=$(EXPECTED_LOCAL_MODIFICATION)$(VERSION_ID)
#ifneq ("$(VERBOSE)", "0")
#  $(info $(SYSTEM_STATUS))
#  $(info $(EXPECTED_STATUS))
#endif

#ifneq ("$(SYSTEM_STATUS)", "$(EXPECTED_STATUS)")
#  $(info New repository check required. Remote fetch...)
#  LOCAL_MODIFICATION =
#  include $(MAKEFILE_VERSION)
#endif

$(info ------------------------------------------------------------)
$(info Build      :. $(VERSION))
$(info Latest tag :. $(VERSION_TAG))
$(info Commit id  :. $(VERSION_ID))
$(info ------------------------------------------------------------)

########################
#     Build target     #

all: bootloader application

release:
	$(NO_ECHO)nrfutil pkg generate --hw-version 52 --application-version 0 --application $(OUTPUT_DIR)/application/$(PROJECT_APP_NAME).hex --sd-req 0x9D --key-file bootloader/config/private.key $(OUTPUT_DIR)/app-$(VERSION).zip
	$(NO_ECHO)nrfutil settings generate --family NRF52 --application $(OUTPUT_DIR)/application/$(PROJECT_APP_NAME).hex --application-version 0 --bootloader-version 0 --bl-settings-version 1 $(OUTPUT_DIR)/bootloader/bootloader_settings.hex
	$(NO_ECHO)mergehex -m $(OUTPUT_DIR)/bootloader/izn_bootloader.hex $(OUTPUT_DIR)/bootloader/bootloader_settings.hex -o $(OUTPUT_DIR)/bootloader/merge-bootloader.hex
	$(NO_ECHO)mergehex -m $(OUTPUT_DIR)/bootloader/merge-bootloader.hex $(OUTPUT_DIR)/application/$(PROJECT_APP_NAME).hex nRF5_SDK/components/softdevice/s132/hex/s132_nrf5.hex -o $(OUTPUT_DIR)/merge-$(VERSION).hex

bootloader:
	$(NO_ECHO)emBuild $(PROJECT_BOOT_NAME).emProject -config $(PROJECT_BOOT_CONFIG)

application: version
	$(NO_ECHO)emBuild $(PROJECT_APP_NAME).emProject -config $(PROJECT_APP_CONFIG)
	
#version: cache
#	$(shell sed -e "s;!VERSION_MAJOR!;$(VERSION_MAJOR);g" \
#-e "s;!VERSION_MINOR!;$(VERSION_MINOR);g" \
#-e "s;!VERSION_DEBUG!;$(VERSION_DEBUG);g" \
#-e "s;!VERSION_MAGIC!;$(VERSION_MAGIC);g" \
#-e "s;!VERSION_STR!;$(VERSION);g" \
#-e "s;!VERSION_TAG!;$(VERSION_TAG);g" \
#-e "s;!VERSION_COMMIT_ID!;$(VERSION_ID);g" \
#-e "s;!VERSION_DATETIME!;$(shell date +"%Y-%m-%dT%H:%M:%S.%uZ");g" $(VERSION_TEMPLATE_FILE) > $(VERSION_FILE))

clean:
	$(NO_ECHO)emBuild $(PROJECT_APP_NAME).emProject -config $(PROJECT_APP_CONFIG) -clean
	$(NO_ECHO)emBuild $(PROJECT_BOOT_NAME).emProject -config $(PROJECT_BOOT_CONFIG) -clean
	$(NO_ECHO)rm -f $(BUILD_DIR)/*.d
	$(NO_ECHO)rm -rf $(OUTPUT_DIR) $(BUILD_DIR)
	$(NO_ECHO)rm -f $(VERSION_FILE)
	$(NO_ECHO)rm -f $(MAKEFILE_CACHE)


########################
#         Tags         #

tag:
	@if [ ! "$(CONFIRM)" == "1" ]; then \
		echo "[ERROR] Please use 'make tag CONFIRM=1' to confirm!"; \
		exit 1; \
	fi
	@if [ -n "$(OFFLINE)" ]; then \
		echo "[ERROR] OFFLINE mode is not possible"; \
		exit 1; \
	fi
	@if [ -n "$(git branch --remotes --contains HEAD | grep origin/master)" ]; then \
		echo "[ERROR] You are not allowed to tag a non master commit"; \
		exit 1; \
	fi
	@if [ ! "$(VERSION_DEBUG)" == "0" ] && [ ! "$(VERSION_DEBUG)" == "20" ]; then \
		echo "[ERROR] Your repository must be clean and sync to remote"; \
		exit 1; \
	fi	
	git tag $(VERSION_CROSSPLATFORM)
	git push origin $(VERSION_CROSSPLATFORM)


tag-integration:
	@if [ -n "$(OFFLINE)" ]; then \
		echo "[ERROR] OFFLINE mode is not possible"; \
		exit 1; \
	fi

	@if [ ! "$(LOCAL_MODIFICATION)" == "0" ]; then \
		echo "[ERROR] Repository should be clean."; \
		exit 1; \
	fi

	@git tag integration-$(VERSION_CROSSPLATFORM)
	@git push origin integration-$(VERSION_CROSSPLATFORM)


########################
#     Update cache     #

cache:
	$(NO_ECHO)echo "VERSION_MAJOR := $(VERSION_MAJOR)" > $(MAKEFILE_CACHE)
	$(NO_ECHO)echo "VERSION_MINOR := $(VERSION_MINOR)" >> $(MAKEFILE_CACHE)
	$(NO_ECHO)echo "VERSION_DEBUG := $(VERSION_DEBUG)" >> $(MAKEFILE_CACHE)
	$(NO_ECHO)echo "VERSION_MAGIC := $(VERSION_MAGIC)" >> $(MAKEFILE_CACHE)
	$(NO_ECHO)echo "VERSION_TAG := $(VERSION_TAG)" >> $(MAKEFILE_CACHE)
	$(NO_ECHO)echo "VERSION_ID := $(VERSION_ID)" >> $(MAKEFILE_CACHE)
	$(NO_ECHO)echo "VERSION := $(VERSION)" >> $(MAKEFILE_CACHE)
	$(NO_ECHO)echo "LOCAL_MODIFICATION := $(LOCAL_MODIFICATION)" >> $(MAKEFILE_CACHE)


.PHONY: clean version cache application bootloader
