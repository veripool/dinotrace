$! DESCRIPTION: Sub-make for VMS builds
$!	
$! This com file will compile and link together the obj's necessary to
$! create a DINOTRACE executable. This should will work on systems running
$! VMS version 5.1 or better.
$!
$! Simply execute this com file by:
$!
$!	@make
$!
$! This will create an executable file called dinotrace_v?.exe that
$! will run on the version of VMS that you linked with.
$!
$!
$!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
$ qual := """"""
$ if p1 .eqs. "DEBUG" then qual := '/debug'
$ if p2 .eqs. "DEBUG" then qual := """/debug /noopt=(noinline,nodisjoint)"""
$!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
$!
$ write sys$output "Compiling Dinotrace..."
$ write sys$output "**********************************************************************"
$!
$ define /nolog vaxc$include sys$library, sys$disk:[]
$ define /nolog sys vaxc$include
$!
$ recomp   DINOTRACE		 'qual'	dinotrace.h functions.h
$ recomp   DT_VALUE		 'qual'	dinotrace.h functions.h
$ recomp   DT_CONFIG		 'qual'	dinotrace.h functions.h
$ recomp   DT_CURSOR		 'qual'	dinotrace.h functions.h
$ recomp   DT_CUSTOMIZE	 	 'qual'	dinotrace.h functions.h
$ recomp   DT_DISPMGR		 'qual'	dinotrace.h functions.h
$ recomp   DT_DRAW		 'qual'	dinotrace.h functions.h
$ recomp   DT_FILE		 'qual'	dinotrace.h functions.h
$ recomp   DT_GRID		 'qual'	dinotrace.h functions.h
$ recomp   DT_ICON		 'qual'	dinotrace.h functions.h
$ recomp   DT_PRINT		 'qual'	dinotrace.h functions.h dt_post.h
$ recomp   DT_SIGNAL		 'qual'	dinotrace.h functions.h
$ recomp   DT_UTIL		 'qual'	dinotrace.h functions.h
$ recomp   DT_WINDOW		 'qual'	dinotrace.h functions.h
$ recomp   DT_BINARY		 'qual'	dinotrace.h functions.h bintradef.h
$ recomp   DT_ASCII		 'qual'	dinotrace.h functions.h
$ recomp   DT_VERILOG		 'qual'	dinotrace.h functions.h
$ purge *.obj
$!
$!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
$ write sys$output "Linking Dinotrace..."
$!
$ qual := ''
$ if p1 .eqs. "DEBUG" then qual := '/debug'
$ if p1 .eqs. "MAP" then qual := '/debug/map/full'
$!
$ arch_bin_v == "_VV"
$ if f$getsyi("NODE_HWTYPE") .eqs. "ALPH" then arch_bin_v == "_VA"
$ define objdir/nolog [.OBJ_VV]
$ if f$getsyi("NODE_HWTYPE") .eqs. "ALPH" then define/nolog objdir [.OBJ_VA]
$!
$ define lnk$library sys$library:vaxcrtl.olb
$!
$ open/write LNKFILE objdir:link.tmp
$!
$   xmlib = "sys$library:decw$xmlibshr12.exe"
$   if (f$search(xmlib) .eqs "") then xmlib = "sys$library:decw$xmlibshr.exe"
$   if (f$search(xmlib) .nes "") then write lnkfile "''xmlib'/share"
$!
$   xtlib = "sys$library:decw$xtlibshrr5.exe"
$   if (f$search(xtlib) .eqs "") then xtlib = "sys$library:decw$xtlibshr.exe"
$   if (f$search(xtlib) .nes "") then write lnkfile "''xtlib'/share"
$!
$   xlib = "sys$library:decw$xlibshr.exe"
$   if (f$search(xlib) .eqs "") then xlib = "sys$library:decw$xlibshrr5.exe"
$   if (f$search(xlib) .nes "") then write lnkfile "''xlib'/share"
$!
$   if f$getsyi("NODE_HWTYPE") .nes. "ALPH" then write lnkfile "sys$library:vaxcrtl/share"
$   close lnkfile
$!
$!--------------------
$! define sys$output link.log
$ link 'qual'	/exe=dinotrace'arch_bin_v'.exe -
	objdir:dinotrace, objdir:dt_dispmgr, objdir:dt_window, -
	objdir:dt_draw, objdir:dt_file, objdir:dt_verilog, -
	objdir:dt_customize, objdir:dt_cursor, objdir:dt_config, -
	objdir:dt_grid,	objdir:dt_signal, objdir:dt_binary,-
	objdir:dt_util, objdir:dt_icon, objdir:dt_value,-
	objdir:dt_print, objdir:dt_ascii, -
	objdir:link.tmp/options
$! deassign sys$output
$!----
$! delete objdir:link.tmp;*
$!
$ deassign lnk$library
$ write sys$output "---------------"
$!
$ dirname = f$environment("DEFAULT")
$ dt:=="$''dirname'dinotrace''arch_bin_v'"
$ dtt:=="$''dirname'dinotrace''arch_bin_v' -tempest"
$ dtv:=="$''dirname'dinotrace''arch_bin_v' -verilog"
$ dtnd:=="run/nodebug ''dirname'dinotrace''arch_bin_v'"
$ write sys$output "Type DT to run test version"
$!
$ write sys$output "If it works, copy DINOTRACE''arch_bin_v'.EXE to your release area."
$!
