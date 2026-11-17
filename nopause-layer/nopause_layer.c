/*
 * XR_APILAYER_VRCONTROL_no_focus_pause
 *
 * An OpenXR API layer that stops games pausing when the headset briefly takes focus
 * (Meta button / recenter / system overlay) or reports the headset as taken off.
 *
 * It sits between the game and the runtime (WiVRn) and, for listed applications only:
 *  - hides session-state changes FOCUSED -> VISIBLE -> FOCUSED (the game keeps "focus");
 *    if the session really ends while hidden, a VISIBLE event is delivered first so the
 *    game still sees a valid state sequence;
 *  - hides XR_EXT_user_presence "user not present" events.
 *
 * Applications: env VRCONTROL_NOPAUSE_APPS (comma separated, matched against the
 * application name the game gives OpenXR), default "Beat Saber". Set to "*" for all.
 * Disable entirely with DISABLE_VRCONTROL_NOPAUSE=1 (see the manifest).
 *
 * Only the handful of OpenXR definitions needed are declared here, from the OpenXR 1.x
 * specification / loader interface, so no SDK headers are required to build it.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ---- minimal OpenXR declarations (ABI from the OpenXR spec) ---- */
typedef int32_t XrResult;
typedef uint64_t XrVersion;
typedef uint32_t XrBool32;
typedef int64_t XrTime;
typedef int32_t XrStructureType;
typedef int32_t XrSessionState;
typedef struct XrInstance_T *XrInstance;
typedef uint64_t XrSession;
typedef void (*PFN_xrVoidFunction)(void);

#define XR_SUCCESS 0
#define XR_EVENT_UNAVAILABLE 4
#define XR_ERROR_INITIALIZATION_FAILED (-6)
#define XR_ERROR_FUNCTION_UNSUPPORTED (-7)
#define XR_MAKE_VERSION(ma, mi, pa) \
    ((((uint64_t)(ma)&0xffffULL) << 48) | (((uint64_t)(mi)&0xffffULL) << 32) | ((uint64_t)(pa)&0xffffffffULL))

#define XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED 18
#define XR_TYPE_EVENT_DATA_USER_PRESENCE_CHANGED_EXT 1000470000

enum { XR_SESSION_STATE_VISIBLE = 4, XR_SESSION_STATE_FOCUSED = 5 };

typedef struct XrApplicationInfo {
    char applicationName[128];
    uint32_t applicationVersion;
    char engineName[128];
    uint32_t engineVersion;
    XrVersion apiVersion;
} XrApplicationInfo;

typedef struct XrInstanceCreateInfo {
    XrStructureType type;
    const void *next;
    uint64_t createFlags;
    XrApplicationInfo applicationInfo;
    /* remaining members not used */
} XrInstanceCreateInfo;

typedef struct XrEventDataBuffer {
    XrStructureType type;
    const void *next;
    uint8_t varying[4000];
} XrEventDataBuffer;

typedef struct XrEventDataSessionStateChanged {
    XrStructureType type;
    const void *next;
    XrSession session;
    XrSessionState state;
    XrTime time;
} XrEventDataSessionStateChanged;

typedef struct XrEventDataUserPresenceChangedEXT {
    XrStructureType type;
    const void *next;
    XrSession session;
    XrBool32 isUserPresent;
} XrEventDataUserPresenceChangedEXT;

typedef XrResult (*PFN_xrGetInstanceProcAddr)(XrInstance, const char *, PFN_xrVoidFunction *);
typedef XrResult (*PFN_xrPollEvent)(XrInstance, XrEventDataBuffer *);

/* ---- loader <-> layer interface (openxr_loader_negotiation.h) ---- */
typedef enum XrLoaderInterfaceStructs {
    XR_LOADER_INTERFACE_STRUCT_UNINTIALIZED = 0,
    XR_LOADER_INTERFACE_STRUCT_LOADER_INFO,
    XR_LOADER_INTERFACE_STRUCT_API_LAYER_REQUEST,
    XR_LOADER_INTERFACE_STRUCT_RUNTIME_REQUEST,
    XR_LOADER_INTERFACE_STRUCT_API_LAYER_CREATE_INFO,
    XR_LOADER_INTERFACE_STRUCT_API_LAYER_NEXT_INFO,
    XR_LOADER_INTERFACE_STRUCTS_MAX_ENUM = 0x7FFFFFFF
} XrLoaderInterfaceStructs;

#define XR_CURRENT_LOADER_API_LAYER_VERSION 1
#define XR_API_LAYER_INFO_STRUCT_VERSION 1
#define XR_API_LAYER_MAX_SETTINGS_PATH_SIZE 512
#define XR_MAX_API_LAYER_NAME_SIZE 256

typedef struct XrApiLayerCreateInfo XrApiLayerCreateInfo;
typedef XrResult (*PFN_xrCreateApiLayerInstance)(const XrInstanceCreateInfo *, const XrApiLayerCreateInfo *,
                                                 XrInstance *);

typedef struct XrNegotiateLoaderInfo {
    XrLoaderInterfaceStructs structType;
    uint32_t structVersion;
    size_t structSize;
    uint32_t minInterfaceVersion;
    uint32_t maxInterfaceVersion;
    XrVersion minApiVersion;
    XrVersion maxApiVersion;
} XrNegotiateLoaderInfo;

typedef struct XrNegotiateApiLayerRequest {
    XrLoaderInterfaceStructs structType;
    uint32_t structVersion;
    size_t structSize;
    uint32_t layerInterfaceVersion;
    XrVersion layerApiVersion;
    PFN_xrGetInstanceProcAddr getInstanceProcAddr;
    PFN_xrCreateApiLayerInstance createApiLayerInstance;
} XrNegotiateApiLayerRequest;

typedef struct XrApiLayerNextInfo {
    XrLoaderInterfaceStructs structType;
    uint32_t structVersion;
    size_t structSize;
    char layerName[XR_MAX_API_LAYER_NAME_SIZE];
    PFN_xrGetInstanceProcAddr nextGetInstanceProcAddr;
    PFN_xrCreateApiLayerInstance nextCreateApiLayerInstance;
    struct XrApiLayerNextInfo *next;
} XrApiLayerNextInfo;

struct XrApiLayerCreateInfo {
    XrLoaderInterfaceStructs structType;
    uint32_t structVersion;
    size_t structSize;
    void *loaderInstance;
    char settings_file_location[XR_API_LAYER_MAX_SETTINGS_PATH_SIZE];
    XrApiLayerNextInfo *nextInfo;
};

/* ---- layer state (one instance per game process is the normal case) ---- */
static PFN_xrGetInstanceProcAddr next_gipa;
static PFN_xrPollEvent next_poll;
static int active;           /* this application is in the list */
static int hidden;           /* we are hiding a FOCUSED -> VISIBLE change */
static int have_pending;     /* a real event queued behind a synthetic VISIBLE */
static XrEventDataBuffer pending;
static FILE *logf;
static int focused_seen;     /* last state the game saw was FOCUSED */

static void lg(const char *fmt, const char *arg)
{
    if (!logf) {
        const char *home = getenv("HOME");
        char path[1024];
        if (!home)
            return;
        snprintf(path, sizeof path, "%s/.local/state/vr-control/nopause-layer.log", home);
        logf = fopen(path, "a");
        if (!logf)
            return;
    }
    time_t t = time(NULL);
    char ts[32];
    strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", localtime(&t));
    fprintf(logf, "[%s] ", ts);
    fprintf(logf, fmt, arg);
    fputc('\n', logf);
    fflush(logf);
}

static int app_listed(const char *name)
{
    const char *list = getenv("VRCONTROL_NOPAUSE_APPS");
    if (!list || !*list)
        list = "Beat Saber";
    if (strcmp(list, "*") == 0)
        return 1;
    size_t n = strlen(name);
    const char *p = list;
    while (*p) {
        const char *e = strchr(p, ',');
        size_t len = e ? (size_t)(e - p) : strlen(p);
        if (len == n && strncmp(p, name, n) == 0)
            return 1;
        if (!e)
            break;
        p = e + 1;
    }
    return 0;
}

static XrResult nopause_xrPollEvent(XrInstance instance, XrEventDataBuffer *ev)
{
    if (have_pending) {
        memcpy(ev, &pending, sizeof pending);
        have_pending = 0;
        return XR_SUCCESS;
    }
    for (;;) {
        XrResult r = next_poll(instance, ev);
        if (r != XR_SUCCESS || !active)
            return r;

        if (ev->type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
            XrEventDataSessionStateChanged *s = (XrEventDataSessionStateChanged *)ev;
            if (s->state == XR_SESSION_STATE_FOCUSED) {
                if (hidden) {
                    hidden = 0;
                    lg("focus back - hidden from %s", "game");
                    continue; /* game never saw it leave */
                }
                return r;
            }
            if (s->state == XR_SESSION_STATE_VISIBLE && !hidden) {
                /* Heading up (READY->SYNCHRONIZED->VISIBLE) must pass; a focus loss is
                   VISIBLE arriving after FOCUSED, tracked in 'focused_seen'. */
                if (focused_seen) {
                    hidden = 1;
                    lg("focus lost - hidden from %s", "game");
                    continue;
                }
                return r;
            }
            if (hidden) {
                /* Session is really going down while hidden: give the game the VISIBLE
                   it skipped first, then this event. */
                memcpy(&pending, ev, sizeof pending);
                have_pending = 1;
                s->state = XR_SESSION_STATE_VISIBLE;
                hidden = 0;
                lg("session ending - delivering VISIBLE first (%s)", "ok");
                return r;
            }
            return r;
        }

        if (ev->type == XR_TYPE_EVENT_DATA_USER_PRESENCE_CHANGED_EXT) {
            XrEventDataUserPresenceChangedEXT *u = (XrEventDataUserPresenceChangedEXT *)ev;
            if (!u->isUserPresent) {
                lg("headset-removed signal - hidden from %s", "game");
                continue;
            }
        }
        return r;
    }
}

/* Watch every delivered state so we know when the game has been FOCUSED. */
static XrResult tracking_xrPollEvent(XrInstance instance, XrEventDataBuffer *ev)
{
    XrResult r = nopause_xrPollEvent(instance, ev);
    if (r == XR_SUCCESS && ev->type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
        XrSessionState st = ((XrEventDataSessionStateChanged *)ev)->state;
        if (st == XR_SESSION_STATE_FOCUSED)
            focused_seen = 1;
        else if (st != XR_SESSION_STATE_VISIBLE)
            focused_seen = 0;
    }
    return r;
}

static XrResult nopause_xrGetInstanceProcAddr(XrInstance instance, const char *name, PFN_xrVoidFunction *fn)
{
    XrResult r = next_gipa(instance, name, fn);
    if (r == XR_SUCCESS && strcmp(name, "xrPollEvent") == 0) {
        next_poll = (PFN_xrPollEvent)*fn;
        *fn = (PFN_xrVoidFunction)tracking_xrPollEvent;
    }
    return r;
}

static XrResult nopause_xrCreateApiLayerInstance(const XrInstanceCreateInfo *info, const XrApiLayerCreateInfo *layerInfo,
                                                 XrInstance *instance)
{
    if (!layerInfo || !layerInfo->nextInfo)
        return XR_ERROR_INITIALIZATION_FAILED;
    next_gipa = layerInfo->nextInfo->nextGetInstanceProcAddr;

    XrApiLayerCreateInfo nextLayerInfo;
    memcpy(&nextLayerInfo, layerInfo, sizeof nextLayerInfo);
    nextLayerInfo.nextInfo = layerInfo->nextInfo->next;
    XrResult r = layerInfo->nextInfo->nextCreateApiLayerInstance(info, &nextLayerInfo, instance);
    if (r != XR_SUCCESS)
        return r;

    active = app_listed(info->applicationInfo.applicationName);
    lg(active ? "active for \"%s\"" : "loaded, inactive for \"%s\"", info->applicationInfo.applicationName);

    /* Make sure our xrPollEvent hook is in place even if the loader cached it. */
    PFN_xrVoidFunction f;
    if (next_gipa(*instance, "xrPollEvent", &f) == XR_SUCCESS)
        next_poll = (PFN_xrPollEvent)f;
    return XR_SUCCESS;
}

__attribute__((visibility("default"))) XrResult
xrNegotiateLoaderApiLayerInterface(const XrNegotiateLoaderInfo *loaderInfo, const char *layerName,
                                   XrNegotiateApiLayerRequest *req)
{
    (void)layerName;
    if (!loaderInfo || !req || loaderInfo->structType != XR_LOADER_INTERFACE_STRUCT_LOADER_INFO ||
        req->structType != XR_LOADER_INTERFACE_STRUCT_API_LAYER_REQUEST ||
        loaderInfo->minInterfaceVersion > XR_CURRENT_LOADER_API_LAYER_VERSION ||
        loaderInfo->maxInterfaceVersion < XR_CURRENT_LOADER_API_LAYER_VERSION)
        return XR_ERROR_INITIALIZATION_FAILED;
    req->layerInterfaceVersion = XR_CURRENT_LOADER_API_LAYER_VERSION;
    req->layerApiVersion = XR_MAKE_VERSION(1, 0, 0);
    req->getInstanceProcAddr = nopause_xrGetInstanceProcAddr;
    req->createApiLayerInstance = nopause_xrCreateApiLayerInstance;
    return XR_SUCCESS;
}
