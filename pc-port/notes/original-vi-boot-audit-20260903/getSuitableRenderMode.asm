
build/original-vi-boot-audit-20260903/getSuitableRenderMode.o:	file format elf32-powerpc

Disassembly of section .data:

803a7274 <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_start>:
803a7274: 94 21 ff e0  	stwu 1, -32(1)
803a7278: 7c 08 02 a6  	mflr 0
803a727c: 90 01 00 24  	stw 0, 36(1)
803a7280: 39 61 00 20  	addi 11, 1, 32
803a7284: 48 17 17 85  	bl 0x80518a08 <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_end+0x171660>
803a7288: 3f e0 80 54  	lis 31, -32684
803a728c: 3b a0 00 00  	li 29, 0
803a7290: 3b ff b8 e0  	addi 31, 31, -18208
803a7294: 48 12 a9 45  	bl 0x804d1bd8 <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_end+0x12a830>
803a7298: 54 63 06 3e  	clrlwi	3, 3, 24
803a729c: 38 03 ff ff  	addi 0, 3, -1
803a72a0: 7c 00 00 34  	cntlzw	0, 0
803a72a4: 54 1e d9 7e  	srwi 30, 0, 5
803a72a8: 48 10 eb c5  	bl 0x804b5e6c <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_end+0x10eac4>
803a72ac: 28 03 00 01  	cmplwi	3, 1
803a72b0: 40 82 00 68  	bf	2, 0x803a7318 <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_start+0xa4>
803a72b4: 48 12 aa b9  	bl 0x804d1d6c <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_end+0x12a9c4>
803a72b8: 54 63 06 3e  	clrlwi	3, 3, 24
803a72bc: 38 03 ff ff  	addi 0, 3, -1
803a72c0: 7c 00 00 34  	cntlzw	0, 0
803a72c4: 54 00 d9 7f  	rlwinm. 0, 0, 27, 5, 31
803a72c8: 41 82 00 50  	bt	2, 0x803a7318 <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_start+0xa4>
803a72cc: 48 10 eb 41  	bl 0x804b5e0c <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_end+0x10ea64>
803a72d0: 2c 03 00 02  	cmpwi	3, 2
803a72d4: 41 82 00 24  	bt	2, 0x803a72f8 <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_start+0x84>
803a72d8: 40 80 00 14  	bf	0, 0x803a72ec <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_start+0x78>
803a72dc: 2c 03 00 00  	cmpwi	3, 0
803a72e0: 41 82 00 18  	bt	2, 0x803a72f8 <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_start+0x84>
803a72e4: 40 80 00 24  	bf	0, 0x803a7308 <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_start+0x94>
803a72e8: 48 00 00 a4  	b 0x803a738c <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_start+0x118>
803a72ec: 2c 03 00 05  	cmpwi	3, 5
803a72f0: 41 82 00 18  	bt	2, 0x803a7308 <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_start+0x94>
803a72f4: 48 00 00 98  	b 0x803a738c <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_start+0x118>
803a72f8: 1c 7e 00 3c  	mulli 3, 30, 60
803a72fc: 38 1f 00 78  	addi 0, 31, 120
803a7300: 7f a0 1a 14  	add 29, 0, 3
803a7304: 48 00 00 88  	b 0x803a738c <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_start+0x118>
803a7308: 1c 7e 00 3c  	mulli 3, 30, 60
803a730c: 38 1f 01 e0  	addi 0, 31, 480
803a7310: 7f a0 1a 14  	add 29, 0, 3
803a7314: 48 00 00 78  	b 0x803a738c <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_start+0x118>
803a7318: 48 10 ea f5  	bl 0x804b5e0c <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_end+0x10ea64>
803a731c: 2c 03 00 02  	cmpwi	3, 2
803a7320: 41 82 00 24  	bt	2, 0x803a7344 <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_start+0xd0>
803a7324: 40 80 00 14  	bf	0, 0x803a7338 <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_start+0xc4>
803a7328: 2c 03 00 00  	cmpwi	3, 0
803a732c: 41 82 00 18  	bt	2, 0x803a7344 <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_start+0xd0>
803a7330: 40 80 00 24  	bf	0, 0x803a7354 <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_start+0xe0>
803a7334: 48 00 00 58  	b 0x803a738c <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_start+0x118>
803a7338: 2c 03 00 05  	cmpwi	3, 5
803a733c: 41 82 00 40  	bt	2, 0x803a737c <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_start+0x108>
803a7340: 48 00 00 4c  	b 0x803a738c <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_start+0x118>
803a7344: 1c 7e 00 3c  	mulli 3, 30, 60
803a7348: 38 1f 00 00  	addi 0, 31, 0
803a734c: 7c 60 1a 14  	add 3, 0, 3
803a7350: 48 00 00 40  	b 0x803a7390 <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_start+0x11c>
803a7354: 48 12 a9 4d  	bl 0x804d1ca0 <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_end+0x12a8f8>
803a7358: 54 63 06 3e  	clrlwi	3, 3, 24
803a735c: 38 03 ff ff  	addi 0, 3, -1
803a7360: 7c 00 00 34  	cntlzw	0, 0
803a7364: 54 00 d9 7f  	rlwinm. 0, 0, 27, 5, 31
803a7368: 40 82 00 14  	bf	2, 0x803a737c <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_start+0x108>
803a736c: 1c 7e 00 3c  	mulli 3, 30, 60
803a7370: 38 1f 00 f0  	addi 0, 31, 240
803a7374: 7c 60 1a 14  	add 3, 0, 3
803a7378: 48 00 00 18  	b 0x803a7390 <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_start+0x11c>
803a737c: 1c 7e 00 3c  	mulli 3, 30, 60
803a7380: 38 1f 01 68  	addi 0, 31, 360
803a7384: 7c 60 1a 14  	add 3, 0, 3
803a7388: 48 00 00 08  	b 0x803a7390 <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_start+0x11c>
803a738c: 7f a3 eb 78  	mr	3, 29
803a7390: 39 61 00 20  	addi 11, 1, 32
803a7394: 48 17 16 c1  	bl 0x80518a54 <_binary_build_original_vi_boot_audit_20260903_getSuitableRenderMode_bin_end+0x1716ac>
803a7398: 80 01 00 24  	lwz 0, 36(1)
803a739c: 7c 08 03 a6  	mtlr 0
803a73a0: 38 21 00 20  	addi 1, 1, 32
803a73a4: 4e 80 00 20  	blr
