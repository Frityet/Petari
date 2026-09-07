#include "Game/Player/Mario.hpp"
#include "Game/Player/J3DModelX.hpp"
extern "C" u32 set_MovementStatesjumping(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields.jumping = input;
    return value.words[0];
}
extern "C" u32 get_MovementStatesjumping(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields.jumping;
}
extern "C" u32 set_MovementStates_1(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._1 = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_1(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._1;
}
extern "C" u32 set_MovementStates_2(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._2 = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_2(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._2;
}
extern "C" u32 set_MovementStatesturning(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields.turning = input;
    return value.words[0];
}
extern "C" u32 get_MovementStatesturning(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields.turning;
}
extern "C" u32 set_MovementStates_4(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._4 = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_4(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._4;
}
extern "C" u32 set_MovementStates_5(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._5 = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_5(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._5;
}
extern "C" u32 set_MovementStates_6(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._6 = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_6(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._6;
}
extern "C" u32 set_MovementStates_7(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._7 = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_7(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._7;
}
extern "C" u32 set_MovementStates_8(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._8 = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_8(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._8;
}
extern "C" u32 set_MovementStates_9(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._9 = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_9(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._9;
}
extern "C" u32 set_MovementStates_A(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._A = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_A(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._A;
}
extern "C" u32 set_MovementStates_B(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._B = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_B(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._B;
}
extern "C" u32 set_MovementStates_C(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._C = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_C(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._C;
}
extern "C" u32 set_MovementStates_D(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._D = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_D(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._D;
}
extern "C" u32 set_MovementStates_E(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._E = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_E(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._E;
}
extern "C" u32 set_MovementStates_F(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._F = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_F(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._F;
}
extern "C" u32 set_MovementStates_10(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._10 = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_10(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._10;
}
extern "C" u32 set_MovementStates_11(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._11 = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_11(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._11;
}
extern "C" u32 set_MovementStates_12(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._12 = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_12(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._12;
}
extern "C" u32 set_MovementStates_13(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._13 = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_13(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._13;
}
extern "C" u32 set_MovementStates_14(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._14 = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_14(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._14;
}
extern "C" u32 set_MovementStates_15(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._15 = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_15(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._15;
}
extern "C" u32 set_MovementStatesdebugMode(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields.debugMode = input;
    return value.words[0];
}
extern "C" u32 get_MovementStatesdebugMode(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields.debugMode;
}
extern "C" u32 set_MovementStates_17(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._17 = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_17(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._17;
}
extern "C" u32 set_MovementStates_18(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._18 = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_18(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._18;
}
extern "C" u32 set_MovementStates_19(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._19 = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_19(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._19;
}
extern "C" u32 set_MovementStates_1A(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._1A = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_1A(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._1A;
}
extern "C" u32 set_MovementStates_1B(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._1B = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_1B(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._1B;
}
extern "C" u32 set_MovementStates_1C(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._1C = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_1C(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._1C;
}
extern "C" u32 set_MovementStates_1D(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._1D = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_1D(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._1D;
}
extern "C" u32 set_MovementStatesdigitalJump(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields.digitalJump = input;
    return value.words[0];
}
extern "C" u32 get_MovementStatesdigitalJump(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields.digitalJump;
}
extern "C" u32 set_MovementStates_1F(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._1F = input;
    return value.words[0];
}
extern "C" u32 get_MovementStates_1F(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = input;
    value.words[1] = 0;
    return value.fields._1F;
}
extern "C" u32 set_MovementStates_20(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._20 = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_20(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._20;
}
extern "C" u32 set_MovementStates_21(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._21 = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_21(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._21;
}
extern "C" u32 set_MovementStates_22(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._22 = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_22(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._22;
}
extern "C" u32 set_MovementStates_23(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._23 = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_23(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._23;
}
extern "C" u32 set_MovementStates_24(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._24 = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_24(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._24;
}
extern "C" u32 set_MovementStates_25(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._25 = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_25(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._25;
}
extern "C" u32 set_MovementStates_26(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._26 = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_26(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._26;
}
extern "C" u32 set_MovementStates_27(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._27 = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_27(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._27;
}
extern "C" u32 set_MovementStates_28(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._28 = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_28(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._28;
}
extern "C" u32 set_MovementStates_29(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._29 = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_29(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._29;
}
extern "C" u32 set_MovementStates_2A(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._2A = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_2A(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._2A;
}
extern "C" u32 set_MovementStates_2B(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._2B = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_2B(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._2B;
}
extern "C" u32 set_MovementStates_2C(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._2C = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_2C(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._2C;
}
extern "C" u32 set_MovementStates_2D(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._2D = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_2D(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._2D;
}
extern "C" u32 set_MovementStates_2E(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._2E = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_2E(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._2E;
}
extern "C" u32 set_MovementStates_2F(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._2F = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_2F(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._2F;
}
extern "C" u32 set_MovementStates_30(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._30 = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_30(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._30;
}
extern "C" u32 set_MovementStates_31(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._31 = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_31(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._31;
}
extern "C" u32 set_MovementStates_32(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._32 = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_32(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._32;
}
extern "C" u32 set_MovementStates_33(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._33 = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_33(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._33;
}
extern "C" u32 set_MovementStates_34(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._34 = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_34(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._34;
}
extern "C" u32 set_MovementStates_35(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._35 = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_35(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._35;
}
extern "C" u32 set_MovementStates_36(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._36 = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_36(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._36;
}
extern "C" u32 set_MovementStates_37(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._37 = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_37(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._37;
}
extern "C" u32 set_MovementStates_38(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._38 = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_38(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._38;
}
extern "C" u32 set_MovementStates_39(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._39 = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_39(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._39;
}
extern "C" u32 set_MovementStates_3A(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._3A = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_3A(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._3A;
}
extern "C" u32 set_MovementStates_3B(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._3B = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_3B(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._3B;
}
extern "C" u32 set_MovementStates_3C(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._3C = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_3C(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._3C;
}
extern "C" u32 set_MovementStates_3D(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._3D = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_3D(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._3D;
}
extern "C" u32 set_MovementStates_3E(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = 0;
    value.fields._3E = input;
    return value.words[1];
}
extern "C" u32 get_MovementStates_3E(u32 input) {
    union { Mario::MovementStates fields; u32 words[2]; } value;
    value.words[0] = 0;
    value.words[1] = input;
    return value.fields._3E;
}
extern "C" u32 set_DrawStates_0(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._0 = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_0(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._0;
}
extern "C" u32 set_DrawStates_1(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._1 = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_1(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._1;
}
extern "C" u32 set_DrawStates_2(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._2 = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_2(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._2;
}
extern "C" u32 set_DrawStates_3(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._3 = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_3(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._3;
}
extern "C" u32 set_DrawStates_4(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._4 = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_4(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._4;
}
extern "C" u32 set_DrawStates_5(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._5 = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_5(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._5;
}
extern "C" u32 set_DrawStates_6(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._6 = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_6(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._6;
}
extern "C" u32 set_DrawStates_7(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._7 = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_7(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._7;
}
extern "C" u32 set_DrawStates_8(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._8 = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_8(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._8;
}
extern "C" u32 set_DrawStates_9(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._9 = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_9(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._9;
}
extern "C" u32 set_DrawStates_A(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._A = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_A(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._A;
}
extern "C" u32 set_DrawStates_B(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._B = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_B(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._B;
}
extern "C" u32 set_DrawStates_C(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._C = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_C(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._C;
}
extern "C" u32 set_DrawStates_D(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._D = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_D(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._D;
}
extern "C" u32 set_DrawStates_E(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._E = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_E(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._E;
}
extern "C" u32 set_DrawStates_F(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._F = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_F(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._F;
}
extern "C" u32 set_DrawStates_10(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._10 = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_10(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._10;
}
extern "C" u32 set_DrawStates_11(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._11 = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_11(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._11;
}
extern "C" u32 set_DrawStatesmIsUnderwater(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields.mIsUnderwater = input;
    return value.words[0];
}
extern "C" u32 get_DrawStatesmIsUnderwater(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields.mIsUnderwater;
}
extern "C" u32 set_DrawStates_13(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._13 = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_13(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._13;
}
extern "C" u32 set_DrawStates_14(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._14 = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_14(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._14;
}
extern "C" u32 set_DrawStates_15(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._15 = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_15(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._15;
}
extern "C" u32 set_DrawStates_16(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._16 = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_16(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._16;
}
extern "C" u32 set_DrawStates_17(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._17 = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_17(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._17;
}
extern "C" u32 set_DrawStates_18(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._18 = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_18(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._18;
}
extern "C" u32 set_DrawStates_19(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._19 = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_19(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._19;
}
extern "C" u32 set_DrawStates_1A(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._1A = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_1A(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._1A;
}
extern "C" u32 set_DrawStates_1B(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._1B = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_1B(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._1B;
}
extern "C" u32 set_DrawStates_1C(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._1C = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_1C(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._1C;
}
extern "C" u32 set_DrawStates_1D(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._1D = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_1D(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._1D;
}
extern "C" u32 set_DrawStates_1E(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._1E = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_1E(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._1E;
}
extern "C" u32 set_DrawStates_1F(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._1F = input;
    return value.words[0];
}
extern "C" u32 get_DrawStates_1F(u32 input) {
    union { Mario::DrawStates fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._1F;
}
extern "C" u32 set_J3DModelX_Flags_0(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._0 = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_0(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._0;
}
extern "C" u32 set_J3DModelX_Flags_1(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._1 = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_1(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._1;
}
extern "C" u32 set_J3DModelX_Flags_2(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._2 = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_2(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._2;
}
extern "C" u32 set_J3DModelX_Flags_3(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._3 = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_3(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._3;
}
extern "C" u32 set_J3DModelX_Flags_4(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._4 = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_4(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._4;
}
extern "C" u32 set_J3DModelX_Flags_5(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._5 = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_5(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._5;
}
extern "C" u32 set_J3DModelX_Flags_6(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._6 = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_6(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._6;
}
extern "C" u32 set_J3DModelX_Flags_7(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._7 = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_7(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._7;
}
extern "C" u32 set_J3DModelX_Flags_8(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._8 = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_8(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._8;
}
extern "C" u32 set_J3DModelX_Flags_9(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._9 = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_9(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._9;
}
extern "C" u32 set_J3DModelX_Flags_A(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._A = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_A(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._A;
}
extern "C" u32 set_J3DModelX_Flags_B(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._B = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_B(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._B;
}
extern "C" u32 set_J3DModelX_Flags_C(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._C = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_C(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._C;
}
extern "C" u32 set_J3DModelX_Flags_D(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._D = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_D(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._D;
}
extern "C" u32 set_J3DModelX_Flags_E(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._E = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_E(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._E;
}
extern "C" u32 set_J3DModelX_Flags_F(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._F = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_F(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._F;
}
extern "C" u32 set_J3DModelX_Flags_10(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._10 = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_10(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._10;
}
extern "C" u32 set_J3DModelX_Flags_11(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._11 = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_11(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._11;
}
extern "C" u32 set_J3DModelX_Flags_12(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._12 = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_12(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._12;
}
extern "C" u32 set_J3DModelX_Flags_13(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._13 = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_13(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._13;
}
extern "C" u32 set_J3DModelX_Flags_14(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._14 = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_14(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._14;
}
extern "C" u32 set_J3DModelX_Flags_15(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._15 = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_15(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._15;
}
extern "C" u32 set_J3DModelX_Flags_16(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._16 = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_16(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._16;
}
extern "C" u32 set_J3DModelX_Flags_17(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._17 = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_17(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._17;
}
extern "C" u32 set_J3DModelX_Flags_18(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._18 = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_18(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._18;
}
extern "C" u32 set_J3DModelX_Flags_19(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._19 = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_19(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._19;
}
extern "C" u32 set_J3DModelX_Flags_1A(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._1A = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_1A(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._1A;
}
extern "C" u32 set_J3DModelX_Flags_1B(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._1B = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_1B(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._1B;
}
extern "C" u32 set_J3DModelX_Flags_1C(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = 0;
    value.fields._1C = input;
    return value.words[0];
}
extern "C" u32 get_J3DModelX_Flags_1C(u32 input) {
    union { J3DModelX::Flags fields; u32 words[1]; } value;
    value.words[0] = input;
    return value.fields._1C;
}
