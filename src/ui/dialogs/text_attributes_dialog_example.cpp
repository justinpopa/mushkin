// Example usage of TextAttributesDialog
// This file is not part of the build - it's just documentation

#include "text_attributes_dialog.h"
#include <QColor>

void showTextAttributesExample()
{
    // Create the dialog
    TextAttributesDialog dialog;

    // Set the character being inspected
    dialog.setLetter("A");

    // Set color information using color names and RGB strings
    dialog.setTextColour("Red");
    dialog.setBackColour("Black");
    dialog.setTextColourRGB("255, 0, 0");
    dialog.setBackgroundColourRGB("0, 0, 0");

    // Or use QColor objects (this will also set the color swatches)
    dialog.setTextColour(QColor(255, 0, 0)); // Red
    dialog.setBackColour(QColor(0, 0, 0));   // Black

    // Set custom color information (if applicable)
    dialog.setCustomColour("ANSI color code 31");

    // Set text attributes
    dialog.setBold(true);
    dialog.setItalic(false);
    dialog.setInverse(false);

    // Set modification status
    dialog.setModified("Modified by trigger 'color_names'");

    // Show the dialog
    dialog.exec();
}

// Example with full ANSI styling
void showANSIStyledCharacter()
{
    TextAttributesDialog dialog;

    dialog.setLetter("@");

    // Bright yellow on blue background with bold
    dialog.setTextColour(QColor(255, 255, 0));
    dialog.setBackColour(QColor(0, 0, 255));
    dialog.setTextColourRGB("255, 255, 0");
    dialog.setBackgroundColourRGB("0, 0, 255");
    dialog.setCustomColour("ANSI: ESC[1;33;44m");

    dialog.setBold(true);
    dialog.setItalic(false);
    dialog.setInverse(false);

    dialog.setModified("Not modified");

    dialog.exec();
}

// Example showing inverse text
void showInverseCharacter()
{
    TextAttributesDialog dialog;

    dialog.setLetter("!");

    // Inverse swaps foreground and background
    dialog.setTextColour(QColor(192, 192, 192)); // Light gray
    dialog.setBackColour(QColor(0, 0, 0));       // Black
    dialog.setTextColourRGB("192, 192, 192");
    dialog.setBackgroundColourRGB("0, 0, 0");

    dialog.setBold(false);
    dialog.setItalic(false);
    dialog.setInverse(true); // Colors are reversed when rendered

    dialog.setModified("Not modified");

    dialog.exec();
}
