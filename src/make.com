$ qual := """"""
$ if p1 .eqs. "DEBUG" then qual := '/debug'
$ recomp   DINOTRACE		 'qual'	dinotrace.h callbacks.h
$ recomp   DT_VALUE		 'qual'	dinotrace.h callbacks.h
$ recomp   DT_CONFIG		 'qual'	dinotrace.h callbacks.h
$ recomp   DT_CURSOR		 'qual'	dinotrace.h callbacks.h
$ recomp   DT_CUSTOMIZE	 	 'qual'	dinotrace.h callbacks.h
$ recomp   DT_DISPMGR		 'qual'	dinotrace.h callbacks.h
$ recomp   DT_DRAW		 'qual'	dinotrace.h callbacks.h
$ recomp   DT_FILE		 'qual'	dinotrace.h callbacks.h
$ recomp   DT_GRID		 'qual'	dinotrace.h callbacks.h
$ recomp   DT_ICON		 'qual'	dinotrace.h callbacks.h
$ recomp   DT_PRINTSCREEN	 'qual'	dinotrace.h callbacks.h dinopost.h
$ recomp   DT_SIGNAL		 'qual'	dinotrace.h callbacks.h
$ recomp   DT_UTIL		 'qual'	dinotrace.h callbacks.h
$ recomp   DT_WINDOW		 'qual'	dinotrace.h callbacks.h
$ recomp   DT_BINARY		 'qual'	dinotrace.h callbacks.h bintradef.h
$ purge *.obj
$ @link_dt 'p1'
