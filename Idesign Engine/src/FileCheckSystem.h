#pragma once
#include "System.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <deque>
#include <filesystem>
#include <functional>

namespace Framework {

    struct FileEvent {
        std::string path;
        enum class Kind { Created, Modified, Removed } kind;
    };

    class FileCheckSystem : public ISystem {
    public:
        void Initialize() override;
        void Update(float dt) override;
        std::string GetName() override { return "FileCheckSystem"; }

        void WatchFolder(const std::string& folder, const std::vector<std::string>& extensions);
        bool PopEvent(FileEvent& outEvent);
        void WatchAllFolders();

        static inline std::string s_ProjectRoot;

        static void SetProjectRoot(const std::string& root) {
            std::error_code ec;
            auto canonical = std::filesystem::canonical(root, ec);
            if (!ec) {
                s_ProjectRoot = canonical.generic_string();
                std::cout << "[FileCheckSystem] Project root: " << s_ProjectRoot << "\n";
            }
        }

    private:
        struct WatchRule {
            std::string folder;
            std::vector<std::string> extensions;
        };
        struct FileMeta {
            uint64_t lastWrite = 0;
            uintmax_t size = 0;
        };

        std::vector<WatchRule> rules_;
        std::unordered_map<std::string, FileMeta> cache_;
        std::deque<FileEvent> eventQueue_;
        float pollTimer_ = 0.0f;
        float pollInterval_ = 0.5f;
        std::chrono::steady_clock::time_point lastPoll_;

        static uint64_t ToTicks(const std::filesystem::file_time_type& t);
        static bool MatchExtension(const std::string& path, const std::vector<std::string>& extensions);
    };

} // namespace Framework
