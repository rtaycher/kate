#ifndef ARTISTICCOMMENT_H
#define ARTISTICCOMMENT_H

#include <QString>

struct ArtisticComment
{
    QString begin, end;
    QString lineBegin, lineEnd;
    QString textBegin, textEnd;
    QChar lfill, rfill;
    size_t minfill;
    size_t realWidth;
    bool truncate;
    enum type_t { LeftNoFill, Left, Center, Right } type;
    ArtisticComment() {}
    ArtisticComment(QString begin, QString end,
                    QString lineBegin, QString lineEnd,
                    QString textBegin, QString textEnd,
                    QChar lfill, QChar rfill,
                    size_t minfill, size_t realWidth,
                    bool truncate, type_t);
    QString apply(const QString& text);
};

#endif

