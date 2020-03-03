// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// 
//
//  Copyright (c) 2019-2020 checkra1n team
//  This file is part of pongoOS.
//
#define LL_KTRW_INTERNAL 1
#include <pongo.h>
extern dt_node_t *gDeviceTree;
extern uint64_t gIOBase;
extern int socnum;
#include <libfdt.h>
#include <lzma/lzmadec.h>
#define LINUX_DTREE_SIZE 65536

void * fdt;
bool fdt_initialized = false;
void linux_dtree_init(void) {
    char compatible_apple[64];
    char * arm_io_type = dt_get_prop("arm-io", "device_type", NULL);
    char soc_name[64];
    char fdt_nodename[64];
    strncpy(soc_name, arm_io_type, 6);
    soc_name[5] = 0;
    compatible_apple[0] = 0;
    strcat(compatible_apple, "apple,");
    strcat(compatible_apple, soc_name);
    if (fdt_initialized)
        free(fdt);
    fdt = malloc(LINUX_DTREE_SIZE);
    int status = fdt_create_empty_tree(fdt, LINUX_DTREE_SIZE);

    int node = 0, node1 = 0;
    fdt_appendprop_string(fdt, node, "compatible", compatible_apple);
    uint32_t size;
    void * prop = dt_prop(gDeviceTree, "product-name", &size);
    char name[20];
    strncpy(name,prop,size);
    name[size]=0;
    fdt_appendprop_string(fdt, node, "name", name);
    fdt_appendprop_cell(fdt, 0, "#address-cells", 0x2);
    fdt_appendprop_cell(fdt, 0, "#size-cells", 0x2);
    fdt_appendprop_cell(fdt, 0, "interrupt-parent", 0x1);

    /* Alias */
    node = fdt_add_subnode(fdt, 0, "/aliases");
    char serials[64];
    siprintf(serials, "/soc/serial@%lx",((uint64_t)dt_get_u32_prop("uart0", "reg"))+ gIOBase);
    fdt_appendprop_string(fdt, node, "serial0", serials);

    /* CPU */
    node = fdt_add_subnode(fdt, 0, "/cpus");
    fdt_appendprop_string(fdt, node, "name", "cpus");
    fdt_appendprop_cell(fdt, node, "#address-cells", 0x1);
    fdt_appendprop_cell(fdt, node, "#size-cells", 0x0);

    node1 = fdt_add_subnode(fdt, node, "/cpu@0");
    fdt_appendprop_string(fdt, node1, "device_type", "cpu");
    fdt_appendprop_string(fdt, node1, "compatible", "hx,v1");
    fdt_appendprop_cell(fdt, node1, "reg", 0);

    node1 = fdt_add_subnode(fdt, node, "/cpu@1");
    fdt_appendprop_string(fdt, node1, "device_type", "cpu");
    fdt_appendprop_string(fdt, node1, "compatible", "hx,v1");
    fdt_appendprop_cell(fdt, node1, "reg", 1);

    /* refclk */
    node = fdt_add_subnode(fdt, 0, "/refclk24mhz");
    fdt_appendprop_string(fdt, node, "compatible", "fixed-clock");
    fdt_appendprop_string(fdt, node, "clock-output-names", "refclk24mhz");
    fdt_appendprop_cell(fdt, node, "#clock-cells", 0x00000000);
    fdt_appendprop_cell(fdt, node, "clock-frequency", 0x016e3600);
    fdt_appendprop_cell(fdt, node, "phandle", 2);

    /* Timer */
    node = fdt_add_subnode(fdt, 0, "/timer");
    fdt_appendprop_string(fdt, node, "name", "timer");
    fdt_appendprop_string(fdt, node, "device_type", "timer");
    fdt_appendprop_string(fdt, node, "compatible", "arm,armv8-timer");
    fdt_appendprop_cell(fdt, node, "interrupts", 0x00000001);
    fdt_appendprop_cell(fdt, node, "interrupts", 0x00000001);
    fdt_appendprop_cell(fdt, node, "interrupts", 0x00000f08);
    fdt_appendprop_cell(fdt, node, "interrupts", 0x00000001);
    fdt_appendprop_cell(fdt, node, "interrupts", 0x00000000);
    fdt_appendprop_cell(fdt, node, "interrupts", 0x00000f08);

    /* SoC */
    node = fdt_add_subnode(fdt, 0, "/soc");
    fdt_appendprop_string(fdt, node, "compatible", "simple-bus");
    fdt_appendprop_cell(fdt, node, "#address-cells", 0x2);
    fdt_appendprop_cell(fdt, node, "#size-cells", 0x2);
    fdt_appendprop(fdt, node, "ranges", "", 0);

    /* Interrupt controller: Apple AIC */
    siprintf(fdt_nodename, "/interrupt-controller@%lx", (uint64_t)dt_get_u32_prop("aic", "reg") + gIOBase);
    node1 = fdt_add_subnode(fdt, node, fdt_nodename);
    fdt_appendprop_string(fdt, node1, "name", "interrupt_controller");
    fdt_appendprop_string(fdt, node1, "device_type", "interrupt_controller");
    fdt_appendprop_string(fdt, node1, "compatible", "hx,aic");
    fdt_appendprop_cell(fdt, node1, "phandle", 0x1);
    fdt_appendprop_cell(fdt, node1, "linux,phandle", 0x1);
    fdt_appendprop_cell(fdt, node1, "#interrupt-cells", 0x3);
    fdt_appendprop_addrrange(fdt,0, node1, "reg",
                                ((uint64_t)dt_get_u32_prop("aic", "reg")) + gIOBase,
                                0x8000);
    fdt_appendprop(fdt, node1, "interrupt-controller", "", 0);

    /* UART */
    siprintf(fdt_nodename, "/serial@%lx", (uint64_t)dt_get_u32_prop("uart0", "reg") + gIOBase);
    node1 = fdt_add_subnode(fdt, node, fdt_nodename);
    fdt_appendprop_string(fdt, node1, "compatible", "hx,uart");
    fdt_appendprop_addrrange(fdt,0, node1, "reg",
                                ((uint64_t)dt_get_u32_prop("uart0", "reg")) + gIOBase,
                                0x4000);
    fdt_appendprop_cell(fdt, node1, "interrupts", 0);
    fdt_appendprop_cell(fdt, node1, "interrupts", dt_get_u32_prop("uart0", "interrupts"));
    fdt_appendprop_cell(fdt, node1, "interrupts", 4);
    fdt_appendprop_cell(fdt, node1, "clocks", 2);
    fdt_appendprop_string(fdt, node1, "clock-names", "refclk");

    /* DRAM */
    node = fdt_add_subnode(fdt, 0, "/memory@800000000");
    fdt_appendprop_addrrange(fdt, 0, node, "reg", 0x800000000, (gBootArgs->memSize - 0x02000000) & ~0x1FFFFFF);
    fdt_appendprop_string(fdt, node, "device_type", "memory");

    /* Reserved memory */
/*
    node = fdt_add_subnode(fdt, 0, "/reserved-memory");
    fdt_appendprop_cell(fdt, node, "#address-cells", 0x2);
    fdt_appendprop_cell(fdt, node, "#size-cells", 0x2);
    fdt_appendprop(fdt, node, "ranges", "", 0);

    uint64_t nomap_area = 0x800000000 + gBootArgs->memSize - 0x02000000;
    siprintf(fdt_nodename, "/fw_area@%lx", nomap_area);
    node1 = fdt_add_subnode(fdt, node, fdt_nodename);
    fdt_appendprop_addrrange(fdt, 0, node1, "reg", nomap_area, 0x04000000);
    fdt_appendprop(fdt, node1, "no-map", "", 0);
*/
}

void linux_dtree_late(void) {
    /* Chosen subnode for arguments */
    char fdt_nodename[64];
    int node = fdt_add_subnode(fdt, 0, "/chosen");
    int node1 = 0;
    fdt_appendprop(fdt, node, "ranges", "", 0);

    char cmdline[256];
    siprintf(cmdline, "debug earlycon=hx_uart,0x%llx console=tty0 console=ttyHX0", ((uint64_t)dt_get_u32_prop("uart0", "reg"))+ gIOBase);
    fdt_appendprop_string(fdt, node, "bootargs", cmdline);

    /* simplefb dart-apcie3*/
    sprintf(fdt_nodename, "/framebuffer@%lx", gBootArgs->Video.v_baseAddr);
    node1 = fdt_add_subnode(fdt, node, fdt_nodename);
    fdt_appendprop_addrrange(fdt,0, node1, "reg", gBootArgs->Video.v_baseAddr, gBootArgs->Video.v_height * gBootArgs->Video.v_rowBytes);
    fdt_appendprop_cell(fdt, node1, "width", gBootArgs->Video.v_width);
    fdt_appendprop_cell(fdt, node1, "height", gBootArgs->Video.v_height);
    fdt_appendprop_cell(fdt, node1, "stride", gBootArgs->Video.v_rowBytes);
    if (gBootArgs->Video.v_depth == 0x1001e) {
        fdt_appendprop_string(fdt, node1, "format", "a2r10g10b10");
    } else {
        fdt_appendprop_string(fdt, node1, "format", "a8r8g8b8");
    }
    fdt_appendprop_string(fdt, node1, "status", "okay");
    fdt_appendprop_string(fdt, node1, "compatible", "simple-framebuffer");
}

void linux_dtree_overlay(char *boot_args) {
    char fdt_nodename[64];
    int node = 0, node1 = 0;
    sprintf(fdt_nodename, "/framebuffer@%lx", gBootArgs->Video.v_baseAddr);
    node1 = fdt_add_subnode(fdt, node, fdt_nodename);
    fdt_appendprop_addrrange(fdt,0, node1, "reg", gBootArgs->Video.v_baseAddr, gBootArgs->Video.v_height * gBootArgs->Video.v_rowBytes);
    fdt_appendprop_cell(fdt, node1, "width", gBootArgs->Video.v_width);
    fdt_appendprop_cell(fdt, node1, "height", gBootArgs->Video.v_height);
    fdt_appendprop_cell(fdt, node1, "stride", gBootArgs->Video.v_rowBytes);
    fdt_appendprop_string(fdt, node1, "format", "a8b8g8r8");
    fdt_appendprop_string(fdt, node1, "status", "okay");
    fdt_appendprop_string(fdt, node1, "compatible", "simple-framebuffer");
 
    if(boot_args) {
        node = fdt_path_offset(fdt, "/chosen");
        if (node < 0) {
            iprintf("Failed to find /chosen");
	        return;
        }

        if (fdt_delprop(fdt, node, "bootargs") < 0) {
	    iprintf("Failed to delete bootargs");
            return;
        }

        fdt_appendprop_string(fdt, node, "bootargs", boot_args);
    }
    
    dt_node_t* dev = dt_find(gDeviceTree, "dart-apcie3");
    if (dev) {
        // this is d10
        node = fdt_path_offset(fdt, "/soc");
        fdt_for_each_subnode(node, fdt, node) {
            const char* nm = fdt_get_name(fdt, node, NULL);
            if (strncmp(nm, "dart-pcie3@", strlen("dart-pcie3@")) == 0) {
                fdt_set_name(fdt, node, "disabled");
            } 
        };
        fdt_for_each_subnode(node, fdt, node) {
            const char* nm = fdt_get_name(fdt, node, NULL);
            if (strncmp(nm, "dart-pcie2@", strlen("dart-pcie2@")) == 0) {
                iprintf("found node: %s\n", nm);                
                uint32_t tmp[3];
                tmp[0] = 0;
                tmp[1] = dt_get_u32_prop("dart-apcie3", "interrupts");
                tmp[2] = 4;
                uint32_t len = 0;
                fdt_setprop_inplace(fdt, node, "interrupts", &tmp, 12);
                uint64_t* dtr = (uint64_t*)dt_prop(dev, "reg", &len);
                fdt_setprop_inplace(fdt, node, "reg", dtr, 16);   
                if (len != 16) panic("invalid xnu dtre");  
                char nodename[64];
                siprintf(nodename, "dart-pcie3@%lx",dtr[0]);
                fdt_set_name(fdt, node, nodename);           
                iprintf("it's now %s\n", fdt_get_name(fdt, node, NULL));
            } else if (strncmp(nm, "pcie@", strlen("pcie@")) == 0) {
                iprintf("found node: %s\n", nm);
                fdt_delprop(fdt, node, "devpwr-on-2");
                uint32_t w[] = {0,0,1,0,0,1,1,1};
                fdt_appendprop(fdt, node, "devpwr-on-3", &w, sizeof(w));
            }
        } 
    }
    

}

bool linux_can_boot() {
    if (!loader_xfer_recv_count) return false;
    return true;
}

void* gLinuxStage;
uint32_t gLinuxStageSize;

void linux_prep_boot() {
    // invoked in sched task with MMU on
    if (!fdt_initialized) {
        linux_dtree_init();
        linux_dtree_late();
    } else {
    	if (fdt_open_into((void *)fdt, (void *)fdt, LINUX_DTREE_SIZE)) {
        	iprintf("failed to apply overlay to fdt\n");
	        return;
    	}
	    linux_dtree_overlay(NULL); // later we can pass bootargs
    }

    //else if (socnum == 0x8010) {	
#define pixfmt0 (&disp[0x402c/4])
#define colormatrix_bypass (&disp[0x40b4/4])
#define colormatrix_mul_31 (&disp[0x40cc/4])
#define colormatrix_mul_32 (&disp[0x40d4/4])
#define colormatrix_mul_33 (&disp[0x40dc/4])
    
    volatile uint32_t* disp = ((uint32_t*)(dt_get_u32_prop("disp0", "reg") + gIOBase));
    
    *pixfmt0 = (*pixfmt0 & 0xF00FFFFFu) | 0x05200000u;

    *colormatrix_bypass = 0;
    *colormatrix_mul_31 = 4095;
    *colormatrix_mul_32 = 4095;
    *colormatrix_mul_33 = 4095;
    //}
    //else {
	puts("This is only supported on iPhone 7 for now and works to a lesser extent on other A10 devices. Behavior on non-A10 devices is undefined!!");
    //}

    gEntryPoint = (void*)(0x800080000);
    uint64_t image_size = loader_xfer_recv_count; 
    gLinuxStage = (void*)alloc_contig(image_size+LINUX_DTREE_SIZE);
    size_t dest_size = 0x10000000;
    int res = unlzma_decompress((uint8_t *)gLinuxStage, &dest_size, loader_xfer_recv_data, image_size);
    if (res != SZ_OK) {
	    puts("Assuming decompressed kernel.");
	    image_size = *(uint64_t *)(loader_xfer_recv_data + 16);
	    memcpy(gLinuxStage, loader_xfer_recv_data, image_size);
    }
    else {
	    image_size = *(uint64_t *)(gLinuxStage + 16);
    }
    void *gLinuxDtre = (void*)((((uint64_t)gLinuxStage) + image_size + 7ull) & -8ull);
    memcpy(gLinuxDtre, fdt, LINUX_DTREE_SIZE);
    gLinuxStageSize = image_size + LINUX_DTREE_SIZE;
    gBootArgs = (void*)((((uint64_t)gEntryPoint) + image_size + 7ull) & -8ull);
    iprintf("Booting Linux: %p(%p)\n", gEntryPoint, gBootArgs);
    gLinuxStage = (void*)(((uint64_t)gLinuxStage) - kCacheableView + 0x800000000);
}

void linux_boot() {
    memcpy(gEntryPoint, gLinuxStage, gLinuxStageSize);
}
