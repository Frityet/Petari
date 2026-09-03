#!/usr/bin/env python3
"""Check exact root copies and separate actor base-transform integration."""

import hashlib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]


def body(path, key):
    text = (ROOT / path).read_text()
    start = text.index(key)
    end = text.index("{", start) + 1
    depth = 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


def main():
    copies = (
        ("src/Game/LiveActor/LiveActor.cpp", "pc-port/src/Game/LiveActor/LiveActor.cpp",
         "void LiveActor::calcAndSetBaseMtx()"),
        ("src/Game/Util/ActorSensorUtil.cpp", "pc-port/src/compat/GameActorSensorCompat.cpp",
         "HitSensor* getTaken("),
    )
    for original, native, key in copies:
        left, right = body(original, key), body(native, key)
        assert left == right, key
        print(key, hashlib.sha256(left.encode()).hexdigest())
    original = ROOT / "include/Game/LiveActor/HitSensorKeeper.hpp"
    native = ROOT / "pc-port/src/Game/LiveActor/HitSensorKeeper.hpp"
    assert original.read_bytes() == native.read_bytes()
    print("HitSensorKeeper header", hashlib.sha256(original.read_bytes()).hexdigest())
    actor = (ROOT / "pc-port/src/Game/LiveActor/LiveActor.cpp").read_text()
    assert "make_trs_matrix" not in actor
    calc = body("pc-port/src/Game/LiveActor/LiveActor.cpp", "void LiveActor::calcAnmMtx()")
    assert calc.index("MR::setBaseScale(this, mScale)") < calc.index("calcAndSetBaseMtx()")
    setter = body("pc-port/src/compat/LiveActorUtilCompat.cpp",
                  "void setBaseTRMtx(LiveActor* pActor, const smgpc::render::J3dMatrix3x4&")
    assert "j3d_apply_matrix_scale" not in setter
    assert "set_actor_base_matrix(pActor, matrix)" in setter
    renderer = (ROOT / "pc-port/src/render/live_actor/LiveActorModel.cpp").read_text()
    assert "options.base_scale = mBaseScale" in renderer
    for key in ("const smgpc::render::J3dMatrix3x4 *LiveActorModel::joint_world_matrix(",
                "void LiveActorModel::refresh_resolved_joint_matrices("):
        query = body("pc-port/src/render/live_actor/LiveActorModel.cpp", key)
        assert "joint_matrix(name, animation_frame, actor_matrix, mBaseScale)" in query
        assert "j3d_concat_matrix" not in query
    print("Separate retained model scale and literal base-TR query/draw connections agree.")


if __name__ == "__main__":
    main()
