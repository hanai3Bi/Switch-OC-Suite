@echo off
if not exist .\prod.keys (
echo prod.key NOT FOUND!
pause
EXIT
)
if not exist .\pcv-backup (
echo pcv-backup NOT FOUND!
pause
EXIT
)
if not exist .\pcv-bspatch (
echo pcv-bspatch NOT FOUND!
pause
EXIT
)
if exist .\pcv-module (
echo You have generated patched pcv module, do you want to delete and regenerate it?
pause
del .\pcv-module
)
rd /S /Q .\temp\
rd /S /Q .\hacpack_backup\
del .\*.nca
mkdir .\temp
mkdir .\temp\pcv_exefs
hactoolnet -k prod.keys -t -nca .\pcv-backup --exefsdir .\temp
nx2elf .\temp\main
bspatch .\temp\main.elf .\temp\main-mod.elf .\pcv-bspatch
elf2nso .\temp\main-mod.elf .\temp\pcv_exefs\main
copy .\temp\main.npdm .\temp\pcv_exefs\
hacpack -k prod.keys -o .\ --type nca --ncatype program --titleid 010000000000001A --exefsdir .\temp\pcv_exefs\
ren *.nca pcv-module
echo Press any key to remove temp files
pause
rd /S /Q .\temp\
rd /S /Q .\hacpack_backup\
exit
