@echo off
@echo Gathering files...
cd /d %~dp0
if not exist "data" mkdir data
xcopy /Y "assets\data" "data\assets\data" /E /I /Q > nul
xcopy /Y "assets\sprites" "data\assets\sprites" /E /I /Q > nul
xcopy /Y "assets\textures" "data\assets\textures" /E /I /Q > nul
xcopy /Y "assets\levels" "data\assets\levels" /E /I /Q > nul
xcopy /Y "assets\misc" "data\assets\misc" /E /I /Q > nul
xcopy /Y "assets\music" "data\assets\music" /E /I /Q > nul
xcopy /Y "assets\sounds" "data\assets\sounds" /E /I /Q > nul
xcopy /Y "licenses" "data\licenses" /E /I /Q > nul
xcopy /Y "src\lib\x86\libfreetype-6.dll" "data" /I /Q > nul
xcopy /Y "src\lib\x86\libmodplug-1.dll" "data" /I /Q > nul
xcopy /Y "src\lib\x86\libpng16-16.dll" "data" /I /Q > nul
xcopy /Y "src\lib\x86\SDL2.dll" "data" /I /Q > nul
xcopy /Y "src\lib\x86\SDL2_image.dll" "data" /I /Q > nul
xcopy /Y "src\lib\x86\SDL2_mixer.dll" "data" /I /Q > nul
xcopy /Y "src\lib\x86\SDL2_ttf.dll" "data" /I /Q > nul
xcopy /Y "src\lib\x86\zlib1.dll" "data" /I /Q > nul
xcopy /Y "src\lib\x86\libvorbis-0.dll" "data" /I /Q > nul
xcopy /Y "src\lib\x86\libvorbisfile-3.dll" "data" /I /Q > nul
xcopy /Y "src\lib\x86\libogg-0.dll" "data" /I /Q > nul
xcopy /Y "src\lib\x86\smpeg2.dll" "data" /I /Q > nul
@echo Copying into release folder...
xcopy /Y "data" "release" /E /I /Q > nul
@echo Sanitizing release folder...
del Release\*.obj Release\*.sbr Release\*.bsc Release\*.res Release\*.pdb Release\*.iobj Release\*.ipdb/Q > nul
rmdir "Release\platformer.tlog" /S /Q > nul
@echo Cleaning up...
rmdir "data" /S /Q > nul