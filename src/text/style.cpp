#include "style.h"
#include "action.h"

/**
 * Constructor
 *
 * Source: OtherTypes.h (original MUSHclient)
 *
 * Initializes Style with sensible defaults:
 * - iForeColour = WHITE (ANSI index 7)
 * - iBackColour = BLACK (ANSI index 0)
 * - iLength = 0 (will be set when style is added to text)
 * - iFlags = 0 (NORMAL - no styling, COLOUR_ANSI mode)
 * - pAction = nullptr (no action/hyperlink)
 */
Style::Style() : iLength(0), iFlags(0), iForeColour(WHITE), iBackColour(BLACK), pAction(nullptr)
{
    // All initialization done in initializer list
}

/**
 * Destructor
 *
 * Source: OtherTypes.h (original MUSHclient)
 *
 * Automatic cleanup via shared_ptr - no manual management needed.
 */
Style::~Style()
{
    // shared_ptr automatically handles Action lifetime
}
