/*
 * GLFW_LOGGER a simple event logger for GLFW 3
 * @copywrite 	DigiPen Institute of Technology Singapore
 * @author		parminder.singh@digipen.edu
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would
 *    be appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not
 *    be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 *    distribution.
 */

#ifndef GLFW_LOGGER_HEADER_FILE
#define GLFW_LOGGER_HEADER_FILE

#include <GLFW/glfw3.h>
#include <functional>
#include <string>
#include <chrono>

using namespace std;

// Macro to suppress unused variable warnings
#define UNUSED(expr) (void)(expr)

// Forward declaration of GLEQevent struct
struct GLEQevent;

// Enumeration defining different types of rendering operations
enum RendererType
{
    NONE,               // No recording, No playback
    RECORD_LOG,         // Record the event in a log
    PLAYBACK_LOG        // Playback the log
};

// Class for parsing command line arguments
class ParseArguments {
public:
    bool help = false;                          // Flag to show usage
    int window_width = 1600;                    // Flag to specify window width
    int window_height = 900;                    // Flag to specify window height
    RendererType type = RendererType::NONE;     // Type of rendering operation
    std::string filename;                       // Playback or Recording filename
    std::string exec_name;                      // Name of the executable with extension

public:
    // Static member function to access the singleton instance
    static ParseArguments& getInstance() {
        static ParseArguments instance; // This is instantiated only once
        return instance;
    }

    int parseArguments(int argc, char* argv[]);
private:
    // Private constructor and destructor to prevent instantiation and deletion
    ParseArguments() {} // Private constructor
    ~ParseArguments() {} // Private destructor

    // Private copy constructor and assignment operator to prevent copying
    ParseArguments(const ParseArguments&);
    ParseArguments& operator=(const ParseArguments&);
};

// Function to print GLEQevent data
void printEvents(const GLEQevent& event);

// Function to convert GLEQevent type to GLFW event type
int convertGleqToGlfwEvent(int type);

// Default event handler function
void defaultEventHandler(GLFWwindow* window, const GLEQevent& event);

// Function to record events into a log file
int recordIntoLogFile(GLFWwindow* window, std::string file, const std::function<void()>& lambda, const std::function<void(GLEQevent)>& eventHandlerImpl);

// Function to playback events from a log file
int playbackFromLogFile(GLFWwindow* window, std::string file, const std::function<void()>& updateAndDraw, const std::function<void(GLEQevent)>& userEventHandler);

// Default renderer function
int defaultRenderer(GLFWwindow* window, std::string file, const std::function<void()>& lambda, const std::function<void(GLEQevent)>& eventHandlerImpl);

// Function to render based on specified parameters
void render(GLFWwindow* window, const std::function<void()>& updateAndDraw, const std::function<void(GLEQevent)>& userEventHandler, RendererType type, std::string file = "data.bin");

// Function to save an image as PNG file
int savePng(std::string filename, unsigned int width, unsigned int height);

// Function to save a screenshot to file (original implementation)
int saveScreenshotToFileOrig(std::string filename, unsigned int width, unsigned int height);

// Function to get the executable name from a full path
std::string getExecutableName(const char* fullPath);

// Function to create a directory
bool createDirectory(const std::string& foldername);

// Function to check if a directory exists
bool directoryExists(const std::string& foldername);

// delete a folder
bool deleteFolder(const std::string& folderPath);

static std::string fb_screen_shot_folder_name = "screenshots";
#endif /* GLFW_LOGGER_HEADER_FILE */

