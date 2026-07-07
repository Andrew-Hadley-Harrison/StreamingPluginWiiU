/****************************************************************************
 * Minimal self-contained CThread wrapper.
 *
 * Based on the Dimok/Maschell libutils CThread, re-implemented here against
 * the modern wut coreinit OSThread API so the plugin no longer depends on the
 * old precompiled libutils.a (which was built for an incompatible wut ABI).
 *
 * Only the subset actually used by this plugin is implemented:
 *   CThread::create(callback, arg, attr, priority, stackSize)
 *   thread->resumeThread()
 *   thread->setThreadPriority(prio)
 * plus the eAttributeAffCoreX flags. The affinity flag values intentionally
 * match wut's OS_THREAD_ATTRIB_AFFINITY_CPUx bits.
 ****************************************************************************/
#pragma once

#include <malloc.h>
#include <string.h>
#include <stdint.h>
#include <coreinit/thread.h>

class CThread {
public:
    typedef void (*Callback)(CThread *thread, void *arg);

    enum eCThreadAttributes {
        eAttributeNone      = 0x07,
        eAttributeAffCore0  = 0x01, // OS_THREAD_ATTRIB_AFFINITY_CPU0
        eAttributeAffCore1  = 0x02, // OS_THREAD_ATTRIB_AFFINITY_CPU1
        eAttributeAffCore2  = 0x04, // OS_THREAD_ATTRIB_AFFINITY_CPU2
        eAttributeDetach    = 0x08, // OS_THREAD_ATTRIB_DETACHED
        eAttributePinnedAff = 0x10,
    };

    CThread(int32_t iAttr, int32_t iPriority = 16, int32_t iStackSize = 0x8000,
            CThread::Callback callback = NULL, void *callbackArg = NULL)
        : pThread(NULL), pThreadStack(NULL),
          pCallback(callback), pCallbackArg(callbackArg) {
        pThread      = (OSThread *) memalign(8, sizeof(OSThread));
        pThreadStack = (uint8_t *) memalign(0x20, iStackSize);
        if (pThread == NULL || pThreadStack == NULL) {
            return;
        }
        memset(pThread, 0, sizeof(OSThread));
        // Only the affinity/detach bits are valid OS thread attributes.
        uint32_t osAttr = (uint32_t) iAttr & 0x0F;
        // 'this' is smuggled in via the argv pointer (PowerPC is 32-bit).
        OSCreateThread(pThread, &CThread::threadEntry, 1, (char *) this,
                       (void *) (pThreadStack + iStackSize), iStackSize,
                       iPriority, osAttr);
    }

    virtual ~CThread() {
        shutdownThread();
    }

    static CThread *create(CThread::Callback callback, void *callbackArg,
                           int32_t iAttr = eAttributeNone, int32_t iPriority = 16,
                           int32_t iStackSize = 0x8000) {
        return new CThread(iAttr, iPriority, iStackSize, callback, callbackArg);
    }

    void resumeThread() {
        if (pThread) OSResumeThread(pThread);
    }

    void setThreadPriority(int32_t prio) {
        if (pThread) OSSetThreadPriority(pThread, prio);
    }

    void shutdownThread() {
        if (pThread) {
            if (!OSIsThreadTerminated(pThread)) {
                OSJoinThread(pThread, NULL);
            }
            free(pThread);
            pThread = NULL;
        }
        if (pThreadStack) {
            free(pThreadStack);
            pThreadStack = NULL;
        }
    }

private:
    static int threadEntry(int /*argc*/, const char **argv) {
        CThread *thread = (CThread *) argv; // 'this' passed through argv
        if (thread && thread->pCallback) {
            thread->pCallback(thread, thread->pCallbackArg);
        }
        return 0;
    }

    OSThread          *pThread;
    uint8_t           *pThreadStack;
    CThread::Callback  pCallback;
    void              *pCallbackArg;
};
