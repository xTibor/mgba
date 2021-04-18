#ifndef INSPECTOR_H
#define INSPECTOR_H

#include <mgba-util/common.h>

CXX_GUARD_START

#include <mgba/core/cpu.h>
#include <mgba/core/log.h>
#include <mgba-util/table.h>
#include <mgba-util/vector.h>

enum mInspectorEventKind {
	EVENT_READ,
	EVENT_WRITE,
	EVENT_EXECUTE,
};

struct mInspectorRegion {
	char* name;
	size_t blockId;
	uint32_t start;
	uint32_t size;
	uint32_t* timestampRead;
	uint32_t* timestampWrite;
	uint32_t* timestampExecute;
};

mLOG_DECLARE_CATEGORY(INSPECTOR);

DECLARE_VECTOR(mInspectorRegionList, struct mInspectorRegion);

struct mInspectorDevice {
	struct mCPUComponent d;
	struct mCore* p;

	struct mInspectorRegionList regionList;
};

void mInspectorDeviceCreate(struct mInspectorDevice*);
void mInspectorDeviceDestroy(struct mInspectorDevice*);
void mInspectorDeviceProcessEvent(struct mInspectorDevice*, enum mInspectorEventKind, uint32_t address, uint32_t size);
uint32_t mInspectorDeviceCurrentTimestamp(struct mInspectorDevice*);

CXX_GUARD_END

#endif
