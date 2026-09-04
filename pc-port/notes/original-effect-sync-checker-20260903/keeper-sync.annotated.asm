
build/original-effect-sync-checker-20260903/keeper-sync.o:	file format elf32-powerpc

Disassembly of section .data:

80162b8c <_binary_build_original_effect_sync_checker_20260903_keeper_sync_bin_start>:
80162b8c: 94 21 fe e0  	stwu 1, -288(1)
80162b90: 7c 08 02 a6  	mflr 0
80162b94: 90 01 01 24  	stw 0, 292(1)
80162b98: 39 61 01 20  	addi 11, 1, 288
80162b9c: 48 3b 5e 61  	bl 0x805189fc <_binary_build_original_effect_sync_checker_20260903_keeper_sync_bin_end+0x3b5d78>  # _savegpr_26
80162ba0: 83 a4 00 24  	lwz 29, 36(4)
80162ba4: 7c 7a 1b 78  	mr	26, 3
80162ba8: 7c 9b 23 78  	mr	27, 4
80162bac: 2c 1d 00 00  	cmpwi	29, 0
80162bb0: 41 82 00 bc  	bt	2, 0x80162c6c <_binary_build_original_effect_sync_checker_20260903_keeper_sync_bin_start+0xe0>
80162bb4: 80 63 00 20  	lwz 3, 32(3)
80162bb8: 7f a4 eb 78  	mr	4, 29
80162bbc: 38 a0 00 01  	li 5, 1
80162bc0: 4b f6 8b 9d  	bl 0x800cb75c <_binary_build_original_effect_sync_checker_20260903_keeper_sync_bin_end+0xfffffffffff68ad8>  # isCreate__20SyncBckEffectCheckerCFPC17SyncBckEffectInfob
80162bc4: 7c 7e 1b 78  	mr	30, 3
80162bc8: 80 7a 00 20  	lwz 3, 32(26)
80162bcc: 7f a4 eb 78  	mr	4, 29
80162bd0: 38 a0 00 00  	li 5, 0
80162bd4: 4b f6 8b 89  	bl 0x800cb75c <_binary_build_original_effect_sync_checker_20260903_keeper_sync_bin_end+0xfffffffffff68ad8>  # isCreate__20SyncBckEffectCheckerCFPC17SyncBckEffectInfob
80162bd8: 2c 1e 00 00  	cmpwi	30, 0
80162bdc: 7c 7f 1b 78  	mr	31, 3
80162be0: 40 82 00 0c  	bf	2, 0x80162bec <_binary_build_original_effect_sync_checker_20260903_keeper_sync_bin_start+0x60>
80162be4: 2c 03 00 00  	cmpwi	3, 0
80162be8: 41 82 00 68  	bt	2, 0x80162c50 <_binary_build_original_effect_sync_checker_20260903_keeper_sync_bin_start+0xc4>
80162bec: 80 7b 00 28  	lwz 3, 40(27)
80162bf0: 7f 7c db 78  	mr	28, 27
80162bf4: 4b f6 26 b5  	bl 0x800c52a8 <_binary_build_original_effect_sync_checker_20260903_keeper_sync_bin_end+0xfffffffffff62624>  # getName__14AutoEffectInfoCFv
80162bf8: 80 8d 85 30  	lwz 4, -31440(13)
80162bfc: 48 29 bf bd  	bl 0x803febb8 <_binary_build_original_effect_sync_checker_20260903_keeper_sync_bin_end+0x29bf34>  # isEqualSubString__2MRFPCcPCc
80162c00: 2c 03 00 00  	cmpwi	3, 0
80162c04: 41 82 00 2c  	bt	2, 0x80162c30 <_binary_build_original_effect_sync_checker_20260903_keeper_sync_bin_start+0xa4>
80162c08: 80 7b 00 28  	lwz 3, 40(27)
80162c0c: 4b f6 26 9d  	bl 0x800c52a8 <_binary_build_original_effect_sync_checker_20260903_keeper_sync_bin_end+0xfffffffffff62624>  # getName__14AutoEffectInfoCFv
80162c10: 7c 65 1b 78  	mr	5, 3
80162c14: 38 61 00 08  	addi 3, 1, 8
80162c18: 38 80 01 00  	li 4, 256
80162c1c: 4b ff f5 a5  	bl 0x801621c0 <_binary_build_original_effect_sync_checker_20260903_keeper_sync_bin_end+0xfffffffffffff53c>  # makeAttibuteEffectBaseName__26@unnamed@EffectKeeper_cpp@FPcUlPCc
80162c20: 7f 43 d3 78  	mr	3, 26
80162c24: 38 81 00 08  	addi 4, 1, 8
80162c28: 4b ff fc e9  	bl 0x80162910 <_binary_build_original_effect_sync_checker_20260903_keeper_sync_bin_end+0xfffffffffffffc8c>  # getEmitter__12EffectKeeperCFPCc
80162c2c: 7c 7c 1b 78  	mr	28, 3
80162c30: 2c 1e 00 00  	cmpwi	30, 0
80162c34: 41 82 00 0c  	bt	2, 0x80162c40 <_binary_build_original_effect_sync_checker_20260903_keeper_sync_bin_start+0xb4>
80162c38: 7f 83 e3 78  	mr	3, 28
80162c3c: 4b f6 48 e5  	bl 0x800c7520 <_binary_build_original_effect_sync_checker_20260903_keeper_sync_bin_end+0xfffffffffff6489c>  # createOneTimeEmitter__12MultiEmitterFv
80162c40: 2c 1f 00 00  	cmpwi	31, 0
80162c44: 41 82 00 0c  	bt	2, 0x80162c50 <_binary_build_original_effect_sync_checker_20260903_keeper_sync_bin_start+0xc4>
80162c48: 7f 83 e3 78  	mr	3, 28
80162c4c: 4b f6 49 4d  	bl 0x800c7598 <_binary_build_original_effect_sync_checker_20260903_keeper_sync_bin_end+0xfffffffffff64914>  # createForeverEmitter__12MultiEmitterFv
80162c50: 80 7a 00 20  	lwz 3, 32(26)
80162c54: 7f a4 eb 78  	mr	4, 29
80162c58: 4b f6 8b ed  	bl 0x800cb844 <_binary_build_original_effect_sync_checker_20260903_keeper_sync_bin_end+0xfffffffffff68bc0>  # isDelete__20SyncBckEffectCheckerCFPC17SyncBckEffectInfo
80162c5c: 2c 03 00 00  	cmpwi	3, 0
80162c60: 41 82 00 0c  	bt	2, 0x80162c6c <_binary_build_original_effect_sync_checker_20260903_keeper_sync_bin_start+0xe0>
80162c64: 7f 63 db 78  	mr	3, 27
80162c68: 4b f6 3c b1  	bl 0x800c6918 <_binary_build_original_effect_sync_checker_20260903_keeper_sync_bin_end+0xfffffffffff63c94>  # deleteEmitter__12MultiEmitterFv
80162c6c: 39 61 01 20  	addi 11, 1, 288
80162c70: 48 3b 5d d9  	bl 0x80518a48 <_binary_build_original_effect_sync_checker_20260903_keeper_sync_bin_end+0x3b5dc4>  # _restgpr_26
80162c74: 80 01 01 24  	lwz 0, 292(1)
80162c78: 7c 08 03 a6  	mtlr 0
80162c7c: 38 21 01 20  	addi 1, 1, 288
80162c80: 4e 80 00 20  	blr
