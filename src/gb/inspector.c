#include <mgba/internal/gb/inspector.h>

#include <mgba/core/core.h>
#include <mgba/internal/gb/gb.h>

struct mInspectorDevice* GBInspectorDeviceCreate(void) {
	struct mInspectorDevice* device = malloc(sizeof(*device));
	mInspectorDeviceCreate(device);
	return device;
}
