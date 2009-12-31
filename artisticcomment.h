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
    enum { LeftNoFill, Left, Center, Right } type;
    QString apply(const QString& text);
};
