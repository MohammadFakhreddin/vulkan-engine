#define STB_IMAGE_IMPLEMENTATION
#include "../src/libs/stb_image/stb_image.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "../src/libs/tiny_obj_loader/tiny_obj_loader.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "../src/libs/stb_image/stb_image_resize.h"
#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../src/libs/stb_image/stb_image_write.h"
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE  // For disabling reading images when decoding json files
#define TINYGLTF_IMPLEMENTATION
#include "../src/libs/tiny_gltf_loader/tiny_gltf_loader.h"
#define TINYKTX_IMPLEMENTATION 
#include "../src/libs/tiny_ktx/tinyktx.h"

#include "../src/Application.hpp"

int main(int argc, char* argv[]){
    Application app;
    app.run();
    return 0;
}
