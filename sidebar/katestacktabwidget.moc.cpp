/****************************************************************************
** KateStackTabWidget meta object code from reading C++ file 'katestacktabwidget.h'
**
** Created: Sun Apr 1 17:24:34 2001
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 9
#elif Q_MOC_OUTPUT_REVISION != 9
#error "Moc format conflict - please regenerate all moc files"
#endif

#include "katestacktabwidget.h"
#include <qmetaobject.h>
#include <qapplication.h>



const char *KateStackTabWidget::className() const
{
    return "KateStackTabWidget";
}

QMetaObject *KateStackTabWidget::metaObj = 0;

void KateStackTabWidget::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QWidget::className(), "QWidget") != 0 )
	badSuperclassWarning("KateStackTabWidget","QWidget");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString KateStackTabWidget::tr(const char* s)
{
    return qApp->translate( "KateStackTabWidget", s, 0 );
}

QString KateStackTabWidget::tr(const char* s, const char * c)
{
    return qApp->translate( "KateStackTabWidget", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* KateStackTabWidget::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QWidget::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    typedef void (KateStackTabWidget::*m1_t0)(int);
    typedef void (QObject::*om1_t0)(int);
    typedef void (KateStackTabWidget::*m1_t1)(int);
    typedef void (QObject::*om1_t1)(int);
    m1_t0 v1_0 = &KateStackTabWidget::selected_tabbar;
    om1_t0 ov1_0 = (om1_t0)v1_0;
    m1_t1 v1_1 = &KateStackTabWidget::selected_button;
    om1_t1 ov1_1 = (om1_t1)v1_1;
    QMetaData *slot_tbl = QMetaObject::new_metadata(2);
    QMetaData::Access *slot_tbl_access = QMetaObject::new_metaaccess(2);
    slot_tbl[0].name = "selected_tabbar(int)";
    slot_tbl[0].ptr = (QMember)ov1_0;
    slot_tbl_access[0] = QMetaData::Private;
    slot_tbl[1].name = "selected_button(int)";
    slot_tbl[1].ptr = (QMember)ov1_1;
    slot_tbl_access[1] = QMetaData::Private;
    metaObj = QMetaObject::new_metaobject(
	"KateStackTabWidget", "QWidget",
	slot_tbl, 2,
	0, 0,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    metaObj->set_slot_access( slot_tbl_access );
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    return metaObj;
}


const char *KateStackTabWidgetButton::className() const
{
    return "KateStackTabWidgetButton";
}

QMetaObject *KateStackTabWidgetButton::metaObj = 0;

void KateStackTabWidgetButton::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QPushButton::className(), "QPushButton") != 0 )
	badSuperclassWarning("KateStackTabWidgetButton","QPushButton");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString KateStackTabWidgetButton::tr(const char* s)
{
    return qApp->translate( "KateStackTabWidgetButton", s, 0 );
}

QString KateStackTabWidgetButton::tr(const char* s, const char * c)
{
    return qApp->translate( "KateStackTabWidgetButton", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* KateStackTabWidgetButton::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QPushButton::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    typedef void (KateStackTabWidgetButton::*m1_t0)();
    typedef void (QObject::*om1_t0)();
    m1_t0 v1_0 = &KateStackTabWidgetButton::_clicked;
    om1_t0 ov1_0 = (om1_t0)v1_0;
    QMetaData *slot_tbl = QMetaObject::new_metadata(1);
    QMetaData::Access *slot_tbl_access = QMetaObject::new_metaaccess(1);
    slot_tbl[0].name = "_clicked()";
    slot_tbl[0].ptr = (QMember)ov1_0;
    slot_tbl_access[0] = QMetaData::Private;
    typedef void (KateStackTabWidgetButton::*m2_t0)(int);
    typedef void (QObject::*om2_t0)(int);
    m2_t0 v2_0 = &KateStackTabWidgetButton::clicked;
    om2_t0 ov2_0 = (om2_t0)v2_0;
    QMetaData *signal_tbl = QMetaObject::new_metadata(1);
    signal_tbl[0].name = "clicked(int)";
    signal_tbl[0].ptr = (QMember)ov2_0;
    metaObj = QMetaObject::new_metaobject(
	"KateStackTabWidgetButton", "QPushButton",
	slot_tbl, 1,
	signal_tbl, 1,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    metaObj->set_slot_access( slot_tbl_access );
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    return metaObj;
}

// SIGNAL clicked
void KateStackTabWidgetButton::clicked( int t0 )
{
    activate_signal( "clicked(int)", t0 );
}
