$ ! VMS command file to recompile a .C file which needs recompilation.
$ ! This is a .C files that has no .OBJ file or that is newer
$ ! than the corresponding .OBJ file.  This file is self contained
$ ! and does not require you to do anything before running it.
$
$    file = f$search(f$parse(p1, ".C"), 1)
$    name = f$parse(file,,,"NAME")
$    obj = name + ".OBJ"
$    if f$search(obj) .eqs. "" then goto docmd
$    if ((f$cvtime(f$file(file, "RDT")) .les. f$cvtime(f$file(obj, "RDT"))) .and -
	 (f$cvtime(f$file("dinotrace.h", "RDT")) .les. f$cvtime(f$file(obj, "RDT"))) .and -
	 (f$cvtime(f$file("callbacks.h", "RDT")) .les. f$cvtime(f$file(obj, "RDT")))) -
	 then exit
$docmd:
$    write sys$output "Compiling ''name'..."
$    cc 'p2' 'file'
$    purge /nolog 'obj'
$    write sys$output "---------------"
