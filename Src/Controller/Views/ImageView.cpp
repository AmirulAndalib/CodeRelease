/**
 * @file Controller/Views/ImageView.cpp
 *
 * Implementation of class ImageView
 *
 * @author <a href="mailto:Thomas.Roefer@dfki.de">Thomas Röfer</a>
 * @author Colin Graf
 */

#include <QPainter>
#include <QApplication>
#include <QMouseEvent>
#include <QPinchGesture>
#include <QWidget>
#include <QSettings>
#include <QSignalMapper>
#include <QMenu>
#include <sstream>

#include "ImageView.h"
#include "Controller/RobotConsole.h"
#include "Controller/RoboCupCtrl.h"
#include "Controller/Visualization/PaintMethods.h"
#include "Controller/ImageViewAdapter.h"
#include "Representations/Infrastructure/Image.h"
#include "Tools/ColorModelConversions.h"
#include <QFileDialog>

ImageView::ImageView(const QString& fullName, RobotConsole& console, const std::string& background, const std::string& name, bool segmented, bool upperCam, float gain)
    : upperCam(upperCam), widget(0), fullName(fullName), icon(":/Icons/tag_green.png"), console(console), background(background), name(name), gain(gain),
      isActImage(strcmp(name.c_str(), "act") == 0)
{
}

SimRobot::Widget* ImageView::createWidget()
{
  widget = new ImageWidget(*this);
  return widget;
}

ImageWidget::ImageWidget(ImageView& imageView)
    : imageView(imageView), imageData(0), imageWidth(Image::maxResolutionWidth), imageHeight(Image::maxResolutionHeight), lastImageTimeStamp(0), lastDrawingsTimeStamp(0),
      dragStart(-1, -1), zoom(1.f), offset(0, 0), headControlMode(false)
{
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);
  grabGesture(Qt::PinchGesture);
  setAttribute(Qt::WA_AcceptTouchEvents);

  QSettings& settings = RoboCupCtrl::application->getLayoutSettings();
  settings.beginGroup(imageView.fullName);
  zoom = (float)settings.value("Zoom", 1.).toDouble();
  offset = settings.value("Offset", QPoint()).toPoint();
  settings.endGroup();
}

ImageWidget::~ImageWidget()
{
  //saveLayout();

  imageView.widget = 0;

  if (imageData)
    delete imageData;
}

void ImageWidget::paintEvent(QPaintEvent* event)
{
  painter.begin(this);
  paint(painter);
  painter.end();
}

void ImageWidget::paint(QPainter& painter)
{
  SYNC_WITH(imageView.console);

  const Image* image = 0;
  RobotConsole::Images& currentImages = imageView.console.camImages;
  RobotConsole::Images::const_iterator i = currentImages.find(imageView.background);
  if (i != currentImages.end())
  {
    image = i->second.get();
    imageWidth = image->width;
    imageHeight = image->height;
  }
  else if (!currentImages.empty())
  {
    imageWidth = currentImages.begin()->second->width;
    imageHeight = currentImages.begin()->second->height;
  }

  const QSize& size = painter.window().size();
  float xScale = float(size.width()) / float(imageWidth);
  float yScale = float(size.height()) / float(imageHeight);
  scale = xScale < yScale ? xScale : yScale;
  scale *= zoom;
  float imageXOffset = (float(size.width()) - float(imageWidth) * scale) * 0.5f + offset.x() * scale;
  float imageYOffset = (float(size.height()) - float(imageHeight) * scale) * 0.5f + offset.y() * scale;

  painter.setTransform(QTransform(scale, 0, 0, scale, imageXOffset, imageYOffset));

  if (image)
    paintImage(painter, *image);
  else
    lastImageTimeStamp = 0;

  paintDrawings(painter);
}

void ImageWidget::saveLayout()
{
  QSettings& settings = RoboCupCtrl::application->getLayoutSettings();
  settings.beginGroup(imageView.fullName);
  settings.setValue("Zoom", (double)zoom);
  settings.setValue("Offset", offset);
  settings.endGroup();
}

void ImageWidget::paintDrawings(QPainter& painter)
{
  const QTransform baseTrans = painter.transform();
  const std::list<std::string>& drawings = imageView.console.imageViews[imageView.name];
  for (const std::string& drawing : drawings)
  {
    auto& camDrawings = imageView.console.camImageDrawings;
    auto debugDrawing = camDrawings.find(drawing);
    if (debugDrawing != camDrawings.end())
    {
      PaintMethods::paintDebugDrawing(painter, debugDrawing->second, baseTrans);
      if (debugDrawing->second.timeStamp > lastDrawingsTimeStamp)
        lastDrawingsTimeStamp = debugDrawing->second.timeStamp;
    }
    auto& motinoDrawings = imageView.console.motionImageDrawings;
    debugDrawing = motinoDrawings.find(drawing);
    if (debugDrawing != motinoDrawings.end())
    {
      PaintMethods::paintDebugDrawing(painter, debugDrawing->second, baseTrans);
      if (debugDrawing->second.timeStamp > lastDrawingsTimeStamp)
        lastDrawingsTimeStamp = debugDrawing->second.timeStamp;
    }
  }
  painter.setTransform(baseTrans);
}

void ImageWidget::copyImage(const Image& srcImage)
{
  int width = srcImage.width;
  int height = srcImage.height;

  unsigned* p = (unsigned*)imageData->bits();
  int r, g, b;
  int yImage, uImage, vImage;
  for (int y = 0; y < height; ++y)
  {
    const Image::Pixel* cur = &srcImage[y][0];
    const Image::Pixel* end = cur + width;
    for (; cur < end; ++cur)
    {
      yImage = int(cur->y) << 14;
      uImage = int(cur->cr) - 128;
      vImage = int(cur->cb) - 128;

      r = (yImage + 22972 * uImage) >> 14;
      g = (yImage - 5662 * vImage - 11706 * uImage) >> 14;
      b = (yImage + 29016 * vImage) >> 14;

      *p++ = (r < 0 ? 0 : r > 255 ? 255 : r) << 16 | (g < 0 ? 0 : g > 255 ? 255 : g) << 8 | (b < 0 ? 0 : b > 255 ? 255 : b) | 0xff000000;
    }
  }
  if (imageView.gain != 1.f)
  {
    p = (unsigned*)imageData->bits();
    float gain = imageView.gain;
    for (unsigned* pEnd = p + width * height; p < pEnd; ++p)
    {
      r = (int)(gain * (float)((*p >> 16) & 0xff));
      g = (int)(gain * (float)((*p >> 8) & 0xff));
      b = (int)(gain * (float)((*p) & 0xff));

      *p++ = (r < 0 ? 0 : r > 255 ? 255 : r) << 16 | (g < 0 ? 0 : g > 255 ? 255 : g) << 8 | (b < 0 ? 0 : b > 255 ? 255 : b) | 0xff000000;
    }
  }
}

void ImageWidget::paintImage(QPainter& painter, const Image& srcImage)
{
  // make sure we have a buffer
  if (!imageData || imageWidth != imageData->width() || imageHeight != imageData->height())
  {
    if (imageData)
      delete imageData;
    imageData = new QImage(imageWidth, imageHeight, QImage::Format_RGB32);
  }

  if (srcImage.timeStamp != lastImageTimeStamp)
  {
    copyImage(srcImage);

    lastImageTimeStamp = srcImage.timeStamp;
  }

  painter.drawImage(QRectF(0, 0, imageWidth, imageHeight), *imageData);
}

bool ImageWidget::needsRepaint() const
{
  SYNC_WITH(imageView.console);
  Image* image = 0;
  RobotConsole::Images& currentImages = imageView.console.camImages;
  RobotConsole::Images::const_iterator j = currentImages.find(imageView.background);
  if (j != currentImages.end())
    image = j->second.get();

  if (!image)
  {
    const std::list<std::string>& drawings(imageView.console.imageViews[imageView.name]);
    for (std::list<std::string>::const_iterator i = drawings.begin(), end = drawings.end(); i != end; ++i)
    {
      const DebugDrawing& debugDrawing(imageView.console.camImageDrawings[*i]);
      if (debugDrawing.timeStamp > lastDrawingsTimeStamp)
        return true;
    }
    return lastImageTimeStamp != 0;
  }
  else
    return image->timeStamp != lastImageTimeStamp;
}

void ImageWidget::window2viewport(QPoint& point)
{
  const QSize& size(this->size());
  float xScale = float(size.width()) / float(imageWidth);
  float yScale = float(size.height()) / float(imageHeight);
  float scale = xScale < yScale ? xScale : yScale;
  scale *= zoom;
  float xOffset = (float(size.width()) - float(imageWidth) * scale) * 0.5f + offset.x() * scale;
  float yOffset = (float(size.height()) - float(imageHeight) * scale) * 0.5f + offset.y() * scale;
  point = QPoint(static_cast<int>((point.x() - xOffset) / scale), static_cast<int>((point.y() - yOffset) / scale));
}

void ImageWidget::mouseMoveEvent(QMouseEvent* event)
{
  QWidget::mouseMoveEvent(event);
  SYNC_WITH(imageView.console);
  QPoint pos(event->pos());

  if (dragStart != QPoint(-1, -1))
  {
    offset = dragStartOffset + (pos - dragStart) / scale;
    QWidget::update();
    return;
  }

  window2viewport(pos);

  const char* text = 0;
  const std::list<std::string>& drawings(imageView.console.imageViews[imageView.name]);
  for (std::list<std::string>::const_iterator i = drawings.begin(), end = drawings.end(); i != end; ++i)
  {
    text = imageView.console.camImageDrawings[*i].getTip(pos.rx(), pos.ry());

    if (text)
      break;
  }

  if (text)
    setToolTip(QString(text));
  else
  {
    Image* image = 0;
    RobotConsole::Images& currentImages = imageView.console.camImages;
    if (auto i = currentImages.find(imageView.background); i != currentImages.end())
      image = i->second.get();
    if (image && pos.rx() >= 0 && pos.ry() >= 0 && pos.rx() < image->width && pos.ry() < image->height)
    {
      Image::Pixel& pixel = (*image)[pos.ry()][pos.rx()];
      char color[128];

      static const int factor1 = 29016;
      static const int factor2 = 5662;
      static const int factor3 = 22972;
      static const int factor4 = 11706;

      int yImage = int(pixel.y) << 14;
      int uImage = int(pixel.cr) - 128;
      int vImage = int(pixel.cb) - 128;

      int r = (yImage + factor3 * uImage) >> 14;
      int g = (yImage - factor2 * vImage - factor4 * uImage) >> 14;
      int b = (yImage + factor1 * vImage) >> 14;

      unsigned char h, s, i;
      ColorModelConversions::fromYCbCrToHSI(pixel.y, pixel.cb, pixel.cr, h, s, i);
      sprintf(color, "x=%d, y=%d\ny=%d, cb=%d, cr=%d\nr=%d, g=%d, b=%d\nh=%d, s=%d, i=%d", pos.rx(), pos.ry(), pixel.y, pixel.cb, pixel.cr, r, g, b, h, s, i);
      setToolTip(QString(color));
    }
    else
      setToolTip(QString());
  }
}

void ImageWidget::mousePressEvent(QMouseEvent* event)
{
  QWidget::mousePressEvent(event);

  if (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton)
  {
    dragStart = event->pos();
    dragStartOffset = offset;
  }
}

void ImageWidget::mouseReleaseEvent(QMouseEvent* event)
{
  QWidget::mouseReleaseEvent(event);
  QPoint pos = QPoint(event->pos());
  if (dragStart != pos && dragStart != QPoint(-1, -1))
  {
    dragStart = QPoint(-1, -1);
    QWidget::update();
    return;
  }
  dragStart = QPoint(-1, -1);
  window2viewport(pos);
  Vector2i v = Vector2i(pos.x(), pos.y());
  if (event->modifiers() & Qt::ShiftModifier)
  {
    if (!headControlMode)
    {
      imageView.console.handleConsole("mr HeadMotionRequest ManualHeadMotionProvider");
      headControlMode = true;
    }
    std::stringstream command;
    command << "set parameters:ManualHeadMotionProvider xImg = " << v.x() << "; yImg = " << v.y() << "; camera = " << (imageView.upperCam ? "upper" : "lower") << ";";
    imageView.console.handleConsole(command.str());
  }
  {
    SYNC_WITH(imageView.console);
    if (!(event->modifiers() & Qt::ShiftModifier))
    {
      if (event->modifiers() & Qt::ControlModifier)
      {
        ImageViewAdapter::fireClick(imageView.name, v, imageView.upperCam, false);
      }
      else
        ImageViewAdapter::fireClick(imageView.name, v, imageView.upperCam, true);
    }
  }
}

void ImageWidget::keyPressEvent(QKeyEvent* event)
{
  switch (event->key())
  {
  case Qt::Key_PageUp:
  case Qt::Key_Plus:
    event->accept();
    if (zoom < 3.f)
      zoom += 0.1f;
    if (zoom > 3.f)
      zoom = 3.f;
    QWidget::update();
    break;
  case Qt::Key_PageDown:
  case Qt::Key_Minus:
    event->accept();
    if (zoom > 0.1f)
      zoom -= 0.1f;
    QWidget::update();
    break;
  case Qt::Key_Up:
    offset += QPoint(0, 20);
    QWidget::update();
    break;
  case Qt::Key_Down:
    offset += QPoint(0, -20);
    QWidget::update();
    break;
  case Qt::Key_Left:
    offset += QPoint(20, 0);
    QWidget::update();
    break;
  case Qt::Key_Right:
    offset += QPoint(-20, 0);
    QWidget::update();
    break;
  default:
    QWidget::keyPressEvent(event);
    break;
  }
}

bool ImageWidget::event(QEvent* event)
{
  if (event->type() == QEvent::Gesture)
  {
    QPinchGesture* pinch = static_cast<QPinchGesture*>(static_cast<QGestureEvent*>(event)->gesture(Qt::PinchGesture));
    if (pinch && (pinch->changeFlags() & QPinchGesture::ScaleFactorChanged))
    {
      QPoint before(static_cast<int>(pinch->centerPoint().x()), static_cast<int>(pinch->centerPoint().y()));
      window2viewport(before);
      scale /= zoom;
      zoom *= static_cast<float>(pinch->scaleFactor() / pinch->lastScaleFactor());
      if (zoom > 3.f)
        zoom = 3.f;
      else if (zoom < 0.1f)
        zoom = 0.1f;
      scale *= zoom;
      QPoint after(static_cast<int>(pinch->centerPoint().x()), static_cast<int>(pinch->centerPoint().y()));
      window2viewport(after);
      offset -= before - after;
      QWidget::update();
      return true;
    }
  }
  return QWidget::event(event);
}

void ImageWidget::wheelEvent(QWheelEvent* event)
{
  QWidget::wheelEvent(event);

  zoom += 0.1f * event->angleDelta().y() / 120.f;
  if (zoom > 3.f)
    zoom = 3.f;
  else if (zoom < 0.1f)
    zoom = 0.1f;
  QWidget::update();
}

void ImageWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
  zoom = 1.f;
  offset.setX(0);
  offset.setY(0);
  QWidget::update();
}

QMenu* ImageWidget::createUserMenu() const
{
  QMenu* menu = new QMenu(tr("&Image"));

  menu->addSeparator();

  QAction* saveImgAct = new QAction(tr("&Save Image"), menu);
  connect(saveImgAct, SIGNAL(triggered()), this, SLOT(saveImg()));
  menu->addAction(saveImgAct);

  return menu;
}

void ImageWidget::saveImg()
{
  QSettings& settings = RoboCupCtrl::application->getSettings();
  QString fileName = QFileDialog::getSaveFileName(this, tr("Save as PNG"), settings.value("ExportDirectory", "").toString(), tr("(*.png)"));
  if (fileName.isEmpty())
    return;
  settings.setValue("ExportDirectory", QFileInfo(fileName).dir().path());


  SYNC_WITH(imageView.console);

  const Image* image = 0;
  RobotConsole::Images& currentImages = imageView.console.camImages;
  RobotConsole::Images::const_iterator i = currentImages.find(imageView.background);
  if (i != currentImages.end())
  {
    image = i->second.get();
    imageWidth = image->width;
    imageHeight = image->height;
  }
  if (image)
  {
    QPixmap pixmap(image->width, image->height);
    QPainter painter(&pixmap);
    paintImage(painter, *image);
    paintDrawings(painter);
    pixmap.save(fileName, "PNG");
  }
}
