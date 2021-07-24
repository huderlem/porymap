/****************************************************************************
** Copyright (c) 2013 Debao Zhang <hello@debao.me>
** All right reserved.
**
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
** LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
** WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
****************************************************************************/
#include "qgifimage.h"
#include "qgifimage_p.h"
#include <QFile>
#include <QImage>
#include <QDebug>
#include <QScopedPointer>

namespace
{
int writeToIODevice(GifFileType *gifFile, const GifByteType *data, int maxSize)
{
    return static_cast<QIODevice *>(gifFile->UserData)->write(reinterpret_cast<const char *>(data), maxSize);
}

int readFromIODevice(GifFileType *gifFile, GifByteType *data, int maxSize)
{
    return static_cast<QIODevice *>(gifFile->UserData)->read(reinterpret_cast<char *>(data), maxSize);
}
}

QGifImagePrivate::QGifImagePrivate(QGifImage *p)
    : loopCount(0), defaultDelayTime(1000), q_ptr(p)
{

}

QGifImagePrivate::~QGifImagePrivate()
{

}

QVector<QRgb> QGifImagePrivate::colorTableFromColorMapObject(ColorMapObject *colorMap, int transColorIndex) const
{
    QVector<QRgb> colorTable;
    if (colorMap) {
        for (int idx=0; idx<colorMap->ColorCount; ++idx) {
            GifColorType gifColor = colorMap->Colors[idx];
            QRgb color = gifColor.Blue | (gifColor.Green << 8) | (gifColor.Red << 16);
            // For non-transparent color, set the alpha to opaque.
            if (idx != transColorIndex)
                color |= 0xff << 24;
            colorTable.append(color);
        }
    }
    return colorTable;
}

ColorMapObject *QGifImagePrivate::colorTableToColorMapObject(QVector<QRgb> colorTable) const
{
    if (colorTable.isEmpty())
        return 0;

    ColorMapObject *cmap = (ColorMapObject *)malloc(sizeof(ColorMapObject));
    // num of colors must be a power of 2
    int numColors = 1 << GifBitSize(colorTable.size());
    cmap->ColorCount = numColors;
    //Maybe a bug of giflib, BitsPerPixel is used as size of the color table size.
    cmap->BitsPerPixel = GifBitSize(colorTable.size()); //Todo!
    cmap->SortFlag = false;

    GifColorType* colorValues = (GifColorType*)calloc(numColors, sizeof(GifColorType));
    for(int idx=0; idx < colorTable.size(); ++idx) {
        colorValues[idx].Red = qRed(colorTable[idx]);
        colorValues[idx].Green = qGreen(colorTable[idx]);
        colorValues[idx].Blue = qBlue(colorTable[idx]);
    }

    cmap->Colors = colorValues;

    return cmap;
}

QSize QGifImagePrivate::getCanvasSize() const
{
    //If canvasSize has been set by user.
    if (canvasSize.isValid())
        return canvasSize;

    //Calc the right canvasSize from the frame size.
    int width = -1;
    int height = -1;
    foreach (QGifFrameInfoData info, frameInfos) {
        int w = info.image.width() + info.offset.x();
        int h = info.image.height() + info.offset.y();
        if (w > width) width = w;
        if (h > height) height = h;
    }
    return QSize(width, height);
}

int QGifImagePrivate::getFrameTransparentColorIndex(const QGifFrameInfoData &frameInfo) const
{
    int index = -1;

    QColor transColor = frameInfo.transparentColor.isValid() ? frameInfo.transparentColor : defaultTransparentColor;

    if (transColor.isValid()) {
        if (!frameInfo.image.colorTable().isEmpty())
            index = frameInfo.image.colorTable().indexOf(transColor.rgb());
        else if (!globalColorTable.isEmpty())
            index = globalColorTable.indexOf(transColor.rgb());
    }

    return index;
}

bool QGifImagePrivate::load(QIODevice *device)
{
    static int interlacedOffset[] = { 0, 4, 2, 1 }; /* The way Interlaced image should. */
    static int interlacedJumps[] = { 8, 8, 4, 2 };    /* be read - offsets and jumps... */

    int error;
    GifFileType *gifFile = DGifOpen(device, readFromIODevice, &error);
    if (!gifFile) {
        qWarning(GifErrorString(error));
        return false;
    }

    if (DGifSlurp(gifFile) == GIF_ERROR)
        return false;

    canvasSize.setWidth(gifFile->SWidth);
    canvasSize.setHeight(gifFile->SHeight);
    if (gifFile->SColorMap) {
        globalColorTable = colorTableFromColorMapObject(gifFile->SColorMap);
        if (gifFile->SBackGroundColor < globalColorTable.size())
            bgColor = QColor(globalColorTable[gifFile->SBackGroundColor]);
    }

    for (int idx=0; idx<gifFile->ImageCount; ++idx) {
        SavedImage gifImage = gifFile->SavedImages[idx];
        int top = gifImage.ImageDesc.Top;
        int left = gifImage.ImageDesc.Left;
        int width = gifImage.ImageDesc.Width;
        int height = gifImage.ImageDesc.Height;

        QGifFrameInfoData frameInfo;
        GraphicsControlBlock gcb;
        DGifSavedExtensionToGCB(gifFile, idx, &gcb);
        int transColorIndex = gcb.TransparentColor;

        QVector<QRgb> colorTable;
        if (gifImage.ImageDesc.ColorMap)
            colorTable = colorTableFromColorMapObject(gifImage.ImageDesc.ColorMap, transColorIndex);
        else if (transColorIndex != -1)
            colorTable = colorTableFromColorMapObject(gifFile->SColorMap, transColorIndex);
        else
            colorTable = globalColorTable;

        if (transColorIndex != -1)
            frameInfo.transparentColor = colorTable[transColorIndex];
        frameInfo.delayTime = gcb.DelayTime * 10; //convert to milliseconds
        frameInfo.interlace = gifImage.ImageDesc.Interlace;
        frameInfo.offset = QPoint(left, top);

        QImage image(width, height, QImage::Format_Indexed8);
        image.setOffset(QPoint(left, top)); //Maybe useful for some users.
        image.setColorTable(colorTable);
        if (transColorIndex != -1)
            image.fill(transColorIndex);
        else if (!globalColorTable.isEmpty())
            image.fill(gifFile->SBackGroundColor); //!ToDo

        if (gifImage.ImageDesc.Interlace) {
            int line = 0;
            for (int i = 0; i < 4; i++) {
                for (int row = interlacedOffset[i]; row <height; row += interlacedJumps[i]) {
                    memcpy(image.scanLine(row), gifImage.RasterBits+line*width, width);
                    line ++;
                }
            }
        } else {
            for (int row = 0; row < height; row++) {
                memcpy(image.scanLine(row), gifImage.RasterBits+row*width, width);
            }
        }

        //Extract other data for the image.
        if (idx == 0) {
            if (gifImage.ExtensionBlockCount > 2) {
                ExtensionBlock *extBlock = gifImage.ExtensionBlocks;
                if (extBlock->Function == APPLICATION_EXT_FUNC_CODE && extBlock->ByteCount == 8) {
                    if (QByteArray((char *)extBlock->Bytes) == QByteArray("NETSCAPE2.0")) {
                        ExtensionBlock *block = gifImage.ExtensionBlocks+1;
                        if (block->ByteCount == 3) {
                            loopCount = uchar(block->Bytes[1])
                                    + uchar((block->Bytes[2])<<8);
                        }
                    }
                }
            }
        }

        frameInfo.image = image;
        frameInfos.append(frameInfo);
    }

    DGifCloseFile(gifFile);
    return true;
}

bool QGifImagePrivate::save(QIODevice *device) const
{
    int error;
    GifFileType *gifFile = EGifOpen(device, writeToIODevice, &error);
    if (!gifFile) {
        qWarning(GifErrorString(error));
        return false;
    }

    QSize _canvasSize = getCanvasSize();
    gifFile->SWidth = _canvasSize.width();
    gifFile->SHeight = _canvasSize.height();
    gifFile->SColorResolution = 8;
    if (!globalColorTable.isEmpty()) {
        gifFile->SColorMap = colorTableToColorMapObject(globalColorTable);
        int idx = globalColorTable.indexOf(bgColor.rgba());
        gifFile->SBackGroundColor = idx == -1 ? 0 : idx;
    }

    gifFile->ImageCount = frameInfos.size();
    gifFile->SavedImages = (SavedImage *)calloc(frameInfos.size(), sizeof(SavedImage));
    for (int idx=0; idx < frameInfos.size(); ++idx) {
        const QGifFrameInfoData frameInfo = frameInfos.at(idx);
        QImage image = frameInfo.image;
        if (image.format() != QImage::Format_Indexed8) {
            if (!globalColorTable.isEmpty())
                image = image.convertToFormat(QImage::Format_Indexed8, globalColorTable);
            else
                image = image.convertToFormat(QImage::Format_Indexed8);
        }

        SavedImage *gifImage = gifFile->SavedImages + idx;

        gifImage->ImageDesc.Left = frameInfo.offset.x();
        gifImage->ImageDesc.Top = frameInfo.offset.y();
        gifImage->ImageDesc.Width = image.width();
        gifImage->ImageDesc.Height = image.height();
        gifImage->ImageDesc.Interlace = frameInfo.interlace;

        if (!image.colorTable().isEmpty() && (image.colorTable() != globalColorTable))
            gifImage->ImageDesc.ColorMap = colorTableToColorMapObject(image.colorTable());
        else
            gifImage->ImageDesc.ColorMap = 0;

        GifByteType *data = (GifByteType *)malloc(image.width() * image.height() * sizeof(GifByteType));
        for (int row=0; row<image.height(); ++row) {
            memcpy(data+row*image.width(), image.scanLine(row), image.width());
        }
        gifImage->RasterBits = data;

        if (idx == 0) {
            uchar data8[12] = "NETSCAPE2.0";
            GifAddExtensionBlock(&gifImage->ExtensionBlockCount, &gifImage->ExtensionBlocks
                                 , APPLICATION_EXT_FUNC_CODE, 11, data8);
            uchar data[3];
            data[0] = 0x01;
            data[1] = loopCount & 0xFF;
            data[2] = (loopCount >> 8) & 0xFF;
            GifAddExtensionBlock(&gifImage->ExtensionBlockCount, &gifImage->ExtensionBlocks
                                 , CONTINUE_EXT_FUNC_CODE, 3, data);
        }

        GraphicsControlBlock gcbBlock;
        gcbBlock.DisposalMode = 0;
        gcbBlock.UserInputFlag = false;
        gcbBlock.TransparentColor = getFrameTransparentColorIndex(frameInfo);

        if (frameInfo.delayTime != -1)
            gcbBlock.DelayTime = frameInfo.delayTime / 10; //convert from milliseconds
        else
            gcbBlock.DelayTime = defaultDelayTime / 10;

        EGifGCBToSavedExtension(&gcbBlock, gifFile, idx);
    }

    EGifSpew(gifFile);
    //EGifCloseFile(gifFile);

    return true;
}


/*!
    \class QGifImage
    \inmodule QtGifImage
    \brief Class used to read/wirte .gif files.
*/

/*!
    Constructs a gif image
*/
QGifImage::QGifImage()
    :d_ptr(new QGifImagePrivate(this))
{

}

/*!
    Constructs a gif image and tries to load the image from the
    file with the given \a fileName
*/
QGifImage::QGifImage(const QString &fileName)
    :d_ptr(new QGifImagePrivate(this))
{
    load(fileName);
}

/*!
    Constructs a gif image with the given \a size
*/
QGifImage::QGifImage(const QSize &size)
    :d_ptr(new QGifImagePrivate(this))
{
    d_ptr->canvasSize = size;
}

/*!
    Destroys the gif image and cleans up.
*/
QGifImage::~QGifImage()
{
    delete d_ptr;
}

/*!
    Return global color table.
 */
QVector<QRgb> QGifImage::globalColorTable() const
{
    Q_D(const QGifImage);
    return d->globalColorTable;
}

/*!
    Return background color of the gif canvas. It only makes sense when
    global color table is not empty.
 */
QColor QGifImage::backgroundColor() const
{
    Q_D(const QGifImage);
    return d->bgColor;
}

/*!
    Set the global color table \a colors and background color \a bgColor.
    \a bgColor must be one the color in \a colors.

    Unlike other image formats that support alpha (e.g. png), GIF does not
    support semi-transparent pixels. So the alpha channel of the color table
    will be ignored.
*/
void QGifImage::setGlobalColorTable(const QVector<QRgb> &colors, const QColor &bgColor)
{
    Q_D(QGifImage);
    d->globalColorTable = colors;
    d->bgColor = bgColor;
}

/*!
    Return the default delay in milliseconds. The default value is 1000 ms.

    The time delay can be different for every frame.
*/
int QGifImage::defaultDelay() const
{
    Q_D(const QGifImage);
    return d->defaultDelayTime;
}

/*!
    Set the default \a delay in milliseconds.
*/
void QGifImage::setDefaultDelay(int delay)
{
    Q_D(QGifImage);
    d->defaultDelayTime = delay;
}

/*!
    Return the default transparent color.

    The transparent color can be different for every frame.
*/
QColor QGifImage::defaultTransparentColor() const
{
    Q_D(const QGifImage);
    return d->defaultTransparentColor;
}

/*!
    Set the default transparent \a color.

    Unlike other image formats that support alpha (e.g. png), GIF does
    not support semi-transparent pixels. The way to achieve transparency
    is to set a color that will be transparent when rendering the GIF.
    So, if you set the transparent color to black, the black pixels in
    the gif file will be transparent.
*/
void QGifImage::setDefaultTransparentColor(const QColor &color)
{
    Q_D(QGifImage);
    d->defaultTransparentColor = color;
}

/*!
    Return the loop count.
*/
int QGifImage::loopCount() const
{
    Q_D(const QGifImage);
    return d->loopCount;
}

/*!
    Set the loop count. The default value of \a loop is 0, which means loop forever.
*/
void QGifImage::setLoopCount(int loop)
{
    Q_D(QGifImage);
    d->loopCount = loop;
}

/*!
    Insert the QImage object \a frame at position \a index with \a delay.

    As gif file only support indexed image, so all the \a frame will be converted
    to the QImage::Format_Indexed8 format. Global color table will be used in the
    convertion if it has been set.

    QImage::offset() will be used when insert the QImage to the gif canvas.
*/
void QGifImage::insertFrame(int index, const QImage &frame, int delay)
{
    Q_D(QGifImage);

    QGifFrameInfoData data;
    data.image = frame;
    data.delayTime = delay;
    data.offset = frame.offset();

    d->frameInfos.insert(index, data);
}

/*!
    \overload

    Insert the QImage object \a frame at position \a index with the given \a offset and
    \a delay.

    As gif file only support indexed image, so all the \a frame will be converted
    to the QImage::Format_Indexed8 format. Global color table will be used in the
    convertion if it has been set.
*/
void QGifImage::insertFrame(int index, const QImage &frame, const QPoint &offset, int delay)
{
    Q_D(QGifImage);
    QGifFrameInfoData data;
    data.image = frame;
    data.delayTime = delay;
    data.offset = offset;

    d->frameInfos.insert(index, data);
}

/*!
    Append the QImage object \a frame with \a delay.

    As gif file only support indexed image, so all the \a frame will be converted
    to the QImage::Format_Indexed8 format. Global color table will be used in the
    convertion if it has been set.

    QImage::offset() will be used when insert the QImage to the gif canvas.
*/
void QGifImage::addFrame(const QImage &frame, int delay)
{
    Q_D(QGifImage);

    QGifFrameInfoData data;
    data.image = frame;
    data.delayTime = delay;
    data.offset = frame.offset();

    d->frameInfos.append(data);
}

/*!
    \overload
    Append the QImage object \a frame with the given \a offset and \a delay.
 */
void QGifImage::addFrame(const QImage &frame, const QPoint& offset, int delay)
{
    Q_D(QGifImage);

    QGifFrameInfoData data;
    data.image = frame;
    data.delayTime = delay;
    data.offset = offset;

    d->frameInfos.append(data);
}

/*!
    Return frame count contained in the gif file.
 */
int QGifImage::frameCount() const
{
    Q_D(const QGifImage);
    return d->frameInfos.count();
}

/*!
    Return the image at \a index.
 */
QImage QGifImage::frame(int index) const
{
    Q_D(const QGifImage);
    if (index < 0 || index >= d->frameInfos.size())
        return QImage();

    return d->frameInfos[index].image;
}

/*!
     Return the offset value of the frame at \a index
 */
QPoint QGifImage::frameOffset(int index) const
{
    Q_D(const QGifImage);
    if (index < 0 || index >= d->frameInfos.size())
        return QPoint();

    return d->frameInfos[index].offset;
}

/*!
     Set the \a offset value for the frame at \a index
 */
void QGifImage::setFrameOffset(int index, const QPoint &offset)
{
    Q_D(QGifImage);
    if (index < 0 || index >= d->frameInfos.size())
        return;
    d->frameInfos[index].offset = offset;
}

/*!
     Return the delay value of the frame at \a index
 */
int QGifImage::frameDelay(int index) const
{
    Q_D(const QGifImage);
    if (index < 0 || index >= d->frameInfos.size())
        return -1;

    return d->frameInfos[index].delayTime;
}

/*!
     Set the \a delay value for the frame at \a index
 */
void QGifImage::setFrameDelay(int index, int delay)
{
    Q_D(QGifImage);
    if (index < 0 || index >= d->frameInfos.size())
        return;
    d->frameInfos[index].delayTime = delay;
}

/*!
     Return the transparent color of the frame at \a index
 */
QColor QGifImage::frameTransparentColor(int index) const
{
    Q_D(const QGifImage);
    if (index < 0 || index >= d->frameInfos.size())
        return QColor();

    return d->frameInfos[index].transparentColor;
}

/*!
    Sets the transparent \a color of the frame \a index. Unlike other image formats
    that support alpha (e.g. PNG), GIF does not support semi-transparent pixels.
    The way to achieve transparency is to set a color that will be transparent
    when rendering the GIF. So, if you set the transparent color to black, the
    black pixels in your gif file will be transparent.
*/
void QGifImage::setFrameTransparentColor(int index, const QColor &color)
{
    Q_D(QGifImage);
    if (index < 0 || index >= d->frameInfos.size())
        return;
    d->frameInfos[index].transparentColor = color;
}

/*!
    Saves the gif image to the file with the given \a fileName.
    Returns \c true if the image was successfully saved; otherwise
    returns \c false.
*/
bool QGifImage::save(const QString &fileName) const
{
    Q_D(const QGifImage);
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly))
        return d->save(&file);

    return false;
}

/*!
    \overload

    This function writes a QImage to the given \a device.
*/
bool QGifImage::save(QIODevice *device) const
{
    Q_D(const QGifImage);
    if (device->openMode() | QIODevice::WriteOnly)
        return d->save(device);

    return false;
}

/*!
    Loads an gif image from the file with the given \a fileName. Returns \c true if
    the image was successfully loaded; otherwise invalidates the image
    and returns \c false.
*/
bool QGifImage::load(const QString &fileName)
{
    Q_D(QGifImage);
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly))
        return d->load(&file);

    return false;
}

/*!
    \overload

    This function reads a gif image from the given \a device. This can,
    for example, be used to load an image directly into a QByteArray.
*/
bool QGifImage::load(QIODevice *device)
{
    Q_D(QGifImage);
    if (device->openMode() | QIODevice::ReadOnly)
        return d->load(device);

    return false;
}
