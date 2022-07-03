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
;  2020-06-07: Fixed bug with CGA palette function
;  2021-09-21: Added support for Hercules Graphics Card
.8086


name MISC

LDOWN 		equ 1
RDOWN 		equ 2
LUP 		equ 4
RUP 		equ 8
MDOWN 		equ 16
MUP 		equ 32
INDEX_REG 	equ 0x03B4
DATA_REG 	equ 0x03B5
DISPLAY_CTL 	equ 0x03B8
CONFIG_SW 	equ 0x03BF
VIDEO_SEG 	equ 0xB800		; address of second page
VIDEO_SIZE 	equ 0x4000		; number of words in the 32k page

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

_TEXT segment para public 'CODE' use16
assume cs:_TEXT,ds:_TEXT,ss:_TEXT,es:_TEXT ;

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

gtable		db 0x35, 0x2D, 0x2E, 0x07, 0x5B, 0x02, 0x57, 0x57, 0x02, 0x03, 0x00, 0x00
ttable		db 0x61, 0x50, 0x52, 0x0F, 0x19, 0x06, 0x19, 0x19, 0x02, 0x0D, 0x0B, 0x0C
lpt1		dw 0000h

public _ibm_mouse_init

_ibm_mouse_init proc far

	push    bx

	mov     ax, 0
	int	0x33

	pop     bx
	ret

_ibm_mouse_init endp

public _ibm_mouse_show

_ibm_mouse_show proc far

	mov     ax, 1
	int	0x33
	ret

_ibm_mouse_show endp

public _ibm_mouse_hide

_ibm_mouse_hide proc far

	mov     ax, 2
	int	0x33
	ret

_ibm_mouse_hide endp

public _ibm_mouse_get_time

_ibm_mouse_get_time proc far

	push    bp
	mov     bp,sp

	push    ds
	push    di
	push    cx
	push    dx

	mov	ah, 0
	int	1ah		; cx and dx now have the time

	mov     di, [bp+X]
	mov     [di+2], cx
	mov     [di], dx
	pop     dx
	pop     cx
	pop     di
	pop     ds
	pop     bp
	ret

_ibm_mouse_get_time endp

public _ibm_mouse_install

_ibm_mouse_install proc far

	push	es
	push	ds
	push    bx
	push	cx
	push    dx
	push    di
	push    si

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

	pop     si
	pop     di
	pop	dx
	pop     cx
	pop	bx
	pop     ds
	pop     es
	ret

_ibm_mouse_install endp

public _ibm_mouse_uninstall

_ibm_mouse_uninstall proc far

	push	es
	push	ds
	push    bx
	push    cx
	push    dx
	push    di
	push    si

	mov 	ax,14h
	push	cs
	pop	ds
	mov     cx, oldVectorCX
	mov     es, oldVectorES
	mov     dx, oldVectorDX
	int     33h

	sub     ax, ax
	int     33h

	pop     si
	pop     di
	pop     dx
	pop	cx
	pop     bx
	pop     ds
	pop     es
	ret

_ibm_mouse_uninstall endp

_ibm_mouse_handler proc far
	push 	ds
	push    ax
	push    bx
	push    cx
	push    dx
	push    di
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
	pop 	di
	pop 	dx
	pop 	cx
	pop 	bx
	pop 	ax
	pop 	ds
	ret

_ibm_mouse_handler endp


public _start_graphics

_start_graphics proc far

	push    bp
	mov     bp,sp
	push    ds
	push    si

	lds     si, [bp+X]
	mov     al, [ds:si]
	mov	ah, 0
	int     10h

	pop     si
	pop     ds
	pop     bp
	ret

_start_graphics endp

setmd proc near

	push    bp
	push    ds
	push    es
	push    ax
	mov     dx, DISPLAY_CTL 	; set mode and turn screen off
	out     dx, al
	mov     ax, ds		; initialize registers in the 6845 for the appropriate mode
	mov     es, ax
	mov     dx, INDEX_REG
	mov     cx, 12
	xor     ah, ah
	cld
parms:	mov     al, ah
	out     dx, al
	inc     dx
	lodsb
	out     dx, al
	inc     ah
	dec     dx
	loop    parms

	;; write same value to the control port as initially, but this time turn on the screen
	mov     dx, DISPLAY_CTL
	pop     ax
	add     al, 8
	out     dx, al
	pop     es
	pop     ds
	pop	bp
	ret

setmd endp

public _enable_hercules

_enable_hercules proc far

	push    dx
	mov 	al, 3
	mov	dx, CONFIG_SW
	out	dx, al
	pop	dx
	ret

_enable_hercules endp

public _disable_hercules

_disable_hercules proc far

	push    dx
	mov 	al, 0
	mov	dx, CONFIG_SW
	out	dx, al
	pop	dx
	ret

_disable_hercules endp

public _clear_hercules

_clear_hercules proc far

	push    bp
	push 	es
	push 	ds
	push    di
	push    cx

	mov     cx, VIDEO_SIZE
	mov     ax, VIDEO_SEG
	cld
	mov     es, ax
	xor     di, di
	mov     ax, 0
	rep     stosw

	pop	cx
	pop     di
	pop	ds
	pop	es
	pop	bp
	ret

_clear_hercules endp

public _start_hercules

_start_hercules proc far

	push    bp
	push 	es
	push 	ds
	push    si
	push    dx
	push    cx

	push    cs
	pop     ds

	mov 	al, 0x82
	lea	si, gtable
	call	setmd

	pop     cx
	pop     dx
	pop     si
	pop     ds
	pop     es
	pop	bp
	ret

_start_hercules endp

public _stop_hercules

_stop_hercules proc far

	push    bp
	push    es
	push    ds
	push    si
	push    dx
	push    cx

	push    cs
	pop     ds

	mov	al, 0x20
	lea	si, ttable
	call	setmd

	pop     cx
	pop     dx
	pop     si
	pop     ds
	pop     es
	pop	bp
	ret

_stop_hercules endp

public _set_cga_palette

_set_cga_palette proc far

	push    bp
	mov     bp,sp

	push    ds
	push    si
	push	bx

	lds     si, [bp+X]
	mov     bl, [ds:si]
	mov     bh, 1
	mov 	ah, 0bh
	int 	10h

	xor	bx, bx
	lds     si, [bp+X+4]
	mov     bl, [ds:si]
	int 	10h

	pop     bx
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
	push    cx
	push    dx

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

	pop     dx
	pop     cx
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
	push    es
	push    si
	push    cx

	cli
	lds     si, [bp+X]
	mov	ax, 0a000h
	mov 	es, ax
	xor 	di, di
	mov	cx, 32000
	cld
	rep movsw
	sti

	pop     cx
	pop     si
	pop	es
	pop     ds
	pop     bp
	ret

_write_mcga_graphics endp

public _write_cga_graphics

_write_cga_graphics proc far

	push    bp
	mov     bp,sp

	push    ds
	push	es
	push    si
	push	di
	push	cx

	cli
	lds     si, [bp+X]
	mov	ax, 0b800h
	mov 	es, ax
	xor 	di, di
	mov	cx, 8096
	cld
	rep movsw
	sti

	pop	cx
	pop	di
	pop     si
	pop	es
	pop     ds
	pop     bp
	ret

_write_cga_graphics endp

public _stop_graphics

_stop_graphics proc far

	mov     ax, 3
	int     10h
	ret

_stop_graphics endp

public _initialize_printer

_initialize_printer proc far

	push    bp
	mov     bp,sp

	push    ds
	push    es
	push    si
	push	bx
	push    cx
	push	dx

	xor	ax, ax
	mov	ds, ax
	mov     dx, ds:0408h    ; get base address of first parallel port from BIOS
	push	cs
	pop	ds
	mov     lpt1, dx
	add	dx, 2		; tell control register we are initializing
	mov	al, 00001000b
	;;		|| |____ Strobe (off)
	;;		||______ INIT (on)
	;; 		|_______ SLCT IN (on)
	out	dx, al
	jmp	short $+2
	push	dx
	mov	ah, 0		; read timer counter
	int	1ah
	add	dx, 2
	mov	bx, dx		; bx holds two ticks in future
_delay:
	int	1ah
	cmp	dx, bx		; are we two ticks in the future yet?
	jb	_delay		; if not, keep polling
	pop	dx		; tell control register we are no longer initializing
	mov	al, 00001100b
	;;		|| |____ Strobe (off)
	;;		||______ INIT (off)
	;; 		|_______ SLCT IN (on)
	out	dx, al
	jmp	short $+2
	lds     si, [bp+X]
_lploop:
	mov	al, ds:[si]
	cmp	al, 0
	jz	_endlp
	call	_lpchar
	inc	si
	jmp	_lploop
_endlp:
	pop	dx
	pop     cx
	pop	bx
	pop     si
	pop	es
	pop     ds
	pop     bp
	ret

_initialize_printer endp

_lpchar:			; send ASCII character in al to printer
	push	ds
	push	cs
	pop	ds
	mov     dx, lpt1	; put ASCII character on data line
	pop	ds
	out	dx, al
	jmp 	short $+2
	inc	dx
_chk:				; wait for printer to not be busy
	in	al, dx
	test	al, 10000000b
	jz	_chk

	inc	dx		; strobe the printer so it reads the character
	mov	al, 00001101b
	;;		|| |____ Strobe (on)
	;;		||______ INIT (off)
	;; 		|_______ SLCT IN (on)
	out	dx, al
	jmp	short $+2
	mov	al, 00001100b
	;;		|| |____ Strobe (off)
	;;		||______ INIT (off)
	;; 		|_______ SLCT IN (on)
	out	dx, al
	jmp	short $+2
	ret

public _send_char_to_printer

_send_char_to_printer proc far

	push    bp		; send ASCII character in al to printer
	mov     bp,sp
	push	ds
	push	dx
	push	si
	lds     si, [bp+X]
	mov	al, ds:[si]
	call 	_lpchar
	pop	si
	pop	dx
	pop	ds
	pop	bp
	ret

_send_char_to_printer endp

_TEXT ends

end
