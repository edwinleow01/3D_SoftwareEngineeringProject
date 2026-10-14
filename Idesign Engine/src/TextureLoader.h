#pragma once
#include <string>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <functional>
#include "TextureProperties.h"

using namespace Framework;

class TextureLoader
{
public:
    // ------------------------------------------------------------------
    // Synchronous load — unchanged, all existing call sites still work
    // ------------------------------------------------------------------
    static unsigned int loadTexture(TextureProperties& props, bool flip);

    // ------------------------------------------------------------------
    // Async load — returns immediately (textureID stays 0 until uploaded).
    // Background thread runs stbi_load, main thread does the GL upload
    // via processUploads().
    //
    // Optional callback is fired on the main thread (inside processUploads)
    // once the texture is fully GPU-ready.
    // ------------------------------------------------------------------
    static void loadTextureAsync(
        TextureProperties& props,
        bool                              flip = false,
        std::function<void(unsigned int)> callback = nullptr
    );

    // ------------------------------------------------------------------
    // Call once per frame on the main / GL thread (e.g. your editor loop).
    // Uploads all textures whose stbi_load has completed.
    // Returns the number of textures uploaded this call.
    // ------------------------------------------------------------------
    static int processUploads();

    // ------------------------------------------------------------------
    // How many textures are still waiting to be uploaded?
    // Useful for showing a loading indicator in the editor.
    // ------------------------------------------------------------------
    static int pendingCount();

private:
    // ------------------------------------------------------------------
    // Global cache: file path -> GL texture ID
    // ------------------------------------------------------------------
    static std::unordered_map<std::string, unsigned int> s_texCache;

    // ------------------------------------------------------------------
    // Data produced by background thread, consumed by main thread
    // ------------------------------------------------------------------
    struct PendingUpload
    {
        unsigned char* data = nullptr;  // nullptr == DDS path
        int            width = 0;
        int            height = 0;
        int            channels = 0;

        std::string                       path;
        bool                              flip = false;
        std::function<void(unsigned int)> callback;
        TextureProperties* props = nullptr;
    };

    static std::queue<PendingUpload> s_uploadQueue;
    static std::mutex                s_queueMutex;

    // ------------------------------------------------------------------
    // Internal GL upload helper — MUST be called on the GL thread
    // ------------------------------------------------------------------
    static unsigned int uploadToGPU(PendingUpload& item);
};