; ==============================================================================
; cps2-sdk Default 16-Channel QSound Z80 Driver
; ==============================================================================
; Z80 Memory Map on CPS2:
;   0x0000..0x0037 : Reset entry & boot init
;   0x0038..0x013f : Mode 1 IRQ handler (252 Hz timer: BGM streaming + fade-out)
;   0x0140..0x03ff : Main command loop & QSound helpers
;   0x0400..0x07ff : Sample table (128 entries x 16 bytes, populated by cps2_packer)
;                    +0..1: startAddr, +2..3: endAddr, +4..5: loopLen
;                    +6..7: pitch, +8..9: bank, +10: chunkCount
;                    +11..12: lastEndAddr, +13..14: lastTicks
;   0xc000..0xcfff : 68000 Shared RAM (0x618000..0x619fff odd bytes)
;                    0xc000 = Command strobe, 0xc002 = cmd hi, 0xc003 = cmd lo
;                    0xc004 = Param (pan), 0xc008 = Master vol (0..0x7f)
;   0xd000..0xd002 : QSound DSP write ports (0xd000=hi, 0xd001=lo, 0xd002=reg)
;   0xf000..0xffff : Z80 private work RAM
;                    0xf000 = Next round-robin SFX channel (1..15)
;                    0xf001 = Paused flag (0 = running, 1 = paused)
;                    0xf002 = Fade speed (0 = off, >0 = IRQ ticks per volume step)
;                    0xf003 = Fade tick countdown
;                    0xf004 = Last applied master volume (0x00..0x7f)
;                    0xf005 = Active streaming BGM sample ID (0 = off)
;                    0xf006 = Current streaming chunk index (0..chunkCount-1)
;                    0xf007 = Total streaming chunkCount
;                    0xf008..0xf009 = 16-bit IRQ tick countdown for current chunk
;                    0xf010..0xf02f = Base volume per channel (16 x 2 bytes)
; ==============================================================================

        org     0x0000

start:
        DI
        IM      1
        LD      sp, 0xFFF0
        JP      boot_init

; ------------------------------------------------------------------------------
; 252 Hz QSound IRQ (IM 1 vector at 0x0038) - BGM streaming + smooth fade-out
; ------------------------------------------------------------------------------
        org     0x0038

irq_handler:
        PUSH    AF
        PUSH    BC
        PUSH    DE
        PUSH    HL

        ; Skip streaming tick if paused (0xF001 != 0) or no streaming BGM (0xF005 == 0)
        LD      A, (0xF001)
        or      A
        JP      NZ, irq_fade_check

        LD      A, (0xF005)
        or      A
        JP      Z, irq_fade_check

        ; Decrement 16-bit chunk tick counter at (0xF008)
        LD      HL, (0xF008)
        DEC     HL
        LD      (0xF008), HL
        LD      A, H
        or      L
        JP      NZ, irq_fade_check

        ; Chunk timer expired: advance chunk index (0xF006)
        LD      A, (0xF006)
        INC     A
        LD      B, A
        LD      A, (0xF007)                     ; chunkCount
        CP      B
        JR      NZ, bgm_store_chunk
        LD      B, 0                            ; Wrap back to chunk 0 (loop song)

bgm_store_chunk:
        LD      A, B
        LD      (0xF006), A

        ; Look up sample table entry: HL = 0x0400 + (bgm_id * 16)
        LD      A, (0xF005)
        LD      L, A
        LD      H, 0
        add     HL,HL
        add     HL,HL
        add     HL,HL
        add     HL,HL
        LD      DE, 0x0400
        add     HL,DE

        ; Read startBank (+8, +9) into DE and add chunkIndex (B)
        LD      DE, 8
        add     HL,DE
        LD      D, (HL)
        INC     HL
        LD      E, (HL)
        INC     HL
        INC     HL                              ; HL now points to +11 (lastEndAddr)

        LD      A, E
        add     A,B
        LD      E, A

        ; Write new Bank to Channel 0 (Reg 0x00 on Channel 15)
        LD      C, 15
        LD      A, 0x00
        CALL    write_qsound_ch

        ; Check if B == chunkCount - 1 (final chunk uses lastEndAddr & lastTicks)
        LD      A, (0xF007)
        DEC     A
        CP      B
        JR      NZ, bgm_full_chunk

        ; Final chunk: endAddr = (+11, +12), ticks = (+13, +14)
        LD      D, (HL)
        INC     HL
        LD      E, (HL)
        INC     HL
        LD      C, 0
        LD      A, 0x05
        CALL    write_qsound_ch

        LD      D, (HL)
        INC     HL
        LD      E, (HL)
        EX      DE, HL
        LD      (0xF008), HL
        JR      bgm_retrigger

bgm_full_chunk:
        ; Full chunk: endAddr = 0x7CC1, ticks = 335 (0x014F)
        LD      DE, 0x7CC1
        LD      C, 0
        LD      A, 0x05
        CALL    write_qsound_ch

        LD      HL, 0x014F
        LD      (0xF008), HL

bgm_retrigger:
        ; Reset Channel 0 addr = 0x0000 (Reg 0x01) & phase = 0x0000 (Reg 0x03)
        LD      DE, 0x0000
        LD      C, 0
        LD      A, 0x01
        CALL    write_qsound_ch

        LD      DE, 0x0000
        LD      C, 0
        LD      A, 0x03
        CALL    write_qsound_ch

irq_fade_check:
        LD      A, (0xF002)                     ; Fade speed (0 = inactive)
        or      A
        JR      Z, irq_done

        LD      A, (0xF003)
        DEC     A
        LD      (0xF003), A
        JR      NZ, irq_done

        LD      A, (0xF002)
        LD      (0xF003), A

        LD      A, (0xC008)
        CP      3
        JR      C, fade_finished
        sub     2
        LD      (0xC008), A
        LD      (0xF004), A
        CALL    apply_master_vol
        JR      irq_done

fade_finished:
        XOR     A
        LD      (0xF002), A
        LD      (0xF005), A
        LD      (0xC008), A
        LD      (0xF004), A
        CALL    stop_all_channels

irq_done:
        POP     HL
        POP     DE
        POP     BC
        POP     AF
        EI
        RET

; ------------------------------------------------------------------------------
; Boot & Main Command Loop
; ------------------------------------------------------------------------------
        org     0x0140

boot_init:
        LD      A, 1
        LD      (0xF000), A                     ; Next SFX channel = 1
        XOR     A
        LD      (0xF001), A                     ; Paused = 0
        LD      (0xF002), A                     ; Fade = off
        LD      (0xF003), A
        LD      (0xF005), A                     ; Streaming BGM ID = 0 (off)

        LD      A, 0x7F
        LD      (0xC008), A                     ; Default master volume = 0x7F
        LD      (0xF004), A

        CALL    stop_all_channels

        XOR     A
        LD      (0xC000), A                     ; Signal Z80 ready to 68000
        EI

main_loop:
        ; Check for live master volume changes from 68000 (0xC008)
        LD      A, (0xC008)
        and     0x7F
        LD      B, A
        LD      A, (0xF004)
        CP      B
        JR      Z, check_cmd
        LD      A, B
        LD      (0xF004), A
        CALL    apply_master_vol

check_cmd:
        LD      A, (0xC000)
        or      A
        JR      Z, main_loop

        DI
        LD      A, (0xC002)
        CP      0xFF
        JP      Z, handle_sys_cmd

        LD      A, (0xC003)
        or      A
        JP      Z, cmd_done
        CP      0x80
        JR      NC, trigger_bgm

trigger_sfx:
        LD      A, (0xF000)
        LD      C, A
        INC     A
        CP      16
        JR      C, save_next_sfx
        LD      A, 1
save_next_sfx:
        LD      (0xF000), A

        LD      A, (0xC004)
        LD      B, A
        LD      A, (0xC003)
        CALL    play_sample
        JP      cmd_done

trigger_bgm:
        ; Cancel any active fade-out and restore full volume if faded to 0
        XOR     A
        LD      (0xF002), A
        LD      (0xF005), A
        LD      A, (0xC008)
        or      A
        JR      NZ, bgm_vol_ok
        LD      A, 0x7F
        LD      (0xC008), A
        LD      (0xF004), A
bgm_vol_ok:
        LD      C, 0
        LD      B, 0x10
        LD      A, (0xC003)
        and     0x7F
        PUSH    AF
        CALL    play_sample
        POP     AF

        ; Check if chunkCount (+10 in sample table) > 1 -> enable multi-bank streaming
        LD      L, A
        LD      H, 0
        add     HL,HL
        add     HL,HL
        add     HL,HL
        add     HL,HL
        LD      DE, 0x040A
        add     HL,DE
        LD      B, (HL)                         ; B = chunkCount
        LD      A, B
        CP      2
        JP      C, cmd_done

        LD      A, (0xC003)
        and     0x7F
        LD      (0xF005), A                     ; Active streaming BGM ID
        XOR     A
        LD      (0xF006), A                     ; chunkIndex = 0
        LD      A, B
        LD      (0xF007), A                     ; chunkCount
        LD      HL, 0x014F                      ; 335 IRQ ticks for chunk 0
        LD      (0xF008), HL
        JP      cmd_done

handle_sys_cmd:
        LD      A, (0xC003)
        CP      0x00                            ; 0xFF00 = StopAll
        JR      Z, do_stop_all
        CP      0x01                            ; 0xFF01 = FadeOut (param = speed)
        JR      Z, do_fade_out
        CP      0x02                            ; 0xFF02 = Pause
        JR      Z, do_pause
        CP      0x03                            ; 0xFF03 = Resume
        JR      Z, do_resume
        CP      0x04                            ; 0xFF04 = StopBgm (Channel 0 only)
        JR      Z, do_stop_bgm
        JR      cmd_done

do_stop_all:
        XOR     A
        LD      (0xF002), A
        LD      (0xF005), A
        CALL    stop_all_channels
        JR      cmd_done

do_fade_out:
        LD      A, (0xC004)
        or      A
        JR      NZ, start_fade
        LD      A, 4                            ; Default fade speed = 4
start_fade:
        LD      (0xF002), A
        LD      (0xF003), A
        JR      cmd_done

do_pause:
        LD      A, 1
        LD      (0xF001), A
        CALL    mute_all_hw
        JR      cmd_done

do_resume:
        XOR     A
        LD      (0xF001), A
        CALL    apply_master_vol
        JR      cmd_done

do_stop_bgm:
        XOR     A
        LD      (0xF005), A
        LD      C, 0
        CALL    stop_single_channel

cmd_done:
        XOR     A
        LD      (0xC000), A
        EI
        JP      main_loop

; ==============================================================================
; play_sample: A = sample ID (1..127), B = pan (0..0x20), C = channel (0..15)
; ==============================================================================
play_sample:
        LD      L, A
        LD      H, 0
        add     HL,HL                           ; * 2
        add     HL,HL                           ; * 4
        add     HL,HL                           ; * 8
        add     HL,HL                           ; * 16
        LD      DE, 0x0400
        add     HL,DE

        PUSH    BC

        ; Reg 0x01: Start Address (+0, +1)
        LD      D, (HL)
        INC     HL
        LD      E, (HL)
        INC     HL
        LD      A, 0x01
        CALL    write_qsound_ch

        ; Reg 0x05: End Address (+2, +3)
        LD      D, (HL)
        INC     HL
        LD      E, (HL)
        INC     HL
        LD      A, 0x05
        CALL    write_qsound_ch

        ; Reg 0x04: Loop Length (+4, +5)
        LD      D, (HL)
        INC     HL
        LD      E, (HL)
        INC     HL
        LD      A, 0x04
        CALL    write_qsound_ch

        ; Reg 0x02: Pitch / Step (+6, +7)
        LD      D, (HL)
        INC     HL
        LD      E, (HL)
        INC     HL
        LD      A, 0x02
        CALL    write_qsound_ch

        ; Reg 0x00 on ((C + 15) & 0x0F): Bank (+8, +9)
        LD      D, (HL)
        INC     HL
        LD      E, (HL)
        POP     BC
        PUSH    BC
        LD      A, C
        add     A,15
        and     0x0F
        LD      C, A
        LD      A, 0x00
        CALL    write_qsound_ch
        POP     BC
        PUSH    BC

        ; Reg 0x03: Phase / Key-on offset = 0x0000
        LD      DE, 0x0000
        LD      A, 0x03
        CALL    write_qsound_ch

        POP     BC

        ; Reg 0x80 + C: Pan = 0x0110 + B
        LD      D, 0x01
        LD      A, 0x10
        add     A,B
        LD      E, A
        LD      A, 0x80
        add     A,C
        CALL    write_qsound_raw

        ; Mark channel active (base volume = 0x0FE0) in work RAM (0xF010 + C * 2)
        LD      A, C
        add     A,A
        add     A,0x10
        LD      L, A
        LD      H, 0xF0
        LD      A, 0x0F
        LD      (HL), A
        INC     HL
        LD      A, 0xE0
        LD      (HL), A

        ; If paused, keep hardware muted
        LD      A, (0xF001)
        or      A
        RET     NZ

        CALL    calc_master_vol
        LD      A, 0x06
        CALL    write_qsound_ch
        RET

; ==============================================================================
; calc_master_vol: returns scaled volume [0x0000..0x0FE0] in DE from (0xC008)
; ==============================================================================
calc_master_vol:
        LD      A, (0xC008)
        and     0x7F
        LD      L, A
        LD      H, 0
        add     HL,HL                           ; * 2
        add     HL,HL                           ; * 4
        add     HL,HL                           ; * 8
        add     HL,HL                           ; * 16
        add     HL,HL                           ; * 32
        EX      DE, HL
        RET

; ==============================================================================
; apply_master_vol: updates hardware volume on all active channels (unless paused)
; ==============================================================================
apply_master_vol:
        LD      A, (0xF001)
        or      A
        RET     NZ

        CALL    calc_master_vol                 ; Scaled volume in DE
        LD      C, 0
vol_loop:
        LD      A, C
        add     A,A
        add     A,0x10
        LD      L, A
        LD      H, 0xF0
        LD      A, (HL)
        INC     HL
        or      (HL)
        JR      Z, vol_next                     ; Channel inactive (cached vol == 0)

        PUSH    DE
        LD      A, 0x06
        CALL    write_qsound_ch
        POP     DE
vol_next:
        INC     C
        LD      A, C
        CP      16
        JR      C, vol_loop
        RET

; ==============================================================================
; stop_single_channel: silences channel C and clears its active flag
; ==============================================================================
stop_single_channel:
        LD      A, C
        add     A,A
        add     A,0x10
        LD      L, A
        LD      H, 0xF0
        XOR     A
        LD      (HL), A
        INC     HL
        LD      (HL), A

        LD      DE, 0x0000
        LD      A, 0x06
        CALL    write_qsound_ch
        RET

; ==============================================================================
; stop_all_channels: silences all 16 QSound channels
; ==============================================================================
stop_all_channels:
        LD      C, 0
stop_loop:
        CALL    stop_single_channel
        INC     C
        LD      A, C
        CP      16
        JR      C, stop_loop
        RET

; ==============================================================================
; mute_all_hw: silences hardware volume without clearing active flags
; ==============================================================================
mute_all_hw:
        LD      C, 0
mute_loop:
        LD      DE, 0x0000
        LD      A, 0x06
        CALL    write_qsound_ch
        INC     C
        LD      A, C
        CP      16
        JR      C, mute_loop
        RET

; ==============================================================================
; write_qsound_ch / write_qsound_raw
; ==============================================================================
write_qsound_ch:
        PUSH    AF
        LD      A, C
        add     A,A
        add     A,A
        add     A,A
        LD      B, A
        POP     AF
        or      B

write_qsound_raw:
        PUSH    AF
        LD      A, D
        LD      (0xD000), A
        LD      A, E
        LD      (0xD001), A
        POP     AF
        LD      (0xD002), A
        RET
