call msvc_include.bat
%msbuild% /m /p:Platform=x64 /p:Configuration=Release VC2022/ALL_BUILD.vcxproj
pause
