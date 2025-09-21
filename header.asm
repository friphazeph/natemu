macro update_flags_CZVN {
	setc r14b    ; CF
	setz r13b    ; ZF
	seto r12b    ; OF
	sets r11b    ; SF

    ; CF/C
	and r15b, 11111110b
	or  r15b, r14b

	; ZF/Z
	and r15b, 11111101b
	shl r13b, 1
	or  r15b, r13b

	; OF/V
	and r15b, 10111111b
	shl r12b, 6
	or  r15b, r12b

	; SF/N
	and r15b, 01111111b
	shl r11b, 7
	or  r15b, r11b
}

macro update_flags_CZN {
	setc r14b    ; CF
	setz r13b    ; ZF
	sets r11b    ; SF

    ; CF/C
	and r15b, 11111110b
	or  r15b, r14b

	; ZF/Z
	and r15b, 11111101b
	shl r13b, 1
	or  r15b, r13b

	; SF/N
	and r15b, 01111111b
	shl r11b, 7
	or  r15b, r11b
}

macro update_flags_ZN {
	setz r13b    ; ZF
	sets r11b    ; SF

	; ZF/Z
	and r15b, 11111101b
	shl r13b, 1
	or  r15b, r13b

	; SF/N
	and r15b, 01111111b
	shl r11b, 7
	or  r15b, r11b
}

macro update_flags_C {
	setc r14b    ; CF

    ; CF/C
	and r15b, 11111110b
	or  r15b, r14b
}

macro update_flags_Z {
	setz r13b    ; ZF

	; ZF/Z
	and r15b, 11111101b
	shl r13b, 1
	or  r15b, r13b
}

macro load_flags_CZVN {
	; V
	bt r15, 6
	setc dh
	add dh, 127 ; this should overflow iff CF

	lahf
	; N
	bt r15, 7
	setc dh
	shl dh, 7
	or ah, dh

	; Z
	bt r15, 1
	setc dh
	shl dh, 6
	or ah, dh

	sahf ; load flags from ah (7=SF, 6=ZF, 4=AF, 2=PF, 0=CF)
	; restore CF from emulated flags
	bt r15, 0
}

macro load_flags_V {
	; V
	bt r15, 6
	setc dh
	add dh, 127 ; this should overflow iff CF
}

macro load_flags_N {
	lahf
	; N
	bt r15, 7
	setc dh
	shl dh, 7
	or ah, dh
	sahf ; load flags from ah (7=SF, 6=ZF, 4=AF, 2=PF, 0=CF)
}

macro load_flags_Z {
	lahf
	; Z
	bt r15, 1
	setc dh
	shl dh, 6
	or ah, dh

	sahf ; load flags from ah (7=SF, 6=ZF, 4=AF, 2=PF, 0=CF)
}

macro load_flags_C {
	bt r15, 0
}
