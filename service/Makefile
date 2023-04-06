# Makefile
#
# targets:
#   ui:           compile user interface to python file
#   setup:        install python dependencies
#   distro:       build single executable
#   clean:        remove all build files

TARGET = lsmod

PWD := $(strip $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST)))))

ifeq ($(OS),Windows_NT)
$(info $(TARGET) service windows)
SFX = .exe
else
$(info $(TARGET) service linux)
SFX =
endif

PYTHON = python
PIP = pip
PYUIC = pyuic5
PYRCC = pyrcc5
PYINSTALLER = pyinstaller

.PHONY: all

all: ui

setup: requirements.txt
	$(PIP) install -r requirements.txt

distro: ui
	rm -f dist/$(TARGET)$(SFX)
	$(PYINSTALLER) -F --paths $(PWD) --icon $(TARGET).ico --windowed --onefile $(TARGET).py

ui: $(TARGET).ui $(TARGET).qrc
	rm -f ui_$(TARGET).py
	$(PYUIC) $(TARGET).ui -o ui_$(TARGET).py
	$(PYRCC) $(TARGET).qrc -o $(TARGET)_rc.py

clean:
	rm -f ui_$(TARGET).py $(TARGET)_rc.py $(TARGET).spec dist/$(TARGET)$(SFX)
	rm -r -f build dist
