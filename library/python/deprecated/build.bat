python setup.py build_ext
copy /Y .\build\lib.win32-2.6\pydfhack.pyd ..\..\output\pydfhack.pyd
rmdir /S /Q .\build
pause