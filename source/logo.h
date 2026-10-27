#ifndef LOGO_H
#define LOGO_H

// The title is drawn as sprites so each letter can float and tilt on its
// own. logo_init draws them once, using the top screen as a scratch area.
void logo_init(void);
void logo_show(void);
void logo_hide(void);
// Moves the letters for the given frame.
void logo_update(int frame);

#endif
