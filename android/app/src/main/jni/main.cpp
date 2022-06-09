// Copyright 2016 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#define STB_IMAGE_IMPLEMENTATION
#include "../src/libs/stb_image/stb_image.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "../src/libs/tiny_obj_loader/tiny_obj_loader.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "../src/libs/stb_image/stb_image_resize.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../src/libs/stb_image/stb_image_write.h"
#include "../src/libs/nlohmann/json.hpp"
#define TINYGLTF_ANDROID_LOAD_FROM_ASSETS
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_NO_EXTERNAL_IMAGE  // For disabling reading images when decoding json files
#define TINYGLTF_IMPLEMENTATION
#include "../src/libs/tiny_gltf_loader/tiny_gltf_loader.h"
#define TINYKTX_IMPLEMENTATION
#include "../src/libs/tiny_ktx/tinyktx.h"

#include "vulkan_wrapper/vulkan_wrapper.h"

#define VULKAN_H_
#define VMA_IMPLEMENTATION
#include "../src/libs/vma/vk_mem_alloc.h"

#include "Application.hpp"
#include "engine/InputManager.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"

#include <android/log.h>
#include <android_native_app_glue.h>

Application application {};

// Process the next main command.
void commandListener(android_app* app, int32_t cmd) {
  switch (cmd) {
    case APP_CMD_INIT_WINDOW:
    {// The window is being shown, get it ready.
        if(!InitVulkan()){
            MFA_CRASH("Vulkan is not supported!");
            return;
        }
//        application.Init();
    }
    break;
    case APP_CMD_TERM_WINDOW:
    {// The window is being hidden or closed, clean it up.
      application.Shutdown();
    }
    break;
    case APP_CMD_CONTENT_RECT_CHANGED:
    {
        MFA::RenderFrontend::NotifyDeviceResized();
    }
    break;
    default:
      MFA_LOG_INFO("Event not handled: %d", cmd);
  }
}

void android_main(struct android_app* app) {

  // Set the callback to process system events
  app->onAppCmd = commandListener;

  application.setAndroidApp(app);
  tinygltf::asset_manager = app->activity->assetManager;
  // Main loop
  application.run();
}
