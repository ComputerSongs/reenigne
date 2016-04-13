  %include "../../defaults_bin.asm"

  mov ax,1
  int 0x10
  mov dx,0x3d9
  mov al,6
  out dx,al
  mov dx,0x3d8
  mov al,0x08
  out dx,al

  ; Copy image data
  mov ax,0xb800
  mov es,ax
  mov ax,cs
  mov ds,ax
  xor di,di
  mov si,imageData
  cld
  mov cx,4000 + 0x1000
  rep movsw

  cli
  hlt
imageData:
  dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000
  dw 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000
  dw 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x5000, 0x5000, 0x5000, 0x5000, 0x5000
  dw 0x6000, 0x6000, 0x6000, 0x6000, 0x6000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000

  dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000
  dw 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000
  dw 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x5000, 0x5000, 0x5000, 0x5000, 0x5000
  dw 0x6000, 0x6000, 0x6000, 0x6000, 0x6000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000

  dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000
  dw 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000
  dw 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x5000, 0x5000, 0x5000, 0x5000, 0x5000
  dw 0x6000, 0x6000, 0x6000, 0x6000, 0x6000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000

  dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000
  dw 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000
  dw 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x5000, 0x5000, 0x5000, 0x5000, 0x5000
  dw 0x6000, 0x6000, 0x6000, 0x6000, 0x6000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000

  dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000
  dw 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000
  dw 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x5000, 0x5000, 0x5000, 0x5000, 0x5000
  dw 0x6000, 0x6000, 0x6000, 0x6000, 0x6000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000

  dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000
  dw 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000
  dw 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x5000, 0x5000, 0x5000, 0x5000, 0x5000
  dw 0x6000, 0x6000, 0x6000, 0x6000, 0x6000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000

  dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000
  dw 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000
  dw 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x5000, 0x5000, 0x5000, 0x5000, 0x5000
  dw 0x6000, 0x6000, 0x6000, 0x6000, 0x6000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000

  dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000
  dw 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000
  dw 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x5000, 0x5000, 0x5000, 0x5000, 0x5000
  dw 0x6000, 0x6000, 0x6000, 0x6000, 0x6000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000

  dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000
  dw 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000
  dw 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x5000, 0x5000, 0x5000, 0x5000, 0x5000
  dw 0x6000, 0x6000, 0x6000, 0x6000, 0x6000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000

  dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000
  dw 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000
  dw 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x5000, 0x5000, 0x5000, 0x5000, 0x5000
  dw 0x6000, 0x6000, 0x6000, 0x6000, 0x6000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000

  dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000
  dw 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000
  dw 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x5000, 0x5000, 0x5000, 0x5000, 0x5000
  dw 0x6000, 0x6000, 0x6000, 0x6000, 0x6000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000

  dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000
  dw 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000
  dw 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x5000, 0x5000, 0x5000, 0x5000, 0x5000
  dw 0x6000, 0x6000, 0x6000, 0x6000, 0x6000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000


  dw 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x9000, 0x9000, 0x9000, 0x9000, 0x9000
  dw 0xa000, 0xa000, 0xa000, 0xa000, 0xa000, 0xb000, 0xb000, 0xb000, 0xb000, 0xb000
  dw 0xc000, 0xc000, 0xc000, 0xc000, 0xc000, 0xd000, 0xd000, 0xd000, 0xd000, 0xd000
  dw 0xe000, 0xe000, 0xe000, 0xe000, 0xe000, 0xf000, 0xf000, 0xf000, 0xf000, 0xf000

  dw 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x9000, 0x9000, 0x9000, 0x9000, 0x9000
  dw 0xa000, 0xa000, 0xa000, 0xa000, 0xa000, 0xb000, 0xb000, 0xb000, 0xb000, 0xb000
  dw 0xc000, 0xc000, 0xc000, 0xc000, 0xc000, 0xd000, 0xd000, 0xd000, 0xd000, 0xd000
  dw 0xe000, 0xe000, 0xe000, 0xe000, 0xe000, 0xf000, 0xf000, 0xf000, 0xf000, 0xf000

  dw 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x9000, 0x9000, 0x9000, 0x9000, 0x9000
  dw 0xa000, 0xa000, 0xa000, 0xa000, 0xa000, 0xb000, 0xb000, 0xb000, 0xb000, 0xb000
  dw 0xc000, 0xc000, 0xc000, 0xc000, 0xc000, 0xd000, 0xd000, 0xd000, 0xd000, 0xd000
  dw 0xe000, 0xe000, 0xe000, 0xe000, 0xe000, 0xf000, 0xf000, 0xf000, 0xf000, 0xf000

  dw 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x9000, 0x9000, 0x9000, 0x9000, 0x9000
  dw 0xa000, 0xa000, 0xa000, 0xa000, 0xa000, 0xb000, 0xb000, 0xb000, 0xb000, 0xb000
  dw 0xc000, 0xc000, 0xc000, 0xc000, 0xc000, 0xd000, 0xd000, 0xd000, 0xd000, 0xd000
  dw 0xe000, 0xe000, 0xe000, 0xe000, 0xe000, 0xf000, 0xf000, 0xf000, 0xf000, 0xf000

  dw 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x9000, 0x9000, 0x9000, 0x9000, 0x9000
  dw 0xa000, 0xa000, 0xa000, 0xa000, 0xa000, 0xb000, 0xb000, 0xb000, 0xb000, 0xb000
  dw 0xc000, 0xc000, 0xc000, 0xc000, 0xc000, 0xd000, 0xd000, 0xd000, 0xd000, 0xd000
  dw 0xe000, 0xe000, 0xe000, 0xe000, 0xe000, 0xf000, 0xf000, 0xf000, 0xf000, 0xf000

  dw 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x9000, 0x9000, 0x9000, 0x9000, 0x9000
  dw 0xa000, 0xa000, 0xa000, 0xa000, 0xa000, 0xb000, 0xb000, 0xb000, 0xb000, 0xb000
  dw 0xc000, 0xc000, 0xc000, 0xc000, 0xc000, 0xd000, 0xd000, 0xd000, 0xd000, 0xd000
  dw 0xe000, 0xe000, 0xe000, 0xe000, 0xe000, 0xf000, 0xf000, 0xf000, 0xf000, 0xf000

  dw 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x9000, 0x9000, 0x9000, 0x9000, 0x9000
  dw 0xa000, 0xa000, 0xa000, 0xa000, 0xa000, 0xb000, 0xb000, 0xb000, 0xb000, 0xb000
  dw 0xc000, 0xc000, 0xc000, 0xc000, 0xc000, 0xd000, 0xd000, 0xd000, 0xd000, 0xd000
  dw 0xe000, 0xe000, 0xe000, 0xe000, 0xe000, 0xf000, 0xf000, 0xf000, 0xf000, 0xf000

  dw 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x9000, 0x9000, 0x9000, 0x9000, 0x9000
  dw 0xa000, 0xa000, 0xa000, 0xa000, 0xa000, 0xb000, 0xb000, 0xb000, 0xb000, 0xb000
  dw 0xc000, 0xc000, 0xc000, 0xc000, 0xc000, 0xd000, 0xd000, 0xd000, 0xd000, 0xd000
  dw 0xe000, 0xe000, 0xe000, 0xe000, 0xe000, 0xf000, 0xf000, 0xf000, 0xf000, 0xf000

  dw 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x9000, 0x9000, 0x9000, 0x9000, 0x9000
  dw 0xa000, 0xa000, 0xa000, 0xa000, 0xa000, 0xb000, 0xb000, 0xb000, 0xb000, 0xb000
  dw 0xc000, 0xc000, 0xc000, 0xc000, 0xc000, 0xd000, 0xd000, 0xd000, 0xd000, 0xd000
  dw 0xe000, 0xe000, 0xe000, 0xe000, 0xe000, 0xf000, 0xf000, 0xf000, 0xf000, 0xf000

  dw 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x9000, 0x9000, 0x9000, 0x9000, 0x9000
  dw 0xa000, 0xa000, 0xa000, 0xa000, 0xa000, 0xb000, 0xb000, 0xb000, 0xb000, 0xb000
  dw 0xc000, 0xc000, 0xc000, 0xc000, 0xc000, 0xd000, 0xd000, 0xd000, 0xd000, 0xd000
  dw 0xe000, 0xe000, 0xe000, 0xe000, 0xe000, 0xf000, 0xf000, 0xf000, 0xf000, 0xf000

  dw 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x9000, 0x9000, 0x9000, 0x9000, 0x9000
  dw 0xa000, 0xa000, 0xa000, 0xa000, 0xa000, 0xb000, 0xb000, 0xb000, 0xb000, 0xb000
  dw 0xc000, 0xc000, 0xc000, 0xc000, 0xc000, 0xd000, 0xd000, 0xd000, 0xd000, 0xd000
  dw 0xe000, 0xe000, 0xe000, 0xe000, 0xe000, 0xf000, 0xf000, 0xf000, 0xf000, 0xf000

  dw 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x9000, 0x9000, 0x9000, 0x9000, 0x9000
  dw 0xa000, 0xa000, 0xa000, 0xa000, 0xa000, 0xb000, 0xb000, 0xb000, 0xb000, 0xb000
  dw 0xc000, 0xc000, 0xc000, 0xc000, 0xc000, 0xd000, 0xd000, 0xd000, 0xd000, 0xd000
  dw 0xe000, 0xe000, 0xe000, 0xe000, 0xe000, 0xf000, 0xf000, 0xf000, 0xf000, 0xf000

  dw 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x9000, 0x9000, 0x9000, 0x9000, 0x9000
  dw 0xa000, 0xa000, 0xa000, 0xa000, 0xa000, 0xb000, 0xb000, 0xb000, 0xb000, 0xb000
  dw 0xc000, 0xc000, 0xc000, 0xc000, 0xc000, 0xd000, 0xd000, 0xd000, 0xd000, 0xd000
  dw 0xe000, 0xe000, 0xe000, 0xe000, 0xe000, 0xf000, 0xf000, 0xf000, 0xf000, 0xf000
