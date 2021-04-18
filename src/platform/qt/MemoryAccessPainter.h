#pragma once

#include <QColor>
#include <QWidget>
#include <QVector>

#include <mgba/core/core.h>
#include <mgba/core/interface.h>

namespace QGBA {

class CoreController;

class MemoryAccessPainter : public QWidget {
Q_OBJECT

public:
	MemoryAccessPainter(QWidget* parent = nullptr);

public slots:
	void setRegionIndex(int index);
	void setController(std::shared_ptr<CoreController> controller);

signals:

protected:
	void mouseMoveEvent(QMouseEvent*) override;
	void mousePressEvent(QMouseEvent*) override;
	void mouseReleaseEvent(QMouseEvent*) override;
	void mouseDoubleClickEvent(QMouseEvent*) override;
	void wheelEvent(QWheelEvent*) override;
	void paintEvent(QPaintEvent*) override;
	void resizeEvent(QResizeEvent*) override;

private:
	std::shared_ptr<CoreController> m_controller;
	int m_regionIndex;

	float m_zoom;

	bool m_isPanning;
	QPointF m_panPosition;
	QPointF m_lastMousePosition;
};

}
