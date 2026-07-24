/* flare_layout.h - Shared layout state for reflow v3.0
 *
 * FlareLayout contains dimensions that can change while a document
 * is being rendered (e.g., terminal resize). All components that care
 * about output dimensions receive a borrowed pointer to a FlareLayout.
 */

#ifndef DITTY_FLARE_LAYOUT_H
#define DITTY_FLARE_LAYOUT_H

typedef struct
{
    int width;         /* Current target width for reflow */
    int terminal_rows; /* Current terminal height */
    int resized;       /* Non-zero when resize detected; cleared after handling */
} FlareLayout;

#endif /* DITTY_FLARE_LAYOUT_H */
