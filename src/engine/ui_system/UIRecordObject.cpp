#include "UIRecordObject.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/ui_system/UISystem.hpp"

namespace MFA {

UIRecordObjectId UIRecordObject::NextId = 0;    

UIRecordObject::UIRecordObject(RecordFunction const & recordFunction)
    : mId(NextId)
{
    MFA_ASSERT(recordFunction != nullptr);
    mDrawFunction = recordFunction;
    NextId += 1;
}


UIRecordObject::~UIRecordObject() {
    if (mIsActive) {
        Disable();
    }
}

void UIRecordObject::Enable() {
    if (mIsActive) {
        return;
    }
    UISystem::Register(this);
    mIsActive = true;
}

void UIRecordObject::Record() const {
    MFA_ASSERT(mDrawFunction != nullptr);
    mDrawFunction();
}

void UIRecordObject::Disable() {
    if (mIsActive == false) {
        return;
    }
    UISystem::UnRegister(this);
    mIsActive = false;
}

}
