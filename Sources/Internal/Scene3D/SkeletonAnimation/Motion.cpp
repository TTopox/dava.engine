#include "Motion.h"

#include "BlendTree.h"
#include "Base/GlobalEnum.h"
#include "FileSystem/YamlNode.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"

ENUM_DECLARE(DAVA::Motion::eMotionBlend)
{
    ENUM_ADD_DESCR(DAVA::Motion::eMotionBlend::BLEND_OVERRIDE, "Override");
    ENUM_ADD_DESCR(DAVA::Motion::eMotionBlend::BLEND_ADD, "Add");
    ENUM_ADD_DESCR(DAVA::Motion::eMotionBlend::BLEND_DIFF, "Diff");
    ENUM_ADD_DESCR(DAVA::Motion::eMotionBlend::BLEND_LERP, "LERP");
};

namespace DAVA
{
DAVA_REFLECTION_IMPL(Motion)
{
    ReflectionRegistrator<Motion>::Begin()
    .Field("name", &Motion::name)[M::ReadOnly()]
    .End();
}

Motion::~Motion()
{
    for (MotionTransitionInfo* t : transitions)
        SafeDelete(t);
}

void Motion::TriggerEvent(const FastName& trigger)
{
    MotionState* state = currentState->GetTransitionState(trigger);
    if (state != nullptr)
    {
        pendingState = state;
    }
}

void Motion::Update(float32 dTime)
{
    if (pendingState != nullptr)
    {
        if (currentState != nullptr)
        {
            if (nextState != nullptr && stateTransition.IsStarted())
            {
                MotionTransitionInfo* nextTransition = GetTransition(nextState, pendingState);
                if (nextTransition != nullptr && stateTransition.CanBeInterrupted(nextTransition, nextState, pendingState))
                {
                    stateTransition.Interrupt(nextTransition, nextState, pendingState);
                    currentState = nextState;
                    nextState = pendingState;

                    pendingState = nullptr;
                }
            }
            else
            {
                MotionTransitionInfo* nextTransition = GetTransition(currentState, pendingState);
                if (nextTransition != nullptr)
                {
                    stateTransition.Reset(nextTransition, currentState, pendingState);
                    nextState = pendingState;
                    nextState->Reset();

                    pendingState = nullptr;
                }
            }
        }
        else
        {
            currentState = pendingState;
            currentState->Reset();
        }
    }

    //////////////////////////////////////////////////////////////////////////

    if (nextState != nullptr) //transition is active
    {
        stateTransition.Update(dTime);
    }
    else
    {
        currentState->Update(dTime);
    }

    //////////////////////////////////////////////////////////////////////////

    //TODO: *Skinning* think about retrieving markers from transitions
    endedPhases.clear();
    const UnorderedSet<FastName>& reachedMarkers = currentState->GetReachedMarkers();
    for (const FastName& m : reachedMarkers)
    {
        if (!m.empty())
            endedPhases.emplace_back(currentState->GetID(), m);
    }

    //////////////////////////////////////////////////////////////////////////

    currentPose.Reset();
    if (nextState != nullptr) //transition is active
    {
        stateTransition.Evaluate(&currentPose, &currentRootOffsetDelta);
    }
    else
    {
        currentState->EvaluatePose(&currentPose);
        currentState->GetRootOffsetDelta(&currentRootOffsetDelta);
    }

    //////////////////////////////////////////////////////////////////////////

    if (nextState != nullptr) //transition is active
    {
        if (stateTransition.IsComplete())
        {
            currentState = nextState;
            nextState = nullptr;
        }
    }

    //////////////////////////////////////////////////////////////////////////

    //Temp for debug
    currentRootOffsetDelta.z = 0.f;

    Vector3 rootPosition = currentPose.GetJointTransform(0).GetPosition();
    rootPosition.x = rootPosition.y = 0.f;
    currentPose.SetPosition(0, rootPosition);
}

void Motion::BindSkeleton(const SkeletonComponent* skeleton)
{
    for (MotionState& s : states)
        s.BindSkeleton(skeleton);

    if (currentState != nullptr)
    {
        currentPose.Reset();
        currentState->Reset();
        currentState->EvaluatePose(&currentPose);
        currentState->GetRootOffsetDelta(&currentRootOffsetDelta);
    }
}

bool Motion::BindParameter(const FastName& parameterID, const float32* param)
{
    bool success = false;

    for (MotionState& s : states)
        success |= s.BindParameter(parameterID, param);

    return success;
}

bool Motion::UnbindParameter(const FastName& parameterID)
{
    return BindParameter(parameterID, nullptr);
}

void Motion::UnbindParameters()
{
    for (MotionState& s : states)
        s.UnbindParameters();
}

const Vector<FastName>& Motion::GetStateIDs() const
{
    return statesIDs;
}

uint32 Motion::GetTransitionIndex(const MotionState* srcState, const MotionState* dstState) const
{
    size_t srcStateIndex = std::distance(states.data(), srcState);
    size_t dstStateIndex = std::distance(states.data(), dstState);
    return uint32(srcStateIndex * states.size() + dstStateIndex);
}

MotionTransitionInfo* Motion::GetTransition(const MotionState* srcState, const MotionState* dstState) const
{
    if (srcState == nullptr || dstState == nullptr)
        return nullptr;

    return transitions[GetTransitionIndex(srcState, dstState)];
}

Motion* Motion::LoadFromYaml(const YamlNode* motionNode)
{
    Motion* motion = new Motion();

    int32 enumValue;
    Set<FastName> statesParameters;

    const YamlNode* nameNode = motionNode->Get("name");
    if (nameNode != nullptr && nameNode->GetType() == YamlNode::TYPE_STRING)
    {
        motion->name = nameNode->AsFastName();
    }

    const YamlNode* blendModeNode = motionNode->Get("blend-mode");
    if (blendModeNode != nullptr && blendModeNode->GetType() == YamlNode::TYPE_STRING)
    {
        if (GlobalEnumMap<Motion::eMotionBlend>::Instance()->ToValue(blendModeNode->AsString().c_str(), enumValue))
            motion->blendMode = eMotionBlend(enumValue);
    }

    const YamlNode* statesNode = motionNode->Get("states");
    if (statesNode != nullptr && statesNode->GetType() == YamlNode::TYPE_ARRAY)
    {
        uint32 statesCount = statesNode->GetCount();
        motion->states.resize(statesCount);
        motion->statesIDs.resize(statesCount);
        for (uint32 s = 0; s < statesCount; ++s)
        {
            MotionState& state = motion->states[s];
            state.LoadFromYaml(statesNode->Get(s));

            motion->statesIDs[s] = state.GetID();

            const Vector<FastName>& blendTreeParams = state.GetBlendTreeParameters();
            statesParameters.insert(blendTreeParams.begin(), blendTreeParams.end());
        }

        if (statesCount > 0)
        {
            motion->currentState = motion->states.data();
        }

        motion->transitions.resize(statesCount * statesCount, nullptr);
    }

    const YamlNode* transitionsNode = motionNode->Get("transitions");
    if (transitionsNode != nullptr && transitionsNode->GetType() == YamlNode::TYPE_ARRAY)
    {
        uint32 transitionsCount = transitionsNode->GetCount();
        for (uint32 t = 0; t < transitionsCount; ++t)
        {
            const YamlNode* transitionNode = transitionsNode->Get(t);

            const YamlNode* srcNode = transitionNode->Get("src-state");
            const YamlNode* dstNode = transitionNode->Get("dst-state");
            const YamlNode* triggerNode = transitionNode->Get("trigger");
            if (srcNode != nullptr && srcNode->GetType() == YamlNode::TYPE_STRING &&
                dstNode != nullptr && dstNode->GetType() == YamlNode::TYPE_STRING &&
                triggerNode != nullptr && triggerNode->GetType() == YamlNode::TYPE_STRING)
            {
                FastName srcName = srcNode->AsFastName();
                FastName dstName = dstNode->AsFastName();
                FastName trigger = triggerNode->AsFastName();

                auto foundSrc = std::find_if(motion->states.begin(), motion->states.end(), [&srcName](const MotionState& state) {
                    return state.GetID() == srcName;
                });

                auto foundDst = std::find_if(motion->states.begin(), motion->states.end(), [&dstName](const MotionState& state) {
                    return state.GetID() == dstName;
                });

                if (foundSrc != motion->states.end() && foundDst != motion->states.end())
                {
                    uint32 transitionIndex = motion->GetTransitionIndex(&(*foundSrc), &(*foundDst));
                    DVASSERT(motion->transitions[transitionIndex] == nullptr);
                    motion->transitions[transitionIndex] = MotionTransitionInfo::LoadFromYaml(transitionNode);

                    foundSrc->AddTransitionState(trigger, &(*foundDst));
                }
            }
        }
    }

    motion->parameterIDs.insert(motion->parameterIDs.begin(), statesParameters.begin(), statesParameters.end());

    return motion;
}

} //ns