isHiddenPane__2MRFPC11LayoutActorPCc
stwu r1, -0x10(r1)
mflr r0
stw r0, 0x14(r1)
stw r31, 0xc(r1)
mr r31, r4
bl getLayoutManager__11LayoutActorCFv
mr r4, r31
bl getPane__13LayoutManagerCFPCc
lbz r0, 0xb7(r3)
lwz r31, 0xc(r1)
clrlwi r0, r0, 31
cntlzw r0, r0
srwi r3, r0, 5
lwz r0, 0x14(r1)
mtlr r0
addi r1, r1, 0x10
blr 

copyPaneTrans__2MRFPQ29JGeometry8TVec2<f>PC11LayoutActorPCc
stwu r1, -0x10(r1)
mflr r0
stw r0, 0x14(r1)
stw r31, 0xc(r1)
mr r31, r5
stw r30, 0x8(r1)
mr r30, r3
mr r3, r4
bl getLayoutManager__11LayoutActorCFv
mr r4, r31
bl getPane__13LayoutManagerCFPCc
lfs f0, 0x90(r3)
mr r4, r30
stfs f0, 0x0(r30)
lfs f0, 0xa0(r3)
mr r3, r30
stfs f0, 0x4(r30)
bl convertLayoutPosToScreenPos__2MRFPQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>
lwz r0, 0x14(r1)
lwz r31, 0xc(r1)
lwz r30, 0x8(r1)
mtlr r0
addi r1, r1, 0x10
blr 

getPaneTransX__2MRFPC11LayoutActorPCc
stwu r1, -0x20(r1)
mflr r0
stw r0, 0x24(r1)
stw r31, 0x1c(r1)
mr r31, r4
bl getLayoutManager__11LayoutActorCFv
mr r4, r31
bl getPane__13LayoutManagerCFPCc
mr r4, r3
addi r3, r1, 0x8
lfs f1, 0x90(r4)
lfs f2, 0xa0(r4)
bl __ct<f>__Q29JGeometry8TVec2<f>Fff_Pv
addi r3, r1, 0x8
mr r4, r3
bl convertLayoutPosToScreenPos__2MRFPQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>
lwz r31, 0x1c(r1)
lwz r0, 0x24(r1)
lfs f1, 0x8(r1)
mtlr r0
addi r1, r1, 0x20
blr 

getPaneTransY__2MRFPC11LayoutActorPCc
stwu r1, -0x20(r1)
mflr r0
stw r0, 0x24(r1)
stw r31, 0x1c(r1)
mr r31, r4
bl getLayoutManager__11LayoutActorCFv
mr r4, r31
bl getPane__13LayoutManagerCFPCc
mr r4, r3
addi r3, r1, 0x8
lfs f1, 0x90(r4)
lfs f2, 0xa0(r4)
bl __ct<f>__Q29JGeometry8TVec2<f>Fff_Pv
addi r3, r1, 0x8
mr r4, r3
bl convertLayoutPosToScreenPos__2MRFPQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>
lwz r31, 0x1c(r1)
lwz r0, 0x24(r1)
lfs f1, 0xc(r1)
mtlr r0
addi r1, r1, 0x20
blr 

setLayoutPosAtPaneTrans__2MRFP11LayoutActorPC11LayoutActorPCc
stwu r1, -0x20(r1)
mflr r0
stw r0, 0x24(r1)
stw r31, 0x1c(r1)
mr r31, r3
addi r3, r1, 0x8
bl copyPaneTrans__2MRFPQ29JGeometry8TVec2<f>PC11LayoutActorPCc
mr r3, r31
addi r4, r1, 0x8
bl setTrans__11LayoutActorFRCQ29JGeometry8TVec2<f>
lwz r0, 0x24(r1)
lwz r31, 0x1c(r1)
mtlr r0
addi r1, r1, 0x20
blr 
