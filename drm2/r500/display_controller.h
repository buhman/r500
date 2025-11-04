#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void * map_pci_resource2();
void primary_surface_address(void * rmmio, int colorbuffer_ix);

#ifdef __cplusplus
}
#endif
