void MarioActor::updateRandomTexture(f32 value) {
    _B88 = 1 - _B88;
    u8* pImage = _B80[_B88]->mImage;
    f32 chance = MR::clamp(1.0f - value / 1000.0f, 0.0f, 1.0f);

    for (u32 y = 0; y < 8; y++) {
        for (u32 x = 0; x < 8; x++) {
            s32 intensity = pImage[x + y * 8] >> 4;
            if (MR::getRandom() < chance) {
                intensity += 4;
            } else {
                intensity--;
            }
            pImage[x + y * 8] = MR::clamp(intensity, 0, 15) << 4;
        }
    }
    DCStoreRange(pImage, 64);
}
