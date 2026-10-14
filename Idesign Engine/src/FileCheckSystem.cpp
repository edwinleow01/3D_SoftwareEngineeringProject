#include "pch.h"
#include "FileCheckSystem.h"
#include <iostream>

namespace Framework {

    uint64_t FileCheckSystem::ToTicks(const std::filesystem::file_time_type& t) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(t.time_since_epoch()).count();
    }

    bool FileCheckSystem::MatchExtension(const std::string& path, const std::vector<std::string>& exts) {
        auto ext = std::filesystem::path(path).extension().string();
        for (auto& e : exts) if (ext == e) return true;
        return false;
    }

    void FileCheckSystem::Initialize() {
        std::cout << "[FileCheckSystem] Initialized.\n";
        lastPoll_ = std::chrono::steady_clock::now();
    }

    void FileCheckSystem::WatchFolder(const std::string& folder, const std::vector<std::string>& extensions) {
        std::error_code ec;

        // Build absolute path from project root
        auto fullPath = std::filesystem::path(s_ProjectRoot) / folder;
        auto canonicalPath = std::filesystem::canonical(fullPath, ec);

        if (ec || !std::filesystem::exists(canonicalPath)) {
            std::cout << "[FileCheckSystem] Warning: Cannot watch folder: " << folder
                << " (resolved to: " << fullPath << ")\n";
            return;
        }

        rules_.push_back({ canonicalPath.generic_string(), extensions });
        //std::cout << "[FileCheckSystem] Watching " << canonicalPath.generic_string() << "\n";
    }

    void FileCheckSystem::Update(float dt) {
        using namespace std::chrono;
        auto now = steady_clock::now();

        // convert pollInterval_ (in seconds) to milliseconds
        auto intervalMs = static_cast<int>(pollInterval_ * 1000.0f);

        if (duration_cast<milliseconds>(now - lastPoll_).count() < intervalMs)
            return;

        lastPoll_ = now;
        //std::cout << "[Watcher] POLLING using steady_clock\n";

        std::size_t pushed = 0;

        for (auto& rule : rules_) {
            std::error_code ec;
            // canonicalize the folder so directory_iterator doesn’t throw
            auto canonicalFolder = std::filesystem::weakly_canonical(rule.folder, ec);
            if (ec || !std::filesystem::exists(canonicalFolder)) {
                std::cout << "[Watcher] Skipping missing folder: " << rule.folder << "\n";
                continue;
            }

            for (auto& entry : std::filesystem::directory_iterator(canonicalFolder)) {
                if (!entry.is_regular_file()) continue;
                auto path = entry.path().generic_string(); // normalize to forward slashes

                if (!MatchExtension(path, rule.extensions)) continue;

                auto mod = ToTicks(std::filesystem::last_write_time(entry));
                auto size = std::filesystem::file_size(entry);

                auto it = cache_.find(path);
                if (it == cache_.end()) {
                    cache_[path] = { mod, size };
                    eventQueue_.push_back({ path, FileEvent::Kind::Created });
                    ++pushed;
                }
                else if (it->second.lastWrite != mod || it->second.size != size) {
                    it->second = { mod, size };
                    eventQueue_.push_back({ path, FileEvent::Kind::Modified });
                    ++pushed;
                }
            }
        }

        //std::cout << "[Watcher] events queued this tick: " << pushed << " (total pending=" << eventQueue_.size() << ")\n";
    }


    bool FileCheckSystem::PopEvent(FileEvent& out) {
        if (eventQueue_.empty()) return false;
        out = eventQueue_.front();
        eventQueue_.pop_front();
        return true;
    }

    void FileCheckSystem::WatchAllFolders()
    {
        //Folders and filetypes to check for external change
        WatchFolder("Assets/Scripts", { ".lua" });
        WatchFolder("Assets/JSON", { ".json", ".cfg" });
        WatchFolder("Assets/shaders", { ".glsl" });
        WatchFolder("Assets/Textures", { ".png", ".jpg" });
        WatchFolder("Scenes", { ".scene" });
        WatchFolder("Assets/Prefabs", { ".prefab" });
        WatchFolder("Assets/Materials", { ".material" });
    }

} // namespace Framework
