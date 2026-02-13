#pragma once

namespace MR {

void startStageBGM(const char *name, bool prepare);
[[nodiscard]] bool isPreparedStageBgm();
void unlockStageBGM();
void stopStageBGM(int fadeFrames);
void startSystemSE(const char *name, int, int);
void startCSSound(const char *name, int, int);

}  // namespace MR
