
build/original-multi-emitter-sync-20260903/multi-emitter.o:	file format elf32-powerpc

Disassembly of section .data:

800c6ea8 <_binary_build_original_multi_emitter_sync_20260903_multi_emitter_bin_start>:
800c6ea8: 94 21 ff d0  	stwu 1, -48(1)
800c6eac: 7c 08 02 a6  	mflr 0
800c6eb0: 3c a0 80 58  	lis 5, -32680
800c6eb4: 90 01 00 34  	stw 0, 52(1)
800c6eb8: 93 e1 00 2c  	stw 31, 44(1)
800c6ebc: 93 c1 00 28  	stw 30, 40(1)
800c6ec0: 84 e5 85 88  	lwzu 7, -31352(5)
800c6ec4: 90 81 00 24  	stw 4, 36(1)
800c6ec8: 80 c5 00 04  	lwz 6, 4(5)
800c6ecc: 80 a5 00 08  	lwz 5, 8(5)
800c6ed0: 90 e1 00 18  	stw 7, 24(1)
800c6ed4: 90 c1 00 1c  	stw 6, 28(1)
800c6ed8: 90 a1 00 20  	stw 5, 32(1)
800c6edc: 80 03 00 04  	lwz 0, 4(3)
800c6ee0: 83 c3 00 00  	lwz 30, 0(3)
800c6ee4: 54 00 18 38  	slwi 0, 0, 3
800c6ee8: 90 e1 00 08  	stw 7, 8(1)
800c6eec: 7f fe 02 14  	add 31, 30, 0
800c6ef0: 90 c1 00 0c  	stw 6, 12(1)
800c6ef4: 90 a1 00 10  	stw 5, 16(1)
800c6ef8: 90 81 00 14  	stw 4, 20(1)
800c6efc: 48 00 00 1c  	b 0x800c6f18 <_binary_build_original_multi_emitter_sync_20260903_multi_emitter_bin_start+0x70>
800c6f00: 80 81 00 24  	lwz 4, 36(1)
800c6f04: 7f c3 f3 78  	mr	3, 30
800c6f08: 39 81 00 18  	addi 12, 1, 24
800c6f0c: 48 45 19 a5  	bl 0x805188b0 <_binary_build_original_multi_emitter_sync_20260903_multi_emitter_bin_end+0x4518bc>  # __ptmf_scall
800c6f10: 60 00 00 00  	nop
800c6f14: 3b de 00 08  	addi 30, 30, 8
800c6f18: 7c 1e f8 40  	cmplw	30, 31
800c6f1c: 40 82 ff e4  	bf	2, 0x800c6f00 <_binary_build_original_multi_emitter_sync_20260903_multi_emitter_bin_start+0x58>
800c6f20: 80 01 00 34  	lwz 0, 52(1)
800c6f24: 83 e1 00 2c  	lwz 31, 44(1)
800c6f28: 83 c1 00 28  	lwz 30, 40(1)
800c6f2c: 7c 08 03 a6  	mtlr 0
800c6f30: 38 21 00 30  	addi 1, 1, 48
800c6f34: 4e 80 00 20  	blr
800c6f38: 80 63 00 1c  	lwz 3, 28(3)
800c6f3c: 48 00 16 dc  	b 0x800c8618 <_binary_build_original_multi_emitter_sync_20260903_multi_emitter_bin_end+0x1624>  # forceFollowOn__20MultiEmitterCallBackFv
800c6f40: 80 63 00 1c  	lwz 3, 28(3)
800c6f44: 48 00 16 e4  	b 0x800c8628 <_binary_build_original_multi_emitter_sync_20260903_multi_emitter_bin_end+0x1634>  # forceFollowOff__20MultiEmitterCallBackFv
800c6f48: 80 63 00 1c  	lwz 3, 28(3)
800c6f4c: 48 00 16 ec  	b 0x800c8638 <_binary_build_original_multi_emitter_sync_20260903_multi_emitter_bin_end+0x1644>  # forceScaleOn__20MultiEmitterCallBackFv
800c6f50: 94 21 ff d0  	stwu 1, -48(1)
800c6f54: 7c 08 02 a6  	mflr 0
800c6f58: 90 01 00 34  	stw 0, 52(1)
800c6f5c: db e1 00 20  	stfd 31, 32(1)
800c6f60: f3 e1 00 28  	<unknown>
800c6f64: 39 61 00 20  	addi 11, 1, 32
800c6f68: 48 45 1a 9d  	bl 0x80518a04 <_binary_build_original_multi_emitter_sync_20260903_multi_emitter_bin_end+0x451a10>  # _savegpr_28
800c6f6c: ff e0 08 90  	fmr 31, 1
800c6f70: 7c 7c 1b 78  	mr	28, 3
800c6f74: 7c 9d 23 78  	mr	29, 4
800c6f78: 7c be 2b 78  	mr	30, 5
800c6f7c: 7c df 33 78  	mr	31, 6
800c6f80: 38 60 00 18  	li 3, 24
800c6f84: 48 34 44 d5  	bl 0x8040b458 <_binary_build_original_multi_emitter_sync_20260903_multi_emitter_bin_end+0x344464>  # __nw__FUl
800c6f88: 2c 03 00 00  	cmpwi	3, 0
800c6f8c: 41 82 00 20  	bt	2, 0x800c6fac <_binary_build_original_multi_emitter_sync_20260903_multi_emitter_bin_start+0x104>
800c6f90: c0 22 a1 08  	lfs 1, -24312(2)
800c6f94: 7f a4 eb 78  	mr	4, 29
800c6f98: c0 42 a1 0c  	lfs 2, -24308(2)
800c6f9c: 7f c5 f3 78  	mr	5, 30
800c6fa0: 7f e6 fb 78  	mr	6, 31
800c6fa4: 38 e0 00 00  	li 7, 0
800c6fa8: 48 00 4b 49  	bl 0x800cbaf0 <_binary_build_original_multi_emitter_sync_20260903_multi_emitter_bin_end+0x4afc>  # __ct__17SyncBckEffectInfoFPC12XanimePlayerPCclffb
800c6fac: 90 7c 00 24  	stw 3, 36(28)
800c6fb0: d3 e3 00 0c  	stfs 31, 12(3)
800c6fb4: e3 e1 00 28  	<unknown>
800c6fb8: cb e1 00 20  	lfd 31, 32(1)
800c6fbc: 39 61 00 20  	addi 11, 1, 32
800c6fc0: 48 45 1a 91  	bl 0x80518a50 <_binary_build_original_multi_emitter_sync_20260903_multi_emitter_bin_end+0x451a5c>  # _restgpr_28
800c6fc4: 80 01 00 34  	lwz 0, 52(1)
800c6fc8: 7c 08 03 a6  	mtlr 0
800c6fcc: 38 21 00 30  	addi 1, 1, 48
800c6fd0: 4e 80 00 20  	blr
800c6fd4: 80 63 00 24  	lwz 3, 36(3)
800c6fd8: d0 23 00 10  	stfs 1, 16(3)
800c6fdc: 4e 80 00 20  	blr
800c6fe0: 80 63 00 24  	lwz 3, 36(3)
800c6fe4: 48 00 4b 88  	b 0x800cbb6c <_binary_build_original_multi_emitter_sync_20260903_multi_emitter_bin_end+0x4b78>  # addBck__17SyncBckEffectInfoFPC12XanimePlayerPCc
800c6fe8: 80 63 00 24  	lwz 3, 36(3)
800c6fec: 98 83 00 14  	stb 4, 20(3)
800c6ff0: 4e 80 00 20  	blr
