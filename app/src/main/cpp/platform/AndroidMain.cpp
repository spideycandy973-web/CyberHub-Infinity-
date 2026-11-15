#include <game-activity/GameActivity.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include "core/Log.h"
#include "gfx/Renderer.h"
#include <time.h>
#include <unistd.h>

namespace {

neon::Renderer gRenderer;
bool gHasWindow = false;
bool gFocused   = false;
bool gReady     = false;

timespec nowTs() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
}

float elapsedSec(timespec a, timespec b) {
    return static_cast<float>(b.tv_sec - a.tv_sec) +
           static_cast<float>(b.tv_nsec - a.tv_nsec) * 1e-9f;
}

} // anon

void android_main(struct android_app* app) {
    LOGI("android_main entered");

    app->onAppCmd = [](struct android_app*, int32_t cmd) {
        switch (cmd) {
            case APP_CMD_INIT_WINDOW:
                LOGI("CMD INIT_WINDOW");
                gHasWindow = true;
                break;
            case APP_CMD_TERM_WINDOW:
                LOGI("CMD TERM_WINDOW");
                gHasWindow = false;
                gReady = false;
                gRenderer.destroy();
                break;
            case APP_CMD_GAINED_FOCUS:
                LOGI("CMD GAINED_FOCUS");
                gFocused = true;
                break;
            case APP_CMD_LOST_FOCUS:
                LOGI("CMD LOST_FOCUS");
                gFocused = false;
                break;
            default:
                break;
        }
    };

    timespec lastTime = nowTs();

    while (true) {
        int events;
        struct android_poll_source* source;
        while (ALooper_pollOnce(0, nullptr, &events,
                                reinterpret_cast<void**>(&source)) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) {
                LOGI("Destroy requested, exiting");
                gRenderer.destroy();
                return;
            }
        }

        if (!gHasWindow || !gFocused) {
            ALooper_pollOnce(-1, nullptr, nullptr, nullptr);
            lastTime = nowTs();
            continue;
        }

        timespec now = nowTs();
        float dt = elapsedSec(lastTime, now);
        lastTime = now;
        if (dt > 0.1f) dt = 0.016f;

        if (!gReady) {
            if (app->window && gRenderer.init(app->window, app->activity->assetManager)) {
                gReady = true;
                LOGI("Renderer ready");
            } else {
                LOGW("Renderer init failed, will retry");
                usleep(500000);
            }
            continue;
        }

        auto* inputBuf = android_app_swap_input_buffers(app);
        if (inputBuf) {
            android_app_clear_motion_events(inputBuf);
            android_app_clear_key_events(inputBuf);
        }

        if (!gRenderer.render(dt)) {
            if (gRenderer.surfaceLost()) {
                LOGW("Surface lost, tearing down renderer");
                gRenderer.destroy();
                gReady = false;
            }
        }
    }
}
