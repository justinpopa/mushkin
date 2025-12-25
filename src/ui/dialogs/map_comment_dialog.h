#ifndef MAP_COMMENT_DIALOG_H
#define MAP_COMMENT_DIALOG_H

#include <QDialog>

// Forward declarations
class QPlainTextEdit;

/**
 * MapCommentDialog - Enter a comment/note for a map location
 *
 * Simple dialog that allows the user to enter or edit a comment
 * for a specific map location.
 */
class MapCommentDialog : public QDialog {
    Q_OBJECT

  public:
    explicit MapCommentDialog(QWidget* parent = nullptr);
    ~MapCommentDialog() override = default;

    // Get the comment text
    QString comment() const;

    // Set the comment text
    void setComment(const QString& comment);

  private:
    void setupUi();

    // UI Components
    QPlainTextEdit* m_commentEdit;
};

#endif // MAP_COMMENT_DIALOG_H
