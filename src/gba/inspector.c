#include <mgba/internal/gba/inspector.h>

#include <mgba/core/core.h>
#include <mgba/internal/gba/gba.h>

struct mInspectorDevice* GBAInspectorDeviceCreate(void) {
	struct mInspectorDevice* device = malloc(sizeof(*device));
	mInspectorDeviceCreate(device);
	return device;
}

