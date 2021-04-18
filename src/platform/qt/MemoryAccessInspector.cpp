#include "MemoryAccessInspector.h"

#include "CoreController.h"
#include "GBAApp.h"

#include <mgba/core/core.h>
#include <mgba/core/inspector.h>

using namespace QGBA;

MemoryAccessInspector::MemoryAccessInspector(std::shared_ptr<CoreController> controller, QWidget* parent)
	: QWidget(parent)
	, m_controller(controller)
{
	m_ui.setupUi(this);
	m_ui.memoryAccessPainter->setController(controller);

	CoreController::Interrupter interrupter(m_controller);
	mCore* core = m_controller->thread()->core;
	mInspectorDevice* inspector = core->inspectorDevice(core);

	for (size_t i = 0; i < mInspectorRegionListSize(&inspector->regionList); ++i) {
		const mInspectorRegion* region = mInspectorRegionListGetConstPointer(&inspector->regionList, i);
		m_ui.regions->addItem(region->name);
	}

	connect(m_ui.regions, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
	        this, &MemoryAccessInspector::setRegionIndex);
	setRegionIndex(0);

	connect(controller.get(), &CoreController::frameAvailable, this, &MemoryAccessInspector::updateMemoryAccessPainter);

	connect(controller.get(), &CoreController::stopping, this, &QWidget::close);
}

void MemoryAccessInspector::updateMemoryAccessPainter() {
	m_ui.memoryAccessPainter->update();
}

void MemoryAccessInspector::setRegionIndex(int index) {
	CoreController::Interrupter interrupter(m_controller);
	mCore* core = m_controller->thread()->core;
	mInspectorDevice* inspector = core->inspectorDevice(core);

	// TODO: assert()?
	if (index < 0 || index >= static_cast<int>(mInspectorRegionListSize(&inspector->regionList))) {
		printf("Invalid index %d", index);
		return;
	}
	m_regionIndex = index;
	m_ui.memoryAccessPainter->setRegionIndex(index);
}
