#include "MemoryAccessPainter.h"
#include "CoreController.h"

#include <mgba/core/core.h>
#include <mgba/core/inspector.h>

#include <QImage>
#include <QMouseEvent>
#include <QPainter>

using namespace QGBA;

const size_t BLOCK_WIDTH = 256;
const size_t BLOCK_HEIGHT = 256;
const size_t BLOCK_PER_LINE = 8;

const int GRADIENT_MIN = 32;
const int GRADIENT_MAX = 255;
const int GRADIENT_LENGTH = 150; // frames

const float BYTES_FADEIN_START = 32.0;
const float BYTES_FADEIN_END = 48.0;

const int BORDER_WIDTH_NORMAL = 1;
const int BORDER_WIDTH_EDGE = 10;

MemoryAccessPainter::MemoryAccessPainter(QWidget* parent)
	: QWidget(parent)
{
	m_regionIndex = 0;
	m_zoom = 1.0;

	m_isPanning = false;
	m_panPosition = QPointF(0.0, 0.0);
	m_lastMousePosition = QPointF(0.0, 0.0);
}

void MemoryAccessPainter::setController(std::shared_ptr<CoreController> controller) {
	m_controller = controller;
}

void MemoryAccessPainter::setRegionIndex(int index) {
	// TODO: bounds checking
	m_regionIndex = index;
	update();
}

static float clamp(float value, float min, float max) {
	if (value < min)
		return min;
	if (value > max)
		return max;
	return value;
}

static float linearInterpolation(float a, float b, float t) {
	return (a * t) + (b * (1.0 - t));
}

static int gradient(uint32_t eventTimestamp, uint32_t currentTimestamp) {
	if (eventTimestamp == 0) {
		return 0;
	}

	int delta = currentTimestamp - eventTimestamp;
	if (delta > GRADIENT_LENGTH) {
		return GRADIENT_MIN;
	} else if (delta < 0) {
		return GRADIENT_MAX;
	} else {
		float t = 1.0 - powf(1.0 - (float) delta / (float) GRADIENT_LENGTH, 3.0);
		return (float) GRADIENT_MIN * t + (float) GRADIENT_MAX * (1.0 - t);
		//return linearInterpolation(GRADIENT_MIN, GRADIENT_MAX, t);
	}
}

static int coordToMemoryOffset(int x, int y) {
	int blockDivX = x / BLOCK_WIDTH;
	int blockDivY = y / BLOCK_HEIGHT;
	int blockModX = x % BLOCK_WIDTH;
	int blockModY = y % BLOCK_HEIGHT;

	if ((blockDivX >= 0) && (blockDivY >= 0) && (blockDivX < BLOCK_PER_LINE)) {
		return (blockModX + blockModY * BLOCK_WIDTH) +
			(blockDivX + blockDivY * BLOCK_PER_LINE) * (BLOCK_WIDTH * BLOCK_HEIGHT);
	} else {
		return -1;
	}
}

void MemoryAccessPainter::paintEvent(QPaintEvent*) {
	//CoreController::Interrupter interrupter(m_controller);
	mCore* core = m_controller->thread()->core;
	mInspectorDevice* inspector = core->inspectorDevice(core);

	const mInspectorRegion* region = mInspectorRegionListGetConstPointer(&inspector->regionList, m_regionIndex);
	size_t regionDataSize = 0;
	const uint8_t* regionData = (const uint8_t*) core->getMemoryBlock(core, region->blockId, &regionDataSize);

	QImage image(width(), height(), QImage::Format_RGB32);
	for (size_t y = 0; y < height(); ++y) {
		for (size_t x = 0; x < width(); ++x) {
			int rx = floor((float)(x) / m_zoom + m_panPosition.x());
			int ry = floor((float)(y) / m_zoom + m_panPosition.y());
			int offset = coordToMemoryOffset(rx, ry);

			if ((offset >= 0) && (offset < regionDataSize) && (offset < region->size)) {
				uint32_t now = mInspectorDeviceCurrentTimestamp(inspector);
				uint red = 0;
				uint green = 0;
				uint blue = 0;

				uint memoryValue = regionData[offset];
				red += memoryValue / 4;
				green += memoryValue / 4;
				blue += memoryValue / 4;

				red += gradient(region->timestampWrite[offset], now);
				green += gradient(region->timestampRead[offset], now);
				blue += gradient(region->timestampExecute[offset], now);

				red = (red > 255) ? 255 : red;
				green = (green > 255) ? 255 : green;
				blue = (blue > 255) ? 255 : blue;

				image.setPixel(x, y, qRgb(red, green, blue));

			} else {
				image.setPixel(x, y, qRgb(32, 32, 32));
			}
		}
	}

	if (m_zoom >= BYTES_FADEIN_START) {
		QPainter painter(&image);
		painter.scale(m_zoom, m_zoom);
		painter.translate(-m_panPosition);

		painter.setOpacity(clamp((m_zoom - BYTES_FADEIN_START) / (BYTES_FADEIN_END - BYTES_FADEIN_START), 0.0, 1.0));

		QPen pen;
		QFont font = painter.font();

		pen.setColor(qRgb(255.0, 255.0, 255.0));
		painter.setPen(pen);

		int xStart = floor(m_panPosition.x());
		int yStart = floor(m_panPosition.y());
		int xEnd = floor(m_panPosition.x() + (float) width() / m_zoom);
		int yEnd = floor(m_panPosition.y() + (float) height() / m_zoom);

		for (int y = yStart; y <= yEnd; ++y) {
			for (int x = xStart; x <= xEnd; ++x) {
				int offset = coordToMemoryOffset(x, y);
				if ((offset >= 0) && (offset < regionDataSize) && (offset < region->size)) {
					uint8_t memoryValue = regionData[offset];

					painter.save();
					painter.translate(QPointF(x, y));
					painter.scale(1 / 256.0, 1 / 256.0);

					// Left border
					pen.setWidth(((x % BLOCK_WIDTH) == 0) ? BORDER_WIDTH_EDGE : BORDER_WIDTH_NORMAL);
					painter.setPen(pen);
					painter.drawLine(0, 0, 0, 256);

					// Right border
					pen.setWidth(((x % BLOCK_WIDTH) == (BLOCK_WIDTH - 1)) ? BORDER_WIDTH_EDGE : BORDER_WIDTH_NORMAL);
					painter.setPen(pen);
					painter.drawLine(256, 0, 256, 256);

					// Top border
					pen.setWidth(((y % BLOCK_HEIGHT) == 0) ? BORDER_WIDTH_EDGE : BORDER_WIDTH_NORMAL);
					painter.setPen(pen);
					painter.drawLine(0, 0, 256, 0);

					// Bottom border
					pen.setWidth(((y % BLOCK_HEIGHT) == (BLOCK_HEIGHT - 1)) ? BORDER_WIDTH_EDGE : BORDER_WIDTH_NORMAL);
					painter.setPen(pen);
					painter.drawLine(0, 256, 256, 256);

					font.setPixelSize(128.0);
					painter.setFont(font);
					painter.drawText(QRect(0.0, 64.0, 256.0, 128.0), Qt::AlignCenter, QString("%1").arg(memoryValue, 2, 16, QChar('0')).toUpper());

					font.setPixelSize(32.0);
					painter.setFont(font);
					painter.drawText(QRect(0.0, 0.0, 256.0, 64.0), Qt::AlignCenter, QString("%1").arg(region->start + offset, 8, 16, QChar('0')).toUpper());
					painter.drawText(QRect(0.0, 192.0, 256.0, 64.0), Qt::AlignCenter, QString("%1").arg(memoryValue, 8, 2, QChar('0')).insert(4, QChar(' ')));

					painter.restore();
				}
			}
		}
	}

	QPainter painter(this);
	painter.drawImage(QPoint(0, 0), image);
}

void MemoryAccessPainter::resizeEvent(QResizeEvent*) {
}

void MemoryAccessPainter::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		event->accept();
		m_isPanning = true;
		m_lastMousePosition = event->localPos();
	}
}

void MemoryAccessPainter::mouseMoveEvent(QMouseEvent* event) {
	if (m_isPanning) {
		event->accept();
		m_panPosition -= (event->localPos() - m_lastMousePosition) / m_zoom;
		m_lastMousePosition = event->localPos();
		update();
	}
}

void MemoryAccessPainter::mouseReleaseEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		event->accept();
		m_isPanning = false;
		m_lastMousePosition = QPointF(0.0, 0.0);
	}
}

void MemoryAccessPainter::mouseDoubleClickEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		event->accept();
		m_zoom = 1.0;
		m_panPosition = QPointF(0.0, 0.0);
		update();
	}
}

void MemoryAccessPainter::wheelEvent(QWheelEvent* event) {
	float delta = event->angleDelta().y();
	if (delta != 0) {
		event->accept();
		float newZoom = m_zoom * ((delta < 0) ? 0.9 : 1.0 / 0.9);

		float factorX = (float)(event->position().x()) / (float)(width());
		float factorY = (float)(event->position().y()) / (float)(height());

		m_panPosition += QPointF(
			((float)(width()) / m_zoom) * factorX - ((float)(width()) / newZoom) * factorX,
			((float)(height()) / m_zoom) * factorY - ((float)(height()) / newZoom) * factorY
		);
		m_zoom = newZoom;
		update();
	}
}
