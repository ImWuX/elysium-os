# Memory Management

## PMM
---
### Structs
**`pmm_page_t`**  
A architecture independent representation of a page. Page sizes vary by architecture.
```c
typedef struct pmm_page {
    struct pmm_page *next;
    uintptr_t paddr;
    pmm_page_usage_t usage : 3;
    pmm_page_state_t state : 2;
    int rsv0 : 3;
} pmm_page_t;
```


## VMM
---
### Structs
**`vmm_address_space_t`**