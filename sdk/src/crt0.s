        section vectors,data
        xdef    _vectors

_vectors:
        dc.l    $01000000                       ; 0 ($00): Initial SSP (top of 64 KB WRAM)
        dc.l    _start                          ; 1 ($04): Initial PC (Reset entry)
        dc.l    _irq_ignore                     ; 2 ($08): Bus Error
        dc.l    _irq_ignore                     ; 3 ($0C): Address Error
        dc.l    _irq_ignore                     ; 4 ($10): Illegal Instruction
        dc.l    _irq_ignore                     ; 5 ($14): Zero Divide
        dcb.l   20,_irq_ignore                  ; 6..25 ($18..$64): Standard Exceptions & Level 1 IRQ
        dc.l    _vblank_handler                 ; 26 ($68): Level 2 Autovector (CPS2 VBlank IRQ)
        dc.l    _vblank_handler                 ; 27 ($6C): Level 3 Autovector
        dcb.l   228,_irq_ignore                 ; 28..255 ($70..$3FC): Remaining vectors

        section text,code
        xdef    _start
        xref    main
        xref    _etext
        xref    _sdata
        xref    _edata
        xref    _sbss
        xref    _ebss
        xref    __init_array_start
        xref    __init_array_end

_start:
        move.w  #$2700,sr                       ; Mask all interrupts during init
        lea     $01000000,sp                    ; Reset stack pointer

; Copy .data from ROM (_etext) to WRAM (_sdata.._edata)
        lea     _etext,a0
        lea     _sdata,a1
        lea     _edata,a2
.copy_data:
        cmpa.l  a2,a1
        bge.s   .copy_done
        move.l  (a0)+,(a1)+
        bra.s   .copy_data
.copy_done:

; Zero .bss (_sbss.._ebss)
        lea     _sbss,a0
        lea     _ebss,a1
.zero_bss:
        cmpa.l  a1,a0
        bge.s   .zero_done
        clr.l   (a0)+
        bra.s   .zero_bss
.zero_done:

; Run C++ global/static constructors (__init_array_start..__init_array_end)
        lea     __init_array_start,a2
        lea     __init_array_end,a3
.init_loop:
        cmpa.l  a3,a2
        bge.s   .init_done
        movea.l (a2)+,a0
        jsr     (a0)
        bra.s   .init_loop
.init_done:

        move.w  #$2000,sr                       ; Enable interrupts (Supervisor mode, IPL 0)
        jsr     main
.halt:
        stop    #$2000
        bra.s   .halt

_vblank_handler:
        movem.l d0-d7/a0-a6,-(sp)
        jsr     _ZN4cps29on_vblankEv
        movem.l (sp)+,d0-d7/a0-a6
        rte

_irq_ignore:
        rte

        weak    _ZN4cps29on_vblankEv
_ZN4cps29on_vblankEv:
        rts
