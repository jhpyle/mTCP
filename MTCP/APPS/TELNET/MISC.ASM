;  mTCP misc.asm
;  Copyright (C) 2020 Jonathan H. Pyle (jhpyle@gmail.com)
;  mTCP web page: http://www.brutman.com/mTCP
;
;
;  This file is part of mTCP.
;
;  mTCP is free software: you can redistribute it and/or modify
;  it under the terms of the GNU General Public License as published by
;  the Free Software Foundation, either version 3 of the License, or
;  (at your option) any later version.
;
;  mTCP is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;  GNU General Public License for more details.
;
;  You should have received a copy of the GNU General Public License
;  along with mTCP.  If not, see <http://www.gnu.org/licenses/>.
;
;
;  Description: Mouse and graphics interface
;
;  Changes:
;
;  2020-05-17: Initial release
;  2020-05-25: Renamed from mouse.asm, now includes graphics related functions



.8086


name MISC

LDOWN equ 1
RDOWN equ 2
LUP equ 4
RUP equ 8
MDOWN equ 16
MUP equ 32

ifdef __SMALL__
  ; near code, near data
  X EQU 4
  .model small
else
  ifdef __COMPACT__
    ; near code, far data
    X equ 4
    .model compact
  else
    ifdef __LARGE__
      ; far code, far data
      X equ 6
      .model large
    else
      ifdef __MEDIUM__
        ; far code, near data
        X equ 6
        .model medium
      else
        ifdef __HUGE__
          ; far code, far data
          X equ 6
          .model huge
        endif
      endif
    endif
  endif
endif

;; DGROUP group _DATA
;; _DATA segment word public 'DATA' use16
;; _DATA ends

;; extrn _mouse_changed:BYTE
;; extrn _left_pressed:BYTE
;; extrn _right_pressed:BYTE
;; extrn _middle_pressed:BYTE

_TEXT segment para public 'CODE' use16
assume cs:_TEXT
;; ,ds:_TEXT,ss:_TEXT,es:_TEXT ;

public _mouse_changed
_mouse_changed	db 0

public _left_pressed
_left_pressed	db 0

public _right_pressed
_right_pressed	db 0

public _middle_pressed
_middle_pressed	db 0

oldVectorCX     dw 0
oldVectorDX     dw 0
oldVectorES	dw 0

public _buffer_buttons
_buffer_buttons	dw 16 DUP (0)
public _buffer_col
_buffer_col	dw 16 DUP (255)
public _buffer_row
_buffer_row	dw 16 DUP (255)
public _buffer_time_cx
_buffer_time_cx	dw 16 DUP (0)
public _buffer_time_dx
_buffer_time_dx	dw 16 DUP (0)
public _buffer_start
_buffer_start	db 7
public _buffer_end
_buffer_end	db 7

old_buttons	dw -1
old_row		dw -1
old_col  	dw -1

;; extern  "C", MouseMoveDetected:byte

public _ibm_mouse_init

_ibm_mouse_init proc far

	push    bp
	mov     bp,sp

	push    ds
	push    si

	mov     ax, 0
	int	0x33

	pop     si
	pop     ds
	pop     bp
	ret

_ibm_mouse_init endp

public _ibm_mouse_show

_ibm_mouse_show proc far

	push    bp
	mov     bp,sp

	push    ds
	push    si

	mov     ax, 1
	int	0x33

	pop     si
	pop     ds
	pop     bp
	ret

_ibm_mouse_show endp

public _ibm_mouse_hide

_ibm_mouse_hide proc far

	push    bp
	mov     bp,sp

	push    ds
	push    si

	mov     ax, 2
	int	0x33

	pop     si
	pop     ds
	pop     bp
	ret

_ibm_mouse_hide endp

;; public _ibm_mouse_get_status

;; _ibm_mouse_get_status proc far

;; 	push    bp
;; 	mov     bp,sp

;; 	push    ds
;; 	push    si

;; 	mov     ax, 3
;; 	int     0x33
;; 	mov     di, [bp+X]
;; 	mov     [di], bx
;; 	mov     di, [bp+X+4]
;; 	mov     [di], cx
;; 	mov     di, [bp+X+8]
;; 	mov     [di], dx
;; 	pop     si
;; 	pop     ds
;; 	pop     bp
;; 	ret

;; _ibm_mouse_get_status endp


public _ibm_mouse_get_time

_ibm_mouse_get_time proc far

	push    bp
	mov     bp,sp

	push    ds
	push    si

	mov	ah, 0
	int	1ah		; cx and dx now have the time

	mov     di, [bp+X]
	mov     [di+2], cx
	mov     [di], dx
	pop     si
	pop     ds
	pop     bp
	ret

_ibm_mouse_get_time endp

public _ibm_mouse_install

_ibm_mouse_install proc far

	push	es
	push	ds

	;; mov 	dl, 41h
	;; mov 	ah, 2
        ;; int 	21h

	mov 	ax, 21h
        int 	33h

	mov 	ax,14h
	push	cs
	pop	ds
	push	cs
	pop	es
	mov     cx, 0x7f
	mov     dx, offset _TEXT:_ibm_mouse_handler
	int     33h
	push    cs
	pop     ds
	mov     oldVectorCX, cx
	mov     oldVectorES, es
	mov     oldVectorDX, dx

	pop     ds
	pop     es
	ret

_ibm_mouse_install endp

public _ibm_mouse_uninstall

_ibm_mouse_uninstall proc far

	push	es
	push	ds

	mov 	ax,14h
	push	cs
	pop	ds
	mov     cx, oldVectorCX
	mov     es, oldVectorES
	mov     dx, oldVectorDX
	int     33h

        sub     ax, ax
        int     33h

	pop     ds
	pop     es
	ret

_ibm_mouse_uninstall endp

;; _ibm_mouse_handler proc far
;; 	push 	es
;;         push    cs
;;         pop     ds
;; 	mov 	_mouse_changed, 1
;; 	test    bl, 1
;; 	jz      @@checkright
;; 	mov	_left_pressed, 1
;; @@checkright:
;; 	test    bl, 2
;; 	jz      @@checkmiddle
;; 	mov	_right_pressed, 1
;; @@checkmiddle:
;; 	test    bl, 4
;; 	jz      @@checkedall
;; 	mov	_middle_pressed, 1
;; @@checkedall:
;; 	pop es
;; 	ret

;; _ibm_mouse_handler endp

_ibm_mouse_handler proc far
	push 	es
        push    cs
        pop     ds
	mov 	_mouse_changed, 1
	mov 	ax, cx
	mov	cl, 3
	shr	ax, cl
	mov 	cx, ax
	mov 	ax, dx
	shr	ax, 1
	shr	ax, 1
	shr	ax, 1
	mov 	dx, ax
	cmp	bx, old_buttons
	jne	@@diff
	cmp	cx, old_row
	jne	@@diff
	cmp	dx, old_col
	je	@@continue
@@diff:
	mov	ah, 0
	mov 	old_buttons, bx
	mov 	old_row, cx
	mov 	old_col, dx
	mov	al, _buffer_end
	inc	al
	and	al, 0x0f
	cmp	al, _buffer_start
	je	@@overflow
	;; get clock ticks since midnight
	push	cx		; push the col and row to the stack
	push	dx
	mov	ah, 0
	int	1ah		; cx and dx now have the time
	mov	al, _buffer_end
	shl	al, 1
	mov	ah, 0
	mov	di, offset _buffer_buttons
	add	di, ax
	mov	[di], bx
	mov	di, offset _buffer_time_cx
	add	di, ax
	mov	[di], cx
	mov	di, offset _buffer_time_dx
	add	di, ax
	mov	[di], dx
	pop	dx		; get row and col back from the stack
	pop	cx
	mov	di, offset _buffer_row
	add	di, ax
	mov	[di], cx
	mov	di, offset _buffer_col
	add	di, ax
	mov	[di], dx
	shr	al, 1
	inc	al
	and	al, 0x0f
	mov	_buffer_end, al
	jmp	@@continue
@@overflow:
@@continue:
	pop es
	ret

_ibm_mouse_handler endp


public _start_graphics

_start_graphics proc far

	push    bp
	mov     bp,sp

	push    ds
	push    si

	lds     si, [bp+X]
	mov	ah, 0
        mov     al, [ds:si]
        int     10h

	pop     si
	pop     ds
	pop     bp
	ret

_start_graphics endp

public _set_cga_palette

_set_cga_palette proc far

	push    bp
	mov     bp,sp

	push    ds
	push    si

	lds     si, [bp+X]
        mov     bl, [ds:si]
	mov 	ah, 0bh
	int 	10h

	lds     si, [bp+X+4]
        mov     bl, [ds:si]
	xor	bh, bh
	int 	10h

	pop     si
	pop     ds
	pop     bp
	ret

_set_cga_palette endp

public _set_mcga_palette

_set_mcga_palette proc far

	push    bp
	mov     bp,sp

	push    ds
	push    si

        cli
        mov     dx, 3c8h
        xor     al, al
        out     dx, al
        inc     dx
        mov     cx, (256*3)
	lds     si, [bp+X]
        cld
@@pl1:
        lodsb
        out     dx,al
        loop    @@pl1
	sti

	pop     si
	pop     ds
	pop     bp
	ret

_set_mcga_palette endp

public _write_mcga_graphics

_write_mcga_graphics proc far

	push    bp
	mov     bp,sp

	push    ds
	push    si

        cli
	lds     si, [bp+X]
	mov	ax, 0a000h
	mov 	es, ax
	xor 	di, di
	mov	cx, 32000
        cld
        rep movsw
	sti

	pop     si
	pop     ds
	pop     bp
	ret

_write_mcga_graphics endp

public _write_cga_graphics

_write_cga_graphics proc far

	push    bp
	mov     bp,sp

	push    ds
	push    si

        cli
	lds     si, [bp+X]
	mov	ax, 0b800h
	mov 	es, ax
	xor 	di, di
	mov	cx, 8096
        cld
        rep movsw
	sti

	pop     si
	pop     ds
	pop     bp
	ret

_write_cga_graphics endp

public _stop_graphics

_stop_graphics proc far

	push    bp
	mov     bp,sp

	push    ds
	push    si

        mov     ax, 3
        int     10h

	pop     si
	pop     ds
	pop     bp
	ret

_stop_graphics endp

_TEXT ends

end
