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
- [ ] map
- [ ] dead end detection

## TODO implement
- [ ] test for backtracking, optimize `map_dir_order`
  - I would need to mock all of these:
    - [x] encoder distance
    - [x] crossroad detection
    - [x] line_follower
    - [x] turn
    - [x] Serial / DEBUG
    - [x] error_code


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
