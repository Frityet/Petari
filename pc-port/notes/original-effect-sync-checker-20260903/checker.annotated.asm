
build/original-effect-sync-checker-20260903/checker.o:	file format elf32-powerpc

Disassembly of section .data:

800cb670 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start>:
800cb670: c0 02 a1 68  	lfs 0, -24216(2)
800cb674: 38 00 00 00  	li 0, 0
800cb678: 90 83 00 00  	stw 4, 0(3)
800cb67c: d0 03 00 04  	stfs 0, 4(3)
800cb680: 98 03 00 08  	stb 0, 8(3)
800cb684: 90 03 00 0c  	stw 0, 12(3)
800cb688: 90 03 00 10  	stw 0, 16(3)
800cb68c: 4e 80 00 20  	blr
800cb690: 94 21 ff f0  	stwu 1, -16(1)
800cb694: 7c 08 02 a6  	mflr 0
800cb698: 90 01 00 14  	stw 0, 20(1)
800cb69c: 93 e1 00 0c  	stw 31, 12(1)
800cb6a0: 7c 7f 1b 78  	mr	31, 3
800cb6a4: 80 a3 00 00  	lwz 5, 0(3)
800cb6a8: 88 05 00 54  	lbz 0, 84(5)
800cb6ac: 1c 00 00 18  	mulli 0, 0, 24
800cb6b0: 7c 85 02 14  	add 4, 5, 0
800cb6b4: 88 04 00 29  	lbz 0, 41(4)
800cb6b8: 54 00 07 ff  	clrlwi.	0, 0, 31
800cb6bc: 40 82 00 34  	bf	2, 0x800cb6f0 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x80>
800cb6c0: 80 85 00 20  	lwz 4, 32(5)
800cb6c4: 38 00 00 01  	li 0, 1
800cb6c8: c0 02 a1 68  	lfs 0, -24216(2)
800cb6cc: c0 24 00 0c  	lfs 1, 12(4)
800cb6d0: fc 00 08 00  	fcmpu 0, 0, 1
800cb6d4: 40 82 00 20  	bf	2, 0x800cb6f4 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x84>
800cb6d8: c0 24 00 10  	lfs 1, 16(4)
800cb6dc: c0 03 00 04  	lfs 0, 4(3)
800cb6e0: fc 00 08 00  	fcmpu 0, 0, 1
800cb6e4: 40 82 00 10  	bf	2, 0x800cb6f4 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x84>
800cb6e8: 38 00 00 00  	li 0, 0
800cb6ec: 48 00 00 08  	b 0x800cb6f4 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x84>
800cb6f0: 38 00 00 00  	li 0, 0
800cb6f4: 2c 00 00 00  	cmpwi	0, 0
800cb6f8: 41 82 00 10  	bt	2, 0x800cb708 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x98>
800cb6fc: 7c a3 2b 78  	mr	3, 5
800cb700: 4b f5 0b c9  	bl 0x8001c2c8 <_binary_build_original_effect_sync_checker_20260903_checker_bin_end+0xfffffffffff50844>  # getCurrentBckName__12XanimePlayerCFv
800cb704: 48 00 00 08  	b 0x800cb70c <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x9c>
800cb708: 38 60 00 00  	li 3, 0
800cb70c: 90 7f 00 0c  	stw 3, 12(31)
800cb710: 83 e1 00 0c  	lwz 31, 12(1)
800cb714: 80 01 00 14  	lwz 0, 20(1)
800cb718: 7c 08 03 a6  	mtlr 0
800cb71c: 38 21 00 10  	addi 1, 1, 16
800cb720: 4e 80 00 20  	blr
800cb724: 80 a3 00 0c  	lwz 5, 12(3)
800cb728: 38 00 00 00  	li 0, 0
800cb72c: 98 03 00 08  	stb 0, 8(3)
800cb730: 80 83 00 00  	lwz 4, 0(3)
800cb734: 90 a3 00 10  	stw 5, 16(3)
800cb738: 80 84 00 20  	lwz 4, 32(4)
800cb73c: c0 04 00 10  	lfs 0, 16(4)
800cb740: d0 03 00 04  	stfs 0, 4(3)
800cb744: 4e 80 00 20  	blr
800cb748: c0 02 a1 68  	lfs 0, -24216(2)
800cb74c: 38 00 00 01  	li 0, 1
800cb750: 98 03 00 08  	stb 0, 8(3)
800cb754: d0 03 00 04  	stfs 0, 4(3)
800cb758: 4e 80 00 20  	blr
800cb75c: 94 21 ff d0  	stwu 1, -48(1)
800cb760: 7c 08 02 a6  	mflr 0
800cb764: 90 01 00 34  	stw 0, 52(1)
800cb768: db e1 00 20  	stfd 31, 32(1)
800cb76c: f3 e1 00 28  	<unknown>
800cb770: 39 61 00 20  	addi 11, 1, 32
800cb774: 48 44 d2 95  	bl 0x80518a08 <_binary_build_original_effect_sync_checker_20260903_checker_bin_end+0x44cf84>  # _savegpr_29
800cb778: 7c 9e 23 78  	mr	30, 4
800cb77c: 80 83 00 0c  	lwz 4, 12(3)
800cb780: 7c 7d 1b 78  	mr	29, 3
800cb784: 7c bf 2b 78  	mr	31, 5
800cb788: 7f c3 f3 78  	mr	3, 30
800cb78c: 48 00 04 4d  	bl 0x800cbbd8 <_binary_build_original_effect_sync_checker_20260903_checker_bin_end+0x154>  # isRegisteredBck__17SyncBckEffectInfoCFPCc
800cb790: 2c 03 00 00  	cmpwi	3, 0
800cb794: 40 82 00 0c  	bf	2, 0x800cb7a0 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x130>
800cb798: 38 60 00 00  	li 3, 0
800cb79c: 48 00 00 88  	b 0x800cb824 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x1b4>
800cb7a0: 2c 1f 00 00  	cmpwi	31, 0
800cb7a4: 40 82 00 0c  	bf	2, 0x800cb7b0 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x140>
800cb7a8: 38 60 00 01  	li 3, 1
800cb7ac: 48 00 00 78  	b 0x800cb824 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x1b4>
800cb7b0: 80 9d 00 0c  	lwz 4, 12(29)
800cb7b4: 7f c3 f3 78  	mr	3, 30
800cb7b8: 48 00 04 21  	bl 0x800cbbd8 <_binary_build_original_effect_sync_checker_20260903_checker_bin_end+0x154>  # isRegisteredBck__17SyncBckEffectInfoCFPCc
800cb7bc: 2c 03 00 00  	cmpwi	3, 0
800cb7c0: 40 82 00 0c  	bf	2, 0x800cb7cc <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x15c>
800cb7c4: 38 60 00 00  	li 3, 0
800cb7c8: 48 00 00 5c  	b 0x800cb824 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x1b4>
800cb7cc: 88 1d 00 08  	lbz 0, 8(29)
800cb7d0: 80 7d 00 00  	lwz 3, 0(29)
800cb7d4: 2c 00 00 00  	cmpwi	0, 0
800cb7d8: c3 fe 00 0c  	lfs 31, 12(30)
800cb7dc: 80 63 00 20  	lwz 3, 32(3)
800cb7e0: 41 82 00 38  	bt	2, 0x800cb818 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x1a8>
800cb7e4: c0 23 00 0c  	lfs 1, 12(3)
800cb7e8: c0 02 a1 68  	lfs 0, -24216(2)
800cb7ec: fc 00 08 00  	fcmpu 0, 0, 1
800cb7f0: 41 82 00 28  	bt	2, 0x800cb818 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x1a8>
800cb7f4: ec 1f 08 2a  	fadds 0, 31, 1
800cb7f8: c0 23 00 10  	lfs 1, 16(3)
800cb7fc: c0 42 a1 6c  	lfs 2, -24212(2)
800cb800: ec 20 08 28  	fsubs 1, 0, 1
800cb804: 48 31 b7 bd  	bl 0x803e6fc0 <_binary_build_original_effect_sync_checker_20260903_checker_bin_end+0x31b53c>  # isNearZero__2MRFff
800cb808: 2c 03 00 00  	cmpwi	3, 0
800cb80c: 41 82 00 0c  	bt	2, 0x800cb818 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x1a8>
800cb810: 38 60 00 01  	li 3, 1
800cb814: 48 00 00 10  	b 0x800cb824 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x1b4>
800cb818: fc 20 f8 90  	fmr 1, 31
800cb81c: 7f a3 eb 78  	mr	3, 29
800cb820: 48 00 01 21  	bl 0x800cb940 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x2d0>  # checkPass__20SyncBckEffectCheckerCFf
800cb824: e3 e1 00 28  	<unknown>
800cb828: 39 61 00 20  	addi 11, 1, 32
800cb82c: cb e1 00 20  	lfd 31, 32(1)
800cb830: 48 44 d2 25  	bl 0x80518a54 <_binary_build_original_effect_sync_checker_20260903_checker_bin_end+0x44cfd0>  # _restgpr_29
800cb834: 80 01 00 34  	lwz 0, 52(1)
800cb838: 7c 08 03 a6  	mtlr 0
800cb83c: 38 21 00 30  	addi 1, 1, 48
800cb840: 4e 80 00 20  	blr
800cb844: 94 21 ff e0  	stwu 1, -32(1)
800cb848: 7c 08 02 a6  	mflr 0
800cb84c: 90 01 00 24  	stw 0, 36(1)
800cb850: 39 61 00 20  	addi 11, 1, 32
800cb854: 48 44 d1 b5  	bl 0x80518a08 <_binary_build_original_effect_sync_checker_20260903_checker_bin_end+0x44cf84>  # _savegpr_29
800cb858: 7c 9e 23 78  	mr	30, 4
800cb85c: 80 83 00 0c  	lwz 4, 12(3)
800cb860: 7c 7d 1b 78  	mr	29, 3
800cb864: 7f c3 f3 78  	mr	3, 30
800cb868: 48 00 03 71  	bl 0x800cbbd8 <_binary_build_original_effect_sync_checker_20260903_checker_bin_end+0x154>  # isRegisteredBck__17SyncBckEffectInfoCFPCc
800cb86c: 2c 03 00 00  	cmpwi	3, 0
800cb870: 40 82 00 94  	bf	2, 0x800cb904 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x294>
800cb874: 80 9d 00 0c  	lwz 4, 12(29)
800cb878: 7f c3 f3 78  	mr	3, 30
800cb87c: 48 00 03 e9  	bl 0x800cbc64 <_binary_build_original_effect_sync_checker_20260903_checker_bin_end+0x1e0>  # isBckLoop__17SyncBckEffectInfoCFPCc
800cb880: 2c 03 00 00  	cmpwi	3, 0
800cb884: 41 82 00 1c  	bt	2, 0x800cb8a0 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x230>
800cb888: 80 7d 00 0c  	lwz 3, 12(29)
800cb88c: 80 1d 00 10  	lwz 0, 16(29)
800cb890: 7c 63 00 50  	sub	3, 0, 3
800cb894: 30 03 ff ff  	addic 0, 3, -1
800cb898: 7c 60 19 10  	subfe 3, 0, 3
800cb89c: 48 00 00 8c  	b 0x800cb928 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x2b8>
800cb8a0: 88 1e 00 14  	lbz 0, 20(30)
800cb8a4: 2c 00 00 00  	cmpwi	0, 0
800cb8a8: 41 82 00 44  	bt	2, 0x800cb8ec <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x27c>
800cb8ac: 80 7d 00 00  	lwz 3, 0(29)
800cb8b0: 4b f5 0a 19  	bl 0x8001c2c8 <_binary_build_original_effect_sync_checker_20260903_checker_bin_end+0xfffffffffff50844>  # getCurrentBckName__12XanimePlayerCFv
800cb8b4: 2c 03 00 00  	cmpwi	3, 0
800cb8b8: 7c 7f 1b 78  	mr	31, 3
800cb8bc: 40 82 00 0c  	bf	2, 0x800cb8c8 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x258>
800cb8c0: 38 60 00 00  	li 3, 0
800cb8c4: 48 00 00 64  	b 0x800cb928 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x2b8>
800cb8c8: 7f c3 f3 78  	mr	3, 30
800cb8cc: 7f e4 fb 78  	mr	4, 31
800cb8d0: 48 00 03 09  	bl 0x800cbbd8 <_binary_build_original_effect_sync_checker_20260903_checker_bin_end+0x154>  # isRegisteredBck__17SyncBckEffectInfoCFPCc
800cb8d4: 2c 03 00 00  	cmpwi	3, 0
800cb8d8: 40 82 00 2c  	bf	2, 0x800cb904 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x294>
800cb8dc: 80 7d 00 00  	lwz 3, 0(29)
800cb8e0: 7f e4 fb 78  	mr	4, 31
800cb8e4: 4b f5 07 e9  	bl 0x8001c0cc <_binary_build_original_effect_sync_checker_20260903_checker_bin_end+0xfffffffffff50648>  # isTerminate__12XanimePlayerCFPCc
800cb8e8: 48 00 00 40  	b 0x800cb928 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x2b8>
800cb8ec: 80 7d 00 0c  	lwz 3, 12(29)
800cb8f0: 80 1d 00 10  	lwz 0, 16(29)
800cb8f4: 7c 63 00 50  	sub	3, 0, 3
800cb8f8: 30 03 ff ff  	addic 0, 3, -1
800cb8fc: 7c 60 19 10  	subfe 3, 0, 3
800cb900: 48 00 00 28  	b 0x800cb928 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x2b8>
800cb904: 7f c3 f3 78  	mr	3, 30
800cb908: 48 00 04 01  	bl 0x800cbd08 <_binary_build_original_effect_sync_checker_20260903_checker_bin_end+0x284>  # isExistSyncBckDeleteFrame__Q22MR6EffectFPC17SyncBckEffectInfo
800cb90c: 2c 03 00 00  	cmpwi	3, 0
800cb910: 40 82 00 0c  	bf	2, 0x800cb91c <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x2ac>
800cb914: 38 60 00 00  	li 3, 0
800cb918: 48 00 00 10  	b 0x800cb928 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x2b8>
800cb91c: c0 3e 00 10  	lfs 1, 16(30)
800cb920: 7f a3 eb 78  	mr	3, 29
800cb924: 48 00 00 1d  	bl 0x800cb940 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x2d0>  # checkPass__20SyncBckEffectCheckerCFf
800cb928: 39 61 00 20  	addi 11, 1, 32
800cb92c: 48 44 d1 29  	bl 0x80518a54 <_binary_build_original_effect_sync_checker_20260903_checker_bin_end+0x44cfd0>  # _restgpr_29
800cb930: 80 01 00 24  	lwz 0, 36(1)
800cb934: 7c 08 03 a6  	mtlr 0
800cb938: 38 21 00 20  	addi 1, 1, 32
800cb93c: 4e 80 00 20  	blr
800cb940: 94 21 ff f0  	stwu 1, -16(1)
800cb944: 7c 08 02 a6  	mflr 0
800cb948: 80 a3 00 00  	lwz 5, 0(3)
800cb94c: 90 01 00 14  	stw 0, 20(1)
800cb950: c0 02 a1 68  	lfs 0, -24216(2)
800cb954: 80 85 00 20  	lwz 4, 32(5)
800cb958: c0 44 00 0c  	lfs 2, 12(4)
800cb95c: fc 00 10 00  	fcmpu 0, 0, 2
800cb960: 40 82 00 0c  	bf	2, 0x800cb96c <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x2fc>
800cb964: 48 00 00 2d  	bl 0x800cb990 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x320>  # checkPassIfRate0__20SyncBckEffectCheckerCFf
800cb968: 48 00 00 18  	b 0x800cb980 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x310>
800cb96c: 7c a3 2b 78  	mr	3, 5
800cb970: 4b f5 09 99  	bl 0x8001c308 <_binary_build_original_effect_sync_checker_20260903_checker_bin_end+0xfffffffffff50884>  # checkPass__12XanimePlayerCFf
800cb974: 38 03 ff ff  	addi 0, 3, -1
800cb978: 7c 00 00 34  	cntlzw	0, 0
800cb97c: 54 03 d9 7e  	srwi 3, 0, 5
800cb980: 80 01 00 14  	lwz 0, 20(1)
800cb984: 7c 08 03 a6  	mtlr 0
800cb988: 38 21 00 10  	addi 1, 1, 16
800cb98c: 4e 80 00 20  	blr
800cb990: 94 21 ff f0  	stwu 1, -16(1)
800cb994: 80 83 00 00  	lwz 4, 0(3)
800cb998: 80 a4 00 20  	lwz 5, 32(4)
800cb99c: 88 05 00 04  	lbz 0, 4(5)
800cb9a0: c0 65 00 10  	lfs 3, 16(5)
800cb9a4: 28 00 00 02  	cmplwi	0, 2
800cb9a8: 40 82 00 88  	bf	2, 0x800cba30 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x3c0>
800cb9ac: c0 03 00 04  	lfs 0, 4(3)
800cb9b0: fc 03 00 40  	fcmpo 0, 3, 0
800cb9b4: 40 80 00 7c  	bf	0, 0x800cba30 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x3c0>
800cb9b8: fc 00 08 40  	fcmpo 0, 0, 1
800cb9bc: 4c 40 13 82  	cror 2, 0, 2
800cb9c0: 40 82 00 30  	bf	2, 0x800cb9f0 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x380>
800cb9c4: a8 85 00 08  	lha 4, 8(5)
800cb9c8: 3c 00 43 30  	lis 0, 17200
800cb9cc: 90 01 00 08  	stw 0, 8(1)
800cb9d0: 3c 60 80 53  	lis 3, -32685
800cb9d4: 6c 80 80 00  	xoris 0, 4, 32768
800cb9d8: c8 43 1b d0  	lfd 2, 7120(3)
800cb9dc: 90 01 00 0c  	stw 0, 12(1)
800cb9e0: c8 01 00 08  	lfd 0, 8(1)
800cb9e4: ec 00 10 28  	fsubs 0, 0, 2
800cb9e8: fc 01 00 40  	fcmpo 0, 1, 0
800cb9ec: 41 80 00 3c  	bt	0, 0x800cba28 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x3b8>
800cb9f0: a8 85 00 0a  	lha 4, 10(5)
800cb9f4: 3c 00 43 30  	lis 0, 17200
800cb9f8: 90 01 00 08  	stw 0, 8(1)
800cb9fc: 3c 60 80 53  	lis 3, -32685
800cba00: 6c 80 80 00  	xoris 0, 4, 32768
800cba04: c8 43 1b d0  	lfd 2, 7120(3)
800cba08: 90 01 00 0c  	stw 0, 12(1)
800cba0c: c8 01 00 08  	lfd 0, 8(1)
800cba10: ec 00 10 28  	fsubs 0, 0, 2
800cba14: fc 00 08 40  	fcmpo 0, 0, 1
800cba18: 4c 40 13 82  	cror 2, 0, 2
800cba1c: 40 82 00 5c  	bf	2, 0x800cba78 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x408>
800cba20: fc 01 18 40  	fcmpo 0, 1, 3
800cba24: 40 80 00 54  	bf	0, 0x800cba78 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x408>
800cba28: 38 60 00 01  	li 3, 1
800cba2c: 48 00 00 50  	b 0x800cba7c <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x40c>
800cba30: c0 03 00 04  	lfs 0, 4(3)
800cba34: fc 00 18 40  	fcmpo 0, 0, 3
800cba38: 4c 40 13 82  	cror 2, 0, 2
800cba3c: 40 82 00 20  	bf	2, 0x800cba5c <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x3ec>
800cba40: fc 00 08 40  	fcmpo 0, 0, 1
800cba44: 4c 40 13 82  	cror 2, 0, 2
800cba48: 40 82 00 30  	bf	2, 0x800cba78 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x408>
800cba4c: fc 01 18 40  	fcmpo 0, 1, 3
800cba50: 40 80 00 28  	bf	0, 0x800cba78 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x408>
800cba54: 38 60 00 01  	li 3, 1
800cba58: 48 00 00 24  	b 0x800cba7c <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x40c>
800cba5c: fc 03 08 40  	fcmpo 0, 3, 1
800cba60: 4c 40 13 82  	cror 2, 0, 2
800cba64: 40 82 00 14  	bf	2, 0x800cba78 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x408>
800cba68: fc 01 00 40  	fcmpo 0, 1, 0
800cba6c: 40 80 00 0c  	bf	0, 0x800cba78 <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x408>
800cba70: 38 60 00 01  	li 3, 1
800cba74: 48 00 00 08  	b 0x800cba7c <_binary_build_original_effect_sync_checker_20260903_checker_bin_start+0x40c>
800cba78: 38 60 00 00  	li 3, 0
800cba7c: 38 21 00 10  	addi 1, 1, 16
800cba80: 4e 80 00 20  	blr
