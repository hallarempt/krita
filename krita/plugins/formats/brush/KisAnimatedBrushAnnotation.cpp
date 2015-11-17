#include "KisAnimatedBrushAnnotation.h"

#include <QByteArray>
#include <QBuffer>

#include <kis_pipebrush_parasite.h>

KisAnimatedBrushAnnotation::KisAnimatedBrushAnnotation(const KisPipeBrushParasite &parasite)
    : KisAnnotation("ImagePipe Parasite",
                    i18n("Brush selection information for animated brushes"),
                    QByteArray())
{
    QBuffer buf(&m_annotation);
    buf.open(QBuffer::WriteOnly);
    parasite.saveToDevice(&buf);
    buf.close();
}
