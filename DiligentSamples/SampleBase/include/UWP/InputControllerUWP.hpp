/*     Copyright 2015-2019 Egor Yusov
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
 *
 *  In no event and under no legal theory, whether in tort (including negligence), 
 *  contract, or otherwise, unless required by applicable law (such as deliberate 
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental, 
 *  or consequential damages of any character arising as a result of this License or 
 *  out of the use or inability to use the software (including but not limited to damages 
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and 
 *  all other commercial damages or losses), even if such Contributor has been advised 
 *  of the possibility of such damages.
 */

#pragma once

#include <memory>
#include <mutex>

namespace Diligent
{

class InputControllerUWP
{
public:
    class SharedControllerState : private InputControllerBase
    {
    public:
        MouseState GetMouseState()
        {
            std::lock_guard<std::mutex> lock(mtx);
            return InputControllerBase::GetMouseState();
        }

        INPUT_KEY_STATE_FLAGS GetKeyState(InputKeys Key)
        {
            std::lock_guard<std::mutex> lock(mtx);
            return InputControllerBase::GetKeyState(Key);
        }

        bool IsKeyDown(InputKeys Key)
        {
            return (GetKeyState(Key) & INPUT_KEY_STATE_FLAG_KEY_IS_DOWN) != 0;
        }

        void ClearState()
        {
            std::lock_guard<std::mutex> lock(mtx);
            InputControllerBase::ClearState();
        }

        void OnKeyDown(InputKeys Key)
        {
            std::lock_guard<std::mutex> lock(mtx);

            auto& keyState = m_Keys[static_cast<size_t>(Key)];
            keyState &= ~INPUT_KEY_STATE_FLAG_KEY_WAS_DOWN;
            keyState |= INPUT_KEY_STATE_FLAG_KEY_IS_DOWN;
        }

        void OnKeyUp(InputKeys Key)
        {
            std::lock_guard<std::mutex> lock(mtx);

            auto& keyState = m_Keys[static_cast<size_t>(Key)];
            keyState &= ~INPUT_KEY_STATE_FLAG_KEY_IS_DOWN;
            keyState |= INPUT_KEY_STATE_FLAG_KEY_WAS_DOWN;
        }

        void MouseButtonPressed(MouseState::BUTTON_FLAGS Flags)
        {
            std::lock_guard<std::mutex> lock(mtx);
            m_MouseState.ButtonFlags |= Flags;
        }

        void MouseButtonReleased(MouseState::BUTTON_FLAGS Flags)
        {
            std::lock_guard<std::mutex> lock(mtx);
            m_MouseState.ButtonFlags &= ~Flags;
        }

        void SetMousePose(float x, float y)
        {
            std::lock_guard<std::mutex> lock(mtx);
            m_MouseState.PosX = x;
            m_MouseState.PosY = y;
        }

        void SetMouseWheelDetlta(float w)
        {
            std::lock_guard<std::mutex> lock(mtx);
            m_MouseState.WheelDelta = w;
        }

    private:
        std::mutex mtx;
    };

    MouseState GetMouseState()
    {
        return m_SharedState->GetMouseState();
    }

    INPUT_KEY_STATE_FLAGS GetKeyState(InputKeys Key) const
    {
        return m_SharedState->GetKeyState(Key);
    }

    bool IsKeyDown(InputKeys Key) const
    {
        return m_SharedState->IsKeyDown(Key);
    }

    std::shared_ptr<SharedControllerState> GetSharedState()
    {
        return m_SharedState;
    }

    void ClearState()
    {
        m_SharedState->ClearState();
    }

private:
    std::shared_ptr<SharedControllerState> m_SharedState{new SharedControllerState};
};

} // namespace Diligent
