
build/original-effect-system-20260903/AutoFind.o:	file format elf32-powerpc

Disassembly of section .data:

800c4cc4 <_binary_build_original_effect_system_20260903_AutoFind_bin_start>:
800c4cc4: 94 21 ff e0  	stwu 1, -32(1)
800c4cc8: 7c 08 02 a6  	mflr 0
800c4ccc: 90 01 00 24  	stw 0, 36(1)
800c4cd0: 39 61 00 20  	addi 11, 1, 32
800c4cd4: 48 45 3d 35  	bl 0x80518a08 <_binary_build_original_effect_system_20260903_AutoFind_bin_end+0x453cdc>
800c4cd8: 7c 7d 1b 78  	mr	29, 3
800c4cdc: 7c 9e 23 78  	mr	30, 4
800c4ce0: 7c bf 2b 78  	mr	31, 5
800c4ce4: 48 00 00 08  	b 0x800c4cec <_binary_build_original_effect_system_20260903_AutoFind_bin_start+0x28>
800c4ce8: 3b bd 00 04  	addi 29, 29, 4
800c4cec: 7c 1d f0 40  	cmplw	29, 30
800c4cf0: 41 82 00 20  	bt	2, 0x800c4d10 <_binary_build_original_effect_system_20260903_AutoFind_bin_start+0x4c>
800c4cf4: 80 7d 00 00  	lwz 3, 0(29)
800c4cf8: 80 9f 00 00  	lwz 4, 0(31)
800c4cfc: 80 63 00 00  	lwz 3, 0(3)
800c4d00: 48 33 9e 65  	bl 0x803feb64 <_binary_build_original_effect_system_20260903_AutoFind_bin_end+0x339e38>
800c4d04: 7c 60 00 34  	cntlzw	0, 3
800c4d08: 54 00 d9 7f  	rlwinm. 0, 0, 27, 5, 31
800c4d0c: 41 82 ff dc  	bt	2, 0x800c4ce8 <_binary_build_original_effect_system_20260903_AutoFind_bin_start+0x24>
800c4d10: 39 61 00 20  	addi 11, 1, 32
800c4d14: 7f a3 eb 78  	mr	3, 29
800c4d18: 48 45 3d 3d  	bl 0x80518a54 <_binary_build_original_effect_system_20260903_AutoFind_bin_end+0x453d28>
800c4d1c: 80 01 00 24  	lwz 0, 36(1)
800c4d20: 7c 08 03 a6  	mtlr 0
800c4d24: 38 21 00 20  	addi 1, 1, 32
800c4d28: 4e 80 00 20  	blr
