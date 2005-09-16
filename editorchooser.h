#ifndef _EDITOR_CHOOSER_H_
#define _EDITOR_CHOOSER_H_

#include <ktexteditor/document.h>
#include <ktexteditor/editor.h>

#include <qwidget.h>

class KConfig;
class QString;

namespace KTextEditor
{

/**
 * Editor Component Chooser.
 *
 * The EditorChooser is a simple group box that contains an information
 * label and a combo box which lists all available KTextEditor
 * implementations. To give the user the possibility to choose a text editor
 * implementation create an instance of this class and put it into the GUI.
 *
 * Use EditorChooser::editor() to access the currently used Editor component.
 *
 * You can find this class in action in KDE's control center in
 * "KDE Components > Component Chooser > Embedded Text Editor".
 *
 * @author Joseph Wenninger <jowenn@kde.org>
 */
class KTEXTEDITOR_EXPORT EditorChooser: public QWidget
{
  friend class PrivateEditorChooser;

  Q_OBJECT

  public:
    /**
     * Constructor.
     *
     * Create an editor chooser widget.
     * @param parent the parent widget
     */
    EditorChooser(QWidget *parent=0);
    /**
     * Destructor.
     */
    virtual ~EditorChooser();

   /* void writeSysDefault();*/

    /**
     * Read the configuration from the application's config file. The group
     * is handeled internally (it is called "KTEXTEDITOR:", but it is possible
     * to add a string to extend the group name with @p postfix.
     * @param postfix config group postfix string
     * @see writeAppSetting()
     */
    void readAppSetting(const QString& postfix=QString::null);
    /**
     * Write the configuration to the application's config file.
     * @param postfix config group postfix string
     * @see readAppSetting()
     */
    void writeAppSetting(const QString& postfix=QString::null);

    /**
     * Static accessor to get the Editor instance of the currently used
     * KTextEditor component.
     * @param postfix config group postfix string
     * @param fallBackToKatePart if @e true, the returned Editor component
     *        will be a katepart if no other implementation can be found
     * @return Editor controller or NULL, if no editor component could be
     *        found
     */
    static KTextEditor::Editor *editor (const QString& postfix=QString::null, bool fallBackToKatePart = true);

  signals:
    /**
     * This signal is emitted whenever the selected item in the combo box
     * changed.
     */
    void changed();
  private:
    class PrivateEditorChooser *d;
};

/*
class EditorChooserBackEnd: public ComponentChooserPlugin {

Q_OBJECT
public:
	EditorChooserBackEnd(QObject *parent=0, const char *name=0);
	virtual ~EditorChooserBackEnd();

	virtual QWidget *widget(QWidget *);
	virtual const QStringList &choices();
	virtual void saveSettings();

	void readAppSetting(KConfig *cfg,const QString& postfix);
	void writeAppSetting(KConfig *cfg,const QString& postfix);

public slots:
	virtual void madeChoice(int pos,const QString &choice);

};
*/

}
#endif
