/*
 * Automation hook for module framework: [CSD-2101]
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

#ifndef HOOK_CSD2101_FILE
#define HOOK_CSD2101_FILE

#include <recorder.h>
#include <gleq.h>

#define EVENT_HANDLER_IMPL(event) \
    switch (event.type) { \
    case GLEQ_KEY_PRESSED: \
    case GLEQ_KEY_RELEASED: \
    case GLEQ_KEY_REPEATED: { \
        GLHelper::key_cb(GLHelper::ptr_window, event.keyboard.key, \
            event.keyboard.scancode, \
            convertGleqToGlfwEvent(event.type), \
            event.keyboard.mods); \
        break; \
    } \
    case GLEQ_BUTTON_PRESSED: \
    case GLEQ_BUTTON_RELEASED: { \
        GLHelper::mousebutton_cb(GLHelper::ptr_window, event.mouse.button, \
            convertGleqToGlfwEvent(event.type), \
            event.mouse.mods); \
        break; \
    } \
    case GLEQ_CURSOR_MOVED: { \
        GLHelper::mousepos_cb(GLHelper::ptr_window, event.pos.x, event.pos.y); \
        break; \
    } \
    case GLEQ_WINDOW_RESIZED: { \
        GLHelper::fbsize_cb(GLHelper::ptr_window, event.size.width, event.size.height); \
        break; \
    } \
    default: \
        break; \
    }

#if USE_CSD2101_AUTOMATION == 1
#define AUTOMATION_HOOK_EVENTS() return;
#else
#define AUTOMATION_HOOK_EVENTS() 
#endif

#if USE_CSD2101_AUTOMATION == 1
#define AUTOMATION_HOOK_RENDER(args) \
    auto event_handler = [](GLEQevent event) { EVENT_HANDLER_IMPL(event); }; \
    auto render_loop = []() { \
        update(); \
        draw(); \
    }; \
    render(GLHelper::ptr_window, render_loop, event_handler, args.type, args.filename); \
    cleanup(); \
    return 0;
#else
#define AUTOMATION_HOOK_RENDER(args) 
#endif

#endif /* HOOK_CSD2101_FILE */

