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
$ @[.src]make.com p1 p2 p3 p4 p5 p6 p7 p8
$!
