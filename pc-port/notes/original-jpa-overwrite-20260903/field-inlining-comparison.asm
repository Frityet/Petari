
/Users/frityet/Projects/petari/build/original-jpa-overwrite-20260903/Overwrite.o:	file format elf32-powerpc

Disassembly of section .text:

000003bc <prepare__11JPAFieldAirFP18JPAEmitterWorkDataP13JPAFieldBlock>:
     3bc: 94 21 ff d0  	stwu 1, -48(1)
     3c0: 7c 08 02 a6  	mflr 0
     3c4: 90 01 00 34  	stw 0, 52(1)
     3c8: 39 61 00 30  	addi 11, 1, 48
     3cc: 48 00 00 01  	bl 0x3cc <prepare__11JPAFieldAirFP18JPAEmitterWorkDataP13JPAFieldBlock+0x10>
			000003cc:  R_PPC_REL24	_savegpr_28
     3d0: e0 05 00 1c  	<unknown>
     3d4: 3b e1 00 08  	addi 31, 1, 8
     3d8: c0 25 00 24  	lfs 1, 36(5)
     3dc: 7c 7c 1b 78  	mr	28, 3
     3e0: 7c 9d 23 78  	mr	29, 4
     3e4: 7c be 2b 78  	mr	30, 5
     3e8: f0 1f 00 00  	xsaddsp 0, 31, 0
     3ec: 7f e3 fb 78  	mr	3, 31
     3f0: d0 21 00 10  	stfs 1, 16(1)
     3f4: 48 00 00 01  	bl 0x3f4 <prepare__11JPAFieldAirFP18JPAEmitterWorkDataP13JPAFieldBlock+0x38>
			000003f4:  R_PPC_REL24	normalizeOrZero__2MRFPQ29JGeometry8TVec3<f>
     3f8: 80 7e 00 00  	lwz 3, 0(30)
     3fc: 80 03 00 08  	lwz 0, 8(3)
     400: 54 00 87 bd  	rlwinm. 0, 0, 16, 30, 30
     404: 41 82 00 18  	bt	2, 0x41c <prepare__11JPAFieldAirFP18JPAEmitterWorkDataP13JPAFieldBlock+0x60>
     408: c0 3e 00 28  	lfs 1, 40(30)
     40c: 7f e4 fb 78  	mr	4, 31
     410: 38 7c 00 04  	addi 3, 28, 4
     414: 48 00 00 01  	bl 0x414 <prepare__11JPAFieldAirFP18JPAEmitterWorkDataP13JPAFieldBlock+0x58>
			00000414:  R_PPC_REL24	scale__Q29JGeometry8TVec3<f>FfRCQ29JGeometry8TVec3<f>
     418: 48 00 00 20  	b 0x438 <prepare__11JPAFieldAirFP18JPAEmitterWorkDataP13JPAFieldBlock+0x7c>
     41c: 7f e4 fb 78  	mr	4, 31
     420: 38 7d 00 78  	addi 3, 29, 120
     424: 38 bc 00 04  	addi 5, 28, 4
     428: 48 00 00 01  	bl 0x428 <prepare__11JPAFieldAirFP18JPAEmitterWorkDataP13JPAFieldBlock+0x6c>
			00000428:  R_PPC_REL24	PSMTXMultVecSR
     42c: c0 3e 00 28  	lfs 1, 40(30)
     430: 38 7c 00 04  	addi 3, 28, 4
     434: 48 00 00 01  	bl 0x434 <prepare__11JPAFieldAirFP18JPAEmitterWorkDataP13JPAFieldBlock+0x78>
			00000434:  R_PPC_REL24	scale__Q29JGeometry8TVec3<f>Ff
     438: 39 61 00 30  	addi 11, 1, 48
     43c: 48 00 00 01  	bl 0x43c <prepare__11JPAFieldAirFP18JPAEmitterWorkDataP13JPAFieldBlock+0x80>
			0000043c:  R_PPC_REL24	_restgpr_28
     440: 80 01 00 34  	lwz 0, 52(1)
     444: 7c 08 03 a6  	mtlr 0
     448: 38 21 00 30  	addi 1, 1, 48
     44c: 4e 80 00 20  	blr

000004b4 <calc__14JPAFieldVortexFP18JPAEmitterWorkDataP13JPAFieldBlockP15JPABaseParticle>:
     4b4: 94 21 ff c0  	stwu 1, -64(1)
     4b8: 7c 08 02 a6  	mflr 0
     4bc: 90 01 00 44  	stw 0, 68(1)
     4c0: db e1 00 30  	stfd 31, 48(1)
     4c4: f3 e1 00 38  	xxsel 31, 1, 0, 32
     4c8: 39 61 00 30  	addi 11, 1, 48
     4cc: 48 00 00 01  	bl 0x4cc <calc__14JPAFieldVortexFP18JPAEmitterWorkDataP13JPAFieldBlockP15JPABaseParticle+0x18>
			000004cc:  R_PPC_REL24	_savegpr_29
     4d0: 7c 7d 1b 78  	mr	29, 3
     4d4: 7c be 2b 78  	mr	30, 5
     4d8: 7c df 33 78  	mr	31, 6
     4dc: 38 63 00 10  	addi 3, 3, 16
     4e0: 38 86 00 0c  	addi 4, 6, 12
     4e4: 48 00 00 01  	bl 0x4e4 <calc__14JPAFieldVortexFP18JPAEmitterWorkDataP13JPAFieldBlockP15JPABaseParticle+0x30>
			000004e4:  R_PPC_REL24	dot__Q29JGeometry8TVec3<f>CFRCQ29JGeometry8TVec3<f>
     4e8: 38 61 00 08  	addi 3, 1, 8
     4ec: 38 9d 00 10  	addi 4, 29, 16
     4f0: 48 00 00 01  	bl 0x4f0 <calc__14JPAFieldVortexFP18JPAEmitterWorkDataP13JPAFieldBlockP15JPABaseParticle+0x3c>
			000004f0:  R_PPC_REL24	scale__Q29JGeometry8TVec3<f>FfRCQ29JGeometry8TVec3<f>
     4f4: e0 1f 00 0c  	<unknown>
     4f8: e0 21 00 08  	<unknown>
     4fc: e0 41 80 10  	lq 2, -32752(1)
     500: 10 00 08 28  	vmsumshm 0, 0, 1, 0
     504: f0 01 00 08  	xsmaddasp 0, 1, 0
     508: 10 20 00 32  	<unknown>
     50c: e0 1f 80 14  	<unknown>
     510: 10 00 10 28  	vmsumshm 0, 0, 2, 0
     514: f0 01 80 10  	xxsldwi 0, 1, 16, 0
     518: c0 41 00 10  	lfs 2, 16(1)
     51c: c0 1d 00 1c  	lfs 0, 28(29)
     520: 10 42 08 ba  	<unknown>
     524: 10 42 08 54  	mtvsrbmi 2, 2116
     528: fc 02 00 40  	fcmpo 0, 2, 0
     52c: 40 81 00 10  	bf	1, 0x53c <calc__14JPAFieldVortexFP18JPAEmitterWorkDataP13JPAFieldBlockP15JPABaseParticle+0x88>
     530: 80 7e 00 00  	lwz 3, 0(30)
     534: c3 e3 00 28  	lfs 31, 40(3)
     538: 48 00 00 2c  	b 0x564 <calc__14JPAFieldVortexFP18JPAEmitterWorkDataP13JPAFieldBlockP15JPABaseParticle+0xb0>
     53c: c0 1d 00 20  	lfs 0, 32(29)
     540: 80 7e 00 00  	lwz 3, 0(30)
     544: ec 82 00 32  	fmuls 4, 2, 0
     548: c0 00 00 00  	lfs 0, 0(0)
			00000548:  Unknown	@6069
     54c: c0 43 00 28  	lfs 2, 40(3)
     550: c0 7e 00 28  	lfs 3, 40(30)
     554: ec 20 20 28  	fsubs 1, 0, 4
     558: ec 04 00 b2  	fmuls 0, 4, 2
     55c: ec 21 00 f2  	fmuls 1, 1, 3
     560: ef e1 00 2a  	fadds 31, 1, 0
     564: 38 61 00 08  	addi 3, 1, 8
     568: 48 00 00 01  	bl 0x568 <calc__14JPAFieldVortexFP18JPAEmitterWorkDataP13JPAFieldBlockP15JPABaseParticle+0xb4>
			00000568:  R_PPC_REL24	normalizeOrZero__2MRFPQ29JGeometry8TVec3<f>
     56c: 38 61 00 08  	addi 3, 1, 8
     570: 38 9d 00 10  	addi 4, 29, 16
     574: 38 bd 00 04  	addi 5, 29, 4
     578: 48 00 00 01  	bl 0x578 <calc__14JPAFieldVortexFP18JPAEmitterWorkDataP13JPAFieldBlockP15JPABaseParticle+0xc4>
			00000578:  R_PPC_REL24	PSVECCrossProduct
     57c: fc 20 f8 90  	fmr 1, 31
     580: 38 7d 00 04  	addi 3, 29, 4
     584: 48 00 00 01  	bl 0x584 <calc__14JPAFieldVortexFP18JPAEmitterWorkDataP13JPAFieldBlockP15JPABaseParticle+0xd0>
			00000584:  R_PPC_REL24	scale__Q29JGeometry8TVec3<f>Ff
     588: 7f a3 eb 78  	mr	3, 29
     58c: 7f c4 f3 78  	mr	4, 30
     590: 7f e5 fb 78  	mr	5, 31
     594: 48 00 00 01  	bl 0x594 <calc__14JPAFieldVortexFP18JPAEmitterWorkDataP13JPAFieldBlockP15JPABaseParticle+0xe0>
			00000594:  R_PPC_REL24	calcAffect__12JPAFieldBaseFP13JPAFieldBlockP15JPABaseParticle
     598: e3 e1 00 38  	<unknown>
     59c: 39 61 00 30  	addi 11, 1, 48
     5a0: cb e1 00 30  	lfd 31, 48(1)
     5a4: 48 00 00 01  	bl 0x5a4 <calc__14JPAFieldVortexFP18JPAEmitterWorkDataP13JPAFieldBlockP15JPABaseParticle+0xf0>
			000005a4:  R_PPC_REL24	_restgpr_29
     5a8: 80 01 00 44  	lwz 0, 68(1)
     5ac: 7c 08 03 a6  	mtlr 0
     5b0: 38 21 00 40  	addi 1, 1, 64
     5b4: 4e 80 00 20  	blr
