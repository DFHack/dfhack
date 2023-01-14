call msvc_include.bat
%msbuild% /m /p:Platform=x64 /p:Configuration=RelWithDebInfo VC2022/ALL_BUILD.vcxproj

