
build/mario-shadow-view-20260903/calcViewBlurModel.o:	file format elf32-powerpc

Disassembly of section .data:

802c0ea4 <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_start>:
802c0ea4: 94 21 ff d0  	stwu 1, -48(1)
802c0ea8: 7c 08 02 a6  	mflr 0
802c0eac: 90 01 00 34  	stw 0, 52(1)
802c0eb0: 39 61 00 30  	addi 11, 1, 48
802c0eb4: 48 25 7b 45  	bl 0x805189f8 _savegpr_25 <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_end+0x25797c>
802c0eb8: 7c 7e 1b 78  	mr	30, 3
802c0ebc: 4b ff 63 69  	bl 0x802b7224 getJ3DModel__10MarioActorCFv <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_end+0xffffffffffff61a8>
802c0ec0: 88 03 01 e5  	lbz 0, 485(3)
802c0ec4: 7c 7f 1b 78  	mr	31, 3
802c0ec8: 2c 00 00 00  	cmpwi	0, 0
802c0ecc: 41 82 00 20  	bt	2, 0x802c0eec <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_start+0x48>
802c0ed0: 88 1e 0a 6e  	lbz 0, 2670(30)
802c0ed4: 28 00 00 01  	cmplwi	0, 1
802c0ed8: 40 82 00 0c  	bf	2, 0x802c0ee4 <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_start+0x40>
802c0edc: 38 00 00 02  	li 0, 2
802c0ee0: 98 1e 0a 6e  	stb 0, 2670(30)
802c0ee4: 38 00 00 00  	li 0, 0
802c0ee8: 98 03 01 e5  	stb 0, 485(3)
802c0eec: 48 10 a5 dd  	bl 0x803cb4c8 isDemoActive__2MRFv <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_end+0x10a44c>
802c0ef0: 2c 03 00 00  	cmpwi	3, 0
802c0ef4: 41 82 00 1c  	bt	2, 0x802c0f10 <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_start+0x6c>
802c0ef8: 88 1e 0a 6e  	lbz 0, 2670(30)
802c0efc: 2c 00 00 00  	cmpwi	0, 0
802c0f00: 41 82 01 64  	bt	2, 0x802c1064 <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_start+0x1c0>
802c0f04: 38 00 00 05  	li 0, 5
802c0f08: 98 1e 0a 6e  	stb 0, 2670(30)
802c0f0c: 48 00 01 58  	b 0x802c1064 <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_start+0x1c0>
802c0f10: 88 1e 01 c1  	lbz 0, 449(30)
802c0f14: 2c 00 00 00  	cmpwi	0, 0
802c0f18: 40 82 01 4c  	bf	2, 0x802c1064 <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_start+0x1c0>
802c0f1c: 88 7e 0a 6e  	lbz 3, 2670(30)
802c0f20: 2c 03 00 00  	cmpwi	3, 0
802c0f24: 41 82 01 40  	bt	2, 0x802c1064 <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_start+0x1c0>
802c0f28: 28 03 00 03  	cmplwi	3, 3
802c0f2c: 41 80 00 10  	bt	0, 0x802c0f3c <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_start+0x98>
802c0f30: 38 03 ff ff  	addi 0, 3, -1
802c0f34: 98 1e 0a 6e  	stb 0, 2670(30)
802c0f38: 48 00 01 2c  	b 0x802c1064 <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_start+0x1c0>
802c0f3c: 28 03 00 02  	cmplwi	3, 2
802c0f40: 40 82 00 8c  	bf	2, 0x802c0fcc <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_start+0x128>
802c0f44: 3b 40 00 00  	li 26, 0
802c0f48: 3b a0 00 00  	li 29, 0
802c0f4c: 7f 7e ea 14  	add 27, 30, 29
802c0f50: 3b 20 00 00  	li 25, 0
802c0f54: 3b 80 00 00  	li 28, 0
802c0f58: 48 00 00 3c  	b 0x802c0f94 <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_start+0xf0>
802c0f5c: 7f e3 fb 78  	mr	3, 31
802c0f60: 7f 24 cb 78  	mr	4, 25
802c0f64: 48 00 04 01  	bl 0x802c1364 getDrawMtx__8J3DModelFi <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_end+0x2e8>
802c0f68: 80 1b 0a 70  	lwz 0, 2672(27)
802c0f6c: 7c 80 e2 14  	add 4, 0, 28
802c0f70: 48 1f 74 1d  	bl 0x804b838c PSMTXCopy <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_end+0x1f7310>
802c0f74: 7f e3 fb 78  	mr	3, 31
802c0f78: 7f 24 cb 78  	mr	4, 25
802c0f7c: 48 00 03 e9  	bl 0x802c1364 getDrawMtx__8J3DModelFi <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_end+0x2e8>
802c0f80: 80 1b 0a 90  	lwz 0, 2704(27)
802c0f84: 7c 80 e2 14  	add 4, 0, 28
802c0f88: 48 1f 74 05  	bl 0x804b838c PSMTXCopy <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_end+0x1f7310>
802c0f8c: 3b 39 00 01  	addi 25, 25, 1
802c0f90: 3b 9c 00 30  	addi 28, 28, 48
802c0f94: 7f c3 f3 78  	mr	3, 30
802c0f98: 4b ff 62 a1  	bl 0x802b7238 getModelData__10MarioActorCFv <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_end+0xffffffffffff61bc>
802c0f9c: a0 03 00 44  	lhz 0, 68(3)
802c0fa0: 7c 19 00 40  	cmplw	25, 0
802c0fa4: 41 80 ff b8  	bt	0, 0x802c0f5c <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_start+0xb8>
802c0fa8: 3b 5a 00 01  	addi 26, 26, 1
802c0fac: 3b bd 00 04  	addi 29, 29, 4
802c0fb0: 28 1a 00 08  	cmplwi	26, 8
802c0fb4: 41 80 ff 98  	bt	0, 0x802c0f4c <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_start+0xa8>
802c0fb8: 38 60 00 00  	li 3, 0
802c0fbc: 38 00 00 01  	li 0, 1
802c0fc0: b0 7e 0b 12  	sth 3, 2834(30)
802c0fc4: 98 1e 0a 6e  	stb 0, 2670(30)
802c0fc8: 48 00 00 88  	b 0x802c1050 <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_start+0x1ac>
802c0fcc: 80 9e 03 7c  	lwz 4, 892(30)
802c0fd0: 38 60 00 03  	li 3, 3
802c0fd4: 7c 04 1b 96  	divwu 0, 4, 3
802c0fd8: 7c 00 19 d6  	mullw 0, 0, 3
802c0fdc: 7c 00 20 51  	sub.	0, 4, 0
802c0fe0: 40 82 00 70  	bf	2, 0x802c1050 <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_start+0x1ac>
802c0fe4: a0 7e 0b 12  	lhz 3, 2834(30)
802c0fe8: 3b 20 00 00  	li 25, 0
802c0fec: 3b a0 00 00  	li 29, 0
802c0ff0: 38 03 00 01  	addi 0, 3, 1
802c0ff4: 54 00 07 7e  	clrlwi	0, 0, 29
802c0ff8: b0 1e 0b 12  	sth 0, 2834(30)
802c0ffc: 48 00 00 40  	b 0x802c103c <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_start+0x198>
802c1000: 7f e3 fb 78  	mr	3, 31
802c1004: 7f 24 cb 78  	mr	4, 25
802c1008: 48 00 03 5d  	bl 0x802c1364 getDrawMtx__8J3DModelFi <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_end+0x2e8>
802c100c: a0 9e 0b 10  	lhz 4, 2832(30)
802c1010: a0 1e 0b 12  	lhz 0, 2834(30)
802c1014: 20 84 00 01  	subfic 4, 4, 1
802c1018: 54 85 28 34  	slwi 5, 4, 5
802c101c: 54 04 10 3a  	slwi 4, 0, 2
802c1020: 7c 1e 2a 14  	add 0, 30, 5
802c1024: 7c 84 02 14  	add 4, 4, 0
802c1028: 80 04 0a 70  	lwz 0, 2672(4)
802c102c: 7c 80 ea 14  	add 4, 0, 29
802c1030: 48 1f 73 5d  	bl 0x804b838c PSMTXCopy <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_end+0x1f7310>
802c1034: 3b 39 00 01  	addi 25, 25, 1
802c1038: 3b bd 00 30  	addi 29, 29, 48
802c103c: 7f c3 f3 78  	mr	3, 30
802c1040: 4b ff 61 f9  	bl 0x802b7238 getModelData__10MarioActorCFv <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_end+0xffffffffffff61bc>
802c1044: a0 03 00 44  	lhz 0, 68(3)
802c1048: 7c 19 00 40  	cmplw	25, 0
802c104c: 41 80 ff b4  	bt	0, 0x802c1000 <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_start+0x15c>
802c1050: a0 7e 0b 10  	lhz 3, 2832(30)
802c1054: 38 00 00 01  	li 0, 1
802c1058: 20 63 00 01  	subfic 3, 3, 1
802c105c: b0 7e 0b 10  	sth 3, 2832(30)
802c1060: 98 1f 01 e4  	stb 0, 484(31)
802c1064: 39 61 00 30  	addi 11, 1, 48
802c1068: 48 25 79 dd  	bl 0x80518a44 _restgpr_25 <_binary_build_mario_shadow_view_20260903_calcViewBlurModel_bin_end+0x2579c8>
802c106c: 80 01 00 34  	lwz 0, 52(1)
802c1070: 7c 08 03 a6  	mtlr 0
802c1074: 38 21 00 30  	addi 1, 1, 48
802c1078: 4e 80 00 20  	blr
