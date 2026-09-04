0000013c <initAfterPlacementForAttributeEffect__12EffectKeeperFPA4_f>:
     13c: 94 21 ff 00  	stwu 1, -256(1)
     140: 7c 08 02 a6  	mflr 0
     144: 90 01 01 04  	stw 0, 260(1)
     148: db e1 00 f0  	stfd 31, 240(1)
     14c: f3 e1 00 f8  	xxsel 31, 1, 0, 35
     150: 93 e1 00 ec  	stw 31, 236(1)
     154: 7c 9f 23 78  	mr	31, 4
     158: 93 c1 00 e8  	stw 30, 232(1)
     15c: 7c 7e 1b 78  	mr	30, 3
     160: 80 03 00 24  	lwz 0, 36(3)
     164: 2c 00 00 00  	cmpwi	0, 0
     168: 40 82 01 3c  	bf	2, 0x2a4 <initAfterPlacementForAttributeEffect__12EffectKeeperFPA4_f+0x168>
     16c: 48 00 00 01  	bl 0x16c <initAfterPlacementForAttributeEffect__12EffectKeeperFPA4_f+0x30>
			0000016c:  R_PPC_REL24	checkExistenceAttributeEffect__12EffectKeeperFv
     170: 88 1e 00 30  	lbz 0, 48(30)
     174: 2c 00 00 00  	cmpwi	0, 0
     178: 41 82 01 2c  	bt	2, 0x2a4 <initAfterPlacementForAttributeEffect__12EffectKeeperFPA4_f+0x168>
     17c: 2c 1f 00 00  	cmpwi	31, 0
     180: 41 82 01 24  	bt	2, 0x2a4 <initAfterPlacementForAttributeEffect__12EffectKeeperFPA4_f+0x168>
     184: e0 5f 00 00  	lq 2, 0(31)
     188: 38 61 00 10  	addi 3, 1, 16
     18c: e0 3f 00 10  	lq 1, 16(31)
     190: e0 1f 00 20  	lq 0, 32(31)
     194: e0 bf 00 08  	<unknown>
     198: f0 01 00 6c  	<unknown>
     19c: e0 9f 00 18  	<unknown>
     1a0: e0 1f 00 28  	<unknown>
     1a4: f0 41 00 4c  	xsmaddmsp 2, 33, 0
     1a8: c0 61 00 70  	lfs 3, 112(1)
     1ac: f0 21 00 5c  	xscmpgtdp 1, 33, 0
     1b0: c0 21 00 50  	lfs 1, 80(1)
     1b4: c0 41 00 60  	lfs 2, 96(1)
     1b8: f0 a1 00 54  	xxmrghd	5, 33, 0
     1bc: c3 e0 00 00  	lfs 31, 0(0)
			000001bc:  Unknown	@61911
     1c0: f0 81 00 64  	<unknown>
     1c4: f0 01 00 74  	xxsel 0, 33, 0, 1
     1c8: 48 00 00 01  	bl 0x1c8 <initAfterPlacementForAttributeEffect__12EffectKeeperFPA4_f+0x8c>
			000001c8:  R_PPC_REL24	set<f>__Q29JGeometry8TVec3<f>Ffff_v
     1cc: 38 61 00 10  	addi 3, 1, 16
     1d0: 7c 64 1b 78  	mr	4, 3
     1d4: 48 00 00 01  	bl 0x1d4 <initAfterPlacementForAttributeEffect__12EffectKeeperFPA4_f+0x98>
			000001d4:  R_PPC_REL24	negateInternal__9JGeometryFPCfPf
     1d8: 38 61 00 10  	addi 3, 1, 16
     1dc: 48 00 00 01  	bl 0x1dc <initAfterPlacementForAttributeEffect__12EffectKeeperFPA4_f+0xa0>
			000001dc:  R_PPC_REL24	normalize__2MRFPQ29JGeometry8TVec3<f>
     1e0: c0 61 00 78  	lfs 3, 120(1)
     1e4: 38 61 00 1c  	addi 3, 1, 28
     1e8: c0 41 00 68  	lfs 2, 104(1)
     1ec: c0 21 00 58  	lfs 1, 88(1)
     1f0: 48 00 00 01  	bl 0x1f0 <initAfterPlacementForAttributeEffect__12EffectKeeperFPA4_f+0xb4>
			000001f0:  R_PPC_REL24	set<f>__Q29JGeometry8TVec3<f>Ffff_v
     1f4: 38 61 00 10  	addi 3, 1, 16
     1f8: 38 81 00 40  	addi 4, 1, 64
     1fc: 48 00 00 01  	bl 0x1fc <initAfterPlacementForAttributeEffect__12EffectKeeperFPA4_f+0xc0>
			000001fc:  R_PPC_REL24	negateInternal__9JGeometryFPCfPf
     200: e0 01 00 40  	lq 0, 64(1)
     204: 3b e1 00 1c  	addi 31, 1, 28
     208: c0 21 00 48  	lfs 1, 72(1)
     20c: 38 61 00 7c  	addi 3, 1, 124
     210: f0 01 00 34  	xxsel 0, 33, 0, 0
     214: ec 01 07 f2  	fmuls 0, 1, 31
     218: e0 9f 00 00  	lq 4, 0(31)
     21c: c0 41 00 34  	lfs 2, 52(1)
     220: c0 21 00 38  	lfs 1, 56(1)
     224: ec 42 07 f2  	fmuls 2, 2, 31
     228: d0 01 00 3c  	stfs 0, 60(1)
     22c: ec 01 07 f2  	fmuls 0, 1, 31
     230: e0 7f 80 08  	<unknown>
     234: e0 21 80 3c  	<unknown>
     238: d0 41 00 34  	stfs 2, 52(1)
     23c: 10 23 08 2a  	vsel 1, 3, 1, 0
     240: d0 01 00 38  	stfs 0, 56(1)
     244: e0 01 00 34  	<unknown>
     248: f0 3f 80 08  	xsmaddasp 1, 31, 16
     24c: 10 04 00 2a  	vsel 0, 4, 0, 0
     250: f0 1f 00 00  	xsaddsp 0, 31, 0
     254: 48 00 00 01  	bl 0x254 <initAfterPlacementForAttributeEffect__12EffectKeeperFPA4_f+0x118>
			00000254:  R_PPC_REL24	__ct__8TriangleFv
     258: 7f e5 fb 78  	mr	5, 31
     25c: 38 61 00 28  	addi 3, 1, 40
     260: 38 81 00 7c  	addi 4, 1, 124
     264: 38 c1 00 10  	addi 6, 1, 16
     268: 48 00 00 01  	bl 0x268 <initAfterPlacementForAttributeEffect__12EffectKeeperFPA4_f+0x12c>
			00000268:  R_PPC_REL24	getFirstPolyOnLineToMap__2MRFPQ29JGeometry8TVec3<f>P8TriangleRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>
     26c: 2c 03 00 00  	cmpwi	3, 0
     270: 40 82 00 0c  	bf	2, 0x27c <initAfterPlacementForAttributeEffect__12EffectKeeperFPA4_f+0x140>
     274: 38 60 ff ff  	li 3, -1
     278: 48 00 00 1c  	b 0x294 <initAfterPlacementForAttributeEffect__12EffectKeeperFPA4_f+0x158>
     27c: 38 61 00 7c  	addi 3, 1, 124
     280: 48 00 00 01  	bl 0x280 <initAfterPlacementForAttributeEffect__12EffectKeeperFPA4_f+0x144>
			00000280:  R_PPC_REL24	getAttributes__8TriangleCFv
     284: 90 61 00 08  	stw 3, 8(1)
     288: 38 61 00 08  	addi 3, 1, 8
     28c: 90 81 00 0c  	stw 4, 12(1)
     290: 48 00 00 01  	bl 0x290 <initAfterPlacementForAttributeEffect__12EffectKeeperFPA4_f+0x154>
			00000290:  R_PPC_REL24	getFloorCodeIndex__2MRFRC12JMapInfoIter
     294: 2c 03 00 00  	cmpwi	3, 0
     298: 41 80 00 0c  	bt	0, 0x2a4 <initAfterPlacementForAttributeEffect__12EffectKeeperFPA4_f+0x168>
     29c: 90 7e 00 28  	stw 3, 40(30)
     2a0: 90 7e 00 2c  	stw 3, 44(30)
     2a4: e3 e1 00 f8  	<unknown>
     2a8: 80 01 01 04  	lwz 0, 260(1)
     2ac: cb e1 00 f0  	lfd 31, 240(1)
     2b0: 83 e1 00 ec  	lwz 31, 236(1)
     2b4: 83 c1 00 e8  	lwz 30, 232(1)
     2b8: 7c 08 03 a6  	mtlr 0
     2bc: 38 21 01 00  	addi 1, 1, 256
     2c0: 4e 80 00 20  	blr


00000cec <onDraw__12EffectKeeperFv>:
     cec: 94 21 ff c0  	stwu 1, -64(1)
     cf0: 7c 08 02 a6  	mflr 0
     cf4: 3c 80 00 00  	lis 4, 0
			00000cf6:  R_PPC_ADDR16_HA	@62256
     cf8: 7c 6a 1b 78  	mr	10, 3
     cfc: 90 01 00 44  	stw 0, 68(1)
     d00: 38 c1 00 18  	addi 6, 1, 24
     d04: 85 24 00 00  	lwzu 9, 0(4)
			00000d06:  R_PPC_ADDR16_LO	@62256
     d08: 80 e0 00 00  	lwz 7, 0(0)
			00000d08:  Unknown	@57627
     d0c: 80 a4 00 04  	lwz 5, 4(4)
     d10: 81 04 00 08  	lwz 8, 8(4)
     d14: 91 21 00 18  	stw 9, 24(1)
     d18: 90 a1 00 1c  	stw 5, 28(1)
     d1c: 91 01 00 20  	stw 8, 32(1)
     d20: 90 e1 00 24  	stw 7, 36(1)
     d24: 80 0a 00 14  	lwz 0, 20(10)
     d28: 80 83 00 0c  	lwz 4, 12(3)
     d2c: 38 61 00 28  	addi 3, 1, 40
     d30: 91 21 00 08  	stw 9, 8(1)
     d34: 54 00 10 3a  	slwi 0, 0, 2
     d38: 90 a1 00 0c  	stw 5, 12(1)
     d3c: 7c a4 02 14  	add 5, 4, 0
     d40: 91 01 00 10  	stw 8, 16(1)
     d44: 90 e1 00 14  	stw 7, 20(1)
     d48: 48 00 00 01  	bl 0xd48 <onDraw__12EffectKeeperFv+0x5c>
			00000d48:  R_PPC_REL24	for_each<PP12MultiEmitter,Q23std51binder2nd<Q23std30mem_fun1_t<v,12MultiEmitter,l>,l>>__3stdFPP12MultiEmitterPP12MultiEmitterQ23std51binder2nd<Q23std30mem_fun1_t<v,12MultiEmitter,l>,l>_Q23std51binder2nd<Q23std30mem_fun1_t<v,12MultiEmitter,l>,l>
     d4c: 80 01 00 44  	lwz 0, 68(1)
     d50: 7c 08 03 a6  	mtlr 0
     d54: 38 21 00 40  	addi 1, 1, 64
     d58: 4e 80 00 20  	blr


00000d5c <offDraw__12EffectKeeperFv>:
     d5c: 94 21 ff c0  	stwu 1, -64(1)
     d60: 7c 08 02 a6  	mflr 0
     d64: 3c 80 00 00  	lis 4, 0
			00000d66:  R_PPC_ADDR16_HA	@62292
     d68: 7c 6a 1b 78  	mr	10, 3
     d6c: 90 01 00 44  	stw 0, 68(1)
     d70: 38 c1 00 18  	addi 6, 1, 24
     d74: 85 24 00 00  	lwzu 9, 0(4)
			00000d76:  R_PPC_ADDR16_LO	@62292
     d78: 80 e0 00 00  	lwz 7, 0(0)
			00000d78:  Unknown	@57718
     d7c: 80 a4 00 04  	lwz 5, 4(4)
     d80: 81 04 00 08  	lwz 8, 8(4)
     d84: 91 21 00 18  	stw 9, 24(1)
     d88: 90 a1 00 1c  	stw 5, 28(1)
     d8c: 91 01 00 20  	stw 8, 32(1)
     d90: 90 e1 00 24  	stw 7, 36(1)
     d94: 80 0a 00 14  	lwz 0, 20(10)
     d98: 80 83 00 0c  	lwz 4, 12(3)
     d9c: 38 61 00 28  	addi 3, 1, 40
     da0: 91 21 00 08  	stw 9, 8(1)
     da4: 54 00 10 3a  	slwi 0, 0, 2
     da8: 90 a1 00 0c  	stw 5, 12(1)
     dac: 7c a4 02 14  	add 5, 4, 0
     db0: 91 01 00 10  	stw 8, 16(1)
     db4: 90 e1 00 14  	stw 7, 20(1)
     db8: 48 00 00 01  	bl 0xdb8 <offDraw__12EffectKeeperFv+0x5c>
			00000db8:  R_PPC_REL24	for_each<PP12MultiEmitter,Q23std51binder2nd<Q23std30mem_fun1_t<v,12MultiEmitter,l>,l>>__3stdFPP12MultiEmitterPP12MultiEmitterQ23std51binder2nd<Q23std30mem_fun1_t<v,12MultiEmitter,l>,l>_Q23std51binder2nd<Q23std30mem_fun1_t<v,12MultiEmitter,l>,l>
     dbc: 80 01 00 44  	lwz 0, 68(1)
     dc0: 7c 08 03 a6  	mtlr 0
     dc4: 38 21 00 40  	addi 1, 1, 64
     dc8: 4e 80 00 20  	blr


00000fb4 <updateFloorCode__12EffectKeeperFv>:
     fb4: 80 83 00 24  	lwz 4, 36(3)
     fb8: 2c 04 00 00  	cmpwi	4, 0
     fbc: 4d 82 00 20  	bclr	12, 2
     fc0: 80 03 00 28  	lwz 0, 40(3)
     fc4: 90 03 00 2c  	stw 0, 44(3)
     fc8: 40 82 00 0c  	bf	2, 0xfd4 <updateFloorCode__12EffectKeeperFv+0x20>
     fcc: 38 00 00 00  	li 0, 0
     fd0: 48 00 00 6c  	b 0x103c <updateFloorCode__12EffectKeeperFv+0x88>
     fd4: c0 20 00 00  	lfs 1, 0(0)
			00000fd4:  Unknown	@60637
     fd8: 38 00 00 01  	li 0, 1
     fdc: c0 04 00 c8  	lfs 0, 200(4)
     fe0: 38 a0 00 01  	li 5, 1
     fe4: fc 01 00 40  	fcmpo 0, 1, 0
     fe8: 4c 40 13 82  	cror 2, 0, 2
     fec: 7c c0 00 26  	mfcr 6
     ff0: 54 c6 1f ff  	rlwinm. 6, 6, 3, 31, 31
     ff4: 40 82 00 20  	bf	2, 0x1014 <updateFloorCode__12EffectKeeperFv+0x60>
     ff8: c0 04 01 58  	lfs 0, 344(4)
     ffc: fc 01 00 40  	fcmpo 0, 1, 0
    1000: 4c 40 13 82  	cror 2, 0, 2
    1004: 7c c0 00 26  	mfcr 6
    1008: 54 c6 1f ff  	rlwinm. 6, 6, 3, 31, 31
    100c: 40 82 00 08  	bf	2, 0x1014 <updateFloorCode__12EffectKeeperFv+0x60>
    1010: 38 a0 00 00  	li 5, 0
    1014: 2c 05 00 00  	cmpwi	5, 0
    1018: 40 82 00 24  	bf	2, 0x103c <updateFloorCode__12EffectKeeperFv+0x88>
    101c: c0 20 00 00  	lfs 1, 0(0)
			0000101c:  Unknown	@60637
    1020: c0 04 01 e8  	lfs 0, 488(4)
    1024: fc 01 00 40  	fcmpo 0, 1, 0
    1028: 4c 40 13 82  	cror 2, 0, 2
    102c: 7c a0 00 26  	mfcr 5
    1030: 54 a5 1f ff  	rlwinm. 5, 5, 3, 31, 31
    1034: 40 82 00 08  	bf	2, 0x103c <updateFloorCode__12EffectKeeperFv+0x88>
    1038: 38 00 00 00  	li 0, 0
    103c: 2c 00 00 00  	cmpwi	0, 0
    1040: 4d 82 00 20  	bclr	12, 2
    1044: 38 84 00 3c  	addi 4, 4, 60
    1048: 48 00 00 00  	b 0x1048 <updateFloorCode__12EffectKeeperFv+0x94>
			00001048:  R_PPC_REL24	updateFloorCode__12EffectKeeperFPC8Triangle
    104c: 4e 80 00 20  	blr

