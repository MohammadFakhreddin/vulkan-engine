#define STB_IMAGE_IMPLEMENTATION
#include "./libs/stb_image/stb_image.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "./libs/tiny_obj_loader/tiny_obj_loader.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "./libs/stb_image/stb_image_resize.h"

#include "Application.hpp"
#include "engine/BedrockLog.hpp"

int main(int argc, char* argv[]){
    Application app;
    app.run();
    return 0;
}
