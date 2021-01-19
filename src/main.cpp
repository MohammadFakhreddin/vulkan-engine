#define STB_IMAGE_IMPLEMENTATION
#include "./libs/stb_image/stb_image.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "./libs/tiny_obj_loader/tiny_obj_loader.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "./libs/stb_image/stb_image_resize.h"
#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "./libs/stb_image/stb_image_write.h"
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#define TINYGLTF_IMPLEMENTATION
#include "./libs/tiny_gltf_loader/tiny_gltf_loader.h"
#include "Application.hpp"
#include "engine/BedrockLog.hpp"

int main(int argc, char* argv[]){
    Application app;
    app.run();
    return 0;
}
