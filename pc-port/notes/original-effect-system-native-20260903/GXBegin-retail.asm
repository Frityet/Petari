
pc-port/notes/original-effect-system-native-20260903/GXBegin-retail.o:	file format elf32-powerpc

Disassembly of section .data:

804bcb30 <_binary_pc_port_notes_original_effect_system_native_20260903_GXBegin_retail_bin_start>:
804bcb30: 94 21 ff e0  	stwu 1, -32(1)
804bcb34: 7c 08 02 a6  	mflr 0
804bcb38: 90 01 00 24  	stw 0, 36(1)
804bcb3c: 93 e1 00 1c  	stw 31, 28(1)
804bcb40: 83 e2 25 18  	lwz 31, 9496(2)
804bcb44: 93 c1 00 18  	stw 30, 24(1)
804bcb48: 7c be 2b 78  	mr	30, 5
804bcb4c: 93 a1 00 14  	stw 29, 20(1)
804bcb50: 7c 9d 23 78  	mr	29, 4
804bcb54: 93 81 00 10  	stw 28, 16(1)
804bcb58: 7c 7c 1b 78  	mr	28, 3
804bcb5c: 80 1f 05 fc  	lwz 0, 1532(31)
804bcb60: 2c 00 00 00  	cmpwi	0, 0
804bcb64: 41 82 00 08  	bt	2, 0x804bcb6c <_binary_pc_port_notes_original_effect_system_native_20260903_GXBegin_retail_bin_start+0x3c>
804bcb68: 4b ff fd 51  	bl 0x804bc8b8 <_binary_pc_port_notes_original_effect_system_native_20260903_GXBegin_retail_bin_end+0xfffffffffffffc3c>
804bcb6c: 80 1f 00 00  	lwz 0, 0(31)
804bcb70: 2c 00 00 00  	cmpwi	0, 0
804bcb74: 40 82 00 d8  	bf	2, 0x804bcc4c <_binary_pc_port_notes_original_effect_system_native_20260903_GXBegin_retail_bin_start+0x11c>
804bcb78: 80 e2 25 18  	lwz 7, 9496(2)
804bcb7c: 3c 60 cc 01  	lis 3, -13311
804bcb80: 38 00 00 98  	li 0, 152
804bcb84: 38 c0 00 00  	li 6, 0
804bcb88: a0 a7 00 04  	lhz 5, 4(7)
804bcb8c: a0 87 00 06  	lhz 4, 6(7)
804bcb90: 98 03 80 00  	stb 0, -32768(3)
804bcb94: 7c a5 21 d7  	mullw. 5, 5, 4
804bcb98: a0 07 00 04  	lhz 0, 4(7)
804bcb9c: b0 03 80 00  	sth 0, -32768(3)
804bcba0: 41 82 00 a4  	bt	2, 0x804bcc44 <_binary_pc_port_notes_original_effect_system_native_20260903_GXBegin_retail_bin_start+0x114>
804bcba4: 38 65 00 03  	addi 3, 5, 3
804bcba8: 38 e5 ff e0  	addi 7, 5, -32
804bcbac: 54 60 f0 be  	srwi 0, 3, 2
804bcbb0: 28 00 00 08  	cmplwi	0, 8
804bcbb4: 40 81 00 68  	bf	1, 0x804bcc1c <_binary_pc_port_notes_original_effect_system_native_20260903_GXBegin_retail_bin_start+0xec>
804bcbb8: 28 03 00 03  	cmplwi	3, 3
804bcbbc: 38 00 00 00  	li 0, 0
804bcbc0: 41 80 00 10  	bt	0, 0x804bcbd0 <_binary_pc_port_notes_original_effect_system_native_20260903_GXBegin_retail_bin_start+0xa0>
804bcbc4: 7c 05 18 40  	cmplw	5, 3
804bcbc8: 41 81 00 08  	bt	1, 0x804bcbd0 <_binary_pc_port_notes_original_effect_system_native_20260903_GXBegin_retail_bin_start+0xa0>
804bcbcc: 38 00 00 01  	li 0, 1
804bcbd0: 2c 00 00 00  	cmpwi	0, 0
804bcbd4: 41 82 00 48  	bt	2, 0x804bcc1c <_binary_pc_port_notes_original_effect_system_native_20260903_GXBegin_retail_bin_start+0xec>
804bcbd8: 38 07 00 1f  	addi 0, 7, 31
804bcbdc: 38 80 00 00  	li 4, 0
804bcbe0: 54 00 d9 7e  	srwi 0, 0, 5
804bcbe4: 3c 60 cc 01  	lis 3, -13311
804bcbe8: 7c 09 03 a6  	mtctr 0
804bcbec: 28 07 00 00  	cmplwi	7, 0
804bcbf0: 40 81 00 2c  	bf	1, 0x804bcc1c <_binary_pc_port_notes_original_effect_system_native_20260903_GXBegin_retail_bin_start+0xec>
804bcbf4: 90 83 80 00  	stw 4, -32768(3)
804bcbf8: 38 c6 00 20  	addi 6, 6, 32
804bcbfc: 90 83 80 00  	stw 4, -32768(3)
804bcc00: 90 83 80 00  	stw 4, -32768(3)
804bcc04: 90 83 80 00  	stw 4, -32768(3)
804bcc08: 90 83 80 00  	stw 4, -32768(3)
804bcc0c: 90 83 80 00  	stw 4, -32768(3)
804bcc10: 90 83 80 00  	stw 4, -32768(3)
804bcc14: 90 83 80 00  	stw 4, -32768(3)
804bcc18: 42 00 ff dc  	bdnz 0x804bcbf4 <_binary_pc_port_notes_original_effect_system_native_20260903_GXBegin_retail_bin_start+0xc4>
804bcc1c: 38 05 00 03  	addi 0, 5, 3
804bcc20: 38 80 00 00  	li 4, 0
804bcc24: 7c 06 00 50  	sub	0, 0, 6
804bcc28: 3c 60 cc 01  	lis 3, -13311
804bcc2c: 54 00 f0 be  	srwi 0, 0, 2
804bcc30: 7c 09 03 a6  	mtctr 0
804bcc34: 7c 06 28 40  	cmplw	6, 5
804bcc38: 40 80 00 0c  	bf	0, 0x804bcc44 <_binary_pc_port_notes_original_effect_system_native_20260903_GXBegin_retail_bin_start+0x114>
804bcc3c: 90 83 80 00  	stw 4, -32768(3)
804bcc40: 42 00 ff fc  	bdnz 0x804bcc3c <_binary_pc_port_notes_original_effect_system_native_20260903_GXBegin_retail_bin_start+0x10c>
804bcc44: 38 00 00 01  	li 0, 1
804bcc48: b0 1f 00 02  	sth 0, 2(31)
804bcc4c: 3c 60 cc 01  	lis 3, -13311
804bcc50: 7f a0 e3 78  	or 0, 29, 28
804bcc54: 98 03 80 00  	stb 0, -32768(3)
804bcc58: b3 c3 80 00  	sth 30, -32768(3)
804bcc5c: 80 01 00 24  	lwz 0, 36(1)
804bcc60: 83 e1 00 1c  	lwz 31, 28(1)
804bcc64: 83 c1 00 18  	lwz 30, 24(1)
804bcc68: 83 a1 00 14  	lwz 29, 20(1)
804bcc6c: 83 81 00 10  	lwz 28, 16(1)
804bcc70: 7c 08 03 a6  	mtlr 0
804bcc74: 38 21 00 20  	addi 1, 1, 32
804bcc78: 4e 80 00 20  	blr
