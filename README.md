# Gem Heist

Author: Isa Lie

Design: This game introduces a unique blend of stealth and strategy, where players must blend in with computer controlled characters while trying to reach a gem, all while avoiding detection by a player-controlled defender who can trigger laser alarms.

Networking: My game commuicates largely with the recv/send_state_message() function, where information about player's status, 
position, and controls are make known to everyone. Other information communicate this way includes: NPC position and direction (so it is consistent 
across players), gem status, ammo status, and laser trap status.

Each player is assigned an individual index, and players with different roles
 sees different things.
For example, the guard sees only white circles. The thieves sees each other's color, and an additional X mark in the middle of 
the circle they control.

Screen Shot:

![Screen Shot](screenshot.png)

How To Play:

The first player to join is the guard. 

[SPACE] to trigger the laser traps. Starting with 5 ammo, catching a thief will gain the guard 3 ammos.

All the following players are thieves.

Use [W][A][S][D] to blend in with the NPCs and steal all gems!

This game was built with [NEST](NEST.md).

