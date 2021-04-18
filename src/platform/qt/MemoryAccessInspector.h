#pragma once

#include <QWidget>

#include <memory>

#include "ui_MemoryAccessInspector.h"

namespace QGBA {

class CoreController;

class MemoryAccessInspector : public QWidget {
Q_OBJECT

public:
	MemoryAccessInspector(std::shared_ptr<CoreController> controller, QWidget* parent = nullptr);

private slots:
	void updateMemoryAccessPainter();
	void setRegionIndex(int);

private:
	Ui::MemoryAccessInspector m_ui;
	std::shared_ptr<CoreController> m_controller;

	int m_regionIndex;
};

}
