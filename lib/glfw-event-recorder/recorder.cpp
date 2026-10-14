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

#include "recorder.h"

#include <chrono>
#include <vector>
#include <list>
#include <thread>
#include <fstream>
#include <iostream>

#include "lodepng.h" // Include the LodePNG library
#define GLEQ_IMPLEMENTATION
#include <gleq.h>

#ifdef _WIN32 // Windows
#include <direct.h> // For _mkdir()
#include <io.h>     // For _access()
#include <windows.h>
#else // Unix-based systems (Linux, macOS)
#include <unistd.h>
#include <sys/stat.h>
#endif

using namespace std;

struct TimeStampEvent {
    std::time_t timeStamp;
    GLEQevent event;
};

void printEvents(const GLEQevent& event)
{
#ifndef _DEBUG
    return;
#endif

    switch (event.type)
    {
    case GLEQ_WINDOW_MOVED:
        printf("Window moved to %i,%i\n", event.pos.x, event.pos.y);
        break;

    case GLEQ_WINDOW_RESIZED:
        printf("Window resized to %ix%i\n",
            event.size.width,
            event.size.height);
        break;

    case GLEQ_WINDOW_CLOSED:
        printf("Window close request\n");
        break;

    case GLEQ_WINDOW_REFRESH:
        printf("Window refresh request\n");
        break;

    case GLEQ_WINDOW_FOCUSED:
        printf("Window focused\n");
        break;

    case GLEQ_WINDOW_DEFOCUSED:
        printf("Window defocused\n");
        break;

    case GLEQ_WINDOW_ICONIFIED:
        printf("Window iconified\n");
        break;

    case GLEQ_WINDOW_UNICONIFIED:
        printf("Window uniconified\n");
        break;

#if GLFW_VERSION_MINOR >= 3
    case GLEQ_WINDOW_MAXIMIZED:
        printf("Window maximized\n");
        break;

    case GLEQ_WINDOW_UNMAXIMIZED:
        printf("Window unmaximized\n");
        break;

    case GLEQ_WINDOW_SCALE_CHANGED:
        printf("Window content scale %0.2fx%0.2f\n",
            event.scale.x, event.scale.y);
        break;
#endif // GLFW_VERSION_MINOR

    case GLEQ_FRAMEBUFFER_RESIZED:
        printf("Framebuffer resized to %ix%i\n",
            event.size.width,
            event.size.height);
        break;

    case GLEQ_BUTTON_PRESSED:
        printf("Mouse button %i pressed (mods 0x%x)\n",
            event.mouse.button,
            event.mouse.mods);
        break;

    case GLEQ_BUTTON_RELEASED:
        printf("Mouse button %i released (mods 0x%x)\n",
            event.mouse.button,
            event.mouse.mods);
        break;

    case GLEQ_CURSOR_MOVED:
        printf("Cursor moved to %i,%i\n", event.pos.x, event.pos.y);
        break;

    case GLEQ_CURSOR_ENTERED:
        printf("Cursor entered window\n");
        break;

    case GLEQ_CURSOR_LEFT:
        printf("Cursor left window\n");
        break;

    case GLEQ_SCROLLED:
        printf("Scrolled %0.2f,%0.2f\n",
            event.scroll.x, event.scroll.y);
        break;

    case GLEQ_KEY_PRESSED:
        printf("Key 0x%02x pressed (scancode 0x%x mods 0x%x)\n",
            event.keyboard.key,
            event.keyboard.scancode,
            event.keyboard.mods);
        break;

    case GLEQ_KEY_REPEATED:
        printf("Key 0x%02x repeated (scancode 0x%x mods 0x%x)\n",
            event.keyboard.key,
            event.keyboard.scancode,
            event.keyboard.mods);
        break;

    case GLEQ_KEY_RELEASED:
        printf("Key 0x%02x released (scancode 0x%x mods 0x%x)\n",
            event.keyboard.key,
            event.keyboard.scancode,
            event.keyboard.mods);
        break;

    case GLEQ_CODEPOINT_INPUT:
        printf("Codepoint U+%05X input\n", event.codepoint);
        break;

#if GLFW_VERSION_MINOR >= 1
    case GLEQ_FILE_DROPPED:
    {
        int i;

        printf("%i files dropped\n", event.file.count);
        for (i = 0; i < event.file.count; i++)
            printf("\t%s\n", event.file.paths[i]);

        break;
    }
#endif // GLFW_VERSION_MINOR

    case GLEQ_MONITOR_CONNECTED:
        printf("Monitor \"%s\" connected\n",
            glfwGetMonitorName(event.monitor));
        break;

    case GLEQ_MONITOR_DISCONNECTED:
        printf("Monitor \"%s\" disconnected\n",
            glfwGetMonitorName(event.monitor));
        break;

#if GLFW_VERSION_MINOR >= 2
    case GLEQ_JOYSTICK_CONNECTED:
        printf("Joystick %i \"%s\" connected\n",
            event.joystick,
            glfwGetJoystickName(event.joystick));
        break;

    case GLEQ_JOYSTICK_DISCONNECTED:
        printf("Joystick %i disconnected\n", event.joystick);
        break;
#endif // GLFW_VERSION_MINOR

    default:
        fprintf(stderr, "Error: Unknown event %i\n", event.type);
        break;
    }
}

int convertGleqToGlfwEvent(int type)
{
    switch (type)
    {
    case GLEQ_KEY_PRESSED: return GLFW_PRESS;
    case GLEQ_KEY_RELEASED: return GLFW_RELEASE;
    case GLEQ_KEY_REPEATED: return GLFW_REPEAT;
    case GLEQ_BUTTON_PRESSED: return GLFW_PRESS;
    case GLEQ_BUTTON_RELEASED: return GLFW_RELEASE;
    }

    return 0xFFFFFFFF;
}

void defaultEventHandler(GLFWwindow* window, const GLEQevent& event)
{
    switch (event.type)
    {
    case GLEQ_KEY_PRESSED:
        if (event.keyboard.key == GLFW_KEY_PRINT_SCREEN) {
            static int screenshot_count = 0;
            const std::string& fileName =
                getExecutableName(ParseArguments::getInstance().exec_name.c_str()) +
                "_" +
                std::to_string(screenshot_count++) +
                ".png";

            int width = 0; int height = 0;
            glfwGetFramebufferSize(window, &width, &height);
            savePng(fileName, width, height);

            std::cout << "png saved" << std::endl;
        }
        break;

    default:
        break;
    }

    // The below event are recursive in nature. 
    // For example, a window move handle will spwan same event
    if (ParseArguments::getInstance().type == RendererType::RECORD_LOG ||
        ParseArguments::getInstance().type == RendererType::NONE) {
        return;
    }

    switch (event.type)
    {
    case GLEQ_WINDOW_MOVED:
        glfwSetWindowPos(window, event.pos.x, event.pos.y);
        break;

    case GLEQ_WINDOW_CLOSED:
        glfwSetWindowShouldClose(window, true);
        break;

    case GLEQ_WINDOW_RESIZED:
        glfwSetWindowSize(window,
            event.size.width,
            event.size.height
        );
        break;

    case GLEQ_WINDOW_MAXIMIZED:
        glfwMaximizeWindow(window);
        break;

    case GLEQ_WINDOW_UNMAXIMIZED:
        glfwRestoreWindow(window);
        break;

    case GLEQ_CURSOR_MOVED:
        glfwSetCursorPos(window, event.pos.x, event.pos.y);
        break;

    default:
        break;
    }
}

int recordIntoLogFile(GLFWwindow* window, std::string file, const std::function<void()>& lambda, const std::function<void(GLEQevent)>& eventHandlerImpl)
{
    gleqTrackWindow(window);
    ofstream outFile(file, ios::binary);
    if (!outFile) {
        cerr << "Error in opening log file!: " << file << endl;
        return 1;
    }

    int totalEventCount = 0;

    while (!glfwWindowShouldClose(window))
    {
        TimeStampEvent event;

        while (gleqNextEvent(&event.event))
        {
            printEvents(event.event);

            auto currentTime = std::chrono::system_clock::now();
            event.timeStamp = std::chrono::system_clock::to_time_t(currentTime);
            outFile.write(reinterpret_cast<const char*>(&event), sizeof(TimeStampEvent));
            totalEventCount++;

            defaultEventHandler(window, event.event);
            eventHandlerImpl(event.event);

            gleqFreeEvent(&event.event);
        }

        lambda();
    }

    return 0;
}

// Retire this 
int playbackFromLogFileOldImplementation(GLFWwindow* window, std::string file, const std::function<void()>& updateAndDraw, const std::function<void(GLEQevent)>& userEventHandler)
{
    ifstream inFile(file, ios::binary);
    if (!inFile) {
        cerr << "Error opening file!" << endl;
        return 1;
    }

    // Determine the size of the file
    inFile.seekg(0, ios::end);
    streampos fileSize = inFile.tellg();
    inFile.seekg(0, ios::beg);

    // Allocate memory to store the read data
    vector<TimeStampEvent> messageQueue(fileSize / sizeof(TimeStampEvent));

    // Read the data from the file into the allocated memory
    inFile.read(reinterpret_cast<char*>(&messageQueue[0]), fileSize);

    vector<TimeStampEvent>::iterator it;
    int totalEventCount = 0;

    std::time_t currentTime = time(NULL);
    std::time_t nextEventTime = time(NULL);

    std::cout << "Opening file events recorded: " << std::endl;
    for (it = messageQueue.begin(); it != messageQueue.end(); it++) {
        if (totalEventCount == 0) {
            currentTime = it->timeStamp;
            nextEventTime = it->timeStamp;
        }
        else {
            nextEventTime = it->timeStamp;
            std::chrono::duration<double> difference{ (nextEventTime - currentTime) };
            currentTime = nextEventTime;

            // Sleep until the next event
            std::this_thread::sleep_for(difference);
        }

        ++totalEventCount;
        // printEvents(it->event);

        defaultEventHandler(window, it->event);

        userEventHandler(it->event);

        updateAndDraw();
    }

    return 0;
}

int defaultRenderer(GLFWwindow* window, std::string file, const std::function<void()>& lambda, const std::function<void(GLEQevent)>& eventHandlerImpl)
{
    gleqTrackWindow(window);

    while (!glfwWindowShouldClose(window))
    {
        TimeStampEvent event;

        while (gleqNextEvent(&event.event))
        {
            printEvents(event.event);

            defaultEventHandler(window, event.event);
            eventHandlerImpl(event.event);

            gleqFreeEvent(&event.event);
        }

        lambda();
    }

    return 0;
}

int playbackFromLogFile(GLFWwindow* window, std::string file, const std::function<void()>& updateAndDraw, const std::function<void(GLEQevent)>& userEventHandler)
{
    ifstream inFile(file, ios::binary);
    if (!inFile) {
        cerr << "Error opening file!" << endl;
        return 1;
    }

    // Determine the size of the file
    inFile.seekg(0, ios::end);
    streampos fileSize = inFile.tellg();
    inFile.seekg(0, ios::beg);

    // Allocate memory to store the read data
    vector<TimeStampEvent> messageQueueVec(fileSize / sizeof(TimeStampEvent));

    // Read the data from the file into the allocated memory
    inFile.read(reinterpret_cast<char*>(&messageQueueVec[0]), fileSize);

    std::list<TimeStampEvent> messageQueue(messageQueueVec.begin(), messageQueueVec.end());


    int totalEventCount = 0;

    std::time_t lastEventTime = time(NULL);
    std::time_t nextEventTime = time(NULL);
    std::time_t nowtimeStamp = time(NULL);
    std::time_t futuretimeStamp = time(NULL);

    std::list<TimeStampEvent>::iterator it = messageQueue.begin();
    lastEventTime = it->timeStamp;
    nextEventTime = it->timeStamp;

    std::cout << "Opening file events recorded: " << std::endl;
    bool processEvent = true;
    while (!messageQueue.empty()) {
        {
            auto currentTime = std::chrono::system_clock::now();
            nowtimeStamp = std::chrono::system_clock::to_time_t(currentTime);

            // Check if the future timestamp is reached, if yes process 
            processEvent = (nowtimeStamp >= futuretimeStamp);
        }

        ++totalEventCount;
        // printEvents(it->event);

        updateAndDraw();

        if (processEvent) {
            processEvent = false;

            // Handle events
            defaultEventHandler(window, it->event);
            userEventHandler(it->event);

            // Remove the message from queue top
            messageQueue.pop_front();

            // Get the next message from the queue
            it = messageQueue.begin();

            // If the message queue is empty return
            if (it == messageQueue.end()) break;

            // Compute the difference between the current and next event
            nextEventTime = it->timeStamp;
            std::chrono::duration<double> difference{ (nextEventTime - lastEventTime) };
            lastEventTime = nextEventTime;

            // Use the difference interval and compute the future time when the next event would be fired
            auto futureTime = std::chrono::system_clock::now() + difference;
            auto futureTimePoint = std::chrono::time_point_cast<std::chrono::seconds>(futureTime);
            futuretimeStamp = std::chrono::system_clock::to_time_t(futureTimePoint);
        }
    }

    return 0;
}

void render(GLFWwindow* window, const std::function<void()>& updateAndDraw, const std::function<void(GLEQevent)>& userEventHandler, RendererType type, std::string file)
{
    if (type == RendererType::PLAYBACK_LOG) {
        deleteFolder(fb_screen_shot_folder_name);
        playbackFromLogFile(window, file, updateAndDraw, userEventHandler);
    }
    else if (type == RendererType::RECORD_LOG) {
        recordIntoLogFile(window, file, updateAndDraw, userEventHandler);
    }
    else {
        defaultRenderer(window, file, updateAndDraw, userEventHandler);
    }
}

void saveTga(std::string filename, unsigned int width, unsigned int height) {
    size_t numberOfPixels = static_cast<size_t>(width * height * 3);
    unsigned char* pixels = new unsigned char[numberOfPixels];

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, width, height, GL_BGR_EXT, GL_UNSIGNED_BYTE, pixels);

    //FILE* outputFile = fopen(filename.c_str(), "w");
    //short header[] = { 0, 2, 0, 0, 0, 0, (short)width, (short)height, 24 };

    //fwrite(&header, sizeof(header), 1, outputFile);
    //fwrite(pixels, numberOfPixels, 1, outputFile);
    //fclose(outputFile);

    delete pixels;
    printf("Finish writing to file.\n");
}

int savePng(std::string filename, unsigned int width, unsigned int height) {
    createDirectory(fb_screen_shot_folder_name);

    // Create a vector to store pixel data (RGBA format)
    std::vector<unsigned char> image;
    image.resize(width * height * 4);

    size_t numberOfPixels = static_cast<size_t>(width * height * 3);
    unsigned char* pixels = new unsigned char[numberOfPixels];

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, width, height, GL_BGR_EXT, GL_UNSIGNED_BYTE, pixels);

    // Correct the upside-down pixel data
    unsigned char* flippedPixels = new unsigned char[width * height * 3];
    for (unsigned int y = 0; y < height; y++) {
        memcpy(&flippedPixels[y * width * 3], &pixels[(height - y - 1) * width * 3], width * 3);
    }

    // Fill the image with some sample data (e.g., a gradient)
    for (unsigned int y = 0; y < height; y++) {
        for (unsigned int x = 0; x < width; x++) {
            unsigned int index = 4 * (y * width + x);
            image[index] = flippedPixels[3 * (y * width + x) + 2] % 256; // Blue component
            image[index + 1] = flippedPixels[3 * (y * width + x) + 1] % 256; // Green component
            image[index + 2] = flippedPixels[3 * (y * width + x)] % 256; // Red component
            image[index + 3] = 255; // Alpha component (fully opaque)
        }
    }

    short header[] = { 0, 2, 0, 0, 0, 0, (short)width, (short)height, 24 };

    // Encode the pixel data into a PNG file
    unsigned error = lodepng::encode(fb_screen_shot_folder_name + "/" + filename, image, width, height);

    // Check for encoding errors
    if (error) {
        std::cout << "PNG encoding error: " << lodepng_error_text(error) << std::endl;
        return 1;
    }

    std::cout << "PNG file created successfully! " << width << "x" << height << std::endl;
    return 0;
}

int saveScreenshotToFileOrig(std::string filename, unsigned int width, unsigned int height) {

    // Create a vector to store pixel data (RGBA format)
    std::vector<unsigned char> image;
    image.resize(width * height * 4);

    // Fill the image with some sample data (e.g., a gradient)
    for (unsigned int y = 0; y < height; y++) {
        for (unsigned int x = 0; x < width; x++) {
            unsigned int index = 4 * (y * width + x);
            image[index + 0] = x % 256; // Red component
            image[index + 1] = y % 256; // Green component
            image[index + 2] = (x + y) % 256; // Blue component
            image[index + 3] = 255; // Alpha component (fully opaque)
        }
    }

    // Encode the pixel data into a PNG file
    unsigned error = lodepng::encode("output.png", image, width, height);

    // Check for encoding errors
    if (error) {
        std::cout << "PNG encoding error: " << lodepng_error_text(error) << std::endl;
        return 1;
    }

    std::cout << "PNG file created successfully! " << width << "x" << height << std::endl;
    return 0;
}

std::string getExecutableName(const char* fullPath) {
#ifdef _WIN32
    // Extract only the file name from the full path
    std::string path(fullPath);
    size_t lastSlash = path.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        return path.substr(lastSlash + 1);
    }
#endif
    // If not running on Windows, return the original string
    return fullPath;
}

bool directoryExists(const std::string& foldername) {
#ifdef _WIN32
    return (_access(foldername.c_str(), 0) == 0);
#else
    return (access(foldername.c_str(), F_OK) == 0);
#endif
}

bool deleteFolder(const std::string& dirPath) {
    bool success = true;
#ifdef _WIN32
    std::string searchPath = dirPath + "\\*";
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile(searchPath.c_str(), &findFileData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(findFileData.cFileName, ".") != 0 && strcmp(findFileData.cFileName, "..") != 0) {
                std::string filePath = dirPath + "\\" + findFileData.cFileName;
                if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    // Recursively delete subdirectories
                    if (!deleteFolder(filePath)) {
                        success = false;
                    }
                }
                else {
                    // Delete files
                    if (!DeleteFile(filePath.c_str())) {
                        success = false;
                    }
                }
            }
        } while (FindNextFile(hFind, &findFileData) != 0);

        FindClose(hFind);
    }

    // Remove the empty directory
    if (!RemoveDirectory(dirPath.c_str())) {
        success = false;
    }
#else
    std::string command = "rm -rf " + dirPath; // Using rm command with -rf option to recursively force delete folder and its contents 
    int status = system(command.c_str());
    if (status == 0) { 
        std::cout << "Folder and its contents deleted successfully." << std::endl; return true; 
    } else { 
        std::cerr << "Error: Failed to delete folder and its contents." << std::endl; return false;
    }
#endif

    return success;
}

bool createDirectory(const std::string& foldername)
{
#ifdef _WIN32    
    if (directoryExists(foldername)) {
        std::cout << "Directory already exists." << std::endl;
        return true;
    }

    int status = _mkdir(foldername.c_str());
#else
    if (access(foldername.c_str(), F_OK) == 0) {
        std::cout << "Directory already exists." << std::endl;
        return true;
    }

    int status = mkdir(foldername.c_str(), 0777); // 0777 sets permissions
#endif

    // Check if directory creation was successful
    if (status == 0) {
        std::cout << "Directory created successfully." << std::endl;
        return true;
    } else {
        std::cerr << "Error: Failed to create directory." << std::endl;
    }    

    return false;
}

int ParseArguments::parseArguments(int argc, char* argv[])
{
    ParseArguments& args = ParseArguments::getInstance();
    args.exec_name = argv[0];

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-a" || arg == "--help") {
            args.help = true;
        }
        else if (arg == "-w" || arg == "--width") {
            if (i + 1 < argc) {
                args.window_width = atoi(argv[++i]);
            }
            else {
                std::cerr << "Error: No filename provided after -f option." << std::endl;
                args.help = true; // Display help if filename is missing
            }
        }
        else if (arg == "-h" || arg == "--height") {
            if (i + 1 < argc) {
                args.window_height = atoi(argv[++i]);
            }
            else {
                std::cerr << "Error: No filename provided after -f option." << std::endl;
                args.help = true; // Display help if filename is missing
            }
        }
        else if (arg == "-r" || arg == "--record") {
            args.type = RendererType::RECORD_LOG;
        }
        else if (arg == "-p" || arg == "--play") {
            args.type = RendererType::PLAYBACK_LOG;
        }
        else if (arg == "-f" || arg == "--file") {
            if (i + 1 < argc) {
                args.filename = argv[++i];
            }
            else {
                std::cerr << "Error: No filename provided after -f option." << std::endl;
                args.help = true; // Display help if filename is missing
            }
        }
    }

    if (args.help) {
        std::cout << "Usage: " << args.exec_name << " [-h] [-f <filename>]" << " [-r ]" << " [-p ]" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  -a, --help     Display this help message" << std::endl;
        std::cout << "  -f, --file     Specify a record or playback filename" << std::endl;
        std::cout << "  -r, --record   Render the executable and record event into log file" << std::endl;
        std::cout << "  -p, --play     Render the executable and playback the event from log file" << std::endl;
        std::cout << "  -w, --width    Specify window width" << std::endl;
        std::cout << "  -h, --height   Specify window height" << std::endl;
        return 0;
    }

    return 1;
}
