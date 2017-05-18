@echo off
REM Run the validating test collector.

python ..\..\code\collector\collector.py ^
       --config ..\..\config\collector.conf ^
       --section windows ^
       --verbose
