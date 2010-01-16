#include "artisticcomment.h"
#include <QList>

static QList<QString> explode(QString str, size_t max)
{
    QList<QString> ret;
    QString curr;
    QString currWord;
    if(str[str.size() - 1] != '\n')
        str += '\n';
    for(size_t i = 0; i != str.size(); ++i)
    {
        if(str[i] == '\n')
        {
            if(currWord.size() + (curr.size() == 0 ? 0 : curr.size() + 1) > max)
            {
                ret.push_back(curr);
                curr = currWord;
            }
            else
            {
                ret.push_back(curr + (curr.size() == 0 ? "" : " ") + currWord);
                curr = "";
            }
            currWord = "";
        }
        else if(str[i] == ' ')
        {
            if(currWord.size() + (curr.size() == 0 ? 0 : curr.size() + 1) > max)
            {
                ret.push_back(curr);
                curr = currWord;
            }
            else
                curr += (curr.size() == 0 ? "" : " ") + currWord;
            currWord = "";
        }
        else
        {
            currWord += str[i];
        }
    }
    return ret;
}

ArtisticComment::ArtisticComment(
                    QString begin, QString end,
                    QString lineBegin, QString lineEnd,
                    QString textBegin, QString textEnd,
                    QChar lfill, QChar rfill,
                    size_t minfill, size_t realWidth,
                    bool truncate, type_t type)
    : begin(begin), end(end),
      lineBegin(lineBegin), lineEnd(lineEnd),
      textBegin(textBegin), textEnd(textEnd),
      lfill(lfill), rfill(rfill),
      minfill(minfill), realWidth(realWidth),
      truncate(truncate), type(type)
{
}

QString ArtisticComment::apply(const QString& text)
{
    size_t textWidth = realWidth - lineBegin.size() - lineEnd.size() - textBegin.size() - textEnd.size() - minfill;
    QList<QString> expl = explode(text, textWidth);
    QString ret = begin + "\n";
    switch(type)
    {
        case LeftNoFill:
        {
            for(size_t i = 0; i != expl.size(); ++i)
                ret += lineBegin + textBegin + expl[i] + textEnd + lineEnd + '\n';
            break;
        }
        case Left:
        {
            for(size_t i = 0; i != expl.size(); ++i)
            {
                QString line = lineBegin + textBegin + expl[i] + textEnd;
                size_t max = realWidth - lineEnd.size();
                while(line.size() < max)
                    line += rfill;
                line += lineEnd + '\n';
                ret += line;
            }
            break;
        }
        case Center:
        {
            for(size_t i = 0; i != expl.size(); ++i)
            {
                size_t numChars = realWidth - lineBegin.size() - lineEnd.size() - textBegin.size() - textEnd.size() - expl[i].size();
                size_t num = (numChars + !truncate) / 2;
                QString line = lineBegin;
                for(size_t i = 0; i != num; ++i)
                    line += lfill;
                line += textBegin + expl[i] + textEnd;
                num = numChars - num;
                for(size_t i = 0; i != num; ++i)
                    line += rfill;
                line += lineEnd + '\n';
                ret += line;
            }
            break;
        }
        case Right:
        {
            for(size_t i = 0; i != expl.size(); ++i)
            {
                size_t num = realWidth - lineBegin.size() - lineEnd.size() - textBegin.size() - textEnd.size() - expl[i].size();
                QString line = lineBegin;
                for(size_t i = 0; i != num; ++i)
                    line += lfill;
                line += textBegin + expl[i] + textEnd + lineEnd;
                ret += line;
            }
            break;
        }
    }
    return ret + end;
}
