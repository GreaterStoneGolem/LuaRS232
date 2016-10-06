@echo OFF

REM Folder where you have the Lua source & compiled binary
set LUA53=..\lua-5.3.1

REM Clean previous results
del *.o  *.a  *.dll 2> NUL
REM Check if compiler is present in path, add MinGW to path if no 
g++ --help > NUL 2>&1 || set PATH=C:\MinGW\bin;%PATH%

REM Compile all the .c and into a .o
g++ -c *.c RS-232/*.c -I %LUA53%/src -I RS-232 || exit /b 1
REM Turn the .o into the .dll
g++ -L%LUA53%\bin -LRS-232 -lsetupapi -shared -o LuaRS232.dll *.o -llua53 -lwinmm -I %LUA53%/src -static-libgcc -static-libstdc++ || exit /b 1

REM Delete useless temp files
del *.o *.a

REM Test the .dll

REM Print interface info: first the initial table, then the list of comm by every method, then open a comm and list method of opened comm
%LUA53%\bin\lua53.exe PrintInterfaceInfo.lua || exit /b 1

REM Get the name of the first Comm Port from last line of stdout of this lua one-liner 
for /f "usebackq" %%C in (`%LUA53%\bin\lua53.exe -e "print(require('LuaRS232').SerialPortList()[1].name)"`) do (
	set PORTNAME=%%C
)

REM Do some send and receive on the first comm port found
%LUA53%\bin\lua53.exe TestRS232.lua %PORTNAME% || exit /b 1

