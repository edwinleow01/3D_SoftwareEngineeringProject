#include "pch.h"
#include "TextureLoader.h"
#include <glew.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>
#include <algorithm>
#include <thread>
#include "TextureLoaderDDS.h"
#include "EnginePath.h"
#include <filesystem>

// ------------------------------------------------------------------
// Static member definitions
// ------------------------------------------------------------------
std::unordered_map<std::string, unsigned int>  TextureLoader::s_texCache;
std::queue<TextureLoader::PendingUpload>       TextureLoader::s_uploadQueue;
std::mutex                                     TextureLoader::s_queueMutex;

// ======================================================================
//  SYNCHRONOUS  loadTexture  (unchanged behaviour for existing callers)
// ======================================================================
unsigned int TextureLoader::loadTexture(TextureProperties& props, bool flip)
{
    // ------------------------------------------------------------------
    // Cache hit
    // ------------------------------------------------------------------
    auto it = s_texCache.find(props.path);
    if (it != s_texCache.end()) {
        props.textureID = it->second;
        return props.textureID;
    }

    // ------------------------------------------------------------------
    // Resolve absolute path
    // ------------------------------------------------------------------
    std::filesystem::path root = GetEngineRootPath();
    std::filesystem::path absolutePath = root / props.path;
    std::string           pathStr = absolutePath.string();

    if (!std::filesystem::exists(absolutePath)) {
        std::cerr << "[TextureLoader] File not found: " << pathStr << std::endl;
        return 0;
    }

    // ------------------------------------------------------------------
    // Extension detection
    // ------------------------------------------------------------------
    std::string ext;
    size_t dot = props.path.find_last_of('.');
    if (dot != std::string::npos) {
        ext = props.path.substr(dot + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    }

    // ------------------------------------------------------------------
    // DDS � handled separately (already GPU-safe internally)
    // ------------------------------------------------------------------
    if (ext == "dds") {
        GLuint texID = LoadDDS(pathStr);
        if (texID != 0) {
            s_texCache[props.path] = texID;
            props.textureID = texID;
        }
        return props.textureID;
    }

    // ------------------------------------------------------------------
    // STB load  (CPU)
    // ------------------------------------------------------------------
    int            width = 0, height = 0, channels = 0;
    stbi_set_flip_vertically_on_load(flip);
    unsigned char* data = stbi_load(pathStr.c_str(), &width, &height, &channels, 0);
    stbi_set_flip_vertically_on_load(false);

    if (!data || width <= 0 || height <= 0) {
        std::cerr << "[STB Error] " << stbi_failure_reason()
            << " : " << pathStr << std::endl;
        return 0;
    }

    // ------------------------------------------------------------------
    // GPU upload  (must be on main / GL thread � which it is here)
    // ------------------------------------------------------------------
    PendingUpload item;
    item.data = data;
    item.width = width;
    item.height = height;
    item.channels = channels;
    item.path = props.path;
    item.flip = flip;
    item.props = &props;
    item.callback = nullptr;

    unsigned int texID = uploadToGPU(item);   // frees data internally
    return texID;
}

// ======================================================================
//  ASYNC  loadTextureAsync
//  Spawns a detached thread for the stbi_load, then pushes to the queue.
// ======================================================================
void TextureLoader::loadTextureAsync(
    TextureProperties& props,
    bool                              flip,
    std::function<void(unsigned int)> callback)
{
    // ------------------------------------------------------------------
    // If already cached, fire callback immediately and return
    // ------------------------------------------------------------------
    {
        std::lock_guard<std::mutex> lock(s_queueMutex);
        auto it = s_texCache.find(props.path);
        if (it != s_texCache.end()) {
            props.textureID = it->second;
            if (callback) callback(it->second);
            return;
        }
    }

    // ------------------------------------------------------------------
    // Capture what the background thread needs by value
    // ------------------------------------------------------------------
    std::string pathKey = props.path;
    TextureProperties* propsPtr = &props;   // caller must keep props alive

    std::thread([pathKey, propsPtr, flip, callback]()
        {
            // ------------------------------------------------------------------
            // Resolve path  (read-only filesystem ops are fine off main thread)
            // ------------------------------------------------------------------
            std::filesystem::path root = GetEngineRootPath();
            std::filesystem::path absolutePath = root / pathKey;
            std::string           pathStr = absolutePath.string();

            if (!std::filesystem::exists(absolutePath)) {
                std::cerr << "[TextureLoader][Async] File not found: " << pathStr << std::endl;
                return;
            }

            // ------------------------------------------------------------------
            // Extension check  (DDS can't be loaded off-thread easily � fall back
            // to synchronous for DDS files)
            // ------------------------------------------------------------------
            std::string ext;
            size_t dot = pathKey.find_last_of('.');
            if (dot != std::string::npos) {
                ext = pathKey.substr(dot + 1);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            }

            if (ext == "dds") {
                // DDS uses GL calls internally, cannot run off-thread.
                // Push a special marker so processUploads handles it on main thread.
                PendingUpload item;
                item.data = nullptr;   // nullptr signals "DDS path"
                item.path = pathKey;
                item.flip = flip;
                item.props = propsPtr;
                item.callback = callback;

                std::lock_guard<std::mutex> lock(s_queueMutex);
                s_uploadQueue.push(std::move(item));
                return;
            }

            // ------------------------------------------------------------------
            // Heavy CPU work: stbi_load  (safe to run on background thread)
            // NOTE: stbi_set_flip_vertically_on_load is GLOBAL state � not thread
            // safe. We load without flipping, then manually flip rows in-place if
            // needed, so concurrent threads never clobber each other's flip state.
            // ------------------------------------------------------------------
            int            width = 0, height = 0, channels = 0;
            unsigned char* data = stbi_load(pathStr.c_str(), &width, &height, &channels, 0);

            if (!data || width <= 0 || height <= 0) {
                std::cerr << "[STB Async Error] " << stbi_failure_reason()
                    << " : " << pathStr << std::endl;
                return;
            }

            // Manual vertical flip � swap top and bottom rows in-place
            if (flip) {
                int rowSize = width * channels;
                std::vector<unsigned char> rowBuf(rowSize);
                for (int y = 0; y < height / 2; ++y) {
                    unsigned char* top = data + y * rowSize;
                    unsigned char* bottom = data + (height - 1 - y) * rowSize;
                    std::memcpy(rowBuf.data(), top, rowSize);
                    std::memcpy(top, bottom, rowSize);
                    std::memcpy(bottom, rowBuf.data(), rowSize);
                }
            }

            // ------------------------------------------------------------------
            // Push to upload queue � main thread will call glTexImage2D
            // ------------------------------------------------------------------
            PendingUpload item;
            item.data = data;
            item.width = width;
            item.height = height;
            item.channels = channels;
            item.path = pathKey;
            item.flip = flip;
            item.props = propsPtr;
            item.callback = callback;

            std::lock_guard<std::mutex> lock(s_queueMutex);
            s_uploadQueue.push(std::move(item));

        }).detach();   // fire-and-forget background thread
}

// ======================================================================
//  processUploads  � call once per frame on the main / GL thread
// ======================================================================
int TextureLoader::processUploads()
{
    int uploaded = 0;

    // Drain the whole queue each frame (or cap it if you prefer e.g. 4/frame)
    while (true)
    {
        PendingUpload item;

        // ---- pull one item under lock ----
        {
            std::lock_guard<std::mutex> lock(s_queueMutex);
            if (s_uploadQueue.empty()) break;
            item = std::move(s_uploadQueue.front());
            s_uploadQueue.pop();
        }

        // ---- DDS special case ----
        if (item.data == nullptr) {
            // Resolve path again for DDS
            std::filesystem::path root = GetEngineRootPath();
            std::filesystem::path absolutePath = root / item.path;
            std::string           pathStr = absolutePath.string();

            // Check cache first
            auto it = s_texCache.find(item.path);
            if (it != s_texCache.end()) {
                if (item.props)    item.props->textureID = it->second;
                if (item.callback) item.callback(it->second);
                ++uploaded;
                continue;
            }

            GLuint texID = LoadDDS(pathStr);
            if (texID != 0) {
                s_texCache[item.path] = texID;
                if (item.props) {
                    item.props->textureID = texID;
                }
            }
            if (item.callback) item.callback(texID);
            ++uploaded;
            continue;
        }

        // ---- Normal PNG/JPG upload ----
        unsigned int texID = uploadToGPU(item);   // frees item.data internally
        if (item.callback) item.callback(texID);
        ++uploaded;
    }

    return uploaded;
}

// ======================================================================
//  pendingCount
// ======================================================================
int TextureLoader::pendingCount()
{
    std::lock_guard<std::mutex> lock(s_queueMutex);
    return static_cast<int>(s_uploadQueue.size());
}

// ======================================================================
//  uploadToGPU  (private helper � MUST be called on the GL thread)
//  Frees item.data before returning.
// ======================================================================
unsigned int TextureLoader::uploadToGPU(PendingUpload& item)
{
    int memBytes = item.width * item.height * item.channels;
    /*std::cout << "[Texture] " << item.path
        << " | " << item.width << "x" << item.height
        << " | " << (memBytes / 1024 / 1024) << "MB uncompressed" << std::endl;*/

    // ------------------------------------------------------------------
    // Determine OpenGL formats
    // ------------------------------------------------------------------
    GLenum internalFormat = 0;
    GLenum dataFormat = 0;

    switch (item.channels) {
    case 1:
        internalFormat = GL_R8;
        dataFormat = GL_RED;
        break;
    case 3:
        internalFormat = GL_RGB8;
        dataFormat = GL_RGB;
        break;
    case 4:
        internalFormat = GL_RGBA8;
        dataFormat = GL_RGBA;
        break;
    default:
        std::cerr << "[TextureLoader] Unsupported channel count: " << item.channels << " : " << item.path << std::endl;
        stbi_image_free(item.data);
        item.data = nullptr;
        return 0;
    }

    // ------------------------------------------------------------------
    // Generate & bind texture object
    // ------------------------------------------------------------------
    GLuint texID = 0;
    glGenTextures(1, &texID);

    if (texID == 0) {
        stbi_image_free(item.data);
        item.data = nullptr;
        return 0;
    }

    glBindTexture(GL_TEXTURE_2D, texID);

    // ------------------------------------------------------------------
    // Upload
    // ------------------------------------------------------------------
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    while (glGetError() != GL_NO_ERROR);   // clear any pre-existing errors

    glTexImage2D(
        GL_TEXTURE_2D, 0,
        internalFormat,
        item.width, item.height, 0,
        dataFormat, GL_UNSIGNED_BYTE,
        item.data
    );

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "[OpenGL Error] glTexImage2D failed ("
            << err << ") : " << item.path << std::endl;
        glDeleteTextures(1, &texID);
        stbi_image_free(item.data);
        item.data = nullptr;
        return 0;
    }

    glGenerateMipmap(GL_TEXTURE_2D);

    // ------------------------------------------------------------------
    // Sampler parameters
    // ------------------------------------------------------------------
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // ------------------------------------------------------------------
    // Cleanup & cache
    // ------------------------------------------------------------------
    stbi_image_free(item.data);
    item.data = nullptr;

    s_texCache[item.path] = texID;

    if (item.props) {
        item.props->textureID = texID;
        item.props->width = item.width;
        item.props->height = item.height;
    }

    return texID;
}