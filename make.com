$!	
$! DESCRIPTION: Dinotrace: VMS build script
$!	
$! This com file will compule and link together the obj's necessary to
$! create a Dinotrace executable. This should will work on systems running
$! VMS version 5.1 or better.
$!
$! Simply execute this com file by:
$!
$!	@make
$!
$! This will create an executable file called [.src]dinotrace_v?.exe that
$! will run on the version of VMS that you linked with.
$!
$!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
$!
$ if f$sarch("config.h") .eqs. "" then
$    write sys$output "If you get a error on the next line,"
$    write sys$output "find a file like [.src]config and rename it [.src]config.h"
$    copy config_h.in config.h
$    write sys$output "Never mind."
$ endif
$!
$ @[.src]make.com p1 p2 p3 p4 p5 p6 p7 p8
$!
$ rename [.src]dinotrace*.exe []
$!
