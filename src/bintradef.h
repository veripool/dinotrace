#ifdef VMS
/* $Id$ */
/******************************************************************************
 * DESCRIPTION: Dinotrace source: DECSIM binary trace format reading
 *
 * This file is part of Dinotrace.
 *
 * Author: Wilson Snyder <wsnyder@wsnyder.org>
 *
 * Code available from: http://www.veripool.com/dinotrace
 *
 ******************************************************************************
 *
 * Some of the code in this file was originally developed for Digital
 * Semiconductor, a division of Digital Equipment Corporation.  They
 * gratefuly have agreed to share it, and thus the base version has been
 * released to the public with the following provisions:
 *
 *
 * This software is provided 'AS IS'.
 *
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THE INFORMATION
 * (INCLUDING ANY SOFTWARE) PROVIDED, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR ANY PARTICULAR PURPOSE, AND
 * NON-INFRINGEMENT. DIGITAL NEITHER WARRANTS NOR REPRESENTS THAT THE USE
 * OF ANY SOURCE, OR ANY DERIVATIVE WORK THEREOF, WILL BE UNINTERRUPTED OR
 * ERROR FREE.  In no event shall DIGITAL be liable for any damages
 * whatsoever, and in particular DIGITAL shall not be liable for special,
 * indirect, consequential, or incidental damages, or damages for lost
 * profits, loss of revenue, or loss of use, arising out of or related to
 * any use of this software or the information contained in it, whether
 * such damages arise in contract, tort, negligence, under statute, in
 * equity, at law or otherwise. This Software is made available solely for
 * use by end users for information and non-commercial or personal use
 * only.  Any reproduction for sale of this Software is expressly
 * prohibited. Any rights not expressly granted herein are reserved.
 *
 ******************************************************************************
 *
 * Changes made over the basic version are covered by the GNU public licence.
 *
 * Dinotrace is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * Dinotrace is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Dinotrace; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 *****************************************************************************/
/*
	Created 22-FEB-1988 13:36:39 by VAX-11 SDL V3.0-2
	Source:  1-MAY-1986 17:22:48 DSU$:[GIRAMMA.PUBLIC]BINTRADEF.SDL;1

	** MODULE $bintradef IDENT Version 1.0 **
++
 FACILITY: DECSIM/command interpreter, simulator runtime

 ABSTRACT:

	This module contains the constant and structure definitions required
	to build a binary trace record and file.

 ENVIRONMENT: The module will be included into those modules and routines
		accessing a binary trace at compile time

 AUTHOR:	Eric Hildum, CREATION DATE: August 16, 1985

 MODIFIED BY:
	15-June-1988	J.A.Lomicka
	Cosmetic changes to comments to make the damn thing readable.

 VERSION

	1.0 16-AUG-85, deh, creation of module

 SUGGESTED IMPROVEMENTS:

	None

--

	The following constants are the version number of the binary trace
	record and file format described by this file
*/
#define tra$k_majver 2
#define tra$k_minver 0
#define tra$k_revision 0
/*
	The following are the DECSIM data types and fault types
*/
#define tra$k_twosta 1
#define tra$k_fousta 2

#define tra$k_unfaulted 1
#define tra$k_stuone 2
#define tra$k_stuzer 3
#define tra$k_inpstuone 4
#define tra$k_inpstuzer 5
#define tra$k_outstuone 6
#define tra$k_outstuzer 7
/*
	The following define the time units used for user_units and sim_units
*/
#define tra$k_sec 1
#define tra$k_msec 2
#define tra$k_usec 3
#define tra$k_nsec 4
#define tra$k_psec 5
#define tra$k_fsec 6
/*
	The following constants are useful sizes for building and reading
	the binary trace records.
*/
#define tra$k_rmsrecsiz 32763	/* the maximum size in bytes of a binary trace record */
#define tra$k_maxbit 262104	/* the maximum number of bits in a binary trace record */

#define tra$s_verbitsiz 24	/* number of bits required for version number */
#define tra$s_dattimsiz 64
/*
	number of bits required for date and time
*/
#define tra$s_resbitsiz 152
/*
	number of bits required for reserved field
*/
#define tra$s_nrmtimsiz 64		/* number of bits required for current state time, normal */
#define tra$s_vectimsiz 64		/* number of bits required for current state time, vector */
#define tra$s_nodnumsiz 32		/* number of bits required for node number */
#define tra$s_recnumsiz 16		/* number of bits required for record number */
#define tra$s_paccousiz 16		/* number of bits required for data packet count */
#define tra$s_stabitsiz 32		/* number of bits required for status longword */
/*
	The following constants indicate the presence of the different
	record classes and types
*/
#define tra$k_mhr 1
#define tra$k_sir 2
#define tra$k_dr 3
#define tra$k_mtr 4

#define tra$k_mmh 1
#define tra$k_mdr 2
#define tra$k_mcr 3
#define tra$k_mgn 4
#define tra$k_sfn 5

#define tra$k_nns 1
#define tra$k_nss 2
#define tra$k_scr 3

#define tra$k_dcr 1
#define tra$k_nfd 2
#define tra$k_nnr 3
#define tra$k_nsr 4
#define tra$k_ntr 5
#define tra$k_snr 6

#define tra$k_ems 1
#define tra$k_emc 2

#define tra$k_timsta 1
/*
	The following structures are the data sections of the various
	binary trace records.  These sections are combined into one large
	aggregate structure for convenience.  See the binary trace file
	specification for more information on the semantics of the various
	fields declared here.
*/
#define tra$s_mmh_rec_siz 184
#define tra$s_mdr_rec_siz 304
#define tra$s_dcr_rec_siz 32
#define tra$s_nfd_rec_siz 152
#define tra$s_nnr_rec_siz 32
#define tra$s_ems_rec_siz 48
#ifndef __osf__
#pragma nomember_alignment
#endif
struct bintrarec {
    unsigned char tra$b_class;
    unsigned char tra$b_type;
    union  {
/*
	MODULE HEADER RECORDS
*/
	struct  {
	    unsigned char tra$b_majver;
	    unsigned char tra$b_minver;
	    unsigned char tra$b_revision;
	    char tra$r_cretim [8];
	    char tra$r_modtim [8];
	    union  {
		struct {short string_length; char string_text[32740];}
		    tra$t_modnam;
		struct  {
		    unsigned short int tra$w_modnamlen;
		    char tra$t_modnamstr [32740];
		    } tra$r_fill_1;
		} tra$r_fill_0;
	    } tra$$r_mmh_data;
/*
	Main Module Header Record Format

	All the names listed in the table have the prefix tra$.
byte:		3	     2		  1		0
	+-------------+-------------+-------------+-------------+
	! b_minver    ! b_majver    ! b_type	  ! b_class	!
	+-------------+-------------+-------------+-------------+
						  ! b_revision  !
	+-------------+-------------+-------------+-------------+
	!			r_cretim			!
	+-------------+-------------+-------------+-------------+
	!		  r_cretim (continued)			!
	+-------------+-------------+-------------+-------------+
	!			r_modtim			!
	+-------------+-------------+-------------+-------------+
	!		  r_modtim (continued)			!
	+-------------+-------------+-------------+-------------+
	:	      ! t_modnamstr ! w_modnamlen or t_modnam	!
	:	      +-------------+-------------+-------------+
	:							:
	:							:
	+-------------+-------------+-------------+-------------+

	tra$b_class	- the record class
	tra$b_type	- the record type
	tra$b_majver	- the module major version number
	tra$b_minver	- the module minor version number
	tra$b_revision	- the module revision number
	tra$r_cretim	- the creation time
	tra$r_modtim	- the last modification time
	tra$w_modnamlen	- the module name length
	tra$t_modnamstr	- the module name string
	tra$t_modnam	- the module name in varying string format
*/
	struct  {
	    unsigned char tra$b_subtype;
	    union  {
		unsigned char tra$b_tratyp;
		struct  {
		    unsigned tra$v_vector : 1;
		    unsigned tra$v_normal : 1;
		    unsigned tra$v_delayed : 1;
		    unsigned tra$v_immediate : 1;
		    unsigned tra$v_forced : 1;
		    unsigned tra$v_fill_4 : 3;
		    } tra$r_fill_3;
		} tra$r_fill_2;
	    union  {
		unsigned int tra$q_base_time [2];
		struct  {
		    unsigned long int tra$l_base_time_lo;
		    unsigned long int tra$l_base_time_hi;
		    } tra$r_fill_6;
		} tra$r_fill_5;
	    unsigned char tra$b_sim_units;
	    unsigned char tra$b_user_units;
	    unsigned short int tra$w_multiplier;
	    unsigned char tra$r_reserved [19];
	    unsigned char tra$b_fautyp;
	    union  {
		struct {short string_length; char string_text[32725];} tra$t_faunam;
		struct  {
		    unsigned short int tra$w_faunamlen;
		    char tra$t_faunamstr [32725];
		    } tra$r_fill_8;
		} tra$r_fill_7;
	    } tra$$r_mdr_data;
/*
	Module Description Record Record Format

	All the names listed in the table have the prefix tra$.
byte:		3	     2		  1		0
	+-------------+-------------+-------------+-------------+
	! b_tratyp    ! b_subtype   ! b_type	  ! b_class	!
	+-------------+-------------+-------------+-------------+
	! q_base_time, l_base_time_lo				!
	+-------------+-------------+-------------+-------------+
	! q_base_time (continued), l_base_time_hi		!
	+-------------+-------------+-------------+-------------+
	! w_multiplier		    | b_user_units| b_sim_units	!
	+-------------+-------------+-------------+-------------+
	! r_reserved						!
	+-------------+-------------+-------------+-------------+
	! r_reserved (continued)				!
	+-------------+-------------+-------------+-------------+
	! r_reserved (continued)				!
	+-------------+-------------+-------------+-------------+
	! r_reserved (continued)				!
	+-------------+-------------+-------------+-------------+
	! b_fautyp    ! r_reserved (continued)			!
	+-------------+-------------+-------------+-------------+
	: t_faunamstr		    ! w_faunamlen or t_faunam	!
	:			    +-------------+-------------+
	: 							:
	:							:
	+-------------+-------------+-------------+-------------+

	tra$b_class	   - the record class
	tra$b_type	   - the record type
	tra$b_subtype	   - the module description record subtype
	tra$b_tratyp	   - bit flags for type of trace:
			     tra$_normal, tra$_vector,
			     tra$_delayed, tra$_immediate,
			     tra$_forced
	tra$q_base_time	   - base time used in time reports
	tra$l_base_time_lo - low order longword of base time
	tra$l_base_time_hi - high order longword of base time
	tra$b_sim_units	   - simulation units (NS, PS, etc., used for time)
	tra$b_user_units   - units user requested for time reports
	tra$w_multiplier   - multiplier for sim_units (as 10 if units were 10ns)
	tra$r_reserved	   - reserved bytes (must be zero)
	tra$b_fautyp	   - the type of fault
	tra$w_faunamlen	   - the length of the fault name
	tra$t_faunamstr	   - the fault name string
	tra$t_faunam	   - the fault name in varying string format

	SECTION IDENTIFIER RECORDS

	Note that only the records which have data items other than the class and type
	are defined (ie, we do not want to have null data structures).

	Node Name Section Record Format

	All the names listed in the table have the prefix tra$.
byte:		1		0
	+-------------+-------------+
	! b_type      ! b_class	    !
	+-------------+-------------+

	tra$b_class	- the record class
	tra$b_type	- the record type

	Node State Section Record Format

	All the names listed in the table have the prefix tra$.
byte:		1		0
	+-------------+-------------+
	! b_type      ! b_class	    !
	+-------------+-------------+

	tra$b_class	- the record class
	tra$b_type	- the record type

	DATA RECORDS
*/
	struct  {
	    union  {
		struct {short string_length; char string_text[32759];} tra$t_datcom;
		struct  {
		    unsigned short int tra$w_datcomlen;
		    char tra$t_datcomstr [32759];
		    } tra$r_fill_10;
		} tra$r_fill_9;
	    } tra$$r_dcr_data;
/*
	Data Comment Record Record Format

	All the names listed in the table have the prefix tra$.
byte:		3	     2		  1		0
	+-------------+-------------+-------------+-------------+
	! w_datcomlen or t_datcom   ! b_type	  ! b_class	!
	+-------------+-------------+-------------+-------------+
	: t_datcomstr 						:
	:							:
	+-------------+-------------+-------------+-------------+

	tra$b_class	- the record class
	tra$b_type	- the record type
	tra$w_datcomlen	- the length of the comment
	tra$t_datcomstr	- the data comment
	tra$t_datcom	- teh data comment in varying string format
*/
	struct  {
	    unsigned char tra$b_dattyp;
	    unsigned long int tra$l_nodnum;
	    unsigned long int tra$l_nodoff;
	    unsigned short int tra$w_recnum_a;
	    unsigned short int tra$w_bitlen;
	    unsigned long int tra$l_bitpos;
	    } tra$$r_nfd_data;
/*
	Node Format Data Record Format

	All the names listed in the table have the prefix tra$.
byte:		3	     2		  1		0
	+-------------+-------------+-------------+-------------+
		      ! b_dattyp    ! b_type	  ! b_class	!
	+-------------+-------------+-------------+-------------+
	!			l_nodnum			!
	+-------------+-------------+-------------+-------------+
	!			l_nodoff			!
	+-------------+-------------+-------------+-------------+
	!	  w_bitlen	    !	     w_recnum_a		!
	+-------------+-------------+-------------+-------------+
	!			l_bitpos			!
	+-------------+-------------+-------------+-------------+

	tra$b_class	- the record class
	tra$b_type	- the record type
	tra$b_dattyp	- the type of node state data
	tra$l_nodnum	- the node number
	tra$l_nodoff	- the node Fix Top offset
	tra$w_recnum_a	- the record number in which the data is stored (data set number)
	tra$w_bitlen	- the number of bits in the buffer the data occupies
	tra$l_bitpos	- the position in bits that the data occupies
*/
	struct  {
	    union  {
		struct {short string_length; char string_text[32759];} tra$t_nodnam;
		struct  {
		    unsigned short int tra$w_nodnamlen;
		    char tra$t_nodnamstr [32759];
		    } tra$r_fill_12;
		} tra$r_fill_11;
	    } tra$$r_nnr_data;
/*
	Node Name Record Format

	All the names listed in the table have the prefix tra$.
byte:		3	     2		  1		0
	+-------------+-------------+-------------+-------------+
	! w_nodnamlen or t_nodnam   ! b_type	  ! b_class	!
	+-------------+-------------+-------------+-------------+
	: t_nodnamstr						:
	:							:
	+-------------+-------------+-------------+-------------+

	tra$b_class	- the record class
	tra$b_type	- the record type
	tra$w_nodnamlen	- the length of the node name
	tra$t_nodnamstr	- the node name
	tra$t_nodnam	- the node name in varying string format
*/
	struct  {
	    unsigned short int tra$w_recnum;
	    union  {
		unsigned int tra$q_time [2];
		struct  {
		    unsigned long int tra$l_time_lo;
		    unsigned long int tra$l_time_hi;
		    } tra$r_fill_14;
		} tra$r_fill_13;
	    unsigned char tra$r_stabuf [32751];
	    } tra$$r_nsr_data;
/*
	Node State Record Record Format

	All the names listed in the table have the prefix tra$.
byte:		3	     2		  1		0
	+-------------+-------------+-------------+-------------+
	! w_recnum		    ! b_type	  ! b_class	!
	+-------------+-------------+-------------+-------------+
	! q_time, l_time_lo					!
	+-------------+-------------+-------------+-------------+
	! q_time (continued), l_time_hi				!
	+-------------+-------------+-------------+-------------+
	: r_stabuf						:
	:							:
	+-------------+-------------+-------------+-------------+

	tra$b_class	- the record class
	tra$b_type	- the record type
	tra$w_recnum	- the record number within the data set
	tra$q_time	- the user time at which this data occurs
	tra$l_time_lo	- low order longword of time
	tra$l_time_hi	- high order longword of time
	tra$r_stabuf	- the buffer containing the data
*/
	struct  {
	    unsigned short int tra$w_datpaccou;
	    union  {
		unsigned int tra$q_tratim [2];
		struct  {
		    unsigned long int tra$l_tratim_lo;
		    unsigned long int tra$l_tratim_hi;
		    } tra$r_fill_16;
		} tra$r_fill_15;
	    unsigned char tra$r_trabuf [32751];
	    } tra$$r_ntr_data;
/*
	Node Transition Record Record Format

	All the names listed in the table have the prefix tra$.
byte:		3	     2		  1		0
	+-------------+-------------+-------------+-------------+
	! w_datpaccou		    ! b_type	  ! b_class	!
	+-------------+-------------+-------------+-------------+
	! q_tratim, l_tratim_lo					!
	+-------------+-------------+-------------+-------------+
	! q_tratim (continued), l_tratim_hi			!
	+-------------+-------------+-------------+-------------+
	: r_trabuf						:
	:							:
	+-------------+-------------+-------------+-------------+

	tra$b_class	- the record class
	tra$b_type	- the record type
	tra$w_datpaccou	- the number of data packets in this record
	tra$q_tratim	- the time at which the nodes listed in this record
			  transitioned to their new state
	tra$l_tratim_lo - low order longword of tra$q_tratim
	tra$l_tratim_hi - high order longword of tra$q_tratim
	tra$r_trabuf	- the buffer where the transition data is stored

	MODULE TRAILER RECORDS
*/
	struct  {
	    unsigned long int tra$l_modsta;
	    } tra$$r_ems_data;
/*
	End Module Status Record Format

	All the names listed in the table have the prefix tra$.
byte:		3	     2		  1		0
				    +-------------+-------------+
				    ! b_type	  ! b_class	!
	+-------------+-------------+-------------+-------------+
	!			 l_modsta			!
	+-------------+-------------+-------------+-------------+
	tra$b_class	- the record class
	tra$b_type	- the record type
	tra$l_modsta	_ the module completion status
*/
	} tra$r_data;
    } ;

/*	This constant is determined by the above structure definition */
#define tra$s_heabitsiz 16
/*
	number of bits required for class and type code
*/
#endif /* VMS */
