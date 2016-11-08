;; dinotrace.el --- minor mode for displaying waveform signal values

;; Author          : Wilson Snyder <wsnyder@wsnyder.org>
;; Keywords        : languages
;; version: 9.4d

;;; Commentary:
;;
;; Distributed from the web
;;	http://www.veripool.org
;;
;; This also requires verilog-mode.el from
;; 	http://www.veripool.org
;;
;; To use this package, simply put it in a file called "dinotrace.el" in
;; a Lisp directory known to Emacs (see `load-path').
;;
;; Byte-compile the file (in the dinotrace.el buffer, enter dired with C-x d
;; then press B yes RETURN)
;;
;; Put these lines in your ~/.emacs or site's site-start.el file (excluding
;; the START and END lines):
;;
;;	---INSTALLER-SITE-START---
;;	;; Dinotrace mode
;;	(autoload 'dinotrace-update "dinotrace" "Update dinotrace annotations in this buffer" t)
;;	(autoload 'dinotrace-mode   "dinotrace" "Toggle dinotrace annotations in this buffer" t)
;;	(global-set-key "\C-x\C-aa" 'dinotrace-update)
;;	(global-set-key "\C-x\C-ad" 'dinotrace-mode)
;;	---INSTALLER-SITE-END---
;;
;;
;; USAGE:
;;
;; Initial Usage:
;;   In Dinotrace:
;;	Call up a trace.  (Preferrably a trace which has many signals.)
;;     Place cursors where you what to know the values.
;;     Highlight (or search for) interesting signals.
;;     Press 'a'.
;;
;;   In Emacs:
;;	Visit a Verilog file.
;;	Press 'C-x C-a a'
;;	Annotations appear, with values in the same order and color as
;;     the cursors were in dinotrace.  Highlighted dinotrace signals are
;;	highlighted in the buffer also.  (Implicit wires are not annotated.)
;;     Lines at the top of the file show what the annotations are from.
;;
;; To put new annotations into Emacs:
;;
;;   In Dinotrace:
;;     Hit 'a'
;;
;;   In Emacs:
;;	Hit 'C-x C-a a'.
;;	All visible windows will be annotated.
;;
;; To remove annotations in Emacs:
;;   In Emacs:
;;	Make buffer non read-only with C-x C-q
;;	or C-u 0 M-x dinotrace-mode	(toggles dinotrace mode to be off)
;;
;;
;; HOW IT WORKS:
;;
;; Pressing 'a' (or Annotate from the menu) in Dinotrace creates a file
;; dinotrace.danno in your home directory.  This file contains the values
;; and signal colors.  Pressing C-x C-a a in Emacs causes it to read the
;; file, and match up the signals in the file with the verilog code.  This
;; match up does NOT know about design heiarchy.  It presumes that each
;; signal name is unique across the whole design (it strips
;; p_pcs_psm->FOOBAR_L to FOOBAR_L and will highlight any FOOBAR_L
;; signal.)
;;
;; After adding the annotations, Emacs marks the annotated buffer as
;; read only.  Changing the buffer to non-read-only will remove the
;; annotations.  Also, if a new annotation file is read, old annotations
;; will be removed.
;;
;; COPYING:
;;
;; Dinotrace.el is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 3, or (at your option)
;; any later version.
;;
;; Dinotrace.el is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with Dinotrace; see the file COPYING.  If not, write to
;; the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
;; Boston, MA 02111-1307, USA.

;;; History:
;;

;;; Code:

(require 'verilog-mode)

(defconst dinotrace-mode-version "9.4d"
  "Version of this dinotrace-mode.")

(eval-when-compile
 (require 'verilog-mode)
 (require 'reporter)
 (condition-case nil
     (unless (fboundp 'sim-log-time-regexp)
       (defvar sim-log-time-regexp "\\[\\([0-9 ]+\\)\\]"))
   (error nil))
 )

;;
;; Global Variables, user configurable
;;

(defgroup dinotrace-mode nil
  "Interface to signal display tool"
  :group 'languages)

(defcustom dinotrace-mode-hook nil
  "*Hook (List of functions) run after dinotrace mode is loaded."
  :type 'hook
  :group 'dinotrace-mode)

(defcustom dinotrace-annotate-hook nil
  "*Hook run after annotation is applied."
  :type 'hook
  :group 'dinotrace-mode)

(defcustom dinotrace-unannotate-hook nil
  "*Hook run after annotation is removed."
  :type 'hook
  :group 'dinotrace-mode)

(defcustom dinotrace-annotate-file-name "~/dinotrace.danno"
  "*Dinotrace annotation file name."
  :type 'string
  :group 'dinotrace-mode)

(defconst dinotrace-read-timeout 120
  "*Seconds to wait for new annotation file to be created by host
before raising error.")

(defvar dinotrace-header-face	'dinotrace-header-face
  "Face name to use for top line information.")
(defface dinotrace-header-face
  '((((class grayscale) (background light))
     (:foreground "DimGray" :underline t))
    (((class grayscale) (background dark))
     (:foreground "LightGray" :underline t))
    (((class color) (background light)) (:foreground "ForestGreen" :underline t))
    (((class color) (background dark)) (:foreground "PaleGreen" :underline t))
    (t (:underline t)))
  "Dinotrace face used for top line information"
  :group 'dinotrace-mode)

(defvar dinotrace-foreground-face	'dinotrace-foreground-face
  "Face name to use for cursors, etc that are in default foreground color.")
(defface dinotrace-foreground-face
  '((t (:bold t :underline t)))
  "Default face to use for cursors, etc that are in default foreground color."
  :group 'dinotrace-mode)

(defconst dinotrace-languages
  ;;     mode         annnotate
  ;;		      signals
  ;;		      quick-cursor
  ;;		      quick-signal
  ;;		      quick-value
  '( ( verilog-mode   dinotrace-annotate-verilog-buffer
		      verilog-read-signals
		      dinotrace-nop
		      dinotrace-annotate-verilog-signal
		      dinotrace-annotate-verilog-value)

     ( sim-log-mode   dinotrace-annotate-sim-log-buffer
		      nil
		      dinotrace-annotate-sim-log-cursor
		      dinotrace-annotate-verilog-signal
		      dinotrace-annotate-verilog-value)

     ( c-mode  	      dinotrace-annotate-verilog-buffer
		      dinotrace-c-read-signals
		      dinotrace-nop
		      dinotrace-annotate-verilog-signal
		      dinotrace-annotate-verilog-value)

     ( c++-mode       dinotrace-annotate-verilog-buffer
		      dinotrace-c-read-signals
		      dinotrace-nop
		      dinotrace-annotate-verilog-signal
		      dinotrace-annotate-verilog-value)

     ( systemc-mode   dinotrace-annotate-verilog-buffer
		      dinotrace-c-read-signals
		      dinotrace-nop
		      dinotrace-annotate-verilog-signal
		      dinotrace-annotate-verilog-value)

     )
  "List of ( (mode-name annotate-function quick-cursor) ...) where mode-name is a
annotatable mode, such as verilog, VHDL or C, and annotate-function highlights
that buffer type.")

;;
;; Bindings
;;

(global-set-key "\C-x\C-aa" 'dinotrace-update)

(defvar dinotrace-mode-map ()
  "Keymap used in Dinotrace minor mode.")
(if dinotrace-mode-map
    ()
  (setq dinotrace-mode-map (make-sparse-keymap))
  (define-key dinotrace-mode-map "\C-x\C-ad" 'dinotrace-mode)
  (define-key dinotrace-mode-map "\C-x\C-aa" 'dinotrace-update)
  (define-key dinotrace-mode-map "\C-x\C-a\C-r" 'dinotrace-reread)
  (define-key dinotrace-mode-map "\C-x\C-ac" 'dinotrace-goto-and-cursor-time)
  (define-key dinotrace-mode-map "\C-x\C-aC" 'dinotrace-goto-and-cursor-time-next-color)
  (define-key dinotrace-mode-map "\C-x\C-as" 'dinotrace-goto-and-highlight-signal)
  (define-key dinotrace-mode-map "\C-x\C-aS" 'dinotrace-goto-and-highlight-signal-next-color)
  (define-key dinotrace-mode-map "\C-x\C-al" 'dinotrace-goto-and-list-signal)
  (define-key dinotrace-mode-map "\C-x\C-aL" 'dinotrace-goto-and-list-signal-next-color)
  (define-key dinotrace-mode-map "\C-x\C-av" 'dinotrace-highlight-value)
  (define-key dinotrace-mode-map "\C-x\C-a<" 'dinotrace-cursor-step-backward)
  (define-key dinotrace-mode-map "\C-x\C-a>" 'dinotrace-cursor-step-forward)
  (define-key dinotrace-mode-map "\C-x\C-a\C-h" 'dinotrace-hierarchy-set)
  (substitute-key-definition `vc-toggle-read-only 'dinotrace-vc-toggle-read-only
			     dinotrace-mode-map global-map)
  (substitute-key-definition `toggle-read-only 'dinotrace-toggle-read-only
			     dinotrace-mode-map global-map)
  (substitute-key-definition `hilit-recenter 'recenter ;; Slow and useless
			     dinotrace-mode-map global-map))

;;
;; Menus
;;

;; If a mode map isn't defvared then create it
(if (not (boundp 'c++-mode-map))
    (defvar c++-mode-map (make-sparse-keymap "C++") "Temp menu for dinotrace."))
(if (not (boundp 'c-mode-map))
    (defvar c-mode-map (make-sparse-keymap "C") "Temp menu for dinotrace."))

(defvar dinotrace-xemacs-menu
  '("Dinotrace"
    ["Annotate"				dinotrace-update t]
    ["Unannotate"			dinotrace-unannotate-all t]
    ["TIME Goto & Cursor"		dinotrace-goto-and-cursor-time t]
    ["     Next Color"			dinotrace-goto-and-cursor-time-next-color t]
    ["SIGNAL Goto & Highlight"		dinotrace-goto-and-highlight-signal t]
    ["     Next Color"			dinotrace-goto-and-highlight-signal-next-color t]
    ["SIGNAL Add to List"		dinotrace-goto-and-list-signal t]
    ["     Next Color"			dinotrace-goto-and-list-signal-next-color t]
    ["VALUE Highlight"			dinotrace-highlight-value t]
    ["     Next Color"			dinotrace-highlight-value-next-color t]
    ["CURSOR Step Forward"		dinotrace-cursor-step-forward t]
    ["     Step Backward"		dinotrace-cursor-step-backward t]
    ["Set Hierarchy"			dinotrace-hierarchy-set t]
    )
  "Emacs menu for dinotrace-mode.")
(or (string-match "XEmacs" emacs-version)
    (easy-menu-define dinotrace-menu dinotrace-mode-map "Menu for Dinotrace mode"
		      dinotrace-xemacs-menu))
;;(list verilog-mode-map sim-log-mode-map c-mode-map c++-mode-map))

;;
;; Internal Global Variables
;;

(defconst dinotrace-help-address "wsnyder@wsnyder.org"
  "Address accepting submissions of bug reports and questions.")

;;
;; Global one-of
(defvar dinotrace-annotate-time nil
  "Dinotrace annotation file time was last read in.")

(defvar dinotrace-socket nil
  "Dinotrace process socket connected via TCP.")

(defvar dinotrace-highlight-color 1
  "Dinotrace color to be used for highlighting.")

;;
;; Per buffer
(defvar dinotrace-mode nil
  "Non-nil if the buffer is dinotrace annotated.")

(defvar dinotrace-buffer-cache-signals nil
  "List of all signals in this buffer.")
(defvar dinotrace-buffer-cache-time nil
  "Time at which list of all signals was last correct.")

(defvar dinotrace-buffer-read-only nil
  "T if the buffer was read-only prior to annotation.")

(defvar dinotrace-buffer-header-lines nil
  "Lines added to head of file.")

(defvar dinotrace-buffer-string nil
  "Old contents of buffer prior to annotation.")

(defvar dinotrace-buffer-annotate-time nil
  "Which annotation time applies to this buffer.")

(defvar dinotrace-buffer-hierarchy-regexp nil
  "Hierarchy that must be part of signal to include this value, or nil for all signals.")

;;;;
;;;; Dinotrace program variables, from dinotrace.danno
;; These variables are created by the emacs annotation file

(defvar dinotrace-program-version nil
  "Name and version number of dinotrace (program not Emacs code).
Created in the annotation file by dinotrace.")

(defvar dinotrace-socket-name nil
  "Name of the machine and port number for communications.
Created in the annotation file by dinotrace.")

(defvar dinotrace-hierarchy-separator nil
  "Character separating hierarchy from basename.
Created in the annotation file by dinotrace.")

(defvar dinotrace-traces nil
  "List of ([trace-name trace-mod-date] ...)
Created in the annotation file by dinotrace.")

(defvar dinotrace-cursors nil
  "Array of [[cursor-time color-number color-name face note] ...].
Created in the annotation file by dinotrace.")

(defvar dinotrace-signal-colors nil
  "List of ([color value face] ...]  ...)
Created in the annotation file by dinotrace.")

(defvar dinotrace-signal-values nil
  "List of ((basename [signal-name sig-color note ([color value] ...)])  ...)
Created in the annotation file by dinotrace.")

(defvar dinotrace-value-searches nil
  "List of ([value search]  ...)
Created in the annotation file by dinotrace.")

;;
;; Array element Macros
;;

(defsubst dinotrace-buffer-annotate-func ()
  (nth 1 (assoc major-mode dinotrace-languages)))
(defsubst dinotrace-buffer-read-signals-func ()
  (nth 2 (assoc major-mode dinotrace-languages)))
(defsubst dinotrace-buffer-quick-cursor-func ()
  (nth 3 (assoc major-mode dinotrace-languages)))
(defsubst dinotrace-buffer-quick-signal-func ()
  (nth 4 (assoc major-mode dinotrace-languages)))
(defsubst dinotrace-buffer-quick-value-func ()
  (nth 5 (assoc major-mode dinotrace-languages)))

(defsubst dinotrace-trace-file-name (trace)
  (aref trace 0))
(defsubst dinotrace-trace-time (trace)
  (aref trace 1))

(defsubst dinotrace-signal-basename (sig)
  (nth 0 sig))
(defsubst dinotrace-signal-wholename (sig)
  (aref (nth 1 sig) 0))
(defsubst dinotrace-signal-color (sig)
  (aref (nth 1 sig) 1))
(defsubst dinotrace-signal-set-color (sig color)
  (aset (nth 1 sig) 1 color))
(defsubst dinotrace-signal-values (sig)
  (aref (nth 1 sig) 3))
(defsubst dinotrace-signal-propertied (sig)
  (aref (nth 1 sig) 4))
(defsubst dinotrace-signal-set-propertied (sig prop)
  (aset (nth 1 sig) 4 prop))

(defsubst dinotrace-value-value (val)
  (aref val 0))
(defsubst dinotrace-value-color (val)
  (aref val 1))

(defsubst dinotrace-cursor-time (cursor-num)
  (aref (aref dinotrace-cursors cursor-num) 0))
(defsubst dinotrace-cursor-color-name (cursor-num)
  (aref (aref dinotrace-cursors cursor-num) 2))
(defsubst dinotrace-cursor-face (cursor-num)
  (aref (aref dinotrace-cursors cursor-num) 3))
(defsubst dinotrace-cursor-set-face (cursor-num face)
  (aset (aref dinotrace-cursors cursor-num) 3 face))
(defsubst dinotrace-cursor-note (cursor-num)
  (aref (aref dinotrace-cursors cursor-num) 4))

(defsubst dinotrace-sigcolor-face (color-num)
  (aref (aref dinotrace-signal-colors color-num) 2))
(defsubst dinotrace-sigcolor-set-face (color-num face)
  (aset (aref dinotrace-signal-colors color-num) 2 face))
(defsubst dinotrace-sigcolor-color-name (color-num)
  (aref (aref dinotrace-signal-colors color-num) 1))

;;
;; Other macros
;;

(eval-when-compile
  (defvar SAVE-read-only nil ""))

(defmacro dinotrace-save-mod-excursion (&rest body)
  "Save excursion, read-only, undo, and mod status.
Set buffer non-read-only, no undo.  Execute BODY.
Restore those things."
  (list 'save-excursion
	(list 'let (list (list 'SAVE-pre-mod (list 'buffer-modified-p))
			 (list 'SAVE-read-only 'buffer-read-only)
			 (list 'after-change-functions 'nil)
			 (list 'SAVE-undoable (list 'not (list 'eq 't 'buffer-undo-list))))
	      (list 'setq 'buffer-read-only nil)
	      (list 'buffer-disable-undo (list 'current-buffer))
	      (append (list 'prog1) body
		      (list (list 'set-buffer-modified-p 'SAVE-pre-mod)
			    (list 'setq 'buffer-read-only 'SAVE-read-only)
			    (list 'if 'SAVE-undoable (list 'buffer-enable-undo)))))))
;;(macroexpand '(dinotrace-save-mod-excursion BODY))
;;==(save-excursion
;;  (let ((SAVE-pre-mod (buffer-modified-p))
;;	(SAVE-read-only buffer-read-only)
;;	(after-change-functions nil)
;;	(SAVE-undoable (not (eq t buffer-undo-list))))
;;    (setq buffer-read-only nil)
;;    (buffer-disable-undo (current-buffer))
;;    (prog1 BODY
;;      (set-buffer-modified-p pre-mod)
;;      (if SAVE-undoable (buffer-enable-undo))))
;;


;;
;; Mode
;;

(defun dinotrace-mode (&optional arg)
  "Toggle Dinotrace Interface mode.

 \\<dinotrace-mode-map>

With arg, turn Dinotrace mode on if and only if arg is positive.

When Dinotrace mode is enabled, the buffer is highlighted and locked to the
Dinotrace display.  The buffer becomes read only.  To make changes you must
exit dinotrace-mode.

To unannotate the buffer, use:
   \\[dinotrace-unannotate-buffer]	dinotrace-unannotate-buffer
   \\[dinotrace-unannotate-all]	dinotrace-unannotate-all
 or, toggle read-only with \\[dinotrace-toggle-read-only]

These bindings are added to the buffer's keymap when you enter this mode:
Mostly, the last letters in these commands match the Dinotrace program keys.

  \\[dinotrace-update]	- Update annotations
  \\[dinotrace-unannotate-buffer]	- Unannotate this buffer
  \\[dinotrace-unannotate-all]	- Unannotate all buffers
  \\[dinotrace-goto-and-cursor-time]	- Goto and add cursor at time
  \\[dinotrace-goto-and-cursor-time-next-color]	- Goto time next color
  \\[dinotrace-goto-and-highlight-signal]	- Goto and highlight signal
  \\[dinotrace-goto-and-highlight-signal-next-color]	- Goto signal next color
  \\[dinotrace-goto-and-list-signal]	- Add signal to bottom of display list
  \\[dinotrace-goto-and-list-signal-next-color]	- Add signal next color
  \\[dinotrace-highlight-value]	- Highlight value
  \\[dinotrace-highlight-value-next-color]	- Highlight value next color"
  (interactive "P")
  (let ((on-p (if arg (> (prefix-numeric-value arg) 0)
		(not dinotrace-mode))))
    ;; Turn on Dinotrace mode?
    (when on-p
      ;; If no file is read, read
      (or dinotrace-annotate-time (dinotrace-read nil))
      ;; Fontify the buffer if we have to.
      (dinotrace-annotate-buffer))
    ;;
    ;; Turn off Dinotrace?
    (unless on-p
      (dinotrace-unannotate-buffer))
    ))

;;
;; Primary Functions
;;

(defun dinotrace-update
  ()
  "Enter dinotrace-mode in this buffer.

If there is a new annotation file, update all annotations in all visible
widows, and remove outdated annotations from invisible buffers.

See \\[dinotrace-mode] for more information on this mode."
  (interactive)
  ;; Read new file, if we need to
  (dinotrace-read nil)
  ;; Make sure we are annotated
  (dinotrace-annotate-buffer)
  ;; Update anyone else that is active
  (dinotrace-reannotate-all-windows)
  ;; Clear any other buffers now out of date
  (dinotrace-unannotate-all t)
  ;;
  (message "")
  )

(defun dinotrace-update-new-anno ()
  "Perform a update due to sending a new annotation command to Emacs."
  (message "Telling Dinotrace to reannotate...")
  (dinotrace-read-till-timeout)
  (dinotrace-update))

(defun dinotrace-toggle-read-only (&optional arg vc)
  "Change whether this buffer is visiting its file read-only.
With arg, set read-only iff arg is positive.

If dinotrace annotated, then remove annotation also."
  (interactive "P")
  (cond (dinotrace-mode
	 (dinotrace-unannotate-buffer)
	 (cond (buffer-read-only (toggle-read-only 0))))
	;;
	(vc (vc-toggle-read-only))
	;;
	(t (toggle-read-only arg))))

(defun dinotrace-vc-toggle-read-only (&optional arg)
  "Change read-only status of current buffer, perhaps via version control.
If the buffer is visiting a file registered with version control,
then check the file in or out.  Otherwise, just change the read-only flag
of the buffer.  With prefix argument, ask for version number.

If dinotrace annotated, then remove annotation also."
  (interactive "P")
  (dinotrace-toggle-read-only arg t))

;;
;; Unannotation
;;

;; Remove annotations from every buffer
(defun dinotrace-unannotate-all (&optional out-of-date-only)
  "Remove annotations from all buffers."
  (interactive)
  (let ((sel-buf (current-buffer))
	(sel-win (selected-window)))
    (mapcar (function (lambda (buffer)
			(set-buffer buffer)
			(unless (and out-of-date-only
				     (equal dinotrace-buffer-annotate-time dinotrace-annotate-time))
			  (dinotrace-unannotate-buffer buffer))))
	    (buffer-list))
    (set-buffer sel-buf)
    (select-window sel-win)))
;; Alias for verilog-mode 3.10 back compatibility
(defun dinotrace-unannotate-all-buffers ()
  (dinotrace-unannotate-all))

(defun dinotrace-unannotate-buffer (&optional buffer)
  "Remove `value' comments for current buffer.
This is just a revert-buffer which keeps the window and point in a
similar place."
  (interactive)
  (if buffer (set-buffer buffer))
  (when (and dinotrace-mode (buffer-file-name))
    ;; check we have latest version, if user reverts, dinotrace-mode will get cleared
    (find-file-noselect (buffer-file-name)))
  (when (and dinotrace-mode (buffer-file-name))
    (message "Unannotating %s" (buffer-name))
    (let ((win-info nil)
	  (sel-win (selected-window))
	  (sel-buf (current-buffer)))
      ;;
      (walk-windows
       (function
	(lambda (win)
	  (when (eq (window-buffer win) sel-buf)
	    (select-window win)
	    (setq win-info (cons
			    (list
			     win
			     ;; line number
			     (max 0 (- (+ (count-lines 1 (window-point win))
					  (if (= (current-column) 0) 1 0))
				       (or dinotrace-buffer-header-lines 0)))
			     ;; column number
			     (current-column)
			     ;; position in window
			     (+ (count-lines (window-start win) (window-point win))
				(if (= (current-column) 0) 0 -1)))
			    win-info)))))
       nil t)
      ;;
      (select-window sel-win)
      (set-buffer sel-buf)
      ;;(revert-buffer t t)
      (dinotrace-save-mod-excursion
       (erase-buffer)
       (insert dinotrace-buffer-string)
       ;;
       (setq dinotrace-mode nil)
       ;;
       (setq SAVE-read-only dinotrace-buffer-read-only)	;; save-mod will read
       )
      ;; for this to work, must not be in a save-excursion
      (mapcar (function (lambda (win-stuff)
			  (select-window (car win-stuff))
			  ;; line number
			  (goto-line (nth 1 win-stuff))
			  ;; column number
			  (move-to-column (nth 2 win-stuff))
			  ;; position in window
			  (recenter (nth 3 win-stuff))))
	      win-info)
      ;;
      ;; global stuff
      (select-window sel-win)
      (run-hooks 'dinotrace-unannotate-hook)
      ;;FIX(sit-for 0)  ;; Dinotrace menu won't disappear without
      (force-mode-line-update)
      )))

;;
;; Annotation
;;

(defun dinotrace-reannotate-all-windows ()
  "Check annotations in all buffers, update those that need it."
  (interactive)
  (let ((sel-buf (current-buffer))
	(sel-win (selected-window)))
    (walk-windows (function (lambda (win)
			      (select-window win)
			      (dinotrace-annotate-buffer (window-buffer win))))
		  nil t)
    (set-buffer sel-buf)
    (select-window sel-win)))

(defun dinotrace-annotate-buffer (&optional buffer)
  "Add `value' comments to BUFFER."
  ;; Do we need to do anything at all?
  (if buffer (set-buffer buffer))
  (unless (and dinotrace-mode
	       (equal dinotrace-buffer-annotate-time dinotrace-annotate-time))
    ;; Clear existing annotations
    (when dinotrace-mode
      (dinotrace-unannotate-buffer))

    ;; Hook (not in dinotrace-mode itself since update may enter the mode)
    (run-hooks 'dinotrace-mode-hook)

    ;; Make sure we have our keymap setup (here not above in case user added some keys)
    (when (boundp 'minor-mode-map-alist)
      (if (not (member (cons 'dinotrace-mode dinotrace-mode-map)
		       minor-mode-map-alist))
	  (setq minor-mode-map-alist
		(cons (cons 'dinotrace-mode dinotrace-mode-map)
		      minor-mode-map-alist))))

    (when (and (assoc major-mode dinotrace-languages)
	       (not dinotrace-mode)
	       (or (cond ((not (buffer-modified-p)) t)
			 ((y-or-n-p (concat "Can't annotate modified buffers, save "
					    (buffer-name) " "))
			  (save-buffer)
			  t)
			 (t nil))))
      ;;
      (find-file-noselect (buffer-file-name))	;; To check we have latest version
      (message "Annotating %s" (buffer-name))
      (set (make-local-variable 'dinotrace-mode) t)
      (if (>= emacs-major-version 21) (jit-lock-mode nil))
      (set (make-local-variable 'fontification-functions) nil) ;; else emacs21 will font-lock what we insert
      (dinotrace-save-mod-excursion
       ;;
       (set (make-local-variable 'dinotrace-buffer-read-only)  SAVE-read-only)  ;; set in save-mod-excursion
       (set (make-local-variable 'dinotrace-buffer-string) (buffer-string))
       (set (make-local-variable 'dinotrace-buffer-annotate-time) dinotrace-annotate-time)
       (make-local-variable 'dinotrace-buffer-hierarchy-regexp)
       (setq SAVE-read-only t)
       ;;
       (let ((case-fold-search nil)
	     (after-change-functions nil))	;; so font-lock doesn't see our inserts
	 (goto-char (point-min))
	 (dinotrace-annotate-add-header)
	 (funcall (dinotrace-buffer-annotate-func))
	 ;;
	 (run-hooks 'dinotrace-annotate-hook)
	 ;;
	 ;;FIX(sit-for 0)  ;; Dinotrace menu won't appear without
	 (force-mode-line-update)
	 (message "Done.")
	 )))))

(defun dinotrace-set-text-properties (start end face &optional object)
  "Call set-text-properties."
  (set-text-properties start end
		       (list `face face
			     `fontified t)
		       object))

(defun dinotrace-insert-faced (strg face)
  "Insert a string with given face."
  (setq strg (copy-sequence strg))
  (dinotrace-set-text-properties 0 (length strg) face strg)
  (insert strg))

(defun dinotrace-annotate-add-header ()
  "Add the header information to the current buffer.  Presumes buffer writable."
  ;;
  (goto-char (point-min))
  (let ((traces dinotrace-traces))
    (while traces
      (dinotrace-insert-faced
       (concat "`Annotation from "
	       (dinotrace-trace-file-name (car traces))
	       " last modified "
	       (dinotrace-trace-time (car traces))
	       "'\n")
       dinotrace-header-face)
      (setq traces (cdr traces))))
  ;;
  (if dinotrace-buffer-hierarchy-regexp
      (dinotrace-insert-faced (concat "`Hierarchy regexp: " dinotrace-buffer-hierarchy-regexp "\n")
			      dinotrace-header-face))
  ;;
  (let ((cursor-num 0))
    (when (> (length dinotrace-cursors) 0)
      (dinotrace-insert-faced "`Cursor times: " dinotrace-header-face)
      (while (< cursor-num (length dinotrace-cursors))
	(let ((tim-str (concat (if (zerop cursor-num) "" ",")
			       (dinotrace-cursor-time cursor-num))))
	  (dinotrace-insert-faced tim-str (dinotrace-cursor-face cursor-num))
	  (setq cursor-num (1+ cursor-num))))
      (dinotrace-insert-faced "'\n" dinotrace-header-face)))
  (set (make-local-variable 'dinotrace-buffer-header-lines)
       (count-lines 1 (point))))

(defsubst dinotrace-annotate-buffer-signal (sig)
  "Internal, annotate a signal inside a buffer."
  (let* ((search (dinotrace-signal-basename sig)))
    (goto-char (point-min))
    (while (re-search-forward search nil t)
      (when (and ;; We start at end, so we can just test it
		 (save-match-data (looking-at "[^a-zA-Z0-9_\\$]"))
		 ;; Check beginning
		 (save-excursion (save-match-data (goto-char (1- (match-beginning 0))) (looking-at "[^a-zA-Z0-9_\\$]"))))
	(goto-char (match-beginning 0))
	(delete-region (point) (match-end 0))
	(insert (dinotrace-signal-propertied sig)) ; point now at end
	))))

(defun dinotrace-annotate-verilog-buffer ()
  "Internal, annotate a `verilog-mode' buffer's signals."
  ;; Already save-excursioned and at beginning
  (let ((signames (dinotrace-read-signals)))
    (while signames
      (let* ((searchpos dinotrace-signal-values)
	     (sigprop (assoc (car signames) searchpos)))
	(while sigprop
	  (cond ((or (not dinotrace-buffer-hierarchy-regexp)
		     (string-match dinotrace-buffer-hierarchy-regexp
				   (dinotrace-signal-wholename sigprop)))
		 (dinotrace-annotate-buffer-signal sigprop)
		 (setq sigprop nil))
		;; Didn't match hierarchy, search for another
		(t
		 (setq searchpos (cdr (memq sigprop searchpos)))
		 (setq sigprop   (assoc (car signames) searchpos))
		 ))))
      (setq signames (cdr signames))))
  ;; Overall
  (dinotrace-annotate-all-values))

(defun dinotrace-read-signals ()
  "Internal, return list of signals in current module."
  (unless (equal dinotrace-buffer-cache-time (visited-file-modtime))
    (set (make-local-variable 'dinotrace-buffer-cache-signals)
	 (and (dinotrace-buffer-read-signals-func)
	      (funcall (dinotrace-buffer-read-signals-func))))
    (set (make-local-variable 'dinotrace-buffer-cache-time)
	 (visited-file-modtime)))
  dinotrace-buffer-cache-signals)

(defun dinotrace-annotate-verilog-signal (signal face)
  "Internal, quick highlight SIGNAL with FACE.
Already save-excursioned"
  (goto-char (point-min))
  (while (search-forward signal nil t)
      (when (and ;; We start at end, so we can just test it
		 (save-match-data (looking-at "[^a-zA-Z0-9_\\$]"))
		 ;; Check beginning
		 (save-excursion (save-match-data (goto-char (1- (match-beginning 0))) (looking-at "[^a-zA-Z0-9_\\$]"))))
	;; signal highlight
	(dinotrace-set-text-properties (match-beginning 0) (match-end 0)
				       face))
      (goto-char (match-end 0))))

(defun dinotrace-annotate-all-values ()
  "Internal, highlight all values in this buffer."
  (let ((vals dinotrace-value-searches))
    (while vals
      (let ((value (dinotrace-value-value (car vals)))
	    (color (dinotrace-value-color (car vals))))
	(dinotrace-annotate-verilog-value value (dinotrace-sigcolor-face color))
	(setq vals (cdr vals))))))

(defun dinotrace-annotate-verilog-value (value face)
  "Internal, highlight a value in verilog OR sim-log-mode buffers.
Already save-excursioned"
  (let ((rexp (concat "[0_]*" value))
	(case-fold-search t))
    (goto-char (point-min))
    (while (re-search-forward rexp nil t)
      (when (and (save-match-data (looking-at "[^0-9a-fux_]"))
		 (save-excursion (save-match-data (goto-char (1- (match-beginning 0))) (looking-at "[^0-9a-fux_]"))))
	;; value highlight
	(dinotrace-set-text-properties (match-beginning 0) (match-end 0)
				       face))
      (goto-char (match-end 0))
      )))

(defun dinotrace-c-read-signals ()
  "Internal, Read the signals in a c file."
  (let (signals word state (state-pos (point-min)))
    (while (search-forward "->" nil t)
      (save-excursion (setq state (parse-partial-sexp state-pos (point) nil nil state)
			    state-pos (point)))
      (when (and (not (nth 3 state)) (not (nth 4 state)))
	(setq word (buffer-substring (point)
				     (save-excursion
				       (while (looking-at "[a-zA-Z0-9@_]")
					 (forward-word 1))
				       (point))))
	(unless (member word signals)
	  (setq signals (cons word signals))))
      (forward-word 1))
    signals
    ))

(defun dinotrace-annotate-sim-log-buffer ()
  "Internal, annotate a sim-log-mode buffer's times.
Already save-excursioned and at beginning"
  (let ((cursor-num 0)
	(last-min (point-min)))
    (while (< cursor-num (length dinotrace-cursors))
      (let* ((ctime (dinotrace-cursor-time cursor-num))
	     min-point max-point)
	(goto-char last-min)
	(dinotrace-annotate-sim-log-cursor
	 ctime (dinotrace-cursor-face cursor-num) (dinotrace-cursor-note cursor-num))
	;;
	;; Next cursor
	(setq cursor-num (1+ cursor-num)))))
  ;; Overall
  (dinotrace-annotate-all-values))

(defun dinotrace-annotate-sim-log-cursor (ctime face &optional note)
  "Internal, add a cursor at TIME with COLOR to a sim-log-mode buffer.
Point must be at proper start position (or point-min)"
  (let ((ctimen (string-to-number ctime))
	max-point)
    (while (and (re-search-forward sim-log-time-regexp nil t)
		(> ctimen (string-to-number (match-string 1)))))
    ;; Skip blanks
    (forward-line 0)
    (setq max-point (point))
    (dinotrace-insert-faced (concat "`Cursor [" ctime "]'"
				    (or (concat " " note "\n") "\n"))
			    face)))

(defvar dinotrace-hierarchy-set-history nil)
(defun dinotrace-hierarchy-set (regexp)
  "When annotating this buffer, all signals from the trace must match
REGEXP in order to be annotated.  This is useful when there are multiple
instantiations of a module:
 	top.foo1.signal	  `1'
        top.foo2.signal	  `2'
If the module \"foo\" is annotated, Emacs does not know to pick up the
instantiation of foo1 or foo2.  It guesses and picks the earlier one in the
trace.  If the REGEXP is \"\\.foo1\\.\", you will get only values for the
foo1 instantiaion, in this case `1'.  Likewise for \"\\.foo2\\.\".  Note
the quoting of the dots, else they will be wildcards and match something
like \"notafoo12\"."
  (interactive (list (read-from-minibuffer
		      (concat "Dinotrace Hierarchy regexp for " (buffer-name) ": ")
		      "" nil nil 'dinotrace-hierarchy-set-history)))
  (if (or (equal regexp "") (equal regexp ".*")) (setq regexp nil))
  (setq dinotrace-buffer-hierarchy-regexp regexp)
  (when dinotrace-mode
    (setq dinotrace-buffer-annotate-time nil)	; so definately do it
    (dinotrace-annotate-buffer)))

;;
;; Quick Annotation - Do all buffers when user hits cursor or signal keys
;;

(defun dinotrace-nop (&rest foo))

;; Quickly add cursor time to all log buffers
(defun dinotrace-quickannotate-cursor (ctime color)
  (let ((face (dinotrace-sigcolor-face color)))
    (mapcar (function (lambda (buffer)
			(save-excursion
			  (set-buffer buffer)
			  (when dinotrace-mode
			    (dinotrace-save-mod-excursion
			     (funcall (dinotrace-buffer-quick-cursor-func)
				      ctime face))))))
	    (buffer-list))))

;; Quickly highlight a signal
(defun dinotrace-quickannotate-signal (signal color)
  (let ((face (dinotrace-sigcolor-face color))
	(sig (assoc signal dinotrace-signal-values)))
    ;; Update highlighting structure
    (unless sig
      (setq sig (list signal (vector
			      signal color nil nil nil))
	    dinotrace-signal-values (cons sig dinotrace-signal-values)))
    (dinotrace-signal-set-color sig color)
    (dinotrace-make-propertied sig)
    ;;
    (mapcar (function (lambda (buffer)
			(save-excursion
			  (set-buffer buffer)
			  (when dinotrace-mode
			    (dinotrace-save-mod-excursion
			     (funcall (dinotrace-buffer-quick-signal-func)
				      signal face))))))
	    (buffer-list))))

;; Quickly highlight a value
(defun dinotrace-quickannotate-value (value color)
  (let ((face (dinotrace-sigcolor-face color)))
    (mapcar (function (lambda (buffer)
			(save-excursion
			  (set-buffer buffer)
			  (when dinotrace-mode
			    (dinotrace-save-mod-excursion
			     (funcall (dinotrace-buffer-quick-value-func)
				      value face))))))
	    (buffer-list))))

;;
;; Reading
;;

(defun dinotrace-read (&optional filename)
  "Read the dinotrace annotation optional FILENAME.
Returns T if the file has changed."
  (setq filename (expand-file-name (or filename dinotrace-annotate-file-name)))
  (cond ((not (file-readable-p filename))
	 (message "Annotation file no longer exists!")
	 nil)
	((equal dinotrace-annotate-time
		(nth 5 (file-attributes filename)))
	 nil)
	(t
	 (message "Reading annotation file %s" filename)
	 (load-file filename)
	 (setq dinotrace-annotate-time
	       (nth 5 (file-attributes filename)))
	 ;;
	 ;; Make a face for each cursor
	 (let ((cursor-num 0))
	   (while (< cursor-num (length dinotrace-cursors))
	     (dinotrace-cursor-set-face
	      cursor-num
	      (or (and (not (equal "" (dinotrace-cursor-color-name cursor-num)))
		       (dinotrace-face-create
			(dinotrace-cursor-color-name cursor-num)
			t))
		  dinotrace-foreground-face))
	     (setq cursor-num (1+ cursor-num))))
	 ;;
	 ;; Make a face for each signal color
	 (let ((color-num 0))
	   (while (< color-num (length dinotrace-signal-colors))
	     (dinotrace-sigcolor-set-face
	      color-num
	      (or (and (not (equal "" (dinotrace-sigcolor-color-name color-num)))
		       (dinotrace-face-create
			(dinotrace-sigcolor-color-name color-num)
			nil))
		  dinotrace-foreground-face))
	     (setq color-num (1+ color-num))))
	 ;;
	 ;; Make value and signal propertied insertions
	 (let ((sigs dinotrace-signal-values))
	   (while sigs
	     (dinotrace-signal-set-propertied
	      (car sigs) (dinotrace-make-propertied (car sigs)))
	     (setq sigs (cdr sigs))))
	 t)))

(defun dinotrace-face-create (color italic)
  (let ((face (intern (concat "dinotrace-" color (if italic "-italic" "")))))
    (cond ((facep face))
	  ((make-face face)
	   (set-face-foreground face color)
	   (set-face-italic-p face italic)
	   (set-face-underline-p face t)))
    face))

(defun dinotrace-read-till-timeout (&optional filename)
  "Look for a new dinotrace file to be ready.  Timeout if nothing in
dinotrace-read-timout time."
  (let ((secs dinotrace-read-timeout))
    (while (and (not (dinotrace-read filename))
		(> secs 0))
      (setq secs (1- secs))
      (sleep-for 1))
    (unless secs
      (error "New annotation file never created; perhaps Dinotrace was killed?"))))

(defun dinotrace-make-propertied (sig)
  "Form text propertied versions of the signal name and value text"
  (let* ((values (dinotrace-signal-values sig))
	 (basename (dinotrace-signal-basename sig))
	 (out basename)
	 (cursor-num 0)
	 oldlen)
    ;; Make propertied version of signal name; may be identical
    (when (dinotrace-signal-color sig)
      (dinotrace-set-text-properties 0 (length out)
				     (dinotrace-sigcolor-face
				      (dinotrace-signal-color sig))
				     out))
    ;; Make propertied version of values
    (while values
      (setq oldlen (length out)
	    out (concat out (car values)))
      (dinotrace-set-text-properties oldlen (length out)
				     (dinotrace-cursor-face cursor-num)
				     out)
      (setq values (cdr values)
	    cursor-num (1+ cursor-num)))
    ;;
    out))
;; Turn off font-lock to test this!
;;(insert (dinotrace-make-propertied (car dinotrace-signal-values)))

;;
;; Command sending utilities
;;

(defun dinotrace-find-time-default (point)
  "Find a time near the cursor.  Works in sim-log-mode."
  (save-excursion
    (goto-char point)
    (forward-line 0)
    (and (re-search-forward sim-log-time-regexp nil t)
	 (match-string 1))))

(defun dinotrace-find-signal-default (point)
  "Find a signal near the cursor.  Works in verilog-mode."
  (save-excursion
    (goto-char point)
    (while (looking-at "[a-zA-Z0-9_]")
      (forward-char 1))
    ;; Can't use syntax of _, because annotation `'s will get matched.
    (when (or (re-search-backward "[a-zA-Z0-9_]"
				  (save-excursion (beginning-of-line) (point))
				  t)
	      (re-search-forward "[a-zA-Z0-9_]+"
				 (save-excursion (end-of-line) (point))
				 t))
      (goto-char (match-end 0))
      (buffer-substring (point)
			(progn (forward-char -1)
			       (while (looking-at "[a-zA-Z0-9_]")
				 (forward-char -1))
			       (1+ (point)))))))

(defun dinotrace-find-value-default (point)
  "Find a value near the cursor.  Works in sim-log-mode."
  (save-excursion
    (goto-char point)
    (while (looking-at "['abcdef0-9]")
      (forward-char 1))
    (when (or (re-search-backward "['abcdef0-9_]"
				  (save-excursion (beginning-of-line) (point))
				  t)
	      (re-search-forward "['abcdef0-9_]+"
				 (save-excursion (end-of-line) (point))
				 t))
      (goto-char (match-end 0))
      (buffer-substring (point)
			(progn (forward-char -1)
			       (while (looking-at "['abcedf0-9_]")
				 (forward-char -1))
			       (1+ (point)))))))

(defun dinotrace-increment-highlight-color ()
  (setq dinotrace-highlight-color (1+ dinotrace-highlight-color))
  (if (> dinotrace-highlight-color 9) (setq dinotrace-highlight-color 1)))

;;
;; Socket Interactive functions
;;

(defun dinotrace-goto-time (point)
  "Goto the time described after POINT.
For example if point is before [1162], it will goto 1162."
  (interactive "d")
  (let ((time (dinotrace-find-time-default point)))
    (if time (dinotrace-send-command
	      (format "time_goto %s\n" time)))))

(defun dinotrace-add-cursor (point &optional color)
  "Add a cursor at the time point is on."
  (interactive "d")
  (if (not color) (setq color dinotrace-highlight-color))
  (let ((time (dinotrace-find-time-default point)))
    (when time
      (dinotrace-send-command
       (format "cursor_add %s %d -user \"Emacs %s:%d\"\n"
	       time color (buffer-name) (count-lines (point-min) point))
       "refresh\nannotate\n")
      (dinotrace-quickannotate-cursor time color))))

(defun dinotrace-goto-signal (point)
  "Goto signal near point."
  (interactive "d")
  (let ((signal (dinotrace-find-signal-default point))
	(hier   (if dinotrace-buffer-hierarchy-regexp
		    (concat "*" dinotrace-buffer-hierarchy-regexp "*")
		  "")))
    (when signal
      (dinotrace-send-command
	(format "signal_goto %s*%s%s[*\n" hier dinotrace-hierarchy-separator signal)
	(format "signal_goto %s*%s%s\n" hier dinotrace-hierarchy-separator signal)
	(format "signal_goto %s%s[*\n" hier signal)
	(format "signal_goto %s%s\n" hier signal)
	"refresh\n")
      )))

(defun dinotrace-move-signal (point)
  "Copy signal near point.  Add to top of display."
  (interactive "d")
  (let ((signal (dinotrace-find-signal-default point))
	(hier   (if dinotrace-buffer-hierarchy-regexp
		    (concat "*" dinotrace-buffer-hierarchy-regexp "*")
		  "")))
    (when signal
      (dinotrace-send-command
	(format "signal_move %s*%s%s[* *\n" hier dinotrace-hierarchy-separator signal)
	(format "signal_move %s*%s%s   *\n" hier dinotrace-hierarchy-separator signal)
	(format "signal_move %s%s[*    *\n" hier signal)
	(format "signal_move %s%s      *\n" hier signal)
	"refresh\n")
      )))

(defun dinotrace-highlight-signal (point &optional color)
  "Highlight signal near POINT with optional COLOR."
  (interactive "d")
  (if (not color) (setq color dinotrace-highlight-color))
  (let ((signal (dinotrace-find-signal-default point))
	(hier   (if dinotrace-buffer-hierarchy-regexp
		    (concat "*" dinotrace-buffer-hierarchy-regexp "*")
		  "")))
    (when signal
      (dinotrace-send-command
	(format "signal_highlight %s*%s%s[* %d\n" hier dinotrace-hierarchy-separator signal color)
	(format "signal_highlight %s*%s%s %d\n" hier dinotrace-hierarchy-separator signal color)
	(format "signal_highlight %s%s[* %d\n" hier signal color)
	(format "signal_highlight %s%s %d\n"   hier signal color)
	"refresh\nannotate\n")
      (dinotrace-quickannotate-signal signal color))))

(defun dinotrace-highlight-value (point &optional color cursor)
  "Highlight value near POINT with optional COLOR.
If prefix-arg, then also put cursors where that value occurs."
  (interactive "d")
  (if current-prefix-arg (setq cursor t))
  (if (not color) (setq color dinotrace-highlight-color))
  (let ((value (dinotrace-find-value-default point)))
    (when value
      (dinotrace-send-command
       (format "value_highlight %s %d %s -value\n"
	       value color (if cursor "-cursor" ""))
       "refresh\nannotate\n")
      (dinotrace-quickannotate-value value color))))

(defun dinotrace-cursor-step-forward ()
  "Step user cursors forward one primary grid (one clock)."
  (interactive)
  (dinotrace-send-command
   "cursor_step_forward\n"
   "refresh\nannotate\n")
  (dinotrace-update-new-anno))

(defun dinotrace-cursor-step-backward ()
  "Step user cursors forward one primary grid (one clock)."
  (interactive)
  (dinotrace-send-command
   "cursor_step_backward\n"
   "refresh\nannotate\n")
  (dinotrace-update-new-anno))

;; Composite Interactives

(defun dinotrace-goto-and-cursor-time (point &optional next-color)
  "Goto time near point and add a cursor"
  (interactive "d")
  (if next-color (dinotrace-increment-highlight-color))
  (dinotrace-add-cursor point)
  (dinotrace-goto-time point))

(defun dinotrace-goto-and-highlight-signal (point &optional next-color)
  "Goto signal near point and highlight"
  (interactive "d")
  (if next-color (dinotrace-increment-highlight-color))
  (dinotrace-highlight-signal point)
  (dinotrace-goto-signal point))

(defun dinotrace-goto-and-list-signal (point &optional next-color)
  "Add signal near point and highlight"
  (interactive "d")
  (if next-color (dinotrace-increment-highlight-color))
  (dinotrace-move-signal point)
  (dinotrace-highlight-signal point)
  (dinotrace-goto-signal point))

(defun dinotrace-goto-and-cursor-time-next-color (point)
  "Goto time near point and add a cursor of next color"
  (interactive "d")
  (dinotrace-goto-and-cursor-time point t))

(defun dinotrace-goto-and-highlight-signal-next-color (point)
  "Goto signal near point and highlight of next color"
  (interactive "d")
  (dinotrace-goto-and-highlight-signal point t))

(defun dinotrace-goto-and-list-signal-next-color (point)
  "Add signal near point and highlight of next color"
  (interactive "d")
  (dinotrace-goto-and-list-signal point t))

(defun dinotrace-highlight-value-next-color (point)
  "Goto signal near point and highlight of next color
If prefix-arg, then also put cursors where that value occurs."
  (interactive "d")
  (dinotrace-increment-highlight-color)
  (dinotrace-highlight-value point))

;;
;; Socket Low Level
;;

(defun dinotrace-send-command (&rest commands)
  "Send a given configuration COMMAND to the dinotrace session."
  (cond ((not dinotrace-socket-name)
	 (error "Dinotrace socket services not available"))
	((string-match "^\\(.*\\) \\([0-9]+\\)$" dinotrace-socket-name)
	 (setq dinotrace-socket
	       (condition-case nil
		   (open-network-stream
		    "dinotrace" nil (match-string 1 dinotrace-socket-name)
		    (string-to-number (match-string 2 dinotrace-socket-name)))
		 (file-error nil)))
	 (cond ((not dinotrace-socket)
		(error "Can't open socket.  Press a in Dinotrace, then C-x C-a a in Emacs"))
	       (dinotrace-socket
		(process-send-string dinotrace-socket
				     (mapconcat 'concat commands ""))
		(delete-process dinotrace-socket))))
	;;
	(t (error "Can't parse socket name"))
	))
;;(dinotrace-send-command "refresh\n")

;;
;; Utilities
;;


;;;###autoload
(defun dinotrace-submit-bug-report ()
  "Submit via mail, a bug report on dinotrace."
  (interactive)
  (and
   (y-or-n-p "Do you want to submit a report on dinotrace? ")
   (require 'reporter)
   (reporter-submit-bug-report
    dinotrace-help-address (concat "dinotrace " dinotrace-mode-version)
    '(dinotrace-program-version
      dinotrace-socket-name
      dinotrace-traces))))

;;
;; Install ourselves
;;

(unless (assq 'dinotrace-mode minor-mode-alist)
  (setq minor-mode-alist
	(cons '(dinotrace-mode " Dinotrace") minor-mode-alist)))

(provide 'dinotrace)

;; dinotrace.el ends here
