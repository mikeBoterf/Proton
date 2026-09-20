/* Minimal test kernel driver: on IOCTL_BUFTEST it computes a rolling hash over
 * the ENTIRE METHOD_BUFFERED input and returns it. A user program computes the
 * same hash on its side; a mismatch means Wine truncated/corrupted the buffer on
 * the way to the driver (i.e., a large-IOCTL marshaling bug). Diagnostic only. */
#include <ddk/ntddk.h>

#define IOCTL_BUFTEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

static UNICODE_STRING dev_name, dos_name;
static PDEVICE_OBJECT dev_obj;

static NTSTATUS finish(PIRP irp, NTSTATUS s, ULONG_PTR info)
{
    irp->IoStatus.Status = s;
    irp->IoStatus.Information = info;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return s;
}

static NTSTATUS create_close(PDEVICE_OBJECT d, PIRP irp) { (void)d; return finish(irp, STATUS_SUCCESS, 0); }

static NTSTATUS ioctl(PDEVICE_OBJECT d, PIRP irp)
{
    PIO_STACK_LOCATION s = IoGetCurrentIrpStackLocation(irp);
    ULONG inlen, outlen, i;
    unsigned char *buf;
    unsigned long long h;
    (void)d;
    if (s->Parameters.DeviceIoControl.IoControlCode != IOCTL_BUFTEST)
        return finish(irp, STATUS_INVALID_DEVICE_REQUEST, 0);
    inlen  = s->Parameters.DeviceIoControl.InputBufferLength;
    outlen = s->Parameters.DeviceIoControl.OutputBufferLength;
    buf    = (unsigned char *)irp->AssociatedIrp.SystemBuffer;
    h = 1469598103934665603ULL ^ inlen;                 /* seed folds in the length */
    for (i = 0; i < inlen; i++) h = (h ^ buf[i]) * 1099511628211ULL;   /* FNV-1a */
    if (outlen < sizeof(h)) return finish(irp, STATUS_BUFFER_TOO_SMALL, 0);
    RtlCopyMemory(buf, &h, sizeof(h));
    return finish(irp, STATUS_SUCCESS, sizeof(h));
}

static VOID unload(PDRIVER_OBJECT drv)
{
    (void)drv;
    IoDeleteSymbolicLink(&dos_name);
    if (dev_obj) IoDeleteDevice(dev_obj);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT drv, PUNICODE_STRING reg)
{
    NTSTATUS st;
    (void)reg;
    RtlInitUnicodeString(&dev_name, L"\\Device\\BufTest");
    RtlInitUnicodeString(&dos_name, L"\\DosDevices\\BufTest");
    st = IoCreateDevice(drv, 0, &dev_name, FILE_DEVICE_UNKNOWN, 0, FALSE, &dev_obj);
    if (st) return st;
    IoCreateSymbolicLink(&dos_name, &dev_name);
    drv->MajorFunction[IRP_MJ_CREATE]         = create_close;
    drv->MajorFunction[IRP_MJ_CLOSE]          = create_close;
    drv->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ioctl;
    drv->DriverUnload = unload;
    return STATUS_SUCCESS;
}
