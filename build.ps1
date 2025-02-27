Remove-Item -Recurse -Force "x64\Release"

.\prebuild.ps1

$env:_CL_="/MD"
msbuild.exe GlosSI.sln /t:Build /p:Configuration=Release /p:Platform=x64


cd ./x64/Release/

$env:Path += ';C:\Qt\6.2.4\msvc2019_64\bin'

Get-ChildItem -Rec | Where {$_.Extension -match "lib"} | Remove-Item
$env:Path = "$env:QTDIR\bin;$env:Path"

windeployqt.exe --release --qmldir ../../GlosSIConfig/qml ./GlosSIConfig.exe

Copy-Item "..\..\deps\SFML\out\Release\lib\RelWithDebInfo\sfml-graphics-2.dll" -Destination "."
Copy-Item "..\..\deps\SFML\out\Release\lib\RelWithDebInfo\sfml-system-2.dll" -Destination "."
Copy-Item "..\..\deps\SFML\out\Release\lib\RelWithDebInfo\sfml-window-2.dll" -Destination "."
Copy-Item "..\..\GlosSIConfig\GetAUMIDs.ps1" -Destination "."
Copy-Item "..\..\HidHideMSI.msi" -Destination "."
Copy-Item "..\..\ViGEmBusSetup_x64.msi" -Destination "."
Copy-Item "..\..\vc_redist.x64.exe" -Destination "."
Copy-Item "..\..\LICENSE" -Destination "./LICENSE"
Copy-Item "..\..\QT_License" -Destination "./QT_License"
Copy-Item "..\..\THIRD_PARTY_LICENSES.txt" -Destination "./THIRD_PARTY_LICENSES.txt"

#7z a GlosSI-snapshot.zip *

cd ../..

