#include <string>
#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#include "Application.hpp"

int main(int argc, char* argv[]){
    Application app;
    app.run();    
    return 0;
}