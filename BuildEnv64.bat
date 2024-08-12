call _BuildEnv64.bat

REM Open an interactive Git Bash
if exist "C:\Program Files (x86)\Git\bin\bash.exe" (
	"C:\Program Files (x86)\Git\bin\bash.exe" --login -i
) else (
	"C:\Program Files\Git\bin\bash.exe" --login -i
)
