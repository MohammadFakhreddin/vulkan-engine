#pragma once

#include <functional>

namespace MFA {

using UIRecordObjectId = uint32_t;

class UIRecordObject 
{
public:

    using RecordFunction = std::function<void()>;

    explicit UIRecordObject(RecordFunction const & recordFunction);

    ~UIRecordObject();

    UIRecordObject & operator= (UIRecordObject && rhs) noexcept = delete;
    UIRecordObject (UIRecordObject const &) noexcept = delete;
    UIRecordObject (UIRecordObject && rhs) noexcept = delete;
    UIRecordObject & operator= (UIRecordObject const &) noexcept = delete;

    bool operator==(const UIRecordObject & rhs) const {
        return mId == rhs.mId;
    }

    void Enable();

    void Record() const;

    void Disable();

private:

    static UIRecordObjectId NextId;
    UIRecordObjectId const mId = 0;
    bool mIsActive = false;
    
    RecordFunction mDrawFunction;

};

}