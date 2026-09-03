
build/mario-shadow-view-20260903/drawModelBlur.o:	file format elf32-powerpc

Disassembly of section .data:

802b77e0 <_binary_build_mario_shadow_view_20260903_drawModelBlur_bin_start>:
802b77e0: 94 21 ff a0  	stwu 1, -96(1)
802b77e4: 7c 08 02 a6  	mflr 0
802b77e8: 90 01 00 64  	stw 0, 100(1)
802b77ec: 39 61 00 60  	addi 11, 1, 96
802b77f0: 48 26 12 05  	bl 0x805189f4 _savegpr_24 <_binary_build_mario_shadow_view_20260903_drawModelBlur_bin_end+0x261070>
802b77f4: 7c 7b 1b 78  	mr	27, 3
802b77f8: 4b ff f1 79  	bl 0x802b6970 isAllHidden__10MarioActorCFv <_binary_build_mario_shadow_view_20260903_drawModelBlur_bin_end+0xffffffffffffefec>
802b77fc: 2c 03 00 00  	cmpwi	3, 0
802b7800: 40 82 01 6c  	bf	2, 0x802b796c <_binary_build_mario_shadow_view_20260903_drawModelBlur_bin_start+0x18c>
802b7804: 88 1b 0a 0a  	lbz 0, 2570(27)
802b7808: 54 00 10 3a  	slwi 0, 0, 2
802b780c: 7c 7b 02 14  	add 3, 27, 0
802b7810: 83 e3 0a 28  	lwz 31, 2600(3)
802b7814: 88 1f 01 e4  	lbz 0, 484(31)
802b7818: 2c 00 00 00  	cmpwi	0, 0
802b781c: 40 82 00 10  	bf	2, 0x802b782c <_binary_build_mario_shadow_view_20260903_drawModelBlur_bin_start+0x4c>
802b7820: 38 00 00 01  	li 0, 1
802b7824: 98 1f 01 e5  	stb 0, 485(31)
802b7828: 48 00 01 44  	b 0x802b796c <_binary_build_mario_shadow_view_20260903_drawModelBlur_bin_start+0x18c>
802b782c: 88 1b 0a 6e  	lbz 0, 2670(27)
802b7830: 38 60 00 00  	li 3, 0
802b7834: 98 7f 01 e4  	stb 3, 484(31)
802b7838: 2c 00 00 00  	cmpwi	0, 0
802b783c: 41 82 01 30  	bt	2, 0x802b796c <_binary_build_mario_shadow_view_20260903_drawModelBlur_bin_start+0x18c>
802b7840: 80 1f 01 b0  	lwz 0, 432(31)
802b7844: 38 7b 0a b0  	addi 3, 27, 2736
802b7848: 38 81 00 08  	addi 4, 1, 8
802b784c: 60 00 10 00  	ori 0, 0, 4096
802b7850: 90 1f 01 b0  	stw 0, 432(31)
802b7854: 48 20 0c 39  	bl 0x804b848c PSMTXInverse <_binary_build_mario_shadow_view_20260903_drawModelBlur_bin_end+0x200b08>
802b7858: 48 11 11 b9  	bl 0x803c8a10 getCameraViewMtx__2MRFv <_binary_build_mario_shadow_view_20260903_drawModelBlur_bin_end+0x11108c>
802b785c: 38 81 00 08  	addi 4, 1, 8
802b7860: 7c 85 23 78  	mr	5, 4
802b7864: 48 20 0b 5d  	bl 0x804b83c0 PSMTXConcat <_binary_build_mario_shadow_view_20260903_drawModelBlur_bin_end+0x200a3c>
802b7868: 3b a0 00 01  	li 29, 1
802b786c: a0 9b 0b 12  	lhz 4, 2834(27)
802b7870: 7f e3 fb 78  	mr	3, 31
802b7874: a0 1b 0b 10  	lhz 0, 2832(27)
802b7878: 7c bd 22 14  	add 5, 29, 4
802b787c: 54 be 16 fa  	rlwinm 30, 5, 2, 27, 29
802b7880: 54 04 28 34  	slwi 4, 0, 5
802b7884: 7c 1e da 14  	add 0, 30, 27
802b7888: 54 bc 07 7e  	clrlwi	28, 5, 29
802b788c: 7c 84 02 14  	add 4, 4, 0
802b7890: 80 84 0a 70  	lwz 4, 2672(4)
802b7894: 4b fe f2 b5  	bl 0x802a6b48 setDrawViewBuffer__9J3DModelXFPA4_f <_binary_build_mario_shadow_view_20260903_drawModelBlur_bin_end+0xfffffffffffef1c4>
802b7898: 88 1b 01 c1  	lbz 0, 449(27)
802b789c: 2c 00 00 00  	cmpwi	0, 0
802b78a0: 40 82 00 60  	bf	2, 0x802b7900 <_binary_build_mario_shadow_view_20260903_drawModelBlur_bin_start+0x120>
802b78a4: 7f 3b f2 14  	add 25, 27, 30
802b78a8: 3b 00 00 00  	li 24, 0
802b78ac: 3b 40 00 00  	li 26, 0
802b78b0: 48 00 00 3c  	b 0x802b78ec <_binary_build_mario_shadow_view_20260903_drawModelBlur_bin_start+0x10c>
802b78b4: a0 1b 0b 10  	lhz 0, 2832(27)
802b78b8: 38 61 00 08  	addi 3, 1, 8
802b78bc: 20 80 00 01  	subfic 4, 0, 1
802b78c0: 54 00 28 34  	slwi 0, 0, 5
802b78c4: 54 84 28 34  	slwi 4, 4, 5
802b78c8: 7c b9 22 14  	add 5, 25, 4
802b78cc: 7c 99 02 14  	add 4, 25, 0
802b78d0: 80 a5 0a 70  	lwz 5, 2672(5)
802b78d4: 80 04 0a 70  	lwz 0, 2672(4)
802b78d8: 7c 85 d2 14  	add 4, 5, 26
802b78dc: 7c a0 d2 14  	add 5, 0, 26
802b78e0: 48 20 0a e1  	bl 0x804b83c0 PSMTXConcat <_binary_build_mario_shadow_view_20260903_drawModelBlur_bin_end+0x200a3c>
802b78e4: 3b 18 00 01  	addi 24, 24, 1
802b78e8: 3b 5a 00 30  	addi 26, 26, 48
802b78ec: 7f 63 db 78  	mr	3, 27
802b78f0: 4b ff f9 49  	bl 0x802b7238 getModelData__10MarioActorCFv <_binary_build_mario_shadow_view_20260903_drawModelBlur_bin_end+0xfffffffffffff8b4>
802b78f4: a0 03 00 44  	lhz 0, 68(3)
802b78f8: 7c 18 00 40  	cmplw	24, 0
802b78fc: 41 80 ff b8  	bt	0, 0x802b78b4 <_binary_build_mario_shadow_view_20260903_drawModelBlur_bin_start+0xd4>
802b7900: 7f 63 db 78  	mr	3, 27
802b7904: 4b ff f9 35  	bl 0x802b7238 getModelData__10MarioActorCFv <_binary_build_mario_shadow_view_20260903_drawModelBlur_bin_end+0xfffffffffffff8b4>
802b7908: a0 9b 0b 10  	lhz 4, 2832(27)
802b790c: a0 03 00 44  	lhz 0, 68(3)
802b7910: 7c 7e da 14  	add 3, 30, 27
802b7914: 54 84 28 34  	slwi 4, 4, 5
802b7918: 7c 64 1a 14  	add 3, 4, 3
802b791c: 1c 80 00 30  	mulli 4, 0, 48
802b7920: 80 63 0a 70  	lwz 3, 2672(3)
802b7924: 48 1e d8 6d  	bl 0x804a5190 DCStoreRange <_binary_build_mario_shadow_view_20260903_drawModelBlur_bin_end+0x1ed80c>
802b7928: 38 1d ff ff  	addi 0, 29, -1
802b792c: 7f e4 fb 78  	mr	4, 31
802b7930: 54 00 18 38  	slwi 0, 0, 3
802b7934: 7c 1c 02 14  	add 0, 28, 0
802b7938: 54 00 10 3a  	slwi 0, 0, 2
802b793c: 7c 7b 02 14  	add 3, 27, 0
802b7940: 80 63 00 94  	lwz 3, 148(3)
802b7944: 48 00 03 29  	bl 0x802b7c6c addDL__9DLchangerFP9J3DModelX <_binary_build_mario_shadow_view_20260903_drawModelBlur_bin_end+0x2e8>
802b7948: 7f e3 fb 78  	mr	3, 31
802b794c: 38 80 00 00  	li 4, 0
802b7950: 4b fe f7 99  	bl 0x802a70e8 directDraw__9J3DModelXFP8J3DModel <_binary_build_mario_shadow_view_20260903_drawModelBlur_bin_end+0xfffffffffffef764>
802b7954: 3b bd 00 01  	addi 29, 29, 1
802b7958: 28 1d 00 08  	cmplwi	29, 8
802b795c: 41 80 ff 10  	bt	0, 0x802b786c <_binary_build_mario_shadow_view_20260903_drawModelBlur_bin_start+0x8c>
802b7960: 80 1f 01 b0  	lwz 0, 432(31)
802b7964: 54 00 05 24  	rlwinm 0, 0, 0, 20, 18
802b7968: 90 1f 01 b0  	stw 0, 432(31)
802b796c: 39 61 00 60  	addi 11, 1, 96
802b7970: 48 26 10 d1  	bl 0x80518a40 _restgpr_24 <_binary_build_mario_shadow_view_20260903_drawModelBlur_bin_end+0x2610bc>
802b7974: 80 01 00 64  	lwz 0, 100(1)
802b7978: 7c 08 03 a6  	mtlr 0
802b797c: 38 21 00 60  	addi 1, 1, 96
802b7980: 4e 80 00 20  	blr
