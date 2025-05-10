# Nightfighter Bot

This program is intended as a game aid for the game "Nightfighter," designed by Lee Brimmicombe-Wood and published by GMT Games. It plays the role of the umpire, thus enabling some of the scenarios to be played on one's own. 


## Prequisites


### 1. A physical copy of Nightfighter.

The bot does not replace any components of the physical game, it merely plays the role of the umpire.


### 2. A PC with a C++ compiler.

In this document, I am assuming you are using a Linux system. If using Windows or MacOS, I suggest you set up an Linux virtual machine in which to run the bot. A simple walkthrough on how create a Linux virtual machine can be found on the Ubuntu website:

[https://ubuntu.com/tutorials/how-to-run-ubuntu-desktop-on-a-virtual-machine-using-virtualbox]



### 3. The source code and settings files for the bot.

On your Linux system (or the virtual machine), open up the Terminal. Make sure you have the git and build-essential packages installed:

```
sudo apt-get install git build-essential
```

Then download the required files from GitHub:

```
git clone https://github.com/makochmanichf/Nightfighter_Bot
```


## Gameplay

The bot only implements part of the game's ruleset: the rules for ground radar, airborne interception (AI) radar, and visual contact (tallies). It only supports scenarios in which the player controls a single night fighter. It enables the following scenarios to be played: Scenario 2: Dunaja, Scenario 2b: Obsolescence, and Scenario 3: The Kammhuber Line. The settings file for each scenario comes in a separate directory. 

The bot only handles the game until the moment that the night fighter has obtained a tally: it has made visual contact with the bomber. The subsequent aerial combat must be handled by the player. 

In the Terminal, cd to the directory for the chosen scenario, and run the bot via the script "run.bash":

```
cd Scenario_2_Dunaja
bash run.bash
```

The bot will read in the scenario setup from the file "settings". It will generate some other parameters (the visibility and moon phase) randomly.

The player controls the night fighter and the radar search. Every game turn, the player is asked to type in the night fighter's position and facing:

```
Turn 4
 Phase 1: Bombers move.
 Phase 2: Fighter moves.
  Enter the fighter's position and facing:
**1508 0     **
  nf_position = 1508
  nf_facing   = 0
 Phase 3: Ground radar search.
 The radar search values are as follows: 2
  Enter the radar search hexes:
**1209**
  search hexes: 1209 (2) 

  Radar search in 1209(2) : NO CONTACT, place a sweep counter at hex 1309
 Phase 4: AI radar search.
 Phase 5: Tally phase.
  The nightfighter gets 3 tally dice.
```

The position is the hex occupied by the night fighter after its movement for the turn. The hex numbers must be entered without the leading zero (e.g., the hex in the top left corner of the map is 101 not 0101).

The facing is either 1 or 0. 1 means the fighter is facing "down", the same direction as the incoming bombers. It can then attempt to detect a bomber, either visually (tallying), or with AI radar. 0 means the fighter is facing any other direction, in which case it cannot attempt detect a bomber - it is moving too fast relative to the bombers. Note that this is a modification of the original rules 

The radar search is conducted by typing in the hex (or hexes) in which to search. The outcome is either contact, meaning that there is a bomber within a radius of 1 or 2 hexes of the chosen hex, or no contact, in which case the program tells the player where to place the sweep counter.
