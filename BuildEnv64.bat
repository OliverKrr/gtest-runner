call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86_amd64

set buildtype=RelWithDebInfo

set QTDIR=C:\Qt\5.6\msvc2015

set CMAKE_PREFIX_PATH=%QTDIR%

"C:\Program Files (x86)\Git\bin\bash.exe" --login -i
REM "C:\Program Files (x86)\Git\git-bash.exe"
