  %include "../defaults_bin.asm"

  mov al,TIMER0 | BOTH | MODE2 | BINARY
  out 0x43,al
  xor al,al
  out 0x40,al
  out 0x40,al

  cli

  mov ax,0
  mov ds,ax
  mov ax,cs
  mov word[0x20],interrupt8
  mov [0x22],ax

  mov ds,ax
  mov es,ax
  mov ss,ax
  mov sp,0

  mov si,experimentData
nextExperiment:
  xor bx,bx
  mov [lastQuotient],bx

  ; Print name of experiment
printLoop:
  lodsb
  cmp al,'$'
  je donePrint
  outputCharacter
  inc bx
  jmp printLoop
donePrint:
  cmp bx,0
  jne printSpaces

  ; Finish
  complete

  ; Print spaces for alignment
printSpaces:
  mov cx,21  ; Width of column
  sub cx,bx
  jg spaceLoop
  mov cx,1
spaceLoop:
  outputCharacter ' '
  loop spaceLoop

  mov cx,5    ; Number of repeats
repeatLoop:
  push cx

  mov cx,480+48  ; Number of iterations in primary measurement
  call doMeasurement
  push bx
  mov cx,48      ; Number of iterations in secondary measurement
  call doMeasurement
  pop ax         ; The primary measurement will have the lower value, since the counter counts down
  sub ax,bx      ; Subtract the secondary value, which will be higher, now AX is negative
  neg ax         ; Negate to get the positive difference.

  xor dx,dx
  mov cx,120
  div cx       ; Divide by 120 to get number of cycles (quotient) and number of extra tcycles (remainder)

  push dx      ; Store remainder

  ; Output quotient
  xor dx,dx
  mov [quotient],ax
  mov cx,10
  div cx
  add dl,'0'
  mov [output+2],dl
  xor dx,dx
  div cx
  add dl,'0'
  mov [output+1],dl
  xor dx,dx
  div cx
  add dl,'0'
  mov [output+0],dl

  ; Output remainder
  pop ax
  xor dx,dx
  div cx
  add dl,'0'
  mov [output+7],dl
  xor dx,dx
  div cx
  add dl,'0'
  mov [output+6],dl
  xor dx,dx
  div cx
  add dl,'0'
  mov [output+5],dl

  ; Emit the final result text
  push si
  mov ax,[quotient]
  cmp ax,[lastQuotient]
  jne fullPrint

  mov cx,6
  mov si,output+4
  jmp doPrint
fullPrint:
  mov [lastQuotient],ax
  mov cx,10
  mov si,output
doPrint:
  outputString
  pop si
  pop cx
  loop repeatLoop1

  ; Advance to the next experiment
  lodsw
  add si,ax

  outputNewLine

  jmp nextExperiment

repeatLoop1:
  jmp repeatLoop

quotient: dw 0
lastQuotient: dw 0

output:
  db "000 +000  "


doMeasurement:
  push si
  push cx  ; Save number of iterations

  ; Copy init
  lodsw    ; Number of init bytes
  mov cx,ax
  mov di,timerStartEnd
  call codeCopy

  ; Copy timer end
  mov si,timerEndStart
  mov cx,timerEndEnd-timerEndStart
  call codeCopy


  ; Enable IRQ0
  mov al,0xfe  ; Enable IRQ 0 (timer), disable others
  out 0x21,al

  mov cx,256
  cli

  ; Increase refresh frequency to ensure all DRAM is refreshed before turning
  ; off refresh.
  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,2
  out 0x41,al  ; Timer 1 rate

  ; Delay for enough time to refresh 512 columns
  rep lodsw

  lockstep
  pop cx

  ; Use IRQ0 to go into lockstep with timer 0
  mov al,TIMER0 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,0x10  ; Count = 0x0004 which should be after the hlt instruction has
  out 0x40,al  ; taken effect.
  sti
  hlt

  ; The actual measurement happens in the the IRQ0 handler which runs here and
  ; returns the timer value in BX.

  ; Pop the flags pushed when the interrupt occurred
  pop ax

  pop si
  ret

codeCopy:
  cmp cx,0
  je codeCopyDone
codeCopyLoop:
  cmp di,0
  je codeCopyOutOfSpace
  movsb
  loop codeCopyLoop
codeCopyDone:
  ret
codeCopyOutOfSpace:
  mov si,outOfSpaceMessage
  mov cx,outOfSpaceMessageEnd-outOfSpaceMessage
  outputString
  complete

outOfSpaceMessage:
  db "Copy out of space - use fewer iterations"
outOfSpaceMessageEnd:


experimentData:


experimentPlasma:
  db "Plasma$"
  dw .endCode - ($+2)
  mov ax,0xb800
  mov es,ax
  mov ax,0x8000
  mov ds,ax
  mov ss,ax
  mov di,0
  mov si,0
  mov dx,0
  mov bp,0
.xloop:
        mov     bl,dl
        mov     al,[bx+si]
        mov     bl,dh
        add     al,[bx+si]
        mov     bl,al
        mov     al,[bx+1234]
        stosb
        inc     di
        inc     dl
        dec     dh
        loop    .xloop
.endCode:




LoopTaken:
  db "LoopTaken$"
  dw .endCode - ($+2)
.foo:
  loop .foo
.endCode:

REPSTOSWsys:
  db "REPSTOSWsys$"
  dw .endCode - ($+2)
  mov ax,0x8000
  mov es,ax
  xor di,di
  xor ax,ax
  cld
  rep stosw
.endCode:

REPSTOSBsys:
  db "REPSTOSBsys$"
  dw .endCode - ($+2)
  mov ax,0x8000
  mov es,ax
  xor di,di
  xor ax,ax
  cld
  rep stosb
.endCode:

REPSTOSWCGA:
  db "REPSTOSWCGA$"
  dw .endCode - ($+2)
  mov ax,0xb800
  mov es,ax
  xor di,di
  xor ax,ax
  cld
  rep stosw
.endCode:

REPSTOSBCGA:
  db "REPSTOSBCGA$"
  dw .endCode - ($+2)
  mov ax,0xb800
  mov es,ax
  xor di,di
  xor ax,ax
  cld
  rep stosb
.endCode:

REPMOVSWsysCGA:
  db "REPMOVSWsysCGA$"
  dw .endCode - ($+2)
  mov ax,0xb800
  mov es,ax
  mov ax,0x8000
  mov ds,ax
  xor si,si
  xor di,di
  xor ax,ax
  cld
  rep movsw
.endCode:

REPMOVSBsysCGA:
  db "REPMOVSBsysCGA$"
  dw .endCode - ($+2)
  mov ax,0xb800
  mov es,ax
  mov ax,0x8000
  mov ds,ax
  xor si,si
  xor di,di
  xor ax,ax
  cld
  rep movsb
.endCode:

REPMOVSWCGAsys:
  db "REPMOVSWCGAsys$"
  dw .endCode - ($+2)
  mov ax,0xb800
  mov ds,ax
  mov ax,0x8000
  mov es,ax
  xor si,si
  xor di,di
  xor ax,ax
  cld
  rep movsw
.endCode:

REPMOVSBCGAsys:
  db "REPMOVSBCGAsys$"
  dw .endCode - ($+2)
  mov ax,0xb800
  mov ds,ax
  mov ax,0x8000
  mov es,ax
  xor si,si
  xor di,di
  xor ax,ax
  cld
  rep movsb
.endCode:

REPMOVSWCGACGA:
  db "REPMOVSWCGACGA$"
  dw .endCode - ($+2)
  mov ax,0xb800
  mov ds,ax
  mov es,ax
  xor si,si
  xor di,di
  xor ax,ax
  cld
  rep movsw
.endCode:

REPMOVSBCGACGA:
  db "REPMOVSBCGACGA$"
  dw .endCode - ($+2)
  mov ax,0xb800
  mov ds,ax
  mov es,ax
  xor si,si
  xor di,di
  xor ax,ax
  cld
  rep movsb
.endCode:

REPMOVSWsyssys:
  db "REPMOVSWsyssys$"
  dw .endCode - ($+2)
  mov ax,0x8000
  mov ds,ax
  mov es,ax
  xor si,si
  xor di,di
  xor ax,ax
  cld
  rep movsw
.endCode:

REPMOVSBsyssys:
  db "REPMOVSBsyssys$"
  dw .endCode - ($+2)
  mov ax,0x8000
  mov ds,ax
  mov es,ax
  xor si,si
  xor di,di
  xor ax,ax
  cld
  rep movsb
.endCode:

REPCMPSWsysCGA:
  db "REPCMPSWsysCGA$"
  dw .endCode - ($+2)
  mov ax,0xb800
  mov es,ax
  mov ax,0x8000
  mov ds,ax
  xor si,si
  xor di,di
  xor ax,ax
  cld
  rep cmpsw
.endCode:

REPCMPSBsysCGA:
  db "REPCMPSBsysCGA$"
  dw .endCode - ($+2)
  mov ax,0xb800
  mov es,ax
  mov ax,0x8000
  mov ds,ax
  xor si,si
  xor di,di
  xor ax,ax
  cld
  rep cmpsb
.endCode:

REPCMPSWCGAsys:
  db "REPCMPSWCGAsys$"
  dw .endCode - ($+2)
  mov ax,0xb800
  mov ds,ax
  mov ax,0x8000
  mov es,ax
  xor si,si
  xor di,di
  xor ax,ax
  cld
  rep cmpsw
.endCode:

REPCMPSBCGAsys:
  db "REPCMPSBCGAsys$"
  dw .endCode - ($+2)
  mov ax,0xb800
  mov ds,ax
  mov ax,0x8000
  mov es,ax
  xor si,si
  xor di,di
  xor ax,ax
  cld
  rep cmpsb
.endCode:

REPCMPSWCGACGA:
  db "REPCMPSWCGACGA$"
  dw .endCode - ($+2)
  mov ax,0xb800
  mov ds,ax
  mov es,ax
  xor si,si
  xor di,di
  xor ax,ax
  cld
  rep cmpsw
.endCode:

REPCMPSBCGACGA:
  db "REPCMPSBCGACGA$"
  dw .endCode - ($+2)
  mov ax,0xb800
  mov ds,ax
  mov es,ax
  xor si,si
  xor di,di
  xor ax,ax
  cld
  rep cmpsb
.endCode:

REPCMPSWsyssys:
  db "REPCMPSWsyssys$"
  dw .endCode - ($+2)
  mov ax,0x8000
  mov ds,ax
  mov es,ax
  xor si,si
  xor di,di
  xor ax,ax
  cld
  rep cmpsw
.endCode:

REPCMPSBsyssys:
  db "REPCMPSBsyssys$"
  dw .endCode - ($+2)
  mov ax,0x8000
  mov ds,ax
  mov es,ax
  xor si,si
  xor di,di
  xor ax,ax
  cld
  rep cmpsb
.endCode:

REPSCASWsys:
  db "REPSCASWsys$"
  dw .endCode - ($+2)
  mov ax,0x8000
  mov es,ax
  xor di,di
  xor ax,ax
  cld
  rep scasw
.endCode:

REPSCASBsys:
  db "REPSCASBsys$"
  dw .endCode - ($+2)
  mov ax,0x8000
  mov es,ax
  xor di,di
  xor ax,ax
  cld
  rep scasb
.endCode:

REPSCASWCGA:
  db "REPSCASWCGA$"
  dw .endCode - ($+2)
  mov ax,0xb800
  mov es,ax
  xor di,di
  xor ax,ax
  cld
  rep scasw
.endCode:

REPSCASBCGA:
  db "REPSCASBCGA$"
  dw .endCode - ($+2)
  mov ax,0xb800
  mov es,ax
  xor di,di
  xor ax,ax
  cld
  rep scasb
.endCode:

REPLODSWsys:
  db "REPLODSWsys$"
  dw .endCode - ($+2)
  mov ax,0x8000
  mov ds,ax
  xor si,si
  xor ax,ax
  cld
  rep lodsw
.endCode:

REPLODSBsys:
  db "REPLODSBsys$"
  dw .endCode - ($+2)
  mov ax,0x8000
  mov ds,ax
  xor si,si
  xor ax,ax
  cld
  rep lodsb
.endCode:

REPLODSWCGA:
  db "REPLODSWCGA$"
  dw .endCode - ($+2)
  mov ax,0xb800
  mov ds,ax
  xor si,si
  xor ax,ax
  cld
  rep lodsw
.endCode:

REPLODSBCGA:
  db "REPLODSBCGA$"
  dw .endCode - ($+2)
  mov ax,0xb800
  mov ds,ax
  xor si,si
  xor ax,ax
  cld
  rep lodsb
.endCode:

lastExperiment:
  db '$'


savedSS: dw 0
savedSP: dw 0

timerEndStart:
  in al,0x40
  mov bl,al
  in al,0x40
  mov bh,al

  refreshOn

  mov al,0x20
  out 0x20,al

  mov ax,cs
  mov ds,ax
  mov es,ax
  mov ss,[savedSS]
  mov sp,[savedSP]

  ; Don't use IRET here - it'll turn interrupts back on and IRQ0 will be
  ; triggered a second time.
  popf
  retf
timerEndEnd:


  ; This must come last in the program so that the experiment code can be
  ; copied after it.

interrupt8:
  pushf

  mov al,TIMER1 | BOTH | MODE2 | BINARY
  out 0x43,al
  mov al,19
  out 0x41,al  ; Timer 1 rate
  mov al,0
  out 0x41,al

  mov ax,cs
  mov ds,ax
  mov es,ax
  mov [savedSS],ss
  mov [savedSP],sp
  mov ss,ax
  mov dx,0;xffff
  mov bx,0
  mov ax,0
  mov si,0
  mov di,0
  mov bp,0
;  mov sp,0

times 528 push cs

  mov al,TIMER0 | BOTH | MODE2 | BINARY
  out 0x43,al
  mov al,0x00
  out 0x40,al
  out 0x40,al
timerStartEnd:


