
build/original-vi-boot-audit-20260903/VIGetTvFormat.o:	file format elf32-powerpc

Disassembly of section .data:

804b5e0c <_binary_build_original_vi_boot_audit_20260903_VIGetTvFormat_bin_start>:
804b5e0c: 94 21 ff f0  	stwu 1, -16(1)
804b5e10: 7c 08 02 a6  	mflr 0
804b5e14: 90 01 00 14  	stw 0, 20(1)
804b5e18: 93 e1 00 0c  	stw 31, 12(1)
804b5e1c: 4b ff 39 5d  	bl 0x804a9778 <_binary_build_original_vi_boot_audit_20260903_VIGetTvFormat_bin_end+0xffffffffffff390c>
804b5e20: 83 ed e1 f0  	lwz 31, -7696(13)
804b5e24: 28 1f 00 08  	cmplwi	31, 8
804b5e28: 41 81 00 28  	bt	1, 0x804b5e50 <_binary_build_original_vi_boot_audit_20260903_VIGetTvFormat_bin_start+0x44>
804b5e2c: 3c 80 80 60  	lis 4, -32672
804b5e30: 57 e0 10 3a  	slwi 0, 31, 2
804b5e34: 38 84 d7 70  	addi 4, 4, -10384
804b5e38: 7c 84 00 2e  	lwzx 4, 4, 0
804b5e3c: 7c 89 03 a6  	mtctr 4
804b5e40: 4e 80 04 20  	bctr
804b5e44: 3b e0 00 00  	li 31, 0
804b5e48: 48 00 00 08  	b 0x804b5e50 <_binary_build_original_vi_boot_audit_20260903_VIGetTvFormat_bin_start+0x44>
804b5e4c: 3b e0 00 01  	li 31, 1
804b5e50: 4b ff 39 51  	bl 0x804a97a0 <_binary_build_original_vi_boot_audit_20260903_VIGetTvFormat_bin_end+0xffffffffffff3934>
804b5e54: 7f e3 fb 78  	mr	3, 31
804b5e58: 83 e1 00 0c  	lwz 31, 12(1)
804b5e5c: 80 01 00 14  	lwz 0, 20(1)
804b5e60: 7c 08 03 a6  	mtlr 0
804b5e64: 38 21 00 10  	addi 1, 1, 16
804b5e68: 4e 80 00 20  	blr
