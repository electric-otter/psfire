   IN al,61h
   PUSH ax

   MOV dx,0Bh

Main:

   MOV bx,477     ; 1,193,180 / 2500 (assume this is timing to frequency)
   CALL WarbCom
   MOV cx, 2600h

Delay1:

   LOOP Delay1
   MOV bx, 36         1; 1,193,180 / 32767
   CALL WarbCom
   MOV cx,1300h

Delay2:

   LOOP Delay2
   DEC dx
   JNZ Main

   POP ax
   OUT 61h,al

--------------O/----------
                  O\

WarbCom:

   MOV al,10110110b
   OUT 43h,al
   MOV ax,bx
   OUT 42h,al
   MOV al,ah
   OUT 42h,al
   IN al, 61h
   OR al,00000011b
   OUT 61h,al

   RET

.lutin   dd music,0,@f-$-4 \
         ,.B,.4th ,.Dd,.4th ,.Gd,.4th ,.Dd,.8th ,.Gd,.8th \
         ,.B,.4th ,.E,.4th ,.Gd,.4th ,.E,.8th ,.Gd,.8th \
         ,.B,.4th ,.Dd,.4th ,.Gd,.4th ,.Dd,.8th ,.Gd,.8th \
         ,.Ad,.4th ,.Dd,.4th ,.G,.4th ,.Dd,.8th ,.Gd,.8th \
         ,.B,.4th ,.E,.4th ,.Gd,.4th ,.E,.8th ,.Gd,.8th \
         ,.Dd shl 1,.4th ,.Gd,.4th ,.B,.4th ,.E,.8th ,.Gd,.8th \
         ,.Cd shl 1,.4th ,.G,.4th ,.Ad,.4th ,.Dd,.8th ,.Gd,.8th \
         ,.Dd shl 1,.4th ,.Fd,.4th ,.B,.4th ,.E,.8th ,.Gd,.8th
         @@:
.C = 261
.Cd= 277
.D = 293
.Dd= 311
.E = 329
.F = 349
.Fd= 370
.G = 392
.Gd= 415
.A = 440
.Ad= 466
.B = 493

.0    dd .C/8,.Cd/8,.D/8,.Dd/8,.E/8,.F/8,.Fd/8,.G/8,.Gd/8,.A/8,.Ad/8,.B/8
.1    dd .C/4,.Cd/4,.D/4,.Dd/4,.E/4,.F/4,.Fd/4,.G/4,.Gd/4,.A/4,.Ad/4,.B/4
.2    dd .C/2,.Cd/2,.D/2,.Dd/2,.E/2,.F/2,.Fd/2,.G/2,.Gd/2,.A/2,.Ad/2,.B/2
.3    dd .C*1,.Cd*1,.D*1,.Dd*1,.E*1,.F*1,.Fd*1,.G*1,.Gd*1,.A*1,.Ad*1,.B*1
.4    dd .C*2,.Cd*2,.D*2,.Dd*2,.E*2,.F*2,.Fd*2,.G*2,.Gd*2,.A*2,.Ad*2,.B*2
.5    dd .C*4,.Cd*4,.D*4,.Dd*4,.E*4,.F*4,.Fd*4,.G*4,.Gd*4,.A*4,.Ad*4,.B*4
.6    dd .C*8,.Cd*8,.D*8,.Dd*8,.E*8,.F*8,.Fd*8,.G*8,.Gd*8,.A*8,.Ad*8,.B*8
.7    dd .C*16,.Cd*16,.D*16,.Dd*16,.E*16,.F*16,.Fd*16,.G*16,.Gd*16,.A*16,.Ad*16,.B*16
.8    dd .C*32,.Cd*32,.D*32,.Dd*32,.E*32,.F*32,.Fd*32,.G*32,.Gd*32,.A*32,.Ad*32,.B*32
.9    dd .C*64,.Cd*64,.D*64,.Dd*64,.E*64,.F*64,.Fd*64,.G*64,.Gd*64,.A*64,.Ad*64,.B*64
.10   dd .C*128,.Cd*128,.D*128,.Dd*128,.E*128,.F*128,.Fd*128,.G*128,.Gd*128,.A*128,.Ad*128,.B*128


.bpm=250
.4th = (60*int8.freq)/.bpm
.8th = .4th/2
.keyboard:
Null

;.do   dd 261.6
;.dod  dd 277.2
;.re   dd 293.7
;.red  dd 311.1
;.mi   dd 329.7
;.fa   dd 349.2
;.fad  dd 370.0
;.sol  dd 392.0
;.sold dd 415.3
;.la   dd 440.0
;.lad  dd 466.2
;.si   dd 493.9
beeper:
        dd beep,100,100

music:  dd speaker.loop,0
.call=0
.current=4
.size=8
.data=12
.freq=0
.time=4

melody: dd speaker.seq,0
.call=0
.current=4
.size=8
.data=12
.freq=0
.time=4

beep:   dd speaker.event,0
.call=0
.freq=4
.time=8

speaker:
.loop:
        cmp [.time],0
        jg .noloop
        mov eax,[esi+melody.current]
        add eax,8
        cmp eax,[esi+melody.size]
        jl @f
        mov eax,0
@@:
        mov [esi+melody.current],eax
        mov ebx,[esi+eax+melody.data+melody.freq]
        mov [.freq],ebx
        mov ebx,[esi+eax+melody.data+melody.time]
        mov [.time],ebx
        mov [.cnt],0
.noloop:
        call .tone
        ret

.seq:
        cmp [.time],0
        jg .noseq
        mov eax,[esi+melody.current]
        add eax,8
        cmp eax,[esi+melody.size]
        jge .endseq
        mov [esi+melody.current],eax
        mov ebx,[esi+eax+melody.data+melody.freq]
        mov [.freq],ebx
        mov ebx,[esi+eax+melody.data+melody.time]
        mov [.time],ebx
        mov [.cnt],0
.noseq:
        call .tone
        ret
.endseq:
        call .off
        ret

.event:
        cmp [.cnt],0
        jne @f
        mov eax,[esi+beep.freq]
        mov [.freq],eax
        mov eax,[esi+beep.time]
        mov [.time],eax
@@:
        call .tone
        ret
.tone:
        cmp [.time],0
        jle .stop
        mov eax,[int8.inc]
        sub [.time],eax
        jl .stop
        cmp [.cnt],0
        jne .end
        call .set
        inc [.cnt]
.end:
        ret
.stop:
        call .off
        ret
.set:
        push eax ebx edx
        mov ebx,[.freq] ; RETURNS:  AX,BX,DX = undefined
        mov edx,[.clk+2]
        mov eax,[.clk] ; ;mov ax,0x34DC ; which is more accurate?
        div bx
        mov bl,al
        mov al,0b6h
        out 43h,al
        mov al,bl
        out 42h,al
        mov al,ah
        out 42h,al
        call .on
        pop edx ebx eax
        ret
.on:
        in al,0x61
        or al,3
        out 0x61,al
        ret
.off:
        in al,0x61
        and al,not 3
        out 0x61,al
        ret
.clk    dd 1193180
.time dd 0
.cnt  dd 0
.freq dd ?



.do   dd 261.6
.dod  dd 277.2
.re   dd 293.7
.red  dd 311.1
.mi   dd 329.7
.fa   dd 349.2
.fad  dd 370.0
.sol  dd 392.0
.sold dd 415.3
.la   dd 440.0
.lad  dd 466.2
.si   dd 493.9

    
