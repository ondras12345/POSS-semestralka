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
- [ ] `s_maze_follow`
- [ ] backtracking mapping
  - separate mapping speed conf
- [ ] speed up in straight segments

## TODO implement
- [ ] imu sometimes measures nonsense
  - There is no magnetic compass on board
  - Gyro is calibrated 500ms after boot. Ensure no movement.
- [ ] test for backtracking, optimize `map_dir_order`
  - I would need to mock all of these:
    - encoder distance
    - crossroad detection
    - line_follower
    - turn
    - Serial / DEBUG
    - error_code


## Notes
O neco mirnejsi manevry
```
conf turn_Kp 3.00
conf turn_Ki 6.00
conf turn_Kd 0.00
conf turn_Tf 0.50
conf turn_Tt 3.00
```
### Test route
```
debug crossroad 1
debug encoder 1
debug maze_follow 1

maze_push X I 100
maze_push 3 I 300
maze_push X L 300
maze_push G R 300
maze_push 7 L 300
```

```
sluka:$ [D] starting maze_follow
[D] went 10mm
sluka:$ [D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] crossroad: G
[D] crossroad: T
[D] went 10mm
[D] went 10mm
[D] crossroad: I
[D] last crossroad: X
[D] expected last crossroad
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] crossroad: 7
[D] went 10mm
[D] crossroad: I
[D] last crossroad: 3
[D] expected last crossroad
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] crossroad: G
[D] crossroad: T
[D] went 10mm
[D] went 10mm
[D] crossroad: G
[D] crossroad: G
[D] crossroad: I
[D] last crossroad: X
[D] expected last crossroad
[D] turning -70.00
[D] went 10mm
[D] crossroad: 0
[D] last crossroad: I
[D] crossroad: G
[D] crossroad: 0
[D] last crossroad: G
[D] last crossroad: 0 (dist)
[D] crossroad: 7
[D] crossroad: I
[D] last crossroad: 3
[D] finished turning                                                                              +-----------------------------+
[W] undexpected crossroad: 3                                                                      |                             |
[D] went 10mm                                                                                     |  Cannot open /dev/ttyUSB0!  |
[D] went 10mm                                                                                     |                             |
[D] went 10mm                                                                                     +-----------------------------+
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] went 10mm
[D] crossroad: G
[D] went 10mm
[D] crossroad: I
[D] last crossroad: E
[W] undexpected crossroad: E
[D] crossroad: 0
[D] last crossroad: I
[D] went 10mm
[W] undexpected crossroad: I
[D] last crossroad: 0 (dist)
[E] cr_0
emergency, waiting for left bumper
emergency, waiting for left bumper
```


### Final route (manual)
```
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
# TODO is this finished?
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
