@echo off

SET MajorVer=1
SET MinorVer=2
SET RevNo=4
SET BuildNum=35

SET PRODUCT_VER_STRING=%MajorVer%.%MinorVer%.%RevNo%.%BuildNum%
SET FILE_VER_STRING=WIN-%MajorVer%.%MinorVer%.%RevNo%.%BuildNum%
SET FILE_VER_NUMBER=%MajorVer%, %MinorVer%, %RevNo%, %BuildNum%


echo /*
echo   FILE GENERATED BY write_build_no.bat
echo                *** DO NOT EDIT ***
echo   TO CHANGE ANY INFORMATION IN HERE PLEASE
echo   EDIT write_build_no.bat
echo   FORMAL BUILD NUMBER:%BuildNum%
echo */
echo #define PRODUCT_VER_STRING "%PRODUCT_VER_STRING%"
echo #define FILE_VER_STRING "%FILE_VER_STRING%"
echo #define FILE_VER_NUMBER %FILE_VER_NUMBER%

