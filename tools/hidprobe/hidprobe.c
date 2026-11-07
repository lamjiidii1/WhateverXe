// hidprobe: print raw input reports of the internal I2C touchpad for N seconds
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/hidsystem/IOHIDLib.h>
#include <string.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <stdlib.h>

static long counts[256];
static long total;
static int printed;

static void onReport(void *ctx, IOReturn r, void *sender, IOHIDReportType type, uint32_t id, uint8_t *rep, CFIndex len) {
    total++; counts[id & 0xFF]++;
    if (printed < 40) {
        printed++;
        printf("report id=%u len=%ld:", id, (long)len);
        for (CFIndex i = 0; i < len && i < 32; i++) printf(" %02x", rep[i]);
        printf("\n");
    }
}

static long num(IOHIDDeviceRef d, CFStringRef k) {
    CFTypeRef v = IOHIDDeviceGetProperty(d, k); long n = 0;
    if (v && CFGetTypeID(v) == CFNumberGetTypeID()) CFNumberGetValue(v, kCFNumberLongType, &n);
    return n;
}

int main(int argc, char **argv) {
    int secs = argc > 1 ? atoi(argv[1]) : 10;
    printf("hidprobe: TCC Input Monitoring access = %d (0 granted, 1 denied, 2 unknown)\n", IOHIDCheckAccess(kIOHIDRequestTypeListenEvent));
    IOHIDRequestAccess(kIOHIDRequestTypeListenEvent);
    IOHIDManagerRef m = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    IOHIDManagerSetDeviceMatching(m, NULL);
    IOHIDManagerScheduleWithRunLoop(m, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOReturn o = IOHIDManagerOpen(m, kIOHIDOptionsTypeNone);
    printf("manager open: 0x%x\n", o);
    CFSetRef set = IOHIDManagerCopyDevices(m);
    if (!set) { printf("no devices\n"); return 1; }
    CFIndex n = CFSetGetCount(set);
    IOHIDDeviceRef *devs = calloc(n, sizeof(*devs));
    CFSetGetValues(set, (const void **)devs);
    int found = 0;
    for (CFIndex i = 0; i < n; i++) {
        long vid = num(devs[i], CFSTR(kIOHIDVendorIDKey)), pid = num(devs[i], CFSTR(kIOHIDProductIDKey));
        long up = num(devs[i], CFSTR(kIOHIDPrimaryUsagePageKey)), u = num(devs[i], CFSTR(kIOHIDPrimaryUsageKey));
        CFTypeRef t = IOHIDDeviceGetProperty(devs[i], CFSTR(kIOHIDTransportKey));
        char tr[64] = "?"; if (t && CFGetTypeID(t) == CFStringGetTypeID()) CFStringGetCString(t, tr, sizeof tr, kCFStringEncodingUTF8);
        printf("device vid=0x%04lx pid=0x%04lx usage=%lx:%lx transport=%s\n", vid, pid, up, u, tr);
        if (vid == 0x4d49) {
            found = 1;
            IOReturn r = IOHIDDeviceOpen(devs[i], kIOHIDOptionsTypeNone);
            printf("  touchpad open: 0x%x\n", r);
            uint8_t *buf = calloc(1, 2048);
            IOHIDDeviceRegisterInputReportCallback(devs[i], buf, 2048, onReport, NULL);
            uint8_t fr[8]; CFIndex fl;
            const char *mode = argc > 2 ? argv[2] : "";
            if (!strcmp(mode, "ptp") || !strcmp(mode, "mouse")) {
                uint8_t im[2] = {4, !strcmp(mode, "ptp") ? 3 : 0};
                r = IOHIDDeviceSetReport(devs[i], kIOHIDReportTypeFeature, 4, im, 2);
                printf("  set input mode %u: ret=0x%x\n", im[1], r);
                if (!strcmp(mode, "ptp")) {
                    uint8_t sw[2] = {6, 3};
                    r = IOHIDDeviceSetReport(devs[i], kIOHIDReportTypeFeature, 6, sw, 2);
                    printf("  set surface+button switch: ret=0x%x\n", r);
                }
            }
            for (int id = 4; id <= 8; id += 2) {
                fl = sizeof fr; memset(fr, 0, sizeof fr);
                r = IOHIDDeviceGetReport(devs[i], kIOHIDReportTypeFeature, id, fr, &fl);
                printf("  feature report %d: ret=0x%x len=%ld:", id, r, (long)fl);
                for (CFIndex j = 0; j < fl; j++) printf(" %02x", fr[j]);
                printf("\n");
            }
        }
    }
    if (!found) printf("touchpad 0x4d49 not found\n");
    printf(">>> MUOVI IL DITO SUL TOUCHPAD ORA (%d secondi)\n", secs); fflush(stdout);
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, secs, false);
    printf(">>> FATTO. reports total=%ld\n", total);
    for (int i = 0; i < 256; i++) if (counts[i]) printf("  id %d: %ld\n", i, counts[i]);
    return 0;
}
