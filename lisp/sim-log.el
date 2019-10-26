;; sim-log.el --- major mode for viewing simulation log files

;; Author          : Wilson Snyder <wsnyder@wsnyder.org>
;; Keywords        : languages
;; version: 9.4e

;;; Commentary:
;;
;; Distributed from the web
;;	https://www.veripool.org/dinotrace
;;
;; To use this package, simply put it in a file called "sim-log.el" in
;; a Lisp directory known to Emacs (see `load-path'), byte-compile it
;; and put the lines (excluding START and END lines):
;;
;;	---INSTALLER-SITE-START---
;;	;; Sim-Log mode
;;	(autoload 'sim-log-mode "sim-log" "Mode for Simulation Log files." t)
;;	(setq auto-mode-alist (append (list '("\\.log$" . sim-log-mode)) auto-mode-alist))
;;	---INSTALLER-SITE-END---
;;
;; in your ~/.emacs file or in the file site-start.el in the site-lisp
;; directory of the Emacs distribution.
;;
;; If you do not wish to bind all .log files to this mode, then make sure the
;; last lines of your log files contain:
;;     ;;; Local Variables: ***
;;     ;;; mode:sim-log ***
;;     ;;; End: ***

;; COPYING:
;;
;; sim-log.el is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 3, or (at your option)
;; any later version.
;;
;; sim-log.el is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with sim-log; see the file COPYING.  If not, write to
;; the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
;; Boston, MA 02111-1307, USA.
;;

;;; History:
;;


;;; Code:
(require 'font-lock)
(provide 'sim-log)

(defconst sim-log-version "9.4e"
  "Version of this sim-log mode.")

;;
;; Global Variables, user configurable
;;

(defgroup sim-log nil
  "Simulation log file viewing"
  :group 'languages)

(defcustom sim-log-mode-hook nil
  "*Hook (List of functions) run after sim-log mode is loaded."
  :type 'hook
  :group 'sim-log)

(defcustom sim-log-warning-regexp "\\(%W\\|Warning:\\)"
  "*Regexp matching warning messages."
  :type 'string
  :group 'sim-log)

(defcustom sim-log-error-regexp "\\(%E\\|Error:\\|Fatal:\\)"
  "*Regexp matching error messages."
  :type 'string
  :group 'sim-log)

(defcustom sim-log-time-regexp "^\\[\\([0-9 .]+\\)\\]"
  "*Regexp for extracting timstamps.  \\1 returns the time."
  :type 'string
  :group 'sim-log)

;;
;; Bindings
;;

(defvar sim-log-mode-map ()
  "Keymap used in Sim-Log mode.")
(if sim-log-mode-map
    ()
  (setq sim-log-mode-map (make-sparse-keymap))
  ;; No keys yet - Amazing isn't it?
  )

;;
;; Menus
;;

;; Not until we have keys!

;;
;; Internal Global Variables
;;

;;
;; Font-Lock
;;

(defun sim-log-font-lock-keywords ()
  "Return the keywords to be used for font-lock."
  (list (list (concat "^.*" sim-log-error-regexp ".*$")
	      0 font-lock-warning-face) ; Redish
	(list (concat "^.*" sim-log-warning-regexp ".*$")
	      0 font-lock-comment-face) ; Orangish
	(list sim-log-time-regexp
	      0 font-lock-constant-face t) ; Blueish override
	))


;;
;; Mode
;;

(defun sim-log-mode ()
  "Major mode for viewing simulation log files.

Turning on Sim-Log mode calls the value of the variable `sim-log-mode-hook'
with no args, if that value is non-nil.

Special commands:\\{sim-log-mode-map}"
  (interactive)
  (kill-all-local-variables)
  (use-local-map sim-log-mode-map)
  (setq major-mode 'sim-log-mode)
  (setq mode-name "Sim-Log")
  ;;
  ;; Local stuff
  (make-local-variable 'sim-log-warning-regexp)
  (make-local-variable 'sim-log-error-regexp)
  (make-local-variable 'sim-log-time-regexp)
  ;;
  ;; Font lock
  (make-local-variable 'font-lock-defaults)
  (setq font-lock-defaults  '((sim-log-font-lock-keywords)
			      nil nil nil beginning-of-line))
  ;;
  (run-hooks 'sim-log-mode-hook))

(provide 'sim-log)

;;; sim-log.el ends here
