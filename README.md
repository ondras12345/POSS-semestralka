# POSS semestralka - tank

## TODO test
- [x] cli set states w/o s_ prefix
- [x] test & tune `turn`
- [x] cli `perf`
- [x] test `encoder` pulse overflow
- [x] calibrate `encoder` pulses to mm
- [x] test `bluetooth`
- [x] cli `maze print`
- [x] cli `maze push`
- [x] cli `maze pop`
- [x] refactored `line_follower` & tune Kp
- [x] `crossroad` debug
- [x] `s_maze_follow`
- [x] speed up in straight segments
- [x] imu sometimes measures nonsense
  - There is no magnetic compass on board
  - Gyro is calibrated 500ms after boot. Ensure no movement.
- [x] test for backtracking, optimize `map_dir_order`
  - I would need to mock all of these:
    - [x] encoder distance
    - [x] crossroad detection
    - [x] line_follower
    - [x] turn
    - [x] Serial / DEBUG
    - [x] error_code
- [x] map
- [x] dead end detection
- [x] set new turn target & tolerance

## TODO implement
- [ ] do not map first cross ??


## Notes
O neco mirnejsi manevry
```
conf turn_Kp 3.00
conf turn_Ki 6.00
conf turn_Kd 0.00
conf turn_Tf 0.50
conf turn_Tt 3.00
```

### Mapping
```
debug crossroad 1
debug encoder 1
debug map 1

state map_start
```


### Final route (mapped)
```
maze_push X     I       197
maze_push 3     I       302
maze_push X     L       310
maze_push G     R       333
maze_push 7     L       280
maze_push G     R       658
maze_push E     R       1342
maze_push X     I       1327
maze_push 3     L       307
maze_push 3     L       645
maze_push X     R       1330
maze_push F     I       319
```


### Final route (manual)
```
debug crossroad 1
debug encoder 1
debug maze_follow 1
maze_push X I 100
maze_push 3 I 300
maze_push X L 300
maze_push G R 300
maze_push 7 L 300
maze_push G R 600
maze_push E R 1200
maze_push X I 1200
maze_push 3 L 300
maze_push 3 L 600
maze_push X R 1200
maze_push F I 300

state maze_follow
```


### Distance calibration
```
247cm

L4R0
L11708 R12818


11704 levy
12808 pravy
12261 avg
2470mm

0.201451757605416 mm_per_pulse


2.47m / 4s = .61 m/s
```


### Mapping optimization
```
k	map_pos_x	map_pos_y	orientation_x	orientation_y	line_state	crossroad	last_crossroad	encoder_pos_left	encoder_pos_right	speed_left	speed_right

crd_left, crd_right, crd_straight
25585	321	101	1	0	0x0	T	F	74116	129668	0	0
# N_turns=51

crd_left, crd_straight, crd_right
28925	321	101	1	0	0x0	T	F	67564	162796	0	0
# N_turns=60

crd_right, crd_left, crd_straight ******************
24365	321	101	1	0	0x0	T	F	77188	116868	0	0
# N_turns=49

crd_right, crd_straight, crd_left
28905	321	101	1	0	0x0	T	F	115244	115244	0	0
# N_turns=54

crd_straight, crd_left, crd_right
33465	321	101	1	0	0x0	T	F	101652	165140	0	0
# N_turns=53

crd_straight, crd_right, crd_left
32245	321	101	1	0	0x0	T	F	104724	152340	0	0
# N_turns=51
```


### CLI demo
```
Welcome to minicom 2.8

OPTIONS: I18n 
Port /dev/ttyUSB0, 16:34:57

Press CTRL-A Z for help on special keys

sluka:$ emergency, left bumper to start mapping, right to start following
emergency, left bumper to start mapping, right to start following

sluka:$ emergency, left bumper to start mapping, right to start following
emergency, left bumper to start mapping, right to start following

sluka:$ help

---- Shortcut Keys ----

Ctrl-A : Jumps the cursor to the beginning of the line.
Ctrl-E : Jumps the cursor to the end of the line.
Ctrl-D : Log Out.
Ctrl-R : Reverse-i-search.
Pg-Up  : History search backwards and auto completion.
Pg-Down: History search forward and auto completion.
Home   : Jumps the cursor to the beginning of the line.
End    : Jumps the cursor to the end of the line.

---- Available commands ----

conf: Get/set config
debug: Enable/disable debug
encoder: Read rotary encoders
imu: Get IMU state
line: Read line follower
maze_pop: Pop a node off the route
maze_print: Print current maze route
maze_push: Push a node onto the route
motor_move: Set motor output (nonlinear)
motor_move_lin: Set motor output (linearized)
perf: Print perf counters
state: Get/set state machine state
turn: Get / set turn state
uptime: Returns the time passed since the program started.

sluka:$ conf
Usage: conf [name value]
Missing value

configuration:
conf mm_per_pulse 0.20
conf base_speed 60
conf fast_speed 220
conf map_speed 60
conf fast_offset_mm 100
conf line_Kp 0.60
conf line_umax 60
conf turn_Kp 15.00
conf turn_Ki 30.00
conf turn_Kd 0.00
conf turn_Tf 0.50
conf turn_Tt 15.00
conf turn_target 90
conf turn_line_tolerance 55
conf line_debounce 2
conf dead_end_dist 30
conf min_cr_dist 80
conf cr_delay_mm 10

sluka:$ debug
crossroad: 0
encoder: 0
maze_follow: 0
map: 0

sluka:$ debug encoder 1
crossroad: 0
encoder: 1
maze_follow: 0
map: 0

sluka:$ [D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm

sluka:$ imu
imu:
  angle_X: -0.37
  angle_Y: 0.01
  angle_Z: -1.25

sluka:$ turn 90
turning 90
turning: 1
target: 89.07
angle_Z: -0.93
usage: turn [[line] target]

sluka:$ imu
imu:
  angle_X: -0.03
  angle_Y: -0.31
  angle_Z: 91.72

sluka:$ turn -90
turning -90
turning: 1
target: 1.88
angle_Z: 91.88
usage: turn [[line] target]

sluka:$ [D] went 10mm
sluka:$ imu
imu:
  angle_X: -0.20
  angle_Y: 0.00
  angle_Z: -0.22

sluka:$ help

---- Shortcut Keys ----

Ctrl-A : Jumps the cursor to the beginning of the line.
Ctrl-E : Jumps the cursor to the end of the line.
Ctrl-D : Log Out.
Ctrl-R : Reverse-i-search.
Pg-Up  : History search backwards and auto completion.
Pg-Down: History search forward and auto completion.
Home   : Jumps the cursor to the beginning of the line.
End    : Jumps the cursor to the end of the line.

---- Available commands ----

conf: Get/set config
debug: Enable/disable debug
encoder: Read rotary encoders
imu: Get IMU state
line: Read line follower
maze_pop: Pop a node off the route
maze_print: Print current maze route
maze_push: Push a node onto the route
motor_move: Set motor output (nonlinear)
motor_move_lin: Set motor output (linearized)
perf: Print perf counters
state: Get/set state machine state
turn: Get / set turn state
uptime: Returns the time passed since the program started.

sluka:$ perf
cli     90484
line_follower   1396
state_machine   155436
imu     2892
turn    640
maze    52

sluka:$ perf
cli     2800
line_follower   1368
state_machine   44
imu     2840
turn    36
maze    44

sluka:$ state
state: idle
emergency: 0

sluka:$ maze_print
maze_route_current (bottom to top)
        crossroad       direction       distance [mm]

sluka:$ maze_push X I 100

sluka:$ maze_push 3 I 300

sluka:$ maze_push X L 300

sluka:$ maze_print
maze_route_current (bottom to top)
        crossroad       direction       distance [mm]
maze_push X     I       100
maze_push 3     I       300
maze_push X     L       300

sluka:$ 
sluka:$ uptime
0 days, 0:1:47
sluka:$ 
```
