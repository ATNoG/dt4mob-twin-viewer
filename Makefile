SHELL := /bin/zsh

PROJECT_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
PROJECT := $(PROJECT_ROOT)/DT4MOB.uproject

ENGINE_ROOT ?= /Users/Shared/Epic Games/UE_5.6
UAT := $(ENGINE_ROOT)/Engine/Build/BatchFiles/RunUAT.sh

RELEASE_ROOT ?= $(PROJECT_ROOT)/Releases/Tester
ARCHIVE_APP := $(RELEASE_ROOT)/Mac/DT4MOB.app
RELEASE_DATE ?= $(shell date +%Y-%m-%d)
RELEASE_ZIP := $(RELEASE_ROOT)/DT4MOB-macOS-Development-$(RELEASE_DATE).zip

.DEFAULT_GOAL := release

.PHONY: help check prepare-mac editor build archive release verify

help:
	@echo "DT4MOB tester release targets"
	@echo "  make editor       Rebuild the Unreal Editor module after source changes"
	@echo "  make release      Build, cook, package, archive, zip, and verify (default)"
	@echo "  make build        Build/cook/package the macOS Development application"
	@echo "  make archive      Create the tester ZIP from the packaged application"
	@echo "  make verify       Verify the application signature and ZIP integrity"
	@echo ""
	@echo "Optional overrides:"
	@echo "  ENGINE_ROOT=/path/to/UE_5.6"
	@echo "  RELEASE_ROOT=/path/to/output"
	@echo "  RELEASE_DATE=YYYY-MM-DD"

check:
	@test -x "$(UAT)" || { echo "Unreal Automation Tool not found at: $(UAT)"; exit 1; }
	@test -f "$(PROJECT)" || { echo "Project not found at: $(PROJECT)"; exit 1; }
	@test -f "$(PROJECT_ROOT)/Config/Secrets.ini" || { echo "Config/Secrets.ini is required for the tester build"; exit 1; }
	@command -v ditto >/dev/null || { echo "The macOS 'ditto' utility is required"; exit 1; }
	@echo "Note: Config/Secrets.ini will be embedded in the tester package."

# Cesium 2.28's prebuilt macOS module can contain a malformed relative rpath for
# Unreal's SunPosition plugin. Ensure both modules validate and add a stable
# search path before the editor starts the cook commandlet.
prepare-mac: check
	@set -e; \
	cesium_dir=$$(find "$(ENGINE_ROOT)/Engine/Plugins/Marketplace" -maxdepth 1 -type d -iname 'Cesium*' | head -1); \
	test -n "$$cesium_dir" || { echo "Cesium for Unreal is not installed in UE 5.6"; exit 1; }; \
	cesium_lib="$$cesium_dir/Binaries/Mac/UnrealEditor-CesiumRuntime.dylib"; \
	sun_dir="$(ENGINE_ROOT)/Engine/Plugins/Runtime/SunPosition/Binaries/Mac"; \
	sun_lib="$$sun_dir/UnrealEditor-SunPosition.dylib"; \
	test -f "$$cesium_lib" || { echo "Missing Cesium macOS runtime module"; exit 1; }; \
	test -f "$$sun_lib" || { echo "Missing Unreal SunPosition macOS module"; exit 1; }; \
	codesign --verify --strict "$$sun_lib" >/dev/null 2>&1 || codesign --force --sign - "$$sun_lib"; \
	if ! otool -l "$$cesium_lib" | grep -Fq "path $$sun_dir "; then \
		install_name_tool -add_rpath "$$sun_dir" "$$cesium_lib"; \
	fi; \
	codesign --verify --strict "$$cesium_lib" >/dev/null 2>&1 || codesign --force --sign - "$$cesium_lib"

editor: prepare-mac
	@"$(ENGINE_ROOT)/Engine/Build/BatchFiles/Mac/Build.sh" \
		DT4MOBEditor Mac Development "$(PROJECT)" \
		-WaitMutex -NoHotReloadFromIDE

build: prepare-mac
	@"$(UAT)" BuildCookRun \
		-project="$(PROJECT)" \
		-noP4 \
		-platform=Mac \
		-clientconfig=Development \
		-build -cook -stage -pak -package -archive \
		-archivedirectory="$(RELEASE_ROOT)" \
		-utf8output

archive: build
	@mkdir -p "$(RELEASE_ROOT)"
	@rm -f "$(RELEASE_ZIP)"
	@ditto -c -k --sequesterRsrc --keepParent "$(ARCHIVE_APP)" "$(RELEASE_ZIP)"
	@echo "Created: $(RELEASE_ZIP)"

verify:
	@test -d "$(ARCHIVE_APP)" || { echo "Packaged app not found: $(ARCHIVE_APP)"; exit 1; }
	@test -f "$(RELEASE_ZIP)" || { echo "Release ZIP not found: $(RELEASE_ZIP)"; exit 1; }
	@codesign --verify --deep --strict --verbose=2 "$(ARCHIVE_APP)"
	@unzip -tq "$(RELEASE_ZIP)"
	@shasum -a 256 "$(RELEASE_ZIP)"
	@du -sh "$(ARCHIVE_APP)" "$(RELEASE_ZIP)"

release: archive verify
