    void findNamePosOnGround(const char* pName, MtxPtr pMtx) {
        Triangle triangle;
        TPos3f mtx;
        findNamePos(pName, mtx.toMtxPtr());

        TVec3f pos;
        mtx.getTrans(pos);
        TVec3f front;
        mtx.getZDir(front);
        TVec3f gravity;
        calcGravityVector(nullptr, pos, &gravity, nullptr, 0);

        TVec3f groundPos;
        if (getFirstPolyOnLineToMap(&groundPos, &triangle, pos - gravity * 100.0f, gravity * 1000.0f)) {
            makeMtxUpFrontPos(&mtx, -gravity, front, groundPos);
        }

        PSMTXCopy(mtx.toMtxPtr(), pMtx);
    }
