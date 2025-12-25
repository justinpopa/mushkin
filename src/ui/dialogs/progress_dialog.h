#ifndef PROGRESS_DIALOG_H
#define PROGRESS_DIALOG_H

#include <QDialog>

// Forward declarations
class QProgressBar;
class QLabel;
class QPushButton;

/**
 * ProgressDialog - Progress indicator dialog for long operations
 *
 * A standalone dialog used by Lua scripts to show progress of long-running
 * operations. Features a progress bar, status message, and optional cancel button.
 */
class ProgressDialog : public QDialog {
    Q_OBJECT

  public:
    explicit ProgressDialog(const QString& title, QWidget* parent = nullptr);
    ~ProgressDialog() override = default;

    // Set progress bar value (0-100)
    void setProgress(int value);

    // Set status message
    void setMessage(const QString& msg);

    // Set progress bar range
    void setRange(int min, int max);

    // Show/hide cancel button
    void setCancelable(bool cancelable);

    // Check if user clicked cancel
    bool wasCanceled() const;

  private:
    void setupUi();

    // UI Components
    QProgressBar* m_progressBar;
    QLabel* m_messageLabel;
    QPushButton* m_cancelButton;
    bool m_canceled;
};

#endif // PROGRESS_DIALOG_H
