;; installer.el --- Install .el files into site-lisp directory
;;
;; Simple routines to put lisp files where they make sense
;; then to byte compile them
;;
;; Author          : Wilson Snyder <wsnyder@wsnyder.org>
;; version: 9.4a         

;;; Commentary:
;;
;; Used by sub programs.  Invoke via Emacs batch
;;	emacs -batch -q -load {package}.el -f {package}-install
;;
;; USE BY PACKAGES:
;;
;; In a package, make a install function, similar to:
;; (defun {package}-install ()
;;  "Install {package}.  Only required for initial installation from distribution."
;;  (if (file-exists-p "{package}.el")
;;      (load (expand-file-name "{package}.el"))
;;    (require `installer))
;;  (installer-add-file "{package}.el"))
;;
;; In the header, add any needed site-start commands, including comments
;; about how to manually add them:
;;
;;	---INSTALLER-SITE-START---
;;	;; {Package} mode
;;	(autoload '{package}-mode   "{package}" "Enter {package} ...." t)
;;	(global-set-key "\C-x\C-somekey" '{package}-update)
;;	---INSTALLER-SITE-END---
;;
;;
;;
;; COPYING:
;;
;; installer.el is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 3, or (at your option)
;; any later version.
;;
;; installer.el is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;; 
;; You should have received a copy of the GNU General Public License
;; along with installer; see the file COPYING.  If not, write to
;; the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
;; Boston, MA 02111-1307, USA.
;;

;;; History:
;; 


;;; Code:

(defun installer-best-dir (filename)
  "Pick the best site-lisp directory for installing FILENAME into."
  (let ((path load-path)
	(best (car load-path)))
    (while path
      (if (and (not best)
	       (string-match "site-lisp" (car path))) 
	  (setq best (car path)))
      (setq path (cdr path)))
    (expand-file-name filename best)))

(defun installer-current-dir (filename)
  "Find the current path for given FILENAME, or nil."
  (let ((path load-path)
	(best (car load-path)))
    (while (and path
		(not (file-exists-p (expand-file-name filename (car path)))))
      (setq path (cdr path)))
    (if path (expand-file-name filename (car path)))))


(defun installer-edit-site (text)
  "Edit the site configuration file to add the needed TEXT."
  (let ((filename (or (installer-current-dir (concat site-run-file ".el"))
		      (installer-best-dir (concat site-run-file ".el")))))
    (find-file filename)
    (goto-char (point-min))
    (cond ((search-forward text nil t)
	   ;; Found, ignore
	   )
	  (t
	   (message "Site-run file %s edited for new mode" filename)
	   (goto-char (point-max))
	   (insert "\n;; Added by installer.el on " (current-time-string) "\n")
	   (insert text)
	   (save-buffer)
	   ))))

(defun installer-comment-process ()
  "Process a file, search for insert text automatically."
  (goto-char (point-min))
  (while (re-search-forward "^\\(.*\\)---INSTALLER-SITE-START---" nil t)
    (let* ((prefix (match-string 1))
	   (strg "")
	   (endpt (save-excursion
		    (or (re-search-forward (concat prefix "---INSTALLER-SITE-END---") nil t)
			(error "Can't find matching installer-site-end"))
		    (forward-line 0)
		    (point))))
      (forward-line 1)
      (while (< (point) endpt)
	(or (looking-at (concat prefix "\\(.*\\)$")) (error "Strange installer line"))
	(setq strg (concat strg (match-string 1) "\n"))
	(forward-line 1))
      (installer-edit-site strg))))

(defun installer-add-file (basename)
  "Add new file with given BASENAME, filename without the .el."
  (let ((filename (or (installer-current-dir basename)
		      (installer-best-dir basename))))
    (message "Installing %s" filename)
    (cond ((file-exists-p filename)
	   (message "Moving old file to %s~" filename)
	   (cond ((file-exists-p (concat filename "~"))
		  (delete-file (concat filename "~"))))
	   (rename-file filename (concat filename "~"))))
    (copy-file basename filename)
    (let ((make-backup-files nil))
      (byte-compile-file filename))
    (find-file filename)
    (installer-comment-process)))
  
(provide 'installer)

;;; installer.el ends here
