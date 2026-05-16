#pragma once

class LayoutActor;

namespace MR {

[[nodiscard]] bool isStarPointerPointingPane(const LayoutActor *pActor, const char *pPaneName, int port, bool check, const char *pStrength);

}  // namespace MR
