// hciprobe: talk HCI directly to an Intel USB Bluetooth controller.
// Sends Intel Read Version (TLV), HCI Reset, Read Local Version and Read
// BD_ADDR over the USB control pipe and prints the events read from the
// interrupt pipe. Tells apart "chip/firmware not answering" from "macOS
// Bluetooth stack not using a working chip".
// Usage: sudo ./hciprobe [vid pid]   (default 8087 0033)
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static IOUSBInterfaceInterface650 **intf;
static UInt8 evPipe, ifNumber;
static volatile int done;
static IOReturn readResult;
static UInt32 readLen;
static uint8_t rbuf[1024];

static void hex(const char *tag, const uint8_t *b, uint32_t n) {
    printf("%s", tag);
    for (uint32_t i = 0; i < n; i++) printf(" %02x", b[i]);
    printf("\n");
}

static void readDone(void *ref, IOReturn r, void *arg) {
    (void)ref;
    readResult = r;
    readLen = (UInt32)(uintptr_t)arg;
    done = 1;
    CFRunLoopStop(CFRunLoopGetCurrent());
}

// One interrupt-pipe read with a timeout. Returns kIOReturnSuccess or an error.
static IOReturn readEvent(double seconds) {
    done = 0;
    IOReturn kr = (*intf)->ReadPipeAsync(intf, evPipe, rbuf, sizeof(rbuf), readDone, NULL);
    if (kr) return kr;
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, seconds, false);
    if (!done) {
        (*intf)->AbortPipe(intf, evPipe);
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.3, false);
        return kIOUSBTransactionTimeout;
    }
    return readResult;
}

static void drain(void) {
    while (readEvent(0.3) == kIOReturnSuccess) hex("   (pending) <<", rbuf, readLen);
}

static int command(const char *name, uint16_t op, const uint8_t *p, uint8_t plen, double seconds) {
    uint8_t buf[260];
    buf[0] = op & 0xff; buf[1] = op >> 8; buf[2] = plen;
    if (plen) memcpy(buf + 3, p, plen);
    printf("\n== %s (opcode 0x%04x)\n", name, op);
    hex("   >>", buf, 3 + plen);
    IOUSBDevRequestTO r = {0};
    r.bmRequestType = USBmakebmRequestType(kUSBOut, kUSBClass, kUSBDevice);
    r.bRequest = 0; r.wValue = 0; r.wIndex = ifNumber;
    r.wLength = 3 + plen; r.pData = buf;
    r.noDataTimeout = 2000; r.completionTimeout = 2000;
    IOReturn kr = (*intf)->ControlRequestTO(intf, 0, &r);
    if (kr) { printf("   control transfer FAILED: 0x%08x\n", kr); return -1; }
    for (int i = 0; i < 8; i++) {
        kr = readEvent(seconds);
        if (kr) {
            printf("   no event: 0x%08x%s\n", kr, kr == kIOUSBTransactionTimeout ? " (TIMEOUT: the chip did not answer)" : "");
            return -1;
        }
        hex("   <<", rbuf, readLen);
        // Command Complete (0x0e): ev, len, ncmd, opcode lo, opcode hi, status
        if (readLen >= 6 && rbuf[0] == 0x0e && (rbuf[3] | rbuf[4] << 8) == op) {
            printf("   Command Complete, status 0x%02x%s\n", rbuf[5], rbuf[5] ? " (ERROR)" : " (OK)");
            return rbuf[5] ? -1 : 0;
        }
        // Command Status (0x0f): ev, len, status, ncmd, opcode lo, opcode hi
        if (readLen >= 6 && rbuf[0] == 0x0f && (rbuf[4] | rbuf[5] << 8) == op) {
            printf("   Command Status 0x%02x\n", rbuf[2]);
            return rbuf[2] ? -1 : 0;
        }
    }
    return -1;
}

static void decodeTLV(void) {
    // Event payload after the 6-byte Command Complete header.
    if (readLen < 7 || rbuf[0] != 0x0e) return;
    const uint8_t *p = rbuf + 6, *end = rbuf + readLen;
    while (p + 2 <= end && p + 2 + p[1] <= end) {
        uint8_t t = p[0], l = p[1];
        const uint8_t *v = p + 2;
        if (t == 0x1c && l >= 1) printf("   TLV image type: 0x%02x (%s)\n", v[0],
            v[0] == 1 ? "BOOTLOADER: operational firmware NOT running" :
            v[0] == 3 ? "OPERATIONAL: firmware running" : "unknown");
        else if (t == 0x10 && l >= 4) printf("   TLV cnvi top: 0x%08x\n", v[0] | v[1] << 8 | v[2] << 16 | (uint32_t)v[3] << 24);
        else if (t == 0x11 && l >= 4) printf("   TLV cnvr top: 0x%08x\n", v[0] | v[1] << 8 | v[2] << 16 | (uint32_t)v[3] << 24);
        else if (t == 0x1d && l >= 2) printf("   TLV fw build: %02x %02x%s\n", v[0], v[1], l >= 3 ? "" : "");
        p += 2 + l;
    }
}

int main(int argc, char **argv) {
    long vid = 0x8087, pid = 0x0033;
    if (argc >= 3) { vid = strtol(argv[1], NULL, 16); pid = strtol(argv[2], NULL, 16); }
    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("hciprobe: looking for USB %04lx:%04lx\n", vid, pid);

    CFMutableDictionaryRef match = IOServiceMatching("IOUSBHostDevice");
    CFNumberRef v = CFNumberCreate(NULL, kCFNumberLongType, &vid), d = CFNumberCreate(NULL, kCFNumberLongType, &pid);
    CFDictionarySetValue(match, CFSTR("idVendor"), v);
    CFDictionarySetValue(match, CFSTR("idProduct"), d);
    io_service_t devSvc = IOServiceGetMatchingService(kIOMainPortDefault, match);
    if (!devSvc) { printf("device not found (is the USB port in the map? is IntelBluetoothFirmware loaded?)\n"); return 1; }
    CFTypeRef loc = IORegistryEntryCreateCFProperty(devSvc, CFSTR("locationID"), NULL, 0);
    long location = 0; if (loc) { CFNumberGetValue(loc, kCFNumberLongType, &location); CFRelease(loc); }
    CFTypeRef fw = IORegistryEntryCreateCFProperty(devSvc, CFSTR("FirmwareLoaded"), NULL, 0);
    printf("found device, locationID 0x%08lx, FirmwareLoaded=%s\n", location, fw ? (CFBooleanGetValue(fw) ? "Yes" : "No") : "(absent)");

    IOCFPlugInInterface **plug = NULL; SInt32 score;
    IOUSBDeviceInterface650 **dev = NULL;
    if (IOCreatePlugInInterfaceForService(devSvc, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plug, &score) || !plug) {
        printf("cannot create device plugin\n"); return 1;
    }
    (*plug)->QueryInterface(plug, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID650), (LPVOID *)&dev);
    IODestroyPlugInInterface(plug);
    if (!dev) { printf("cannot get device interface\n"); return 1; }

    IOUSBFindInterfaceRequest req = {kIOUSBFindInterfaceDontCare, kIOUSBFindInterfaceDontCare, kIOUSBFindInterfaceDontCare, kIOUSBFindInterfaceDontCare};
    io_iterator_t it;
    if ((*dev)->CreateInterfaceIterator(dev, &req, &it)) { printf("cannot iterate interfaces\n"); return 1; }
    io_service_t ifSvc;
    while ((ifSvc = IOIteratorNext(it))) {
        IOUSBInterfaceInterface650 **cand = NULL;
        if (IOCreatePlugInInterfaceForService(ifSvc, kIOUSBInterfaceUserClientTypeID, kIOCFPlugInInterfaceID, &plug, &score) || !plug) continue;
        (*plug)->QueryInterface(plug, CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID650), (LPVOID *)&cand);
        IODestroyPlugInInterface(plug);
        if (!cand) continue;
        UInt8 num, cls; (*cand)->GetInterfaceNumber(cand, &num); (*cand)->GetInterfaceClass(cand, &cls);
        printf("interface %u class 0x%02x\n", num, cls);
        if (num == 0 && !intf) { intf = cand; ifNumber = num; } else (*cand)->Release(cand);
    }
    if (!intf) { printf("interface 0 not found\n"); return 1; }

    IOReturn kr = (*intf)->USBInterfaceOpen(intf);
    if (kr == kIOReturnExclusiveAccess) {
        printf("interface 0 is held by another client (bluetoothd?), trying to seize it\n");
        kr = (*intf)->USBInterfaceOpenSeize(intf);
    }
    if (kr) { printf("cannot open interface 0: 0x%08x\n", kr); return 1; }

    UInt8 n = 0; (*intf)->GetNumEndpoints(intf, &n);
    for (UInt8 p = 1; p <= n; p++) {
        UInt8 dir, num, type, interval; UInt16 mps;
        (*intf)->GetPipeProperties(intf, p, &dir, &num, &type, &mps, &interval);
        printf("pipe %u: ep 0x%02x dir %s type %u mps %u\n", p, num | (dir == kUSBIn ? 0x80 : 0), dir == kUSBIn ? "in" : "out", type, mps);
        if (dir == kUSBIn && type == kUSBInterrupt && !evPipe) evPipe = p;
    }
    if (!evPipe) { printf("no interrupt IN pipe\n"); return 1; }
    CFRunLoopSourceRef src;
    (*intf)->CreateInterfaceAsyncEventSource(intf, &src);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), src, kCFRunLoopDefaultMode);

    printf("\ndraining pending events...\n");
    drain();

    const uint8_t tlv = 0xff;
    if (command("Intel Read Version (TLV)", 0xfc05, &tlv, 1, 2.0) == 0) decodeTLV();
    command("HCI Reset", 0x0c03, NULL, 0, 3.0);
    if (command("Read Local Version Information", 0x1001, NULL, 0, 2.0) == 0 && readLen >= 14)
        printf("   HCI version %u, revision 0x%04x, LMP %u, manufacturer 0x%04x\n", rbuf[6], rbuf[7] | rbuf[8] << 8, rbuf[9], rbuf[10] | rbuf[11] << 8);
    if (command("Read BD_ADDR", 0x1009, NULL, 0, 2.0) == 0 && readLen >= 12)
        printf("   BD_ADDR %02x:%02x:%02x:%02x:%02x:%02x\n", rbuf[11], rbuf[10], rbuf[9], rbuf[8], rbuf[7], rbuf[6]);

    (*intf)->USBInterfaceClose(intf);
    printf("\nhciprobe: done\n");
    return 0;
}
