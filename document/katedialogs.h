#ifndef KATEDIALOGS_H
#define KATEDIALOGS_H
#include <kdialog.h>
#include <kdialogbase.h>
#include "katesyntaxdocument.h"
#include  "katehighlight.h"
#include <klistview.h>
#include <kcolorbutton.h>

class QWidgetStack;
class QVBox;
class  KListView;
class QListViewItem;
struct syntaxContextData;
class QCheckBox;
//class ItemFont;
#define HlEUnknown 0
#define HlEContext 1
#define HlEItem 2
 //--------


class StyleChanger : public QWidget {
    Q_OBJECT
  public:
    StyleChanger(QWidget *parent );
    void setRef(ItemStyle *);
    void setEnabled(bool);
  protected slots:
    void changed();
  protected:
    ItemStyle *style;
    KColorButton *col;
    KColorButton *selCol;
    QCheckBox *bold;
    QCheckBox *italic;
};

class FontChanger : public QWidget {
    Q_OBJECT
  public:
    FontChanger(QWidget *parent );
    void setRef(ItemFont *);
  protected slots:
    void familyChanged(const QString &);
    void sizeChanged(int);
    void charsetChanged(const QString &);
  protected:
    void displayCharsets();
    ItemFont *font;
    QComboBox *familyCombo;
    QComboBox *sizeCombo;
    QComboBox *charsetCombo;
};

//--------

class HighlightDialog : public KDialogBase
{
    Q_OBJECT
  public:
    HighlightDialog(HlManager *, ItemStyleList *, ItemFont *, HlDataList *, int hlNumber,
                    QWidget *parent=0, const char *name=0, bool modal=true);

  protected slots:
    void defaultChanged(int);

    void hlChanged(int);
    void itemChanged(int);
    void changed();
    void hlEdit();
  protected:
    StyleChanger *defaultStyleChanger;
    ItemStyleList *defaultItemStyleList;
    FontChanger *defaultFontChanger;

    void writeback();
    virtual void done(int r);
    QComboBox *itemCombo, *hlCombo;
    QLineEdit *wildcards;
    QLineEdit *mimetypes;
    QCheckBox *styleDefault;
    QCheckBox *fontDefault;
    StyleChanger *styleChanger;
    FontChanger *fontChanger;

    HlDataList *hlDataList;
    HlData *hlData;
    ItemData *itemData;
};



class HlEditDialog : public KDialogBase
{
    Q_OBJECT
  public:
    HlEditDialog(HlManager *,QWidget *parent=0, const char *name=0, bool modal=true, HlData *data=0);
  private:
    class QWidgetStack *stack;
    class QVBox *contextOptions, *itemOptions;
    class KListView *contextList;
    void initContextOptions(class QVBox *co);
    void initItemOptions(class QVBox *co);
    void loadFromDocument(HlData *hl);
    QListViewItem *addContextItem(KListView *cL,QListViewItem *_parent,QListViewItem *prev,struct syntaxContextData *data);
};



#endif
