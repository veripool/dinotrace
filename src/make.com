$ qual := ""
$ if p1 .eqs. "DEBUG" then qual := "/debug"
$ if p2 .eqs. "DEBUG" then qual := "/debug /noopt=(noinline,nodisjoint)"
$!
$ arch_bin_v == "_VV"
$ if f$getsyi("NODE_HWTYPE") .eqs. "ALPH" then arch_bin_v == "_VA"
$ define objdir/nolog [.OBJ_VV]
$ if f$getsyi("NODE_HWTYPE") .eqs. "ALPH" then define/nolog objdir [.OBJ_VA]
$!
$ write sys$output "**********************************************************************"
$	define /nolog vaxc$include sys$library, sys$disk:[]
$	define /nolog decc$include sys$library, sys$disk:[]
$	define /nolog sys vaxc$include
$ mms/desc=dinotrace.mms
$ exit
$!
$ run compile_date
$! sea *.c "&&&"
$ recomp   DINOTRACE		"""/decc /object=objdir:DINOTRACE	''qual'"""	dinotrace.h callbacks.h
$ recomp   DT_VALUE		"""/decc /object=objdir:DT_VALUE	''qual'"""	dinotrace.h callbacks.h
$ recomp   DT_CONFIG		"""/decc /object=objdir:DT_CONFIG	''qual'"""	dinotrace.h callbacks.h
$ recomp   DT_CURSOR		"""/decc /object=objdir:DT_CURSOR	''qual'"""	dinotrace.h callbacks.h
$ recomp   DT_CUSTOMIZE		"""/decc /object=objdir:DT_CUSTOMIZE	''qual'"""	dinotrace.h callbacks.h
$ recomp   DT_DISPMGR		"""/decc /object=objdir:DT_DISPMGR	''qual'"""	dinotrace.h callbacks.h
$ recomp   DT_DRAW		"""/decc /object=objdir:DT_DRAW		''qual'"""	dinotrace.h callbacks.h
$ recomp   DT_FILE		"""/decc /object=objdir:DT_FILE		''qual'"""	dinotrace.h callbacks.h
$ recomp   DT_GRID		"""/decc /object=objdir:DT_GRID		''qual'"""	dinotrace.h callbacks.h
$ recomp   DT_ICON		"""/decc /object=objdir:DT_ICON		''qual'"""	dinotrace.h callbacks.h
$ recomp   DT_PRINTSCREEN	"""/decc /object=objdir:DT_PRINTSCREEN	''qual'"""	dinotrace.h callbacks.h dinopost.h
$ recomp   DT_SIGNAL		"""/decc /object=objdir:DT_SIGNAL	''qual'"""	dinotrace.h callbacks.h
$ recomp   DT_UTIL		"""/decc /object=objdir:DT_UTIL		''qual'"""	dinotrace.h callbacks.h
$ recomp   DT_WINDOW		"""/decc /object=objdir:DT_WINDOW	''qual'"""	dinotrace.h callbacks.h
$ recomp   DT_BINARY		"""/decc /object=objdir:DT_BINARY	''qual'"""	dinotrace.h callbacks.h bintradef.h
$ recomp   DT_VERILOG		"""/decc /object=objdir:DT_VERILOG	''qual'"""	dinotrace.h callbacks.h
$ purge objdir:*.obj
$ @link_dt 'p1'
$!
$ exit
$! Manual:
$ cc   DINOTRACE	/vaxc /object=objdir:DINOTRACE	
$ cc   DT_VALUE		/vaxc /object=objdir:DT_VALUE	
$ cc   DT_CONFIG	/vaxc /object=objdir:DT_CONFIG	
$ cc   DT_CURSOR	/vaxc /object=objdir:DT_CURSOR	
$ cc   DT_CUSTOMIZE	/vaxc /object=objdir:DT_CUSTOMIZE	
$ cc   DT_DISPMGR	/vaxc /object=objdir:DT_DISPMGR	
$ cc   DT_DRAW		/vaxc /object=objdir:DT_DRAW		
$ cc   DT_FILE		/vaxc /object=objdir:DT_FILE		
$ cc   DT_GRID		/vaxc /object=objdir:DT_GRID		
$ cc   DT_ICON		/vaxc /object=objdir:DT_ICON		
$ cc   DT_PRINTSCREEN	/vaxc /object=objdir:DT_PRINTSCREEN	
$ cc   DT_SIGNAL	/vaxc /object=objdir:DT_SIGNAL	
$ cc   DT_UTIL		/vaxc /object=objdir:DT_UTIL		
$ cc   DT_WINDOW	/vaxc /object=objdir:DT_WINDOW	
$ cc   DT_BINARY	/vaxc /object=objdir:DT_BINARY	
$ cc   DT_VERILOG	/vaxc /object=objdir:DT_VERILOG	
$ purge objdir:*.obj
$ @link_dt 'p1'
$!
