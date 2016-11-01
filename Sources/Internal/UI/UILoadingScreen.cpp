#include "UI/UILoadingScreen.h"
#include "Platform/SystemTimer.h"
#include "UI/UIControlSystem.h"
#include "UI/UIScreenManager.h"
#include "Debug/Replay.h"
#include "Job/JobManager.h"
#include "Concurrency/Thread.h"
#include "UI/Update/UIUpdateComponent.h"

namespace DAVA
{
static const uint32 LOADING_THREAD_STACK_SIZE = 1024 * 1024; // 1 mb

UILoadingScreen::UILoadingScreen()
{
    AddComponent<UIUpdateComponent>()->SetUpdateFunction([this](float32 t) { Update(t); });
}

UILoadingScreen::~UILoadingScreen()
{
    DVASSERT(thread == nullptr);
}

void UILoadingScreen::SetScreenToLoad(int32 screenId)
{
    nextScreenId = screenId;
    DVASSERT(!thread.Valid());
    thread = nullptr;
}

void UILoadingScreen::ThreadMessage(BaseObject* obj, void* userData, void* callerData)
{
    UIScreen* nextScreen = (nextScreenId != -1) ? UIScreenManager::Instance()->GetScreen(nextScreenId) : nullptr;
    if (nextScreen != nullptr)
    {
        nextScreen->LoadGroup();
    }
}

void UILoadingScreen::OnActive()
{
    UIScreen::OnActive();

    if (!thread)
    {
        UIControlSystem::Instance()->LockSwitch();
        UIControlSystem::Instance()->LockInput();

        thread = Thread::Create(Message(this, &UILoadingScreen::ThreadMessage));
        thread->SetStackSize(LOADING_THREAD_STACK_SIZE);
        thread->Start();
    }

    if (Replay::IsRecord() || Replay::IsPlayback())
    {
        Replay::Instance()->PauseReplay(true);
    }
}

void UILoadingScreen::Update(float32 timeElapsed)
{
    if ((thread) && (thread->GetState() == Thread::STATE_ENDED))
    {
        JobManager::Instance()->WaitMainJobs(thread->GetId());

        UIControlSystem::Instance()->UnlockInput();
        UIControlSystem::Instance()->UnlockSwitch();

        UIScreenManager::Instance()->SetScreen(nextScreenId);

        thread = nullptr;
    }
}

void UILoadingScreen::OnInactive()
{
    UIScreen::OnInactive();

    if (Replay::Instance())
    {
        Replay::Instance()->PauseReplay(false);
        SystemTimer::Instance()->SetFrameDelta(0.33f); //TODO: this is temporary solution for "first frame after loading" issue
    }
}
};
