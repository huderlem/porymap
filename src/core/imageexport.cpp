#include "imageexport.h"
#include "log.h"
#include <QFile>

// CRC code from: http://www.libpng.org/pub/png/spec/1.2/PNG-CRCAppendix.html

/* Table of CRCs of all 8-bit messages. */
unsigned long crc_table[256];

/* Flag: has the table been computed? Initially false. */
int crc_table_computed = 0;

/* Make the table for a fast CRC. */
void make_crc_table(void)
{
  unsigned long c;
  int n, k;

  for (n = 0; n < 256; n++) {
    c = (unsigned long) n;
    for (k = 0; k < 8; k++) {
      if (c & 1)
        c = 0xedb88320L ^ (c >> 1);
      else
        c = c >> 1;
    }
    crc_table[n] = c;
  }
  crc_table_computed = 1;
}

/* Update a running CRC with the bytes buf[0..len-1]--the CRC
   should be initialized to all 1's, and the transmitted value
   is the 1's complement of the final running CRC (see the
   crc() routine below)). */

unsigned long update_crc(unsigned long crc, QByteArray buf,
                         int len)
{
  unsigned long c = crc;
  int n;

  if (!crc_table_computed)
    make_crc_table();
  for (n = 0; n < len; n++) {
    c = crc_table[(c ^ static_cast<unsigned char>(buf[n])) & 0xff] ^ (c >> 8);
  }
  return c;
}

/* Return the CRC of the bytes buf[0..len-1]. */
unsigned long crc(QByteArray buf, int len)
{
  return update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
}

// Qt does not have the ability to export indexed PNG files with a
// bit depth of 4--it only supports 8. This can cause problems with
// some image editing programs because they will revert the bit depth,
// and re-importing into porymap (Qt), will cause the image to be
// interpreted as having too many colors. By properly exporting 16-palette
// images in porymap, we can effectively avoid that issue.
void exportIndexed4BPPPng(QImage image, QString filepath)
{
    // Verify that the image is not empty
    if (image.isNull()) {
        logError(QString("Failed to export %1: the image is null.").arg(filepath));
        return;
    }

    // Header
    QByteArray pngHeader;
    pngHeader.append(static_cast<char>(0x89));
    pngHeader.append("PNG");
    pngHeader.append(static_cast<char>(0x0D));
    pngHeader.append(static_cast<char>(0x0A));
    pngHeader.append(static_cast<char>(0x1A));
    pngHeader.append(static_cast<char>(0x0A));

    // IHDR Chunk
    QByteArray ihdr;
    ihdr.append("IHDR");
    int width = image.width();
    ihdr.append(static_cast<char>((width >> 24) & 0xFF));
    ihdr.append(static_cast<char>((width >> 16) & 0xFF));
    ihdr.append(static_cast<char>((width >>  8) & 0xFF));
    ihdr.append(static_cast<char>((width >>  0) & 0xFF));
    int height = image.height();
    ihdr.append(static_cast<char>((height >> 24) & 0xFF));
    ihdr.append(static_cast<char>((height >> 16) & 0xFF));
    ihdr.append(static_cast<char>((height >>  8) & 0xFF));
    ihdr.append(static_cast<char>((height >>  0) & 0xFF));
    ihdr.append(static_cast<char>(4)); // bit depth
    ihdr.append(static_cast<char>(3)); // indexed color type
    ihdr.append(static_cast<char>(0)); // compression method
    ihdr.append(static_cast<char>(0)); // filter method
    ihdr.append(static_cast<char>(0)); // interlace method
    unsigned long ihdrCRC = crc(ihdr, 17);
    ihdr.append(static_cast<char>((ihdrCRC >> 24) & 0xFF));
    ihdr.append(static_cast<char>((ihdrCRC >> 16) & 0xFF));
    ihdr.append(static_cast<char>((ihdrCRC >>  8) & 0xFF));
    ihdr.append(static_cast<char>((ihdrCRC >>  0) & 0xFF));

    // PLTE Chunk
    int numColors = image.colorCount();
    QByteArray plte;
    plte.append("PLTE");
    for (int i = 0; i < numColors; i++) {
        QRgb rgb = image.colorTable().at(i);
        plte.append(static_cast<char>(qRed(rgb)));
        plte.append(static_cast<char>(qGreen(rgb)));
        plte.append(static_cast<char>(qBlue(rgb)));
    }
    unsigned long plteCRC = crc(plte, numColors * 3 + 4);
    plte.append(static_cast<char>((plteCRC >> 24) & 0xFF));
    plte.append(static_cast<char>((plteCRC >> 16) & 0xFF));
    plte.append(static_cast<char>((plteCRC >>  8) & 0xFF));
    plte.append(static_cast<char>((plteCRC >>  0) & 0xFF));

    // IDAT Chunk
    QByteArray idat;
    idat.append("IDAT");
    unsigned long count = 0;
    char val = 0;

    QByteArray pixelData;
    for (int y = 0; y < image.height(); y++) {
        pixelData.append(static_cast<char>(0));
        for (int x = 0; x < image.width(); x++) {
            int colorId = image.pixelIndex(x, y);
            if (count % 2 == 0) {
                val = static_cast<char>(val & 0x0F) | static_cast<char>(colorId << 4);
            } else {
                val = static_cast<char>(val & 0xF0) | (static_cast<char>(colorId));
                pixelData.append(val);
            }
            count++;
        }
    }
    QByteArray compressedPixelData = qCompress(pixelData);
    // Qt's qCompress/qDecompress use a pointless 4-byte header, even though
    // they are using DEFLATE under the hood. If we strip the 4-byte header,
    // it's perfectly compatible with the PNG compression spec.
    compressedPixelData.remove(0, 4);
    idat.append(compressedPixelData);
    unsigned long idatCRC = crc(idat, compressedPixelData.length() + 4);
    idat.append(static_cast<char>((idatCRC >> 24) & 0xFF));
    idat.append(static_cast<char>((idatCRC >> 16) & 0xFF));
    idat.append(static_cast<char>((idatCRC >>  8) & 0xFF));
    idat.append(static_cast<char>((idatCRC >>  0) & 0xFF));

    // IEND Chunk
    QByteArray iend;
    iend.append("IEND");
    unsigned long iendCRC = crc(iend, 4);
    iend.append(static_cast<char>((iendCRC >> 24) & 0xFF));
    iend.append(static_cast<char>((iendCRC >> 16) & 0xFF));
    iend.append(static_cast<char>((iendCRC >>  8) & 0xFF));
    iend.append(static_cast<char>((iendCRC >>  0) & 0xFF));

    QByteArray data;
    data.append(pngHeader);
    data.append(static_cast<char>(((ihdr.length() - 8) >> 24) & 0xFF));
    data.append(static_cast<char>(((ihdr.length() - 8) >> 16) & 0xFF));
    data.append(static_cast<char>(((ihdr.length() - 8) >>  8) & 0xFF));
    data.append(static_cast<char>(((ihdr.length() - 8) >>  0) & 0xFF));
    data.append(ihdr);
    data.append(static_cast<char>(((plte.length() - 8) >> 24) & 0xFF));
    data.append(static_cast<char>(((plte.length() - 8) >> 16) & 0xFF));
    data.append(static_cast<char>(((plte.length() - 8) >>  8) & 0xFF));
    data.append(static_cast<char>(((plte.length() - 8) >>  0) & 0xFF));
    data.append(plte);
    data.append(static_cast<char>(((idat.length() - 8) >> 24) & 0xFF));
    data.append(static_cast<char>(((idat.length() - 8) >> 16) & 0xFF));
    data.append(static_cast<char>(((idat.length() - 8) >>  8) & 0xFF));
    data.append(static_cast<char>(((idat.length() - 8) >>  0) & 0xFF));
    data.append(idat);
    data.append(static_cast<char>(((iend.length() - 8) >> 24) & 0xFF));
    data.append(static_cast<char>(((iend.length() - 8) >> 16) & 0xFF));
    data.append(static_cast<char>(((iend.length() - 8) >>  8) & 0xFF));
    data.append(static_cast<char>(((iend.length() - 8) >>  0) & 0xFF));
    data.append(iend);

    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        logError(QString("Could not save '%1'. ").arg(filepath) + file.errorString());
        return;
    }

    file.write(data);
    file.close();
}
