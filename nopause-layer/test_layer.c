/* Test harness: acts as both "loader" and "runtime" for libnopause_layer.so. */
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#define main nopause_unused_main
#include "nopause_layer.c"
#undef main

static const struct { int type, state, present; } script[] = {
    {18, 1, 0}, {18, 2, 0}, {18, 3, 0}, {18, 4, 0}, {18, 5, 0}, /* startup to FOCUSED */
    {18, 4, 0}, {18, 5, 0},                                   /* Meta button: lost + back */
    {1000470000, 0, 0}, {1000470000, 0, 1},                   /* presence false, true */
    {18, 4, 0}, {18, 3, 0}, {18, 6, 0},                       /* focus lost, then session ends */
};
static int pos;

static XrResult rt_poll(XrInstance i, XrEventDataBuffer *ev)
{
    (void)i;
    if (pos >= (int)(sizeof script / sizeof *script))
        return XR_EVENT_UNAVAILABLE;
    memset(ev, 0, sizeof *ev);
    ev->type = script[pos].type;
    if (ev->type == 18)
        ((XrEventDataSessionStateChanged *)ev)->state = script[pos].state;
    else
        ((XrEventDataUserPresenceChangedEXT *)ev)->isUserPresent = script[pos].present;
    pos++;
    return XR_SUCCESS;
}
static XrResult rt_gipa(XrInstance i, const char *n, PFN_xrVoidFunction *f)
{
    (void)i;
    if (strcmp(n, "xrPollEvent") == 0) { *f = (PFN_xrVoidFunction)rt_poll; return XR_SUCCESS; }
    return XR_ERROR_FUNCTION_UNSUPPORTED;
}
static XrResult rt_create(const XrInstanceCreateInfo *c, const XrApiLayerCreateInfo *l, XrInstance *i)
{
    (void)c; (void)l; *i = (XrInstance)0x1234; return XR_SUCCESS;
}

static void run(const char *app)
{
    void *h = dlopen("./libnopause_layer.so", RTLD_NOW | RTLD_LOCAL);
    typeof(&xrNegotiateLoaderApiLayerInterface) neg = dlsym(h, "xrNegotiateLoaderApiLayerInterface");
    XrNegotiateLoaderInfo li = {XR_LOADER_INTERFACE_STRUCT_LOADER_INFO, 1, sizeof li, 1, 1, 0, ~0ULL};
    XrNegotiateApiLayerRequest rq = {XR_LOADER_INTERFACE_STRUCT_API_LAYER_REQUEST, 1, sizeof rq, 0, 0, 0, 0};
    printf("[%s] negotiate=%d ", app, neg(&li, "x", &rq));
    XrApiLayerNextInfo ni = {XR_LOADER_INTERFACE_STRUCT_API_LAYER_NEXT_INFO, 1, sizeof ni, "rt", rt_gipa, rt_create, 0};
    XrApiLayerCreateInfo ci = {XR_LOADER_INTERFACE_STRUCT_API_LAYER_CREATE_INFO, 1, sizeof ci, 0, "", &ni};
    XrInstanceCreateInfo ic; memset(&ic, 0, sizeof ic);
    strcpy(ic.applicationInfo.applicationName, app);
    XrInstance inst;
    printf("create=%d\n  delivered:", rq.createApiLayerInstance(&ic, &ci, &inst));
    PFN_xrVoidFunction f;
    rq.getInstanceProcAddr(inst, "xrPollEvent", &f);
    XrEventDataBuffer ev;
    pos = 0;
    while (((PFN_xrPollEvent)f)(inst, &ev) == XR_SUCCESS) {
        if (ev.type == 18) printf(" state%d", ((XrEventDataSessionStateChanged *)&ev)->state);
        else printf(" presence=%d", ((XrEventDataUserPresenceChangedEXT *)&ev)->isUserPresent);
    }
    printf("\n");
    dlclose(h);
}

int main(void)
{
    run("Beat Saber");
    run("Some Other Game");
    return 0;
}
