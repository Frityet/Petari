#include "Game/System/StationedArchiveLoader.hpp"
#include "Game/Util/FileUtil.hpp"

void StationedArchiveLoader::loadScenarioData(JKRHeap* pHeap) {
    DVDDir dir;
    DVDDirEntry dirEntry;

    DVDOpenDir("/StageData", &dir);

    while (DVDReadDir(&dir, &dirEntry)) {
        if (!dirEntry.isDir) {
            continue;
        }

        char name[256];
        MR::makeScenarioArchiveFileName(name, sizeof(name), dirEntry.name);

        if (MR::isFileExist(name, false)) {
            MR::mountArchive(name, pHeap);
        }
    }

    DVDCloseDir(&dir);
}
