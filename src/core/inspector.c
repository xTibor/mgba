#include <mgba/core/inspector.h>

#include <mgba/core/core.h>
#include <mgba-util/string.h>

const uint32_t M_INSPECTOR_DEVICE_ID = 0xACCE55ED;

mLOG_DEFINE_CATEGORY(INSPECTOR, "Inspector", "core.inspector");

DEFINE_VECTOR(mInspectorRegionList, struct mInspectorRegion);

static void mInspectorDeviceInit(void*, struct mCPUComponent*);
static void mInspectorDeviceDeinit(struct mCPUComponent*);

void mInspectorDeviceCreate(struct mInspectorDevice* device) {
	printf("mInspectorDeviceCreate\n");
	device->d.id = M_INSPECTOR_DEVICE_ID;
	device->d.init = mInspectorDeviceInit;
	device->d.deinit = mInspectorDeviceDeinit;
}

void mInspectorDeviceDestroy(struct mInspectorDevice* device) {
	printf("mInspectorDeviceDestroy\n");
	free(device);
}

void mInspectorDeviceInit(void* cpu, struct mCPUComponent* component) {
	printf("mInspectorDeviceInit\n");
	struct mInspectorDevice* device = (struct mInspectorDevice*) component;

	mInspectorRegionListInit(&device->regionList, 10);

	const struct mCoreMemoryBlock* info;
	size_t nBlocks = device->p->listMemoryBlocks(device->p, &info);
	if (info) {
		for (size_t i = 0; i < nBlocks; ++i) {
			if (info[i].flags & mCORE_MEMORY_VIRTUAL) {
				continue;
			}

			struct mInspectorRegion* region = mInspectorRegionListAppend(&device->regionList);
			region->name = strdup(info[i].longName);
			region->blockId = info[i].id;
			region->start = info[i].start;
			region->size = info[i].size;

			// These allocations may seem excessive, but overcommit takes care of them (at least under Linux).
			region->timestampRead = calloc(region->size, sizeof(uint32_t));
			region->timestampWrite = calloc(region->size, sizeof(uint32_t));
			region->timestampExecute = calloc(region->size, sizeof(uint32_t));
			// TODO: assert(region->TimestampXxxx)
		}
	}
}

void mInspectorDeviceDeinit(struct mCPUComponent* component) {
	printf("mInspectorDeviceDeinit\n");
	struct mInspectorDevice* device = (struct mInspectorDevice*) component;

	for (size_t i = 0; i < mInspectorRegionListSize(&device->regionList); ++i) {
		struct mInspectorRegion* region = mInspectorRegionListGetPointer(&device->regionList, i);
		free(region->name);
		free(region->timestampRead);
		free(region->timestampWrite);
		free(region->timestampExecute);
	}
	mInspectorRegionListDeinit(&device->regionList);
}

void mInspectorDeviceProcessEvent(struct mInspectorDevice* device, enum mInspectorEventKind eventKind, uint32_t address, uint32_t size) {
	for (size_t i = 0; i < mInspectorRegionListSize(&device->regionList); ++i) {
		struct mInspectorRegion* region = mInspectorRegionListGetPointer(&device->regionList, i);
		uint32_t timestamp = mInspectorDeviceCurrentTimestamp(device);

		if ((address >= region->start) && (address + size <= region->start + region->size)) {
			uint32_t start_offset = address - region->start;
			switch (eventKind) {
				case EVENT_READ:
					for (uint32_t offset = start_offset; offset < start_offset + size; ++offset)
						region->timestampRead[offset] = timestamp;
					break;
				case EVENT_WRITE:
					for (uint32_t offset = start_offset; offset < start_offset + size; ++offset)
						region->timestampWrite[offset] = timestamp;
					break;
				case EVENT_EXECUTE:
					for (uint32_t offset = start_offset; offset < start_offset + size; ++offset)
						region->timestampExecute[offset] = timestamp;
					break;
			}
			break;
		}
	}
}

uint32_t mInspectorDeviceCurrentTimestamp(struct mInspectorDevice* device) {
	// TODO: cycles
	return device->p->frameCounter(device->p);
}
