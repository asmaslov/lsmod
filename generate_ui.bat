@echo off
IF EXIST ui_lsmod.py del /F ui_lsmod.py
pyuic4 lsmod.ui -o ui_lsmod.py
