#!/usr/bin/env python

import rospy
from race.msg import drive_param  # import the custom message
import curses

forward = 9780
left = 0

stdscr = curses.initscr()
curses.cbreak()
stdscr.keypad(1)
rospy.init_node('keyboard_talker', anonymous=True)
pub = rospy.Publisher('drive_parameters', drive_param, queue_size=10)

stdscr.refresh()

key = ''
stdscr.addstr(2, 20, "Neutral")
stdscr.addstr(3, 20, "Cntr")
stdscr.addstr(5, 20, "    ")

while key != ord('q'):
    key = stdscr.getch()
    stdscr.refresh()

    # fill in the conditions to increment/decrement throttle/steer
    forward = 9780  # reset forward value because it is now an impulse thrust not continuous

    if key == curses.KEY_UP:
        forward = forward + 300 if forward < 10080 else 10080
        stdscr.addstr(2, 20, "Forward")
        # forward = forward + 1 if forward < 20 else 20;
        # stdscr.addstr(2, 20, "Up  ")
        # stdscr.addstr(2, 25, '%.2f' % forward)
        stdscr.addstr(5, 20, "    ")
    elif key == curses.KEY_DOWN:
        forward = forward - 300 if forward > 9480 else 9480
        stdscr.addstr(2, 20, "Reverse")
        # forward = forward - 1 if forward > -20 else -20;
        # stdscr.addstr(2, 20, "Down")
        # stdscr.addstr(2, 25, '%.2f' % forward)
        stdscr.addstr(5, 20, "    ")
    if key == curses.KEY_LEFT:
        left = left - 1 if left > -10 else -10
        stdscr.addstr(3, 20, "left")
        stdscr.addstr(3, 25, '%.2f' % left)
        stdscr.addstr(5, 20, "    ")
    elif key == curses.KEY_RIGHT:
        left = left + 1 if left < 10 else 10
        stdscr.addstr(3, 20, "rgt ")
        stdscr.addstr(3, 25, '%.2f' % left)
        stdscr.addstr(5, 20, "    ")

    if forward == 9780:
        stdscr.addstr(2, 20, "Neutral")
        stdscr.addstr(5, 20, "    ")

    if key == curses.KEY_BACKSPACE:
        left = 0
        forward = 9780
        stdscr.addstr(2, 20, "Neutral")
        stdscr.addstr(3, 20, "Cntr")
        stdscr.addstr(5, 20, "Stop")

    msg = drive_param()
    msg.velocity = forward

    msg.angle = left

    pub.publish(msg)

curses.endwin()






