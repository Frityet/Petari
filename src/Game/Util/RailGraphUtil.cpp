#include "Game/Util/RailGraphUtil.hpp"
#include "Game/Map/RailGraph.hpp"
#include "Game/Map/RailGraphEdge.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SceneUtil.hpp"

namespace MR {
    RailGraph* createRailGraphFromJMap(const JMapInfoIter& rIter) {
        RailGraph* graph = new RailGraph();

        const JMapInfo* railPointInfo = nullptr;
        JMapInfoIter railIter(nullptr, -1);
        bool hasNext = true;
        MR::getRailInfo(&railIter, &railPointInfo, rIter);

        while (hasNext) {
            JMapInfoIter pathIter(railIter);
            const JMapInfo* curRailPointInfo = railPointInfo;
            s32 pointNum = curRailPointInfo->getNumEntries();
            s32 prevNode = -1;
            RailGraphEdge edge;
            s32 firstNode;

            for (s32 i = 0; i < pointNum; i++) {
                JMapInfoIter pointIter(curRailPointInfo, i);
                TVec3f pointPos;
                MR::getRailPointPos0(pointIter, &pointPos);

                s32 nearNode = MR::getNearNodeIndex(graph, pointPos, 100.0f, nullptr);
                s32 node;
                if (nearNode == -1) {
                    node = graph->addNode(pointPos);
                } else {
                    node = nearNode;
                }

                if (prevNode != -1 && node != prevNode) {
                    graph->connectNodeTwoWay(prevNode, node, &edge);
                }

                if (i == 0) {
                    firstNode = node;
                }

                prevNode = node;
                edge.setArgs(curRailPointInfo, i);
            }

            if (MR::isLoopRailPathIter(pathIter) && prevNode != -1 && firstNode != prevNode) {
                graph->connectNodeTwoWay(prevNode, firstNode, &edge);
            }

            hasNext = MR::getNextLinkRailInfo(&railIter, &railPointInfo, railIter);
        }

        return graph;
    }

    RailGraphIter* createRailGraphIter(const RailGraph* pGraph) {
        return new RailGraphIter(pGraph->getIterator());
    }

    void moveNextNode(RailGraphIter* pIter) {
        pIter->moveNodeNext();
    }

    void moveNodeNearPosition(RailGraphIter* pRailGraphIter, const TVec3f& rVec, f32 f, RailGraphNodeSelecter* pSelector) {
        pRailGraphIter->setNode(getNearNodeIndex(pRailGraphIter->mGraph, rVec, f, pSelector));
    }

    void selectReverseEdge(RailGraphIter* pRailGraphIter) {
        s32 next = pRailGraphIter->mSelectedEdge;
        pRailGraphIter->moveNodeNext();
        pRailGraphIter->selectEdge(next);
    }

    bool isSelectedEdge(const RailGraphIter* pRailGraphIter) {
        return pRailGraphIter->isSelectedEdge();
    }

    bool isWatchedPrevEdge(const RailGraphIter* pRailGraphIter) {
        return pRailGraphIter->isWatchedPrevEdge();
    }

    TVec3f* getCurrentNodePosition(const RailGraphIter* pGraph) {
        return &pGraph->getCurrentNode()->_0;
    }

    TVec3f* getNextNodePosition(const RailGraphIter* pIter) {
        return &pIter->getNextNode()->_0;
    }

#pragma dont_inline on
    void calcWatchEdgeVector(const RailGraphIter* pIter, TVec3f* pEdge) {
        const RailGraphNode* currentNode = pIter->getCurrentNode();
        const RailGraphNode* watchNode = pIter->getWatchNode();
        pEdge->set< f32 >(watchNode->_0 - currentNode->_0);
    }
#pragma dont_inline reset

    void calcWatchEdgeDirection(const RailGraphIter* pRailGraphIter, TVec3f* pVec) {
        calcWatchEdgeVector(pRailGraphIter, pVec);
        MR::normalize(pVec);
    }

    s32 getNearNodeIndex(const RailGraph* pGraph, const TVec3f& rPos, f32 maxDist, RailGraphNodeSelecter* pSelector) {
        f32 nearestDist;
        if (maxDist < 0.0f) {
            nearestDist = FLOAT_MAX;
        } else {
            nearestDist = maxDist;
        }

        s32 nodeCount = pGraph->_8;
        s32 nearest = -1;
        RailGraphIter iter = pGraph->getIterator();

        for (s32 i = 0; i < nodeCount; i++) {
            if (pSelector != nullptr) {
                iter.setNode(i);
                if (!pSelector->isSatisfy(iter)) {
                    continue;
                }
            }

            f32 dist = PSVECDistance((const Vec*)&rPos, (const Vec*)&pGraph->getNode(i)->_0);
            if (dist < nearestDist) {
                nearestDist = dist;
                nearest = i;
            }
        }

        return nearest;
    }

    s32 getSelectEdgeArg0(const RailGraphIter* pIter) {
        return pIter->getCurrentEdge()->mPointArg0;
    }

    s32 getSelectEdgeArg1(const RailGraphIter* pIter) {
        return pIter->getCurrentEdge()->mPointArg1;
    }

    s32 getSelectEdgeArg2(const RailGraphIter* pIter) {
        return pIter->getCurrentEdge()->mPointArg2;
    }

    s32 getSelectEdgeArg3(const RailGraphIter* pIter) {
        return pIter->getCurrentEdge()->mPointArg3;
    }

    s32 getWatchEdgeArg7(const RailGraphIter* pIter) {
        return pIter->getWatchEdge()->mPointArg7;
    }
};  // namespace MR
