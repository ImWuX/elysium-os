#include <stdint.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/utsname.h>
#include <lib/str.h>
#include <lib/mem.h>
#include <common/log.h>
#include <sched/thread.h>
#include <memory/heap.h>
#include <arch/types.h>
#include <arch/sched.h>
#include <arch/x86_64/sched.h>
#include <arch/x86_64/sys/msr.h>
#include <arch/x86_64/sys/gdt.h>

#define MSR_EFER_SCE 1

typedef struct {
    uint64_t value;
    uint64_t errno;
} syscall_return_t;

extern void x86_64_syscall_entry();

void x86_64_syscall_exit(int code) {
    log(LOG_LEVEL_DEBUG, "SYSCALL", "exit(code: %i, tid: %li)", code, arch_sched_thread_current()->id);
    arch_sched_thread_current()->state = THREAD_STATE_DESTROY;
    x86_64_sched_next();
    __builtin_unreachable();
}

syscall_return_t x86_64_syscall_debug(char c) {
    syscall_return_t ret = {};
    log_raw(c);
    return ret;
}

syscall_return_t x86_64_syscall_anon_allocate(uintptr_t size) {
    syscall_return_t ret = {};
    if(size == 0 || size % ARCH_PAGE_SIZE != 0) {
        ret.errno = EINVAL;
        return ret;
    }
    void *p = vmm_map(arch_sched_thread_current()->proc->address_space, NULL, size, VMM_PROT_READ | VMM_PROT_WRITE, VMM_FLAG_NONE, &g_seg_anon, NULL);
    ret.value = (uintptr_t) p;
    log(LOG_LEVEL_DEBUG, "SYSCALL", "anon_alloc(size: %#lx) -> %#lx", size, ret.value);
    return ret;
}

syscall_return_t x86_64_syscall_anon_free(void *pointer, size_t size) {
    syscall_return_t ret = {};
    if(
        size == 0 || size % ARCH_PAGE_SIZE != 0 ||
        pointer == NULL || ((uintptr_t) pointer) % ARCH_PAGE_SIZE != 0
    ) {
        ret.errno = EINVAL;
        return ret;
    }
    // CRITICAL: ensure this is safe for userspace to just do (currently throws a kern panic...)
    vmm_unmap(arch_sched_thread_current()->proc->address_space, pointer, size);
    log(LOG_LEVEL_DEBUG, "SYSCALL", "anon_free(ptr: %#lx, size: %#lx)", (uint64_t) pointer, size);
    return ret;
}

syscall_return_t x86_64_syscall_fs_set(void *ptr) {
    syscall_return_t ret = {};
    x86_64_msr_write(X86_64_MSR_FS_BASE, (uint64_t) ptr);
    log(LOG_LEVEL_DEBUG, "SYSCALL", "fs_set(ptr: %#lx)", (uint64_t) ptr);
    return ret;
}

syscall_return_t x86_64_syscall_uname(struct utsname *buf) {
    syscall_return_t ret = {};
    log(LOG_LEVEL_DEBUG, "SYSCALL", "uname(buf: %#lx)", (uint64_t) buf);

    // TODO: Should standardize copying to/from userspace
    strncpy(buf->sysname, "Elysium", sizeof(buf->sysname));
    strncpy(buf->nodename, "elysium", sizeof(buf->nodename));
    strncpy(buf->release, "pre-alpha", sizeof(buf->release));
    strncpy(buf->version, "pre-alpha (" __DATE__ " " __TIME__ ")", sizeof(buf->version));

    return ret;
}

syscall_return_t x86_64_syscall_open(int dir_resource_id, const char *path, int flags, mode_t mode) {
    syscall_return_t ret = {};

    // TODO: Should standardize copying to/from userspace
    char *safe_path = heap_alloc(PATH_MAX + 1);
    strncpy(safe_path, path, PATH_MAX);
    safe_path[PATH_MAX] = 0;

    log(LOG_LEVEL_DEBUG, "SYSCALL", "open(dir_resource_id: %i, path: %s, flags: %#i, mode: %u)", dir_resource_id, safe_path, flags, mode);

    process_t *proc = arch_sched_thread_current()->proc;

    vfs_context_t context;
    if(dir_resource_id == AT_FDCWD) {
        // TODO: need to setup CWD (vfs context)
        vfs_node_t *parent_node;
        int r = vfs_lookup("/", &parent_node, NULL);
        if(r != 0) {
            ret.errno = -r;
            return ret;
        }
        context = (vfs_context_t) { .cwd = parent_node };
    } else {
        resource_t *parent = resource_get(&proc->resource_table, dir_resource_id);
        if(parent == NULL) {
            ret.errno = EBADF;
            return ret;
        }
        context = (vfs_context_t) { .cwd = parent->node };
    }

    vfs_node_t *node;
    int r = vfs_lookup(safe_path, &node, &context);
    if(r != 0) {
        ret.errno = -r;
        return ret;
    }

    ret.value = (size_t) resource_create(&proc->resource_table, node);
    return ret;
}

syscall_return_t x86_64_syscall_close(int resource_id) {
    syscall_return_t ret = {};
    log(LOG_LEVEL_DEBUG, "SYSCALL", "close(resource_id: %i)", resource_id);

    process_t *proc = arch_sched_thread_current()->proc;
    int r = resource_remove(&proc->resource_table, resource_id);
    if(r != 0) ret.errno = -r;
    return ret;
}

syscall_return_t x86_64_syscall_read(int resource_id, void *buf, size_t count) {
    syscall_return_t ret = {};
    log(LOG_LEVEL_DEBUG, "SYSCALL", "read(resource_id: %i, buf: %#lx, count: %#lx)", resource_id, (uint64_t) buf, count);

    process_t *proc = arch_sched_thread_current()->proc;
    resource_t *resource = resource_get(&proc->resource_table, resource_id);
    if(resource == NULL) {
        ret.errno = EBADF;
        return ret;
    }

    // TODO: Should standardize copying to/from userspace
    void *safe_buf = heap_alloc(count);
    size_t read_count = 0;
    int r = resource->node->ops->rw(resource->node, &(vfs_rw_t) {
        .rw = VFS_RW_READ,
        .buffer = safe_buf,
        .size = count,
        .offset = resource->offset
    }, &read_count);
    memcpy(buf, safe_buf, read_count);
    heap_free(safe_buf);
    resource->offset += read_count;
    ret.value = read_count;
    if(r != 0) ret.errno = -r;
    return ret;
}

syscall_return_t x86_64_syscall_write(int resource_id, void *buf, size_t count) {
    syscall_return_t ret = {};
    log(LOG_LEVEL_DEBUG, "SYSCALL", "write(resource_id: %i, buf: %#lx, count: %#lx)", resource_id, (uint64_t) buf, count);

    process_t *proc = arch_sched_thread_current()->proc;
    resource_t *resource = resource_get(&proc->resource_table, resource_id);
    if(resource == NULL) {
        ret.errno = EBADF;
        return ret;
    }

    // TODO: Should standardize copying to/from userspace
    void *safe_buf = heap_alloc(count);
    memcpy(safe_buf, buf, count);

    size_t write_count = 0;
    int r = resource->node->ops->rw(resource->node, &(vfs_rw_t) {
        .rw = VFS_RW_WRITE,
        .buffer = safe_buf,
        .size = count,
        .offset = resource->offset
    }, &write_count);
    heap_free(safe_buf);
    resource->offset += write_count;
    ret.value = write_count;
    if(r != 0) ret.errno = -r;
    return ret;
}

syscall_return_t x86_64_syscall_seek(int resource_id, off_t offset, int whence) {
    syscall_return_t ret = {};
    log(LOG_LEVEL_DEBUG, "SYSCALL", "seek(resource_id: %i, offset: %#lx, whence: %i)", resource_id, offset, whence);

    process_t *proc = arch_sched_thread_current()->proc;
    resource_t *resource = resource_get(&proc->resource_table, resource_id);
    if(resource == NULL) {
        ret.errno = EBADF;
        return ret;
    }

    ssize_t current_offset = (ssize_t) resource->offset;
    ssize_t new_offset = 0;
    switch(whence) {
        case SEEK_CUR:
            new_offset = current_offset + offset;
            break;
        case SEEK_END:
            vfs_node_attr_t attr;
            int r = resource->node->ops->attr(resource->node, &attr);
            if(r != 0) {
                ret.errno = -r;
                return ret;
            }
            new_offset = offset + attr.file_size;
            break;
        case SEEK_SET:
            new_offset = offset;
            break;
        default:
            ret.errno = EINVAL;
            return ret;
    }

    if(new_offset < 0) {
        ret.errno = EINVAL;
        return ret;
    }

    // TODO: file should grow here already and not on write?

    resource->offset = (size_t) new_offset;
    ret.value = (size_t) new_offset;
    return ret;
}

void x86_64_syscall_init_cpu() {
    x86_64_msr_write(X86_64_MSR_EFER, x86_64_msr_read(X86_64_MSR_EFER) | MSR_EFER_SCE);
    x86_64_msr_write(X86_64_MSR_STAR, ((uint64_t) X86_64_GDT_CODE_RING0 << 32) | ((uint64_t) (X86_64_GDT_CODE_RING3 - 16) << 48));
    x86_64_msr_write(X86_64_MSR_LSTAR, (uint64_t) x86_64_syscall_entry);
    x86_64_msr_write(X86_64_MSR_SFMASK, x86_64_msr_read(X86_64_MSR_SFMASK) | (1 << 9));
}