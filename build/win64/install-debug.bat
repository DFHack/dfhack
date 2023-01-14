call msvc_include.bat
%msbuild% /m /p:Platform=x64 /p:Configuration=RelWithDebInfo VC2022/INSTALL.vcxproj
