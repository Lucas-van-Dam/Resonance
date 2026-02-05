@echo off

pushd ..
Vendor\Binaries\Premake\Windows\premake5.exe --file=Build.lua vs2022

Vendor\Binaries\Premake\Windows\premake5.exe --file=Build.lua --cc=clang --config=Debug ecc
popd
pause