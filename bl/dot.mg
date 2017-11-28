;
; This is my .mg as of 2017-03-31. Note that you have to compile with
; -DALL to use it.

;
; This file contains initialization commands for mg
;
(global-set-key "\eg"		'goto-line)
(global-set-key "\r"		'newline-and-indent)
(global-set-key "\n"		'newline)
;
(global-set-key "\^n"		'next-buffer)
(global-set-key "\en"		'previous-buffer)
(global-set-key "\^o"		'other-window)
(global-set-key "\eo"		'previous-window)
(global-set-key "\^xo"		'open-line)

(global-set-key "\^p"		'call-last-kbd-macro)
(global-set-key "\^v"		'yank)
(global-set-key "\^f"		'search-forward)
(global-set-key "\^r"		'redraw-display)

(global-set-key "\eu"		'ucs-insert)
(global-set-key "\e8"		'insert-8bit)
(global-set-key "\ee"		'explode-character)
(global-set-key "\ei"		'implode-character)
(global-set-key "\ev"		'show-bytes)

(global-set-key "\ek"		'kill-whole-line)
(global-set-key "\ey"		'yank)

(global-set-key "\^x\^r"	'revert-buffer-forget)
(global-set-key "\^xr"		'find-file-read-only)

(global-set-key "\^xg"		'global-set-key)
(global-set-key "\^xG"		'local-set-key)
(global-set-key "\^xl"		'local-set-charset)
(global-set-key "\^xL"		'load)
(global-set-key "\^xv"		'set-variable)
(global-set-key "\^xV"		'local-set-variable)

;
; "helpbuf" is the mode/keymap in a help buffer. "help" is the keymap
; used by the "^H" command.
;

(define-key "helpbuf"	"a"	'apropos)
(define-key "helpbuf"	"b"	'describe-bindings)
(define-key "helpbuf"	"c"	'describe-key-briefly)
(define-key "helpbuf"	"l"	'list-charsets)
(define-key "helpbuf"	"v"	'list-variables)
(define-key "helpbuf"	"k"	'list-keymaps)
(define-key "helpbuf"	"m"	'list-macro)
(ignore-errors define-key "helpbuf"	"M"	'list-macros)

(define-key "help"	"\^b"	'buffer-menu)
(define-key "help" 	"l"	'list-charsets)
(define-key "help" 	"v"	'list-variables)
(define-key "help" 	"k"	'list-keymaps)
(define-key "help" 	"m"	'list-macro)
(ignore-errors define-key "help" 	"M"	'list-macros)

(ignore-errors define-key "dired" "n" 'dired-next-dirline)
(ignore-errors define-key "dired" "p" 'dired-prev-dirline)

;
; "bufmenu" is the mode/keymap in "*Buffer List*"
;
(define-key "bufmenu" "b"	'buffer-menu)

;
; This is how to undefine a key "with define-key"
;
(define-key "bufmenu" "\^o" 	'nil)
(define-key "bufmenu" "0" 	'Buffer-menu-switch-other-window)

(global-set-key "\f1"		'help-help)		; F1
(global-set-key "\f3"		'search-again)		; F3

(ignore-errors global-set-key "\f4"	'search-all-forward)	; F4
(ignore-errors global-set-key "\f5"	'search-all-backward)	; F5

(global-set-key "\e\{right}" 	'tab-region-right)	; ESC-Right
(global-set-key "\e\{left}"  	'tab-region-left)	; ESC-Left

(global-set-key "\^x\{up}"	'shrink-window)		; ^X-Up
(global-set-key "\^x\{down}"	'enlarge-window)	; ^X-Down

(ignore-errors global-set-key "\es"	'slow-mode)
(ignore-errors global-set-key "\^t"	'test)
(ignore-errors global-set-key "\^c"	'charsdebug)
(ignore-errors global-set-key "\ez"	'charsdebug-zero)
(ignore-errors global-set-key "\^s"	'search-all-simple)

(global-set-key "\^x\^?" 	'delete-region)

;
; Assigned "ESC Euro-sign" for test purposes
;
(global-set-key "\e\u20AC"	'emacs-version)
(global-set-key "\^x "		'no-break)

(set-variable	"case-fold-search" 0)
(set-variable	"modeline-show" 15)
(set-variable	"quoted-char-radix" 16)
(set-variable	"trim-whitespace" 1)
(set-variable	"insert-default-directory" 0)
(set-variable	"complete-fold-file" 1)
(set-variable	"fill-options" 115)
(set-variable	"kill-whole-lines" 1)
(set-variable	"blink-wait" 250)

(global-set-key "\^a"		'beginning-of-visual-line)
(global-set-key "\^e"		'end-of-visual-line)
(global-set-key "\e\^?"		'join-line)
(global-set-key "\e\{delete}"	'join-line-forward)
(global-set-key "\^xt"		'local-set-tabs)
(ignore-errors global-set-key "\el"		'local-toggle-mode)

(auto-execute-list "*dos.txt;/users/Public/Documents/backup.log" "local-set-charset cp437")
(auto-execute-list "README.reference;README.ucsnames" "set-fill-column 90; local-mode-name fill90")

(define-key "helpbuf" 	"p" 	'list-patterns)
(define-key "help" 	"p" 	'list-patterns)

(global-set-key "\^x\^k" 'kill-buffer-quickly)
(define-key "dired" "q" 'kill-buffer-quickly)

(ignore-errors set-unicode-data "~/misc/UnicodeData.txt")
(ignore-errors define-key "help" "u" 'list-unicode)
(ignore-errors define-key "helpbuf" "u" 'list-unicode)

;
; Do not want to lose it by copying mistakes
;
(auto-execute "/src/mg2a/Changelog" "local-set-variable make-backup 1")
auto-execute-list "~/.mg;~/.bashrc" "lv make-backup 1"

;
; fill-options for filling block comments in C, and use hard tabs with
; auto-cleanup.
;
create-macro "myprog" "local-set-variable fill-options 9; local-set-tabs 8 0"

;
; Use hard tabs, but clean up whitespace
;
create-macro "myprog2" "local-set-tabs 8 0"

;
; Various C modes.
;
auto-execute "/c/Cache/src/winsup/*/malloc.cc" "myprog; local-set-mode stdc; lv clike-style 4; local-set-tabs 2 1 1"
auto-execute "/c/Cache/src/winsup/*.{cc,c,h}" "myprog; local-set-mode gnuc"
auto-execute-list "*.c;*.h;*.inc" "local-set-mode stdc; myprog"

;
; Python mode
;
shebang "python"
auto-execute "*.py" "python-mode"

;
; Perl mode
;
shebang "perl"
auto-execute-list "*.pl;*.pcmd" "local-set-mode perl; myprog2"

;
; Mode for Windows CMD.EXE
;
create-mode cmd "myprog2"
define-key cmd "\r" clike-newline-and-indent
define-key cmd ")" clike-insert
define-key cmd "\e\t" clike-indent
auto-execute-list "*.cmd;*.bat" "local-set-mode cmd"

;
; Powershell mode
;
copy-mode clike powershell "myprog2"
auto-execute-list "*.ps1;*.psd1;*.psm1" "local-set-mode powershell"

;
; JavaScript mode
;
auto-execute "*.js" "local-set-mode javascript; myprog2"

;
; Re-indent a line in C mode, step to the next line.
;
global-set-key "\f8" clike-indent-next

;
; Re-indent the region in C mode
;
global-set-key "\f9" clike-indent-region

;
; Two different displays of buffer list
;
global-set-key "\f7" "sv buffer-name-width 20; list-buffers 1; sv buffer-name-width 24"
global-set-key "\f6" list-buffers

;
; Calc (http://www.isthe.com/chongo/tech/comp/calc/) mode
;
copy-mode clike calc "myprog2"
auto-execute "*.cal" "local-set-mode calc"

;
; Go to top or bottom of window
;
global-set-key "\e\{up}" "move-to-window-line 0"
global-set-key "\e\{down}" "move-to-window-line -1"

;
; Swift mode, with "case" indented a la Apple
;
auto-execute "*.swift" "local-set-mode swift; lv clike-style 2"

;
; Go mode
;
auto-execute "*.go" "local-set-mode go"

;
; Position function for nice view after 'mgfunc'
;
global-set-key "\ea" "recenter 8"

;
; Undo and redo
;
ignore-errors global-set-key "\f10" list-undo
global-set-key "\f11" undo-only
global-set-key "\f12" redo

;
; Bell off
;
set-variable bell-type 0

;
; Exit quickly
;
global-set-key "\^x\^a\^z" kill-emacs

;
; Edit at the end: "mg -end ..."
;
create-macro nd "end-of-buffer; recenter -3"

;
; Interactive edit at the end
;
global-set-key "\ej" "end-of-buffer; recenter -3"

;
; Put the next end of a function at the bottom of the window,
; and the cursor in the middle of the window.
;
create-macro endfunc "search-forward \"\\n}\"; recenter -1; move-to-window-line"

;
; Insert the date in international format
;
create-macro date "yank-process \"date --rfc-3339=second\""
