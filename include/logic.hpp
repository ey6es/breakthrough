#ifndef LOGIC_H
#define LOGIC_H

void set_player_position (float position);
float get_player_position ();

void maybe_release_player_ball ();

void tick (float dt);

#endif // LOGIC_H