
build/mario-random-texture-20260903/updateRandomTexture.o:	file format elf32-powerpc

Disassembly of section .data:

802c2bfc <_binary_build_mario_random_texture_20260903_updateRandomTexture_bin_start>:
802c2bfc: 94 21 ff d0  	stwu 1, -48(1)
802c2c00: 7c 08 02 a6  	mflr 0
802c2c04: 90 01 00 34  	stw 0, 52(1)
802c2c08: db e1 00 20  	stfd 31, 32(1)
802c2c0c: f3 e1 00 28  	<unknown>
802c2c10: 39 61 00 20  	addi 11, 1, 32
802c2c14: 48 25 5d e9  	bl 0x805189fc <_binary_build_mario_random_texture_20260903_updateRandomTexture_bin_end+0x255cf0>
802c2c18: c0 02 fb ec  	lfs 0, -1044(2)
802c2c1c: a0 03 0b 88  	lhz 0, 2952(3)
802c2c20: ec 21 00 24  	fdivs 1, 1, 0
802c2c24: c0 02 fb c8  	lfs 0, -1080(2)
802c2c28: 20 80 00 01  	subfic 4, 0, 1
802c2c2c: c3 e2 fb cc  	lfs 31, -1076(2)
802c2c30: 54 80 13 ba  	rlwinm 0, 4, 2, 14, 29
802c2c34: b0 83 0b 88  	sth 4, 2952(3)
802c2c38: ec 20 08 28  	fsubs 1, 0, 1
802c2c3c: 7c 63 02 14  	add 3, 3, 0
802c2c40: 80 63 0b 80  	lwz 3, 2944(3)
802c2c44: fc 01 f8 40  	fcmpo 0, 1, 31
802c2c48: 83 a3 00 24  	lwz 29, 36(3)
802c2c4c: 40 80 00 08  	bf	0, 0x802c2c54 <_binary_build_mario_random_texture_20260903_updateRandomTexture_bin_start+0x58>
802c2c50: 48 00 00 18  	b 0x802c2c68 <_binary_build_mario_random_texture_20260903_updateRandomTexture_bin_start+0x6c>
802c2c54: fc 01 00 40  	fcmpo 0, 1, 0
802c2c58: 40 81 00 0c  	bf	1, 0x802c2c64 <_binary_build_mario_random_texture_20260903_updateRandomTexture_bin_start+0x68>
802c2c5c: ff e0 00 90  	fmr 31, 0
802c2c60: 48 00 00 08  	b 0x802c2c68 <_binary_build_mario_random_texture_20260903_updateRandomTexture_bin_start+0x6c>
802c2c64: ff e0 08 90  	fmr 31, 1
802c2c68: 3b 80 00 00  	li 28, 0
802c2c6c: 3b e0 00 00  	li 31, 0
802c2c70: 7f dd fa 14  	add 30, 29, 31
802c2c74: 3b 60 00 00  	li 27, 0
802c2c78: 88 1e 00 00  	lbz 0, 0(30)
802c2c7c: 7c 1a 26 70  	srawi 26, 0, 4
802c2c80: 48 12 13 cd  	bl 0x803e404c <_binary_build_mario_random_texture_20260903_updateRandomTexture_bin_end+0x121340>
802c2c84: fc 01 f8 40  	fcmpo 0, 1, 31
802c2c88: 40 80 00 0c  	bf	0, 0x802c2c94 <_binary_build_mario_random_texture_20260903_updateRandomTexture_bin_start+0x98>
802c2c8c: 3b 5a 00 04  	addi 26, 26, 4
802c2c90: 48 00 00 08  	b 0x802c2c98 <_binary_build_mario_random_texture_20260903_updateRandomTexture_bin_start+0x9c>
802c2c94: 3b 5a ff ff  	addi 26, 26, -1
802c2c98: 2c 1a 00 00  	cmpwi	26, 0
802c2c9c: 40 80 00 0c  	bf	0, 0x802c2ca8 <_binary_build_mario_random_texture_20260903_updateRandomTexture_bin_start+0xac>
802c2ca0: 38 00 00 00  	li 0, 0
802c2ca4: 48 00 00 14  	b 0x802c2cb8 <_binary_build_mario_random_texture_20260903_updateRandomTexture_bin_start+0xbc>
802c2ca8: 2c 1a 00 0f  	cmpwi	26, 15
802c2cac: 38 00 00 0f  	li 0, 15
802c2cb0: 41 81 00 08  	bt	1, 0x802c2cb8 <_binary_build_mario_random_texture_20260903_updateRandomTexture_bin_start+0xbc>
802c2cb4: 7f 40 d3 78  	mr	0, 26
802c2cb8: 3b 7b 00 01  	addi 27, 27, 1
802c2cbc: 54 00 26 36  	rlwinm 0, 0, 4, 24, 27
802c2cc0: 28 1b 00 08  	cmplwi	27, 8
802c2cc4: 98 1e 00 00  	stb 0, 0(30)
802c2cc8: 3b de 00 01  	addi 30, 30, 1
802c2ccc: 41 80 ff ac  	bt	0, 0x802c2c78 <_binary_build_mario_random_texture_20260903_updateRandomTexture_bin_start+0x7c>
802c2cd0: 3b 9c 00 01  	addi 28, 28, 1
802c2cd4: 3b ff 00 08  	addi 31, 31, 8
802c2cd8: 28 1c 00 08  	cmplwi	28, 8
802c2cdc: 41 80 ff 94  	bt	0, 0x802c2c70 <_binary_build_mario_random_texture_20260903_updateRandomTexture_bin_start+0x74>
802c2ce0: 7f a3 eb 78  	mr	3, 29
802c2ce4: 38 80 00 40  	li 4, 64
802c2ce8: 48 1e 24 a9  	bl 0x804a5190 <_binary_build_mario_random_texture_20260903_updateRandomTexture_bin_end+0x1e2484>
802c2cec: e3 e1 00 28  	<unknown>
802c2cf0: 39 61 00 20  	addi 11, 1, 32
802c2cf4: cb e1 00 20  	lfd 31, 32(1)
802c2cf8: 48 25 5d 51  	bl 0x80518a48 <_binary_build_mario_random_texture_20260903_updateRandomTexture_bin_end+0x255d3c>
802c2cfc: 80 01 00 34  	lwz 0, 52(1)
802c2d00: 7c 08 03 a6  	mtlr 0
802c2d04: 38 21 00 30  	addi 1, 1, 48
802c2d08: 4e 80 00 20  	blr
