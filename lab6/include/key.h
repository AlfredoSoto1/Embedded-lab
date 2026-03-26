#ifndef KEY_H
#define KEY_H

// Rows (R1-R4) are outputs; Columns (C1-C3) are inputs with pull-down.
void keyp_init(int c1, int c2, int c3, int r1, int r2, int r3, int r4);
char keyp_scan(void);

#endif // KEY_H