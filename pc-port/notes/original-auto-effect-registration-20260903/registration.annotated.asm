
build/original-auto-effect-registration-20260903/registration.o:	file format elf32-powerpc

Disassembly of section .data:

800c5b1c <_binary_build_original_auto_effect_registration_20260903_registration_bin_start>:

# initEffectSyncBck__Q22MR6EffectFP12EffectKeeperPC12ModelManagerPCcPCclffb
800c5b1c: 80 84 00 18  	lwz 4, 24(4)
800c5b20: 48 09 cb f4  	b 0x80162714 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x9c4e8> # registerSyncBckEffect__12EffectKeeperFP12XanimePlayerPCcPCclffb

# addEffectSyncBck__Q22MR6EffectFP12MultiEmitterPC12ModelManagerPCc
800c5b24: 80 84 00 18  	lwz 4, 24(4)
800c5b28: 48 00 14 b8  	b 0x800c6fe0 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0xdb4> # addSyncBck__12MultiEmitterFPC12XanimePlayerPCc

# getAutoEffectNum__Q22MR6EffectFPCc
800c5b2c: 94 21 ff f0  	stwu 1, -16(1)
800c5b30: 7c 08 02 a6  	mflr 0
800c5b34: 90 01 00 14  	stw 0, 20(1)
800c5b38: 93 e1 00 0c  	stw 31, 12(1)
800c5b3c: 7c 7f 1b 78  	mr	31, 3
800c5b40: 48 33 97 15  	bl 0x803ff254 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x339028> # getParticleResourceHolder__2MRFv
800c5b44: 7f e4 fb 78  	mr	4, 31
800c5b48: 48 00 4d 5d  	bl 0x800ca8a4 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x4678> # getAutoEffectNum__22ParticleResourceHolderCFPCc
800c5b4c: 80 01 00 14  	lwz 0, 20(1)
800c5b50: 83 e1 00 0c  	lwz 31, 12(1)
800c5b54: 7c 08 03 a6  	mtlr 0
800c5b58: 38 21 00 10  	addi 1, 1, 16
800c5b5c: 4e 80 00 20  	blr

# getAutoEffectListBinary__Q22MR6EffectFv
800c5b60: 94 21 ff f0  	stwu 1, -16(1)
800c5b64: 7c 08 02 a6  	mflr 0
800c5b68: 90 01 00 14  	stw 0, 20(1)
800c5b6c: 48 33 96 e9  	bl 0x803ff254 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x339028> # getParticleResourceHolder__2MRFv
800c5b70: 48 00 4d 2d  	bl 0x800ca89c <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x4670> # getAutoEffectListBinary__22ParticleResourceHolderCFv
800c5b74: 80 01 00 14  	lwz 0, 20(1)
800c5b78: 7c 08 03 a6  	mtlr 0
800c5b7c: 38 21 00 10  	addi 1, 1, 16
800c5b80: 4e 80 00 20  	blr

# setupMultiEmitter__Q22MR6EffectFP12EffectKeeperPC12ModelManagerPC14AutoEffectInfo
800c5b84: 94 21 ff e0  	stwu 1, -32(1)
800c5b88: 7c 08 02 a6  	mflr 0
800c5b8c: 90 01 00 24  	stw 0, 36(1)
800c5b90: 39 61 00 20  	addi 11, 1, 32
800c5b94: 48 45 2e 71  	bl 0x80518a04 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x4527d8> # _savegpr_28
800c5b98: 7c be 2b 78  	mr	30, 5
800c5b9c: 7c 7c 1b 78  	mr	28, 3
800c5ba0: 7c 9d 23 78  	mr	29, 4
800c5ba4: 7f c3 f3 78  	mr	3, 30
800c5ba8: 4b ff f7 01  	bl 0x800c52a8 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0xfffffffffffff07c> # getName__14AutoEffectInfoCFv
800c5bac: 7c 64 1b 78  	mr	4, 3
800c5bb0: 7f 83 e3 78  	mr	3, 28
800c5bb4: 48 09 cd 5d  	bl 0x80162910 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x9c6e4> # getEmitter__12EffectKeeperCFPCc
800c5bb8: 7c 7f 1b 78  	mr	31, 3
800c5bbc: 7f 83 e3 78  	mr	3, 28
800c5bc0: 7f a4 eb 78  	mr	4, 29
800c5bc4: 7f c5 f3 78  	mr	5, 30
800c5bc8: 48 00 00 55  	bl 0x800c5c1c <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x100> # setupMultiEmitterSyncBck__Q22MR6EffectFP12EffectKeeperPC12ModelManagerPC14AutoEffectInfo
800c5bcc: 7f e3 fb 78  	mr	3, 31
800c5bd0: 7f c4 f3 78  	mr	4, 30
800c5bd4: 4b ff fd 39  	bl 0x800c590c <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0xfffffffffffff6e0> # setupMultiEmitter__30@unnamed@EffectSystemUtil_cpp@FP12MultiEmitterPC14AutoEffectInfo
800c5bd8: 4b ff fb f5  	bl 0x800c57cc <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0xfffffffffffff5a0> # getEffectSystem__2MRFv
800c5bdc: 7c 64 1b 78  	mr	4, 3
800c5be0: 7f e3 fb 78  	mr	3, 31
800c5be4: 48 00 12 c5  	bl 0x800c6ea8 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0xc7c> # scanParticleEmitter__12MultiEmitterFP12EffectSystem
800c5be8: 80 9e 00 10  	lwz 4, 16(30)
800c5bec: 2c 04 00 00  	cmpwi	4, 0
800c5bf0: 41 82 00 14  	bt	2, 0x800c5c04 <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0xe8>
800c5bf4: 7f 83 e3 78  	mr	3, 28
800c5bf8: 48 09 cd 19  	bl 0x80162910 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x9c6e4> # getEmitter__12EffectKeeperCFPCc
800c5bfc: 7f e4 fb 78  	mr	4, 31
800c5c00: 48 00 16 71  	bl 0x800c7270 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x1044> # addChildEmitter__12MultiEmitterFP12MultiEmitter
800c5c04: 39 61 00 20  	addi 11, 1, 32
800c5c08: 48 45 2e 49  	bl 0x80518a50 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x452824> # _restgpr_28
800c5c0c: 80 01 00 24  	lwz 0, 36(1)
800c5c10: 7c 08 03 a6  	mtlr 0
800c5c14: 38 21 00 20  	addi 1, 1, 32
800c5c18: 4e 80 00 20  	blr

# setupMultiEmitterSyncBck__Q22MR6EffectFP12EffectKeeperPC12ModelManagerPC14AutoEffectInfo
800c5c1c: 94 21 fe 90  	stwu 1, -368(1)
800c5c20: 7c 08 02 a6  	mflr 0
800c5c24: 90 01 01 74  	stw 0, 372(1)
800c5c28: db e1 01 60  	stfd 31, 352(1)
800c5c2c: f3 e1 01 68  	<unknown>
800c5c30: db c1 01 50  	stfd 30, 336(1)
800c5c34: f3 c1 01 58  	<unknown>
800c5c38: 39 61 01 50  	addi 11, 1, 336
800c5c3c: 48 45 2d a9  	bl 0x805189e4 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x4527b8> # _savegpr_20
800c5c40: 80 05 00 04  	lwz 0, 4(5)
800c5c44: 7c 7a 1b 78  	mr	26, 3
800c5c48: 7c 9b 23 78  	mr	27, 4
800c5c4c: 7c bc 2b 78  	mr	28, 5
800c5c50: 2c 00 00 00  	cmpwi	0, 0
800c5c54: 41 82 01 e0  	bt	2, 0x800c5e34 <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x318>
800c5c58: 7f 83 e3 78  	mr	3, 28
800c5c5c: 4b ff f6 4d  	bl 0x800c52a8 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0xfffffffffffff07c> # getName__14AutoEffectInfoCFv
800c5c60: 7c 64 1b 78  	mr	4, 3
800c5c64: 7f 43 d3 78  	mr	3, 26
800c5c68: 48 09 cc a9  	bl 0x80162910 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x9c6e4> # getEmitter__12EffectKeeperCFPCc
800c5c6c: 7c 7f 1b 78  	mr	31, 3
800c5c70: 80 7c 00 04  	lwz 3, 4(28)
800c5c74: 48 33 8f 6d  	bl 0x803febe0 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x3389b4> # hasStringSpace__2MRFPCc
800c5c78: 2c 03 00 00  	cmpwi	3, 0
800c5c7c: 40 82 00 74  	bf	2, 0x800c5cf0 <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x1d4>
800c5c80: 80 7c 00 3c  	lwz 3, 60(28)
800c5c84: 3c 80 43 30  	lis 4, 17200
800c5c88: 80 1c 00 38  	lwz 0, 56(28)
800c5c8c: 3c a0 80 53  	lis 5, -32685
800c5c90: 6c 63 80 00  	xoris 3, 3, 32768
800c5c94: 90 81 01 10  	stw 4, 272(1)
800c5c98: 6c 00 80 00  	xoris 0, 0, 32768
800c5c9c: c8 25 1a 90  	lfd 1, 6800(5)
800c5ca0: 90 61 01 14  	stw 3, 276(1)
800c5ca4: 7f 83 e3 78  	mr	3, 28
800c5ca8: 82 9c 00 04  	lwz 20, 4(28)
800c5cac: c8 01 01 10  	lfd 0, 272(1)
800c5cb0: 90 01 01 1c  	stw 0, 284(1)
800c5cb4: ef c0 08 28  	fsubs 30, 0, 1
800c5cb8: 90 81 01 18  	stw 4, 280(1)
800c5cbc: c8 01 01 18  	lfd 0, 280(1)
800c5cc0: ef e0 08 28  	fsubs 31, 0, 1
800c5cc4: 4b ff f5 e5  	bl 0x800c52a8 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0xfffffffffffff07c> # getName__14AutoEffectInfoCFv
800c5cc8: fc 20 f8 90  	fmr 1, 31
800c5ccc: 7c 65 1b 78  	mr	5, 3
800c5cd0: fc 40 f0 90  	fmr 2, 30
800c5cd4: 80 9b 00 18  	lwz 4, 24(27)
800c5cd8: 7f 43 d3 78  	mr	3, 26
800c5cdc: 7e 86 a3 78  	mr	6, 20
800c5ce0: 38 e0 00 01  	li 7, 1
800c5ce4: 39 00 00 00  	li 8, 0
800c5ce8: 48 09 ca 2d  	bl 0x80162714 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x9c4e8> # registerSyncBckEffect__12EffectKeeperFP12XanimePlayerPCcPCclffb
800c5cec: 48 00 01 2c  	b 0x800c5e18 <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x2fc>
800c5cf0: 83 dc 00 04  	lwz 30, 4(28)
800c5cf4: 38 61 00 0c  	addi 3, 1, 12
800c5cf8: 38 80 01 00  	li 4, 256
800c5cfc: 48 32 2f 8d  	bl 0x803e8c88 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x322a5c> # zeroMemory__2MRFPvUl
800c5d00: 7f d9 f3 78  	mr	25, 30
800c5d04: 3a e1 00 0c  	addi 23, 1, 12
800c5d08: 3a 80 00 00  	li 20, 0
800c5d0c: 3b a0 00 00  	li 29, 0
800c5d10: 3a c0 00 00  	li 22, 0
800c5d14: 48 00 00 f4  	b 0x800c5e08 <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x2ec>
800c5d18: 88 79 00 00  	lbz 3, 0(25)
800c5d1c: 7c 60 07 74  	extsb 0, 3
800c5d20: 2c 00 00 20  	cmpwi	0, 32
800c5d24: 41 82 00 0c  	bt	2, 0x800c5d30 <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x214>
800c5d28: 2c 00 00 00  	cmpwi	0, 0
800c5d2c: 40 82 00 cc  	bf	2, 0x800c5df8 <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x2dc>
800c5d30: 7e d7 a1 ae  	stbx 22, 23, 20
800c5d34: 7f 63 db 78  	mr	3, 27
800c5d38: 92 c1 00 08  	stw 22, 8(1)
800c5d3c: 48 0a 3e 55  	bl 0x80169b90 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0xa3964> # getResourceHolder__12ModelManagerCFv
800c5d40: 7c 64 1b 78  	mr	4, 3
800c5d44: 7e e5 bb 78  	mr	5, 23
800c5d48: 38 61 00 08  	addi 3, 1, 8
800c5d4c: 48 32 3b 35  	bl 0x803e9880 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x323654> # findBckNameStringInResource__2MRFPPCcPC14ResourceHolderPCc
800c5d50: 80 1f 00 24  	lwz 0, 36(31)
800c5d54: 2c 00 00 00  	cmpwi	0, 0
800c5d58: 40 82 00 7c  	bf	2, 0x800c5dd4 <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x2b8>
800c5d5c: 7f d8 f3 78  	mr	24, 30
800c5d60: 3a 80 00 00  	li 20, 0
800c5d64: 3a a0 00 00  	li 21, 0
800c5d68: 48 00 00 28  	b 0x800c5d90 <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x274>
800c5d6c: 88 18 00 00  	lbz 0, 0(24)
800c5d70: 7c 00 07 74  	extsb 0, 0
800c5d74: 2c 00 00 20  	cmpwi	0, 32
800c5d78: 41 82 00 0c  	bt	2, 0x800c5d84 <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x268>
800c5d7c: 2c 00 00 00  	cmpwi	0, 0
800c5d80: 40 82 00 08  	bf	2, 0x800c5d88 <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x26c>
800c5d84: 3a 94 00 01  	addi 20, 20, 1
800c5d88: 3a b5 00 01  	addi 21, 21, 1
800c5d8c: 3b 18 00 01  	addi 24, 24, 1
800c5d90: 7f c3 f3 78  	mr	3, 30
800c5d94: 48 45 25 bd  	bl 0x80518350 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x452124> # strlen
800c5d98: 7c 15 18 40  	cmplw	21, 3
800c5d9c: 40 81 ff d0  	bf	1, 0x800c5d6c <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x250>
800c5da0: 83 01 00 08  	lwz 24, 8(1)
800c5da4: 7f 83 e3 78  	mr	3, 28
800c5da8: 4b ff f5 01  	bl 0x800c52a8 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0xfffffffffffff07c> # getName__14AutoEffectInfoCFv
800c5dac: 80 9b 00 18  	lwz 4, 24(27)
800c5db0: 7c 65 1b 78  	mr	5, 3
800c5db4: c0 22 a0 fc  	lfs 1, -24324(2)
800c5db8: 7f 43 d3 78  	mr	3, 26
800c5dbc: c0 42 a1 00  	lfs 2, -24320(2)
800c5dc0: 7f 06 c3 78  	mr	6, 24
800c5dc4: 7e 87 a3 78  	mr	7, 20
800c5dc8: 39 00 00 00  	li 8, 0
800c5dcc: 48 09 c9 49  	bl 0x80162714 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x9c4e8> # registerSyncBckEffect__12EffectKeeperFP12XanimePlayerPCcPCclffb
800c5dd0: 48 00 00 14  	b 0x800c5de4 <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x2c8>
800c5dd4: 80 9b 00 18  	lwz 4, 24(27)
800c5dd8: 7f e3 fb 78  	mr	3, 31
800c5ddc: 80 a1 00 08  	lwz 5, 8(1)
800c5de0: 48 00 12 01  	bl 0x800c6fe0 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0xdb4> # addSyncBck__12MultiEmitterFPC12XanimePlayerPCc
800c5de4: 38 61 00 0c  	addi 3, 1, 12
800c5de8: 3a 80 00 00  	li 20, 0
800c5dec: 38 80 01 00  	li 4, 256
800c5df0: 48 32 2e 99  	bl 0x803e8c88 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x322a5c> # zeroMemory__2MRFPvUl
800c5df4: 48 00 00 0c  	b 0x800c5e00 <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x2e4>
800c5df8: 7c 77 a1 ae  	stbx 3, 23, 20
800c5dfc: 3a 94 00 01  	addi 20, 20, 1
800c5e00: 3b bd 00 01  	addi 29, 29, 1
800c5e04: 3b 39 00 01  	addi 25, 25, 1
800c5e08: 7f c3 f3 78  	mr	3, 30
800c5e0c: 48 45 25 45  	bl 0x80518350 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x452124> # strlen
800c5e10: 7c 1d 18 40  	cmplw	29, 3
800c5e14: 40 81 ff 04  	bf	1, 0x800c5d18 <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x1fc>
800c5e18: a0 1c 00 18  	lhz 0, 24(28)
800c5e1c: 7f e3 fb 78  	mr	3, 31
800c5e20: 54 04 06 72  	rlwinm 4, 0, 0, 25, 25
800c5e24: 38 04 ff c0  	addi 0, 4, -64
800c5e28: 7c 00 00 34  	cntlzw	0, 0
800c5e2c: 54 04 d9 7e  	srwi 4, 0, 5
800c5e30: 48 00 11 b9  	bl 0x800c6fe8 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0xdbc> # setContinueBckEnd__12MultiEmitterFb
800c5e34: e3 e1 01 68  	<unknown>
800c5e38: cb e1 01 60  	lfd 31, 352(1)
800c5e3c: e3 c1 01 58  	<unknown>
800c5e40: 39 61 01 50  	addi 11, 1, 336
800c5e44: cb c1 01 50  	lfd 30, 336(1)
800c5e48: 48 45 2b e9  	bl 0x80518a30 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x452804> # _restgpr_20
800c5e4c: 80 01 01 74  	lwz 0, 372(1)
800c5e50: 7c 08 03 a6  	mtlr 0
800c5e54: 38 21 01 70  	addi 1, 1, 368
800c5e58: 4e 80 00 20  	blr

# registerAutoEffectInfoGroup__Q22MR6EffectFP12EffectKeeperPC9LiveActorPCc
800c5e5c: 94 21 ff e0  	stwu 1, -32(1)
800c5e60: 7c 08 02 a6  	mflr 0
800c5e64: 90 01 00 24  	stw 0, 36(1)
800c5e68: 39 61 00 20  	addi 11, 1, 32
800c5e6c: 48 45 2b 9d  	bl 0x80518a08 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x4527dc> # _savegpr_29
800c5e70: 7c 7d 1b 78  	mr	29, 3
800c5e74: 7c 9e 23 78  	mr	30, 4
800c5e78: 7c bf 2b 78  	mr	31, 5
800c5e7c: 4b ff f9 51  	bl 0x800c57cc <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0xfffffffffffff5a0> # getEffectSystem__2MRFv
800c5e80: 80 63 00 1c  	lwz 3, 28(3)
800c5e84: 7f e4 fb 78  	mr	4, 31
800c5e88: 4b ff ec d5  	bl 0x800c4b5c <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0xffffffffffffe930> # createAndAddAutoEffectGroup__Q22MR6EffectFP21AutoEffectGroupHolderPCc
800c5e8c: 4b ff f9 41  	bl 0x800c57cc <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0xfffffffffffff5a0> # getEffectSystem__2MRFv
800c5e90: 80 63 00 1c  	lwz 3, 28(3)
800c5e94: 7f a4 eb 78  	mr	4, 29
800c5e98: 7f c5 f3 78  	mr	5, 30
800c5e9c: 7f e6 fb 78  	mr	6, 31
800c5ea0: 4b ff ed 35  	bl 0x800c4bd4 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0xffffffffffffe9a8> # registerAutoEffectInfos__Q22MR6EffectFP21AutoEffectGroupHolderP12EffectKeeperPC9LiveActorPCc
800c5ea4: 39 61 00 20  	addi 11, 1, 32
800c5ea8: 48 45 2b ad  	bl 0x80518a54 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x452828> # _restgpr_29
800c5eac: 80 01 00 24  	lwz 0, 36(1)
800c5eb0: 7c 08 03 a6  	mtlr 0
800c5eb4: 38 21 00 20  	addi 1, 1, 32
800c5eb8: 4e 80 00 20  	blr

# requestMovementOn__Q22MR6EffectFP12EffectKeeper
800c5ebc: 94 21 ff e0  	stwu 1, -32(1)
800c5ec0: 7c 08 02 a6  	mflr 0
800c5ec4: 90 01 00 24  	stw 0, 36(1)
800c5ec8: 39 61 00 20  	addi 11, 1, 32
800c5ecc: 48 45 2b 3d  	bl 0x80518a08 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x4527dc> # _savegpr_29
800c5ed0: 7c 7d 1b 78  	mr	29, 3
800c5ed4: 3b e0 00 00  	li 31, 0
800c5ed8: 48 00 00 38  	b 0x800c5f10 <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x3f4>
800c5edc: 7f a3 eb 78  	mr	3, 29
800c5ee0: 7f e4 fb 78  	mr	4, 31
800c5ee4: 48 09 cb 89  	bl 0x80162a6c <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x9c840> # getEmitter__12EffectKeeperCFl
800c5ee8: 2c 03 00 00  	cmpwi	3, 0
800c5eec: 7c 7e 1b 78  	mr	30, 3
800c5ef0: 41 82 00 1c  	bt	2, 0x800c5f0c <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x3f0>
800c5ef4: 48 00 0d 65  	bl 0x800c6c58 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0xa2c> # isValid__12MultiEmitterCFv
800c5ef8: 2c 03 00 00  	cmpwi	3, 0
800c5efc: 41 82 00 10  	bt	2, 0x800c5f0c <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x3f0>
800c5f00: 7f c3 f3 78  	mr	3, 30
800c5f04: 38 80 ff ff  	li 4, -1
800c5f08: 48 00 21 19  	bl 0x800c8020 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x1df4> # pauseOff__12MultiEmitterFl
800c5f0c: 3b ff 00 01  	addi 31, 31, 1
800c5f10: 80 1d 00 14  	lwz 0, 20(29)
800c5f14: 7c 1f 00 00  	cmpw	31, 0
800c5f18: 41 80 ff c4  	bt	0, 0x800c5edc <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x3c0>
800c5f1c: 39 61 00 20  	addi 11, 1, 32
800c5f20: 48 45 2b 35  	bl 0x80518a54 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x452828> # _restgpr_29
800c5f24: 80 01 00 24  	lwz 0, 36(1)
800c5f28: 7c 08 03 a6  	mtlr 0
800c5f2c: 38 21 00 20  	addi 1, 1, 32
800c5f30: 4e 80 00 20  	blr

# registerAutoEffectInfoGroup__Q22MR6EffectFP16PaneEffectKeeperPC11LayoutActorPCc
800c5f34: 94 21 ff e0  	stwu 1, -32(1)
800c5f38: 7c 08 02 a6  	mflr 0
800c5f3c: 90 01 00 24  	stw 0, 36(1)
800c5f40: 39 61 00 20  	addi 11, 1, 32
800c5f44: 48 45 2a c1  	bl 0x80518a04 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x4527d8> # _savegpr_28
800c5f48: 7c 7c 1b 78  	mr	28, 3
800c5f4c: 7c 9d 23 78  	mr	29, 4
800c5f50: 7c be 2b 78  	mr	30, 5
800c5f54: 4b ff f8 79  	bl 0x800c57cc <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0xfffffffffffff5a0> # getEffectSystem__2MRFv
800c5f58: 7c 7f 1b 78  	mr	31, 3
800c5f5c: 80 63 00 1c  	lwz 3, 28(3)
800c5f60: 7f c4 f3 78  	mr	4, 30
800c5f64: 4b ff eb f9  	bl 0x800c4b5c <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0xffffffffffffe930> # createAndAddAutoEffectGroup__Q22MR6EffectFP21AutoEffectGroupHolderPCc
800c5f68: 80 7f 00 1c  	lwz 3, 28(31)
800c5f6c: 7f 84 e3 78  	mr	4, 28
800c5f70: 7f a5 eb 78  	mr	5, 29
800c5f74: 7f c6 f3 78  	mr	6, 30
800c5f78: 4b ff ec ad  	bl 0x800c4c24 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0xffffffffffffe9f8> # registerAutoEffectInfos__Q22MR6EffectFP21AutoEffectGroupHolderP16PaneEffectKeeperPC11LayoutActorPCc
800c5f7c: 39 61 00 20  	addi 11, 1, 32
800c5f80: 48 45 2a d1  	bl 0x80518a50 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x452824> # _restgpr_28
800c5f84: 80 01 00 24  	lwz 0, 36(1)
800c5f88: 7c 08 03 a6  	mtlr 0
800c5f8c: 38 21 00 20  	addi 1, 1, 32
800c5f90: 4e 80 00 20  	blr

# registerAutoEffectInfoGroup__Q22MR6EffectFP16PaneEffectKeeperPC12EffectSystemPC11LayoutActorPCc
800c5f94: 94 21 ff e0  	stwu 1, -32(1)
800c5f98: 7c 08 02 a6  	mflr 0
800c5f9c: 90 01 00 24  	stw 0, 36(1)
800c5fa0: 39 61 00 20  	addi 11, 1, 32
800c5fa4: 48 45 2a 61  	bl 0x80518a04 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x4527d8> # _savegpr_28
800c5fa8: 7c 7c 1b 78  	mr	28, 3
800c5fac: 7c df 33 78  	mr	31, 6
800c5fb0: 80 64 00 1c  	lwz 3, 28(4)
800c5fb4: 7c 9d 23 78  	mr	29, 4
800c5fb8: 7c be 2b 78  	mr	30, 5
800c5fbc: 7f e4 fb 78  	mr	4, 31
800c5fc0: 4b ff eb 9d  	bl 0x800c4b5c <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0xffffffffffffe930> # createAndAddAutoEffectGroup__Q22MR6EffectFP21AutoEffectGroupHolderPCc
800c5fc4: 80 7d 00 1c  	lwz 3, 28(29)
800c5fc8: 7f 84 e3 78  	mr	4, 28
800c5fcc: 7f c5 f3 78  	mr	5, 30
800c5fd0: 7f e6 fb 78  	mr	6, 31
800c5fd4: 4b ff ec 51  	bl 0x800c4c24 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0xffffffffffffe9f8> # registerAutoEffectInfos__Q22MR6EffectFP21AutoEffectGroupHolderP16PaneEffectKeeperPC11LayoutActorPCc
800c5fd8: 39 61 00 20  	addi 11, 1, 32
800c5fdc: 48 45 2a 75  	bl 0x80518a50 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x452824> # _restgpr_28
800c5fe0: 80 01 00 24  	lwz 0, 36(1)
800c5fe4: 7c 08 03 a6  	mtlr 0
800c5fe8: 38 21 00 20  	addi 1, 1, 32
800c5fec: 4e 80 00 20  	blr

# addAutoEffect__Q22MR6EffectFP12EffectKeeperPC9LiveActorPC14AutoEffectInfo
800c5ff0: 94 21 ff e0  	stwu 1, -32(1)
800c5ff4: 7c 08 02 a6  	mflr 0
800c5ff8: 90 01 00 24  	stw 0, 36(1)
800c5ffc: 39 61 00 20  	addi 11, 1, 32
800c6000: 48 45 2a 05  	bl 0x80518a04 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x4527d8> # _savegpr_28
800c6004: 80 05 00 14  	lwz 0, 20(5)
800c6008: 7c 7c 1b 78  	mr	28, 3
800c600c: 7c 9d 23 78  	mr	29, 4
800c6010: 7c be 2b 78  	mr	30, 5
800c6014: 2c 00 00 00  	cmpwi	0, 0
800c6018: 41 82 00 30  	bt	2, 0x800c6048 <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x52c>
800c601c: 83 e5 00 0c  	lwz 31, 12(5)
800c6020: 7f a3 eb 78  	mr	3, 29
800c6024: 7c 04 03 78  	mr	4, 0
800c6028: 48 31 08 0d  	bl 0x803d6834 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x310608> # getJointMtx__2MRFPC9LiveActorPCc
800c602c: 80 de 00 08  	lwz 6, 8(30)
800c6030: 7c 65 1b 78  	mr	5, 3
800c6034: 7f 83 e3 78  	mr	3, 28
800c6038: 7f e4 fb 78  	mr	4, 31
800c603c: 38 e0 00 00  	li 7, 0
800c6040: 48 09 c5 45  	bl 0x80162584 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x9c358> # registerEffect__12EffectKeeperFPCcPA4_fPCcPCc
800c6044: 48 00 00 74  	b 0x800c60b8 <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x59c>
800c6048: 81 9d 00 00  	lwz 12, 0(29)
800c604c: 7f a3 eb 78  	mr	3, 29
800c6050: 81 8c 00 38  	lwz 12, 56(12)
800c6054: 7d 89 03 a6  	mtctr 12
800c6058: 4e 80 04 21  	bctrl
800c605c: 2c 03 00 00  	cmpwi	3, 0
800c6060: 41 82 00 3c  	bt	2, 0x800c609c <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x580>
800c6064: 81 9d 00 00  	lwz 12, 0(29)
800c6068: 7f a3 eb 78  	mr	3, 29
800c606c: 83 fe 00 0c  	lwz 31, 12(30)
800c6070: 81 8c 00 38  	lwz 12, 56(12)
800c6074: 7d 89 03 a6  	mtctr 12
800c6078: 4e 80 04 21  	bctrl
800c607c: 80 fe 00 08  	lwz 7, 8(30)
800c6080: 7c 65 1b 78  	mr	5, 3
800c6084: 7f 83 e3 78  	mr	3, 28
800c6088: 7f e4 fb 78  	mr	4, 31
800c608c: 38 dd 00 24  	addi 6, 29, 36
800c6090: 39 00 00 00  	li 8, 0
800c6094: 48 09 c5 81  	bl 0x80162614 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x9c3e8> # registerEffect__12EffectKeeperFPCcPA4_fPCQ29JGeometry8TVec3<f>PCcPCc
800c6098: 48 00 00 20  	b 0x800c60b8 <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x59c>
800c609c: 80 9e 00 0c  	lwz 4, 12(30)
800c60a0: 7f 83 e3 78  	mr	3, 28
800c60a4: 81 1e 00 08  	lwz 8, 8(30)
800c60a8: 38 bd 00 0c  	addi 5, 29, 12
800c60ac: 38 dd 00 18  	addi 6, 29, 24
800c60b0: 38 fd 00 24  	addi 7, 29, 36
800c60b4: 48 09 c4 31  	bl 0x801624e4 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x9c2b8> # registerEffect__12EffectKeeperFPCcPCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>PCc
800c60b8: 80 9d 00 48  	lwz 4, 72(29)
800c60bc: 7f 83 e3 78  	mr	3, 28
800c60c0: 7f c5 f3 78  	mr	5, 30
800c60c4: 4b ff fa c1  	bl 0x800c5b84 <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x68> # setupMultiEmitter__Q22MR6EffectFP12EffectKeeperPC12ModelManagerPC14AutoEffectInfo
800c60c8: 39 61 00 20  	addi 11, 1, 32
800c60cc: 48 45 29 85  	bl 0x80518a50 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x452824> # _restgpr_28
800c60d0: 80 01 00 24  	lwz 0, 36(1)
800c60d4: 7c 08 03 a6  	mtlr 0
800c60d8: 38 21 00 20  	addi 1, 1, 32
800c60dc: 4e 80 00 20  	blr

# addAutoEffect__Q22MR6EffectFP16PaneEffectKeeperPC11LayoutActorPC14AutoEffectInfo
800c60e0: 94 21 ff f0  	stwu 1, -16(1)
800c60e4: 7c 08 02 a6  	mflr 0
800c60e8: 80 85 00 14  	lwz 4, 20(5)
800c60ec: 90 01 00 14  	stw 0, 20(1)
800c60f0: 93 e1 00 0c  	stw 31, 12(1)
800c60f4: 7c bf 2b 78  	mr	31, 5
800c60f8: 80 a5 00 0c  	lwz 5, 12(5)
800c60fc: 93 c1 00 08  	stw 30, 8(1)
800c6100: 7c 7e 1b 78  	mr	30, 3
800c6104: 80 df 00 08  	lwz 6, 8(31)
800c6108: 48 2a f0 a1  	bl 0x803751a8 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x2aef7c> # add__16PaneEffectKeeperFPCcPCcPCc
800c610c: 80 9f 00 08  	lwz 4, 8(31)
800c6110: 7f c3 f3 78  	mr	3, 30
800c6114: 48 2a f3 05  	bl 0x80375418 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x2af1ec> # getEmitter__16PaneEffectKeeperCFPCc
800c6118: 7f e4 fb 78  	mr	4, 31
800c611c: 4b ff f7 f1  	bl 0x800c590c <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0xfffffffffffff6e0> # setupMultiEmitter__30@unnamed@EffectSystemUtil_cpp@FP12MultiEmitterPC14AutoEffectInfo
800c6120: 80 01 00 14  	lwz 0, 20(1)
800c6124: 83 e1 00 0c  	lwz 31, 12(1)
800c6128: 83 c1 00 08  	lwz 30, 8(1)
800c612c: 7c 08 03 a6  	mtlr 0
800c6130: 38 21 00 10  	addi 1, 1, 16
800c6134: 4e 80 00 20  	blr

# addAutoEffect__Q22MR6EffectFP22MultiSceneEffectKeeperPC15MultiSceneActorPC14AutoEffectInfo
800c6138: 94 21 ff e0  	stwu 1, -32(1)
800c613c: 7c 08 02 a6  	mflr 0
800c6140: 90 01 00 24  	stw 0, 36(1)
800c6144: 39 61 00 20  	addi 11, 1, 32
800c6148: 48 45 28 c1  	bl 0x80518a08 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x4527dc> # _savegpr_29
800c614c: 80 05 00 14  	lwz 0, 20(5)
800c6150: 7c 7d 1b 78  	mr	29, 3
800c6154: 7c 87 23 78  	mr	7, 4
800c6158: 7c be 2b 78  	mr	30, 5
800c615c: 2c 00 00 00  	cmpwi	0, 0
800c6160: 41 82 00 2c  	bt	2, 0x800c618c <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x670>
800c6164: 83 e5 00 0c  	lwz 31, 12(5)
800c6168: 7c e3 3b 78  	mr	3, 7
800c616c: 7c 04 03 78  	mr	4, 0
800c6170: 48 27 bd ad  	bl 0x80341f1c <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x27bcf0> # getJointMtx__10MultiSceneFPC15MultiSceneActorPCc
800c6174: 80 de 00 08  	lwz 6, 8(30)
800c6178: 7c 65 1b 78  	mr	5, 3
800c617c: 7f a3 eb 78  	mr	3, 29
800c6180: 7f e4 fb 78  	mr	4, 31
800c6184: 48 27 b3 55  	bl 0x803414d8 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x27b2ac> # add__22MultiSceneEffectKeeperFPCcPA4_fPCc
800c6188: 48 00 00 1c  	b 0x800c61a4 <_binary_build_original_auto_effect_registration_20260903_registration_bin_start+0x688>
800c618c: 80 85 00 0c  	lwz 4, 12(5)
800c6190: 38 a7 00 0c  	addi 5, 7, 12
800c6194: 38 c7 00 18  	addi 6, 7, 24
800c6198: 81 1e 00 08  	lwz 8, 8(30)
800c619c: 38 e7 00 24  	addi 7, 7, 36
800c61a0: 48 27 b2 9d  	bl 0x8034143c <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x27b210> # add__22MultiSceneEffectKeeperFPCcPCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>PCc
800c61a4: 80 9e 00 08  	lwz 4, 8(30)
800c61a8: 7f a3 eb 78  	mr	3, 29
800c61ac: 48 27 b5 55  	bl 0x80341700 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x27b4d4> # get__22MultiSceneEffectKeeperCFPCc
800c61b0: 7f c4 f3 78  	mr	4, 30
800c61b4: 4b ff f7 59  	bl 0x800c590c <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0xfffffffffffff6e0> # setupMultiEmitter__30@unnamed@EffectSystemUtil_cpp@FP12MultiEmitterPC14AutoEffectInfo
800c61b8: 39 61 00 20  	addi 11, 1, 32
800c61bc: 48 45 28 99  	bl 0x80518a54 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x452828> # _restgpr_29
800c61c0: 80 01 00 24  	lwz 0, 36(1)
800c61c4: 7c 08 03 a6  	mtlr 0
800c61c8: 38 21 00 20  	addi 1, 1, 32
800c61cc: 4e 80 00 20  	blr

# registerAutoEffectInfoGroup__Q22MR6EffectFP22MultiSceneEffectKeeperPC12EffectSystemPC15MultiSceneActorPCc
800c61d0: 94 21 ff e0  	stwu 1, -32(1)
800c61d4: 7c 08 02 a6  	mflr 0
800c61d8: 90 01 00 24  	stw 0, 36(1)
800c61dc: 39 61 00 20  	addi 11, 1, 32
800c61e0: 48 45 28 25  	bl 0x80518a04 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x4527d8> # _savegpr_28
800c61e4: 7c 7c 1b 78  	mr	28, 3
800c61e8: 7c df 33 78  	mr	31, 6
800c61ec: 80 64 00 1c  	lwz 3, 28(4)
800c61f0: 7c 9d 23 78  	mr	29, 4
800c61f4: 7c be 2b 78  	mr	30, 5
800c61f8: 7f e4 fb 78  	mr	4, 31
800c61fc: 4b ff e9 61  	bl 0x800c4b5c <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0xffffffffffffe930> # createAndAddAutoEffectGroup__Q22MR6EffectFP21AutoEffectGroupHolderPCc
800c6200: 80 7d 00 1c  	lwz 3, 28(29)
800c6204: 7f 84 e3 78  	mr	4, 28
800c6208: 7f c5 f3 78  	mr	5, 30
800c620c: 7f e6 fb 78  	mr	6, 31
800c6210: 4b ff ea 65  	bl 0x800c4c74 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0xffffffffffffea48> # registerAutoEffectInfos__Q22MR6EffectFP21AutoEffectGroupHolderP22MultiSceneEffectKeeperPC15MultiSceneActorPCc
800c6214: 39 61 00 20  	addi 11, 1, 32
800c6218: 48 45 28 39  	bl 0x80518a50 <_binary_build_original_auto_effect_registration_20260903_registration_bin_end+0x452824> # _restgpr_28
800c621c: 80 01 00 24  	lwz 0, 36(1)
800c6220: 7c 08 03 a6  	mtlr 0
800c6224: 38 21 00 20  	addi 1, 1, 32
800c6228: 4e 80 00 20  	blr
