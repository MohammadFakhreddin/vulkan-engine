#include "engine/BedrockPath.hpp"

#import <Foundation/Foundation.h>

std::string MFA::Path::GetAssetPath() {
    return [NSBundle.mainBundle.resourcePath stringByAppendingString: @"/data/"].UTF8String;
}